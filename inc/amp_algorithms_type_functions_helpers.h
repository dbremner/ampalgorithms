#pragma once
#ifndef _AMP_ALGORITHMS_TYPE_FUNCTIONS_HELPERS_H_BUMPTZI
#define _AMP_ALGORITHMS_TYPE_FUNCTIONS_HELPERS_H_BUMPTZI

#include <amp.h>
#include <cassert>
#include <climits>
#include <type_traits>

namespace amp_stl_algorithms
{
	//----------------------------------------------------------------------------
	// Type functions (and related machinery)
	//----------------------------------------------------------------------------

	template<typename O, typename... T, typename = decltype(std::declval<O>()(std::declval<T>()...))>
	std::true_type has_op(O&&, T&&...);
	std::false_type has_op(...);

	template<typename> struct has {};
	template<typename O, typename... T> struct has<O(T...)> : decltype(has_op(std::declval<O>(), std::declval<T>()...)) {};

	template<typename T> using Pointer = T*;

	template<typename...> struct voider { using type = void; };
	template<typename... Ts> using void_t = typename voider<Ts...>::type;
	template<typename, typename = void> struct has_value_type : std::false_type {};
	template<typename T> struct has_value_type<T, void_t<typename T::value_type>> : std::true_type{};

	template<typename T, typename E = void> struct Val_t;
	template<typename T> struct Val_t<T, std::enable_if_t<!has_value_type<T>::value>> { using type = std::remove_pointer_t<std::remove_reference_t<T>>; };
	template<typename T> struct Val_t<T, std::enable_if_t<has_value_type<T>::value>> { using type = typename T::value_type; };
	template<typename T> using Value_type = typename Val_t<T>::type;

	template<typename I> using Difference_type = typename std::iterator_traits<I>::difference_type;
	template<typename I> using Iterator_category = typename std::iterator_traits<I>::iterator_category;

	template<typename Op, typename... Args> struct _foo_work { using type = std::result_of_t<Op(Args...)>; }; // Annoying workaround
	template<typename Op, typename... Args> using Codomain = typename _foo_work<Op, Args...>::type;

	//--------------------------------------------
	// forward and move for restrict(amp) contexts
	//--------------------------------------------

	template<typename T>
	constexpr inline T&& forward(std::remove_reference_t<T>& t) restrict(cpu, amp)
	{
		return static_cast<T&&>(t);
	}

	template<typename T>
	constexpr inline T&& forward(std::remove_reference_t<T>&& t) restrict(cpu, amp)
	{
		return static_cast<T&&>(t);
	}

	template<typename T>
	constexpr inline std::remove_reference_t<T>&& move(T&& t) restrict(cpu, amp)
	{
		return static_cast<std::remove_reference_t<T>&&>(t);
	}

	//----------------------------------------------
	// Dark hateful magic against overeager caching.
	//----------------------------------------------

	template<typename T, typename = std::enable_if_t<(sizeof(T) == sizeof(unsigned int))>>
	inline T _uncached_load(Pointer<T> p) restrict(amp)
	{
		const unsigned int x = concurrency::atomic_fetch_and(reinterpret_cast<Pointer<unsigned int>>(p),
														     UINT_MAX);
		return reinterpret_cast<const T&>(x);
	}

	template<typename T, int tsz>
	inline void _refresh_n(Pointer<T> p,
						   Difference_type<Pointer<T>> n,
						   const concurrency::tiled_index<tsz>& tidx) restrict(amp)
	{
		static constexpr unsigned int cacheline_sz = 64; // 64-byte cachelines are adequately common, but this is frail.
		static constexpr unsigned int stride = cacheline_sz / sizeof(T);
		for (Difference_type<Pointer<T>> i = tidx.local[0] * stride; i < n; i += tidx.tile_dim0 * stride) {
			concurrency::atomic_fetch_and(reinterpret_cast<Pointer<unsigned int>>(p), UINT_MAX);
		}
	}

	//----------------------------------------------------------------------------
	// static_if helper for compile-time type selection
	//----------------------------------------------------------------------------

	template<typename, typename, bool> struct static_if {};
	template<typename T, typename U> struct static_if<T, U, true> { using type = typename T; };
	template<typename T, typename U> struct static_if<T, U, false> { using type = typename U; };

	template<typename T, typename U, bool condition>
	using static_if_t = typename static_if<T, U, condition>::type;

	//----------------------------------------------------------------------------
	// Reference
	// A reference wrapper type which is usable in AMP contexts.
	//----------------------------------------------------------------------------

	template<typename T>
	class Reference {
		template<typename U>
		struct T_wrapper {
			alignas(sizeof(int)) U x;
			operator U&() restrict(cpu, amp) { return x; };
		};
		using T_AMP = static_if_t<T_wrapper<T>, T, sizeof(T) < sizeof(int)>;
	public:
		using type = T;

		Reference() = delete;

		Reference(T& r) restrict(cpu, amp) : data(1, &reinterpret_cast<T_wrapper<T>&>(r)) {};
		Reference(T&&) = delete;
		Reference(const Reference&) = default;

		Reference& operator=(const Reference&) = default;

		operator T&() const restrict(cpu, amp) { return reinterpret_cast<T&>(data[0]); };
		T& get() const restrict(cpu, amp) { return reinterpret_cast<T&>(data[0]); };
	private:
		concurrency::array_view<T_wrapper<T>> data;
	};

	template<typename T>
	inline Reference<T> ref(T& t) restrict(cpu, amp)
	{
		return Reference<T>(t);
	}

	template<typename T>
	inline Reference<T> ref(Reference<T> tref) restrict(cpu, amp)
	{
		return ref(tref.get());
	}

	template<typename T>
	inline Reference<const T> cref(const T& t) restrict(cpu, amp)
	{
		return Reference<const T>(t);
	}

	template<typename T>
	inline Reference<const T> cref(Reference<T> tref) restrict(cpu, amp)
	{
		return cref(tref.get());
	}

	//----------------------------------------------------------------------------
	// various asserts
	//----------------------------------------------------------------------------

	inline void amp_assert(bool cond) restrict(amp)
	{
		// TODO_NOT_IMPLEMENTED: amp_assert
		cond; // Silence warnings about unreferenced formal parameters.
	}

	template<typename array_type>
	inline void assert_arrays_are_same_toplevel_resource(const array_type& a1, const array_type& a2) restrict(cpu, amp)
	{
		// TODO_NOT_IMPLEMENTED: assert_arrays_are_same_toplevel_resource
	}

	//////////////////////////////////////////////////////////////////////////
	// Empty array factories
	//
	// Random iterators require a default constructors. Because array_view
	// iterators must hold an array view or equivalent, there must be a way
	// to create a "default" array_view. It is possible to simply not provide
	// a default constructor, but then some algorithms may not work.
	//
	// Instead the following classes and functions make a good effort to create
	// a "real" array_view pointing to at least superficially valid data -- as
	// long as it is not dereferenced.
	//
	// Note: only array_view<T, 1> is supported.
	///////////////////////////////////////////////////////////////////////////

	// Specialization for array_views
	template<typename T, typename = void> struct empty_array_view_factory; // Undefined.

	template<typename T>
	struct empty_array_view_factory<T, std::enable_if_t<!std::is_const<T>::value>> {
		// On the CPU, use the source-less array_view constructor
		static concurrency::array_view<T> create() restrict(cpu)
		{
			return concurrency::array_view<T>(1);
		}

		// On the accelerator, we use an unclean cast.
		static concurrency::array_view<T> create() restrict(amp)
		{
			return concurrency::array_view<T>(1, reinterpret_cast<T*>(nullptr));
		}
	};

	template<typename T>
	struct empty_array_view_factory<T, std::enable_if_t<std::is_const<T>::value>> {
		// On the CPU, use a statc variable
		static concurrency::array_view<T> create() restrict(cpu)
		{
			static T stable_storage;
			return concurrency::array_view<T>(1, &stable_storage);
		}

		// On the accelerator, we use an unclean cast.
		static concurrency::array_view<T> create() restrict(amp)
		{
			return concurrency::array_view<T>(1, reinterpret_cast<const T*>(nullptr));
		}
	};

	template <class value_type, int rank>
	concurrency::array_view<value_type> make_array_view(concurrency::array<value_type, rank>& arr) restrict(cpu, amp)
	{
		return arr.view_as(concurrency::extent<1>(arr.get_extent().size()));
	}

	template <class value_type, int rank>
	concurrency::array_view<const value_type> make_array_view(const concurrency::array<value_type, rank>& arr) restrict(cpu, amp)
	{
		return arr.view_as(concurrency::extent<1>(arr.get_extent().size()));
	}

   //----------------------------------------------------------------------------
   // Computational bases for types
   //----------------------------------------------------------------------------

	template<typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
	static inline constexpr T successor(T x) restrict(cpu, amp)
	{
		return x + T(1);
	}

	template<typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
	static inline constexpr T predecessor(T x) restrict(cpu, amp)
	{
		return x - T(1);
	}

	template<typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
	static inline constexpr T twice(T x) restrict(cpu, amp)
	{
		return x << 1u;
	}

	template<typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
	static inline constexpr T half_nonnegative(T x) restrict(cpu, amp)
	{
		return x >> 1u;
	}

	template<typename T, typename U, typename = std::enable_if_t<std::is_integral<T>::value &&
																 std::is_integral<U>::value>>
	static inline constexpr T binary_scale_down_nonnegative(T x, U k) restrict(cpu, amp)
	{
		while (k) {
			x = half_nonnegative(x);
			k = predecessor(k);
		}
		return x;
	}

	template<typename T, typename U, typename = std::enable_if_t<std::is_integral<T>::value &&
																 std::is_integral<U>::value>>
	static inline constexpr T binary_scale_up_nonnegative(T x, U k) restrict(cpu, amp)
	{
		while (k) {
			x = twice(x);
			k = predecessor(k);
		}
		return x;
	}

	template<typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
	static inline constexpr bool positive(T x) restrict(cpu, amp)
	{
		return x > T(0);
	}

	template<typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
	static inline constexpr bool negative(T x) restrict(cpu, amp)
	{
		return x < T(0);
	}

	template<typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
	static inline constexpr bool zero(T x) restrict(cpu, amp)
	{
		return x == T(0);
	}

	template<typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
	static inline constexpr bool one(T x) restrict(cpu, amp)
	{
		return x == T(1);
	}

	template<typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
	static inline constexpr bool even(T x) restrict(cpu, amp)
	{
		return zero(x & 1u);
	}

	template<typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
	static inline constexpr bool odd(T x) restrict(cpu, amp)
	{
		return !even(x);
	}

	template<typename T, typename U, typename = std::enable_if_t<std::is_integral<T>::value &&
																 std::is_integral<U>::value>>
	static inline constexpr T rounded_up_quotient(T dividend, U divisor) restrict(cpu, amp)
	{
		return (dividend + predecessor(divisor)) / divisor;
	}

	template<typename T, typename U, typename = std::enable_if_t<std::is_integral<T>::value &&
																 std::is_integral<U>::value>>
	static inline constexpr T even_division_remainder(T dividend, U divisor) restrict(cpu, amp)
	{	// If the divisor is not even, results are undefined.
		return dividend & (predecessor(divisor));
	}

	template<typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
	static inline unsigned int binary_logarithm(T x) restrict(cpu)
	{
		static constexpr unsigned int b[] = { 0x2, 0xC, 0xF0, 0xFF00, 0xFFFF0000 };
		static constexpr unsigned int s[] = { 1, 2, 4, 8, 16 };

		unsigned int r = 0u;
		for (auto i = std::crbegin(b), j = std::crbegin(s); i != std::crend(b); ++i, ++j) {
			if (x & *i) {
				x >>= *j;
				r |= *j;
			}
		}

		return r;
	}

	template<typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
	static inline unsigned int binary_logarithm(T x) restrict(amp)
	{
		return concurrency::direct3d::firstbithigh(x);
	}

	template<typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
	static inline T round_up_to_next_binary_power(T x) restrict(cpu, amp)
	{
		x = predecessor(x);
		x |= x >> T(1);
		x |= x >> T(2);
		x |= x >> T(4);
		x |= x >> T(8);
		x |= x >> T(16);
		x = successor(x);
		return zero(x) ? T(1) : x;
		//return binary_scale_up_nonnegative(T(1), successor(binary_logarithm(predecessor(x))));
	}

	//----------------------------------------------------------------------------
	// static_for helper for compile-time looping / unrolling
	//----------------------------------------------------------------------------

	// TODO: since the compiler is better now with constexpr, this can be cleaner.
	enum class Inc { add, sub, mul, div, mod, last };
	template<unsigned int i_0, unsigned int i_N, Inc incr = Inc::add, unsigned int modifier = 1u>
	struct static_for {
		template<typename F>
		void operator()(F&& fn) const restrict(cpu, amp)
		{
			forward<F>(fn)(i_0);
			static constexpr unsigned int i_next = (incr == Inc::add) ? (i_0 + modifier) :
				(incr == Inc::sub) ? (i_0 - modifier) :
				(incr == Inc::mul) ? (i_0 * modifier) :
				(incr == Inc::div) ? (i_0 / modifier) :
				(incr == Inc::mod) ? (i_0 % modifier) : i_N;
			static constexpr Inc incr_next = i_next == i_N ? Inc::last : incr;
			static constexpr unsigned int modifier_next = i_next == i_N ? 0u : modifier;
			static_for<i_next, i_N, incr_next, modifier_next>()(forward<F>(fn));
		}
	};

	template<unsigned int i_N>
	struct static_for<i_N, i_N, Inc::last, 0u> {
		template<typename F>
		void operator()(F&&) const restrict(cpu, amp) { return; };
	};
}		// namespace amp_stl_algorithms
#endif  // _AMP_ALGORITHMS_TYPE_FUNCTIONS_HELPERS_H_BUMPTZI