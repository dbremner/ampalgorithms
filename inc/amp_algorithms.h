/*----------------------------------------------------------------------------
* Copyright (c) Microsoft Corp.
*
* Licensed under the Apache License, Version 2.0 (the "License"); you may not
* use this file except in compliance with the License.  You may obtain a copy
* of the License at http://www.apache.org/licenses/LICENSE-2.0
*
* THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
* WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
* MERCHANTABLITY OR NON-INFRINGEMENT.
*
* See the Apache Version 2.0 License for specific language governing
* permissions and limitations under the License.
*---------------------------------------------------------------------------
*
* C++ AMP algorithms library.
*
* This file contains the C++ AMP algorithms
*---------------------------------------------------------------------------*/

// TODO: Does it really make a lot of sense to declare two namespaces or should everything be flattened into amp_algorithms?
// TODO: Here the functions are defined here. In the STL implementation they are defined in the main header file
// and just declared in the public one. Is this by design?

#pragma once
#ifndef _AMP_ALGORITHMS_H_BUMPTZI
#define _AMP_ALGORITHMS_H_BUMPTZI

#include <amp_indexable_view.h>
#include <amp_algorithms_type_functions_helpers.h>
#include <xx_amp_algorithms_impl.h>

#include <amp.h>
#include <cassert>
#include <functional>
#include <type_traits>

namespace amp_algorithms
{
#pragma region Arithmetic, comparison, logical and bitwise operators

    //----------------------------------------------------------------------------
    // Arithmetic operations
    //----------------------------------------------------------------------------

    template<typename T = void>
    struct plus : std::binary_function<T, T, T> {
        constexpr T operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return a + b;
        }
    };
    template<>
    struct plus<void> {
        template<typename T, typename U>
        constexpr decltype(auto) operator()(T&& a, U&& b) const restrict(cpu, amp)
        {
            return amp_stl_algorithms::forward<T>(a) + amp_stl_algorithms::forward<U>(b);
        }
    };

    template<typename T = void>
    struct minus : std::binary_function<T, T, T> {
        constexpr T operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return a - b;
        }
    };
    template<>
    struct minus<void> {
        template<typename T, typename U>
        constexpr decltype(auto) operator()(T&& a, U&& b) const restrict(cpu, amp)
        {
            return amp_stl_algorithms::forward<T>(a) - amp_stl_algorithms::forward<U>(b);
        }
    };

    template<typename T = void>
    struct multiplies : std::binary_function<T, T, T> {
        constexpr T operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return a * b;
        }
    };
    template<>
    struct multiplies<void> {
        template<typename T, typename U>
        constexpr decltype(auto) operator()(T&& a, U&& b) const restrict(cpu, amp)
        {
            return amp_stl_algorithms::forward<T>(a) * amp_stl_algorithms::forward<U>(b);
        }
    };

    template<typename T = void>
    struct divides : std::binary_function<T, T, T> {
        constexpr T operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return a / b;
        }
    };
    template<>
    struct divides<void> {
        template<typename T, typename U>
        constexpr decltype(auto) operator()(T&& a, U&& b) const restrict(cpu, amp)
        {
            return amp_stl_algorithms::forward<T>(a) / amp_stl_algorithms::forward<U>(b);
        }
    };

    template<typename T = void>
    struct modulus : std::binary_function<T, T, T> {
        constexpr T operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return a % b;
        }
    };
    template<>
    struct modulus<void> {
        template<typename T, typename U>
        constexpr decltype(auto) operator()(T&& a, U&& b) const restrict(cpu, amp)
        {
            return amp_stl_algorithms::forward<T>(a) % amp_stl_algorithms::forward<U>(b);
        }
    };

    template<typename T = void>
    struct negate : std::unary_function<T, T> {
        constexpr T operator()(const T &a) const restrict(cpu, amp)
        {
            return -a;
        }
    };
    template<>
    struct negate<void> {
        template<typename T>
        constexpr T operator()(const T& a) const restrict(cpu, amp)
        {
            return negate<T>()(a);
        }
    };

    //----------------------------------------------------------------------------
    // Additional arithmetic operations with no STL equivalents
    //----------------------------------------------------------------------------
	// TODO: constexpr-esize all of these functions. Also, optimize.
	template<typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
	static inline constexpr unsigned int static_log2(T val) restrict(cpu, amp)
	{
		return (val == T(0) || val == T(1)) ? 0u : (static_log2(val / 2) + 1);
	}

    template<typename N, std::enable_if_t<std::is_integral<N>::value>* = nullptr>
    inline constexpr bool static_is_power_of_two(N x) restrict(cpu, amp)
    {
        return x && !(x & (x - N(1)));
    }

    // TODO: Generalize this for other integer types.
    template<typename T>
    constexpr inline bool is_power_of_two(T value)
    {
        return count_bits(value) == 1;
    }

    //----------------------------------------------------------------------------
    // Comparison operations
    //----------------------------------------------------------------------------

    template<typename T = void>
    struct equal_to : std::binary_function<T, T, bool> {
        constexpr bool operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return a == b;
        }
    };
    template<>
    struct equal_to<void> {
        template<typename T, typename U>
        constexpr bool operator()(T&& a, U&& b) const restrict(cpu, amp)
        {
            return amp_stl_algorithms::forward<T>(a) == amp_stl_algorithms::forward<U>(b);
        }
    };

    template<typename T = void>
    struct not_equal_to : std::binary_function<T, T, bool> {
        /*constexpr*/ bool operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return !amp_algorithms::equal_to<T>()(a, b);
        }
    };
    template<>
    struct not_equal_to<void> {
        template<typename T, typename U>
        /*constexpr*/ bool operator()(T&& a, U&& b) const restrict(cpu, amp)
        {
            return !amp_algorithms::equal_to<>()(amp_stl_algorithms::forward<T>(a),
												 amp_stl_algorithms::forward<U>(b));
        }
    };

    template<typename T = void>
    struct less : std::binary_function<T, T, bool> {
        constexpr bool operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return a < b;
        }
    };
    template<>
    struct less<void> {
        template<typename T, typename U>
        constexpr bool operator()(T&& a, U&& b) const restrict(cpu, amp)
        {
            return amp_stl_algorithms::forward<T>(a) < amp_stl_algorithms::forward<U>(b);
        }
    };

    template<typename T = void>
    struct less_equal : std::binary_function<T, T, bool> {
        /*constexpr*/ bool operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return !amp_algorithms::less<>()(b, a);
        }
    };
    template<>
    struct less_equal<void> {
        template<typename T, typename U>
        /*constexpr*/ bool operator()(T&& a, U&& b) const restrict(cpu, amp)
        {
            return !amp_algorithms::less<>()(amp_stl_algorithms::forward<U>(b),
											 amp_stl_algorithms::forward<T>(a));
        }
    };

    template<typename T = void>
    struct greater : std::binary_function<T, T, T> {
        /*constexpr*/ bool operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return !amp_algorithms::less_equal<>()(a, b);
        }
    };
    template<>
    struct greater<void> {
        template<typename T, typename U>
        /*constexpr*/ bool operator()(T&& a, U&& b) const restrict(cpu, amp)
        {
            return !amp_algorithms::less_equal<>()(amp_stl_algorithms::forward<T>(a),
												   amp_stl_algorithms::forward<U>(b));
        }
    };

    template<typename T = void>
    struct greater_equal : std::binary_function<T, T, T> {
        /*constexpr*/ bool operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return !amp_algorithms::less<>()(a, b);
        }
    };
    template<>
    struct greater_equal<void> {
        template<typename T, typename U>
        /*constexpr*/ bool operator()(T&& a, U&& b) const restrict(cpu, amp)
        {
            return !amp_algorithms::less<>()(amp_stl_algorithms::forward<T>(a),
											 amp_stl_algorithms::forward<U>(b));
        }
    };

    //----------------------------------------------------------------------------
    // Logical operations
    //----------------------------------------------------------------------------

    template<class T>
    struct logical_not : std::unary_function<T, bool> {
        constexpr bool operator()(const T& a) const restrict(cpu, amp)
        {
            return !a;
        }
    };

    template<class T>
    struct logical_and : public std::binary_function<T, T, bool> {
        constexpr bool operator()(const T& a, const T& b) const restrict(cpu, amp)
        {
            return a && b;
        }
    };

    template<class T>
    struct logical_or : public std::binary_function<T, T, bool> {
        constexpr bool operator()(const T& a, const T& b) const restrict(cpu, amp)
        {
            return a || b;
        }
    };

    template<typename Predicate>
    struct unary_negate : public std::unary_function<typename Predicate::argument_type, bool> {
        explicit constexpr unary_negate(const Predicate& pred) restrict(cpu, amp)
			: m_pred(pred) {}

        constexpr result_type operator()(const argument_type& a) const restrict(cpu, amp)
        {
            return !m_pred(a);
        }

	private:
		Predicate m_pred;
    };

    template<typename Predicate>
    inline constexpr amp_algorithms::unary_negate<Predicate> not1(const Predicate& pred) restrict(cpu, amp)
    {
        return amp_algorithms::unary_negate<Predicate>(pred);
    }

    template<typename Predicate>
    struct binary_negate : public std::binary_function<typename Predicate::first_argument_type, typename Predicate::second_argument_type, bool> {
        explicit constexpr binary_negate(const Predicate& pred) restrict(cpu, amp)
			: m_pred(pred) {}

        constexpr result_type operator()(const first_argument_type& a, const second_argument_type& b) const restrict(cpu, amp)
        {
            return !m_pred(a, b);
        }

	private:
		Predicate m_pred;
    };

    template<typename Predicate>
    inline constexpr amp_algorithms::binary_negate<Predicate> not2(const Predicate& pred) restrict(cpu, amp)
    {
        return amp_algorithms::binary_negate<Predicate>(pred);
    }

    //----------------------------------------------------------------------------
    // Bitwise operations
    //----------------------------------------------------------------------------

    template<typename T>
    struct bit_and : std::binary_function<T, T, T> {
        constexpr T operator()(const T& a, const T& b) const restrict(cpu, amp)
        {
            return a & b;
        }
    };

    template<typename T>
    struct bit_or : std::binary_function<T, T, T> {
        constexpr T operator()(const T& a, const T& b) const restrict(cpu, amp)
        {
            return a | b;
        }
    };

    template<typename T>
    struct bit_xor : std::binary_function<T, T, T> {
        constexpr T operator()(const T& a, const T& b) const restrict(cpu, amp)
        {
            return a ^ b;
        }
    };

    template <typename T>
    struct bit_not : std::unary_function<T, T> {
        constexpr T operator()(const T& a) const restrict(cpu, amp)
        {
            return ~a;
        }
    };

    //----------------------------------------------------------------------------
    // Additional bitwise operations with no STL equivalent
    //----------------------------------------------------------------------------

    static constexpr unsigned int bit08 = 8;//0x80;
    static constexpr unsigned int bit16 = 16;//0x8000;
    static constexpr unsigned int bit32 = 32;//0x80000000;

    namespace _details
    {
		template<typename N, std::enable_if_t<std::is_integral<N>::value>* = nullptr>
		inline constexpr bool is_bit_set(N x, N bit) restrict(cpu, amp)
		{
			return (x & (1u << bit)) != N(0);
		}
    }

	template<typename N, std::enable_if_t<std::is_integral<N>::value>* = nullptr>
	inline constexpr unsigned int static_count_bits(N x, unsigned int max_bit = sizeof(N) * CHAR_BIT) restrict(cpu, amp)
	{
		return (x == N(0) || max_bit == 0u) ? 0u
											: (_details::is_bit_set(x, 0) +
											  static_count_bits(x >> 1, max_bit - 1));
	}

    template<typename T, std::enable_if_t<std::is_integral<T>::value>* = nullptr>
    inline unsigned int count_bits(T value) restrict(cpu)
    {
        value = value - ((value >> 1) & 0x55555555);
        value = (value & 0x33333333) + ((value >> 2) & 0x33333333);
        return (((value + (value >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
    }

	template<typename T, std::enable_if_t<std::is_integral<T>::value>* = nullptr>
	inline unsigned int count_bits(T value) restrict(amp)
	{
		return concurrency::direct3d::countbits(value);
	}

#pragma endregion

#pragma region Byte pack and unpack, padded read and write

    //----------------------------------------------------------------------------
    // Byte pack and unpack
    //----------------------------------------------------------------------------

    template<int index, typename T>
    inline constexpr unsigned long pack_byte(const T& value) restrict(cpu, amp)
    {
        assert(value < 256);
        static_assert(index < sizeof(T), "Index out of range.");
        return (static_cast<unsigned long>(value) && 0xFF) << (index * CHAR_BIT);
    }

    template<typename T>
    inline constexpr unsigned long pack_byte(const T& value, unsigned index) restrict(cpu, amp)
    {
        //assert(value < 256);
        //assert(index < sizeof(T));
        return (static_cast<unsigned long>(value) && 0xFF) << (index * CHAR_BIT);
    }

    template<int index, typename T>
    inline constexpr unsigned int unpack_byte(const T& value) restrict(cpu, amp)
    {
        static_assert(index < sizeof(T), "Index out of range.");
        return (value >> (index * CHAR_BIT)) & 0xFF;
    }

    template<typename T>
    inline constexpr unsigned int unpack_byte(const T& value, unsigned index) restrict(cpu, amp)
    {
        //assert(index < sizeof(T));
        return (value >> (index * CHAR_BIT)) & 0xFF;
    }

    template<typename T>
    inline constexpr unsigned int bit_count() restrict(cpu, amp)
    {
        return sizeof(T) * CHAR_BIT;
    }

    //----------------------------------------------------------------------------
    // container padded_read & padded_write
    //----------------------------------------------------------------------------

    template<typename T, int R>
    inline decltype(auto) padded_read(const concurrency::array_view<T, R>& arr,
									  const concurrency::index<R>& idx) restrict(cpu, amp)
    {
        return arr.extent.contains(idx) ? arr[idx] : T();
    }

    template <typename T>
    inline decltype(auto) padded_read(const concurrency::array_view<T>& arr, int idx) restrict(cpu, amp)
    {
        return padded_read(arr, concurrency::index<1>(idx));
    }

    template <typename T, typename U, int R>
    inline void padded_write(const concurrency::array_view<T, R>& arr,
							 const concurrency::index<R>& idx, U&& value) restrict(cpu, amp)
    {
        if (arr.extent.contains(idx)) {
            arr[idx] = amp_stl_algorithms::forward<U>(value);
        }
    }

    template <typename T, typename U>
    inline void padded_write(const concurrency::array_view<T>& arr, int idx, U&& value) restrict(cpu, amp)
    {
        padded_write(arr, concurrency::index<1>(idx), amp_stl_algorithms::forward<U>(value));
    }

    // TODO: Should this return an extent? Better name.
    template <int N, typename InputIndexableView>
    inline int tile_partial_data_size(const InputIndexableView& arr, const tiled_index<N>& tidx) restrict(amp)
    {
        return arr.extent.size() - tidx.tile[0] * tidx.tile_extent[0];
    }

#pragma endregion

    //----------------------------------------------------------------------------
    // bitvector
    //----------------------------------------------------------------------------

    template <typename T>
    class uniform_segments
    {
    private:
        int m_step;

    public:
        uniform_segments(int step) : m_step(step) { }

        bool operator()(const T &i) const
        {
            return (i % m_step == 0);
        }
    };

    struct bitvector
    {
    private:
        unsigned int m_data_size;

    public:
        std::vector<unsigned> data;

#if _MSC_VER >= 1800
        bitvector() = delete;
        ~bitvector() = default;
        bitvector(const bitvector&) = default;
        bitvector(bitvector&&) = default;
        bitvector& operator=(const bitvector&) = default;
        bitvector& operator=(bitvector&&) = default;
#endif
        bitvector(unsigned data_size) : m_data_size(data_size)
        {
            data = std::vector<unsigned>(bits_pad_to_uint(data_size), 0);
        }

        // Initialize bitvector with constant segment width.

        void initialize(int segment_width)
        {
            initialize(uniform_segments<int>(segment_width));
        }

        // Initialize bitvector with custom segment widths.

        template <typename Func>
        void initialize(Func pred)
        {
            unsigned flag_counter = 0;
            for (unsigned idx = 0; idx < data.size() && flag_counter < m_data_size; ++idx) {
                unsigned bag_of_bits = data[idx];
                for (unsigned offset = 0; offset < amp_algorithms::bit_count<unsigned>() &&
									      flag_counter < m_data_size; ++offset) {
                    if (pred(flag_counter)) {
                        bag_of_bits |= 1 << offset;
                    }
                    ++flag_counter;
                }
                data[idx] = bag_of_bits;
            }
        }

        bool is_bit_set(unsigned pos,
						amp_algorithms::scan_direction direction = amp_algorithms::scan_direction::forward)
        {
            // When we encounter flag going direction it means,
            // that it is the first element of this segment (last element to be scanned going direction)
            // for simplification we increment 'pos' and always look for flags behind our current position.

            if (direction == amp_algorithms::scan_direction::backward) {
                ++pos;
            }
            unsigned idx = pos / bit_count<unsigned>();
            unsigned offset = pos % bit_count<unsigned>();
            unsigned bag_of_bits = data[idx];
            return (1 << offset & bag_of_bits) > 0;
        }

    private:
        unsigned bits_pad_to_uint(unsigned bits) const
        {
            return (bits + amp_algorithms::bit_count<unsigned>() - 1) / amp_algorithms::bit_count<unsigned>();
        }
    };

    //----------------------------------------------------------------------------
    // fill
    //----------------------------------------------------------------------------

    template<typename T>
    inline void fill(const concurrency::accelerator_view &accl_view,
			         const concurrency::array_view<T>& output_view,
					 const T& value)
    {
        ::amp_algorithms::generate(accl_view, output_view, [=]() restrict(amp) { return value; });
    }

    template<typename T>
    inline void fill(const concurrency::array_view<T>& output_view, const T& value)
    {
        ::amp_algorithms::generate(output_view, [=]() restrict(amp) { return value; });
    }

    //----------------------------------------------------------------------------
    // generate
    //----------------------------------------------------------------------------

    template<typename T, typename Generator>
    inline void generate(const concurrency::accelerator_view &accl_view,
				  const concurrency::array_view<T>& output_view,
				  Generator generator)
    {
        _details::parallel_for_each(accl_view,
									output_view.extent,
									[=, generator = std::move(generator)](auto&& idx) restrict(amp) {
            output_view[idx] = generator();
        });
    }

    template<typename T, typename Generator>
    void generate(const concurrency::array_view<T>& output_view, Generator generator)
    {
        ::amp_algorithms::generate(_details::auto_select_target(), output_view, std::move(generator));
    }

    //----------------------------------------------------------------------------
    // merge_sort
    //----------------------------------------------------------------------------

    // TODO_NOT_IMPLEMENTED: merge_sort
    template<typename T, typename BinaryOperator>
    inline void merge_sort(const concurrency::accelerator_view& accl_view,
					       const concurrency::array_view<T>& input_view,
					       BinaryOperator op)
    {
    }

    template<typename T>
    inline void merge_sort(const concurrency::accelerator_view& accl_view,
					       const concurrency::array_view<T>& input_view)
    {
        ::amp_algorithms::merge_sort(accl_view, input_view, amp_algorithms::less<T>());
    }

    //----------------------------------------------------------------------------
    // radix_sort
    //----------------------------------------------------------------------------

    template<typename T>
    inline void radix_sort(const concurrency::accelerator_view& accl_view,
						   concurrency::array_view<T>& input_view,
						   concurrency::array_view<T>& output_view)
    {
        static constexpr int bin_width = 2;
        static constexpr int tile_size = 128;
        _details::radix_sort<T, tile_size, bin_width>(accl_view, input_view, output_view);
    }

    // TODO: input_view should be a const.
    template<typename T>
    inline void radix_sort(concurrency::array_view<T>& input_view,
						   concurrency::array_view<T>& output_view)
    {
        radix_sort(_details::auto_select_target(), input_view, output_view);
    }

    template<typename T>
    inline void radix_sort(const concurrency::accelerator_view& accl_view,
						   const concurrency::array_view<T>& input_view)
    {
        radix_sort(accl_view, input_view, input_view);
    }

    template<typename T>
    inline void radix_sort(concurrency::array_view<T>& input_view)
    {
        radix_sort(_details::auto_select_target(), input_view, input_view);
    }

    //----------------------------------------------------------------------------
    // reduce
    //----------------------------------------------------------------------------

    // Generic reduction template for binary operators that are commutative and associative
    template<typename IndexableInputView, typename BinaryFunction>
    inline decltype(auto) reduce(const concurrency::accelerator_view& accl_view,
								 const IndexableInputView& input_view, BinaryFunction binary_op)
    {
        static constexpr int tile_size = 512;
        return _details::reduce<tile_size, 10000>(accl_view, input_view, binary_op);
    }

    template<typename IndexableInputView, typename BinaryFunction>
    inline decltype(auto) reduce(const IndexableInputView& input_view, BinaryFunction binary_op)
    {
        return reduce(_details::auto_select_target(), input_view, binary_op);
    }

    //----------------------------------------------------------------------------
    // scan
    //----------------------------------------------------------------------------

    // This header needs these enums but they also have to be defined in the _impl header for use
	// by the main STL header, which includes the _impl.
#if !defined(AMP_ALGORITHMS_ENUMS)
#define AMP_ALGORITHMS_ENUMS
    enum class scan_mode : int
    {
        exclusive = 0,
        inclusive = 1
    };

    enum class scan_direction : int
    {
        forward = 0,
        backward = 1
    };
#endif

    template<int TileSize, scan_mode _Mode, typename T, typename _BinaryFunc>
    inline void scan(const concurrency::array_view<T>& input_view,
					 const concurrency::array_view<T>& output_view,
					 _BinaryFunc op)
    {
        _details::scan<TileSize, _Mode>(_details::auto_select_target(),
										input_view,
										output_view,
										std::move(op));
    }

    template<typename T>
    inline void scan_exclusive(const concurrency::accelerator_view& accl_view,
							   const concurrency::array_view<T>& input_view,
							   const concurrency::array_view<T>& output_view)
    {
        _details::scan<_details::scan_default_tile_size,
					   amp_algorithms::scan_mode::exclusive>(accl_view,
															 input_view,
															 output_view,
															 amp_algorithms::plus<>());
    }

    template <typename T>
    inline void scan_exclusive(const concurrency::array_view<T>& input_view,
						       const concurrency::array_view<T>& output_view)
    {
        _details::scan<_details::scan_default_tile_size,
					   amp_algorithms::scan_mode::exclusive>(_details::auto_select_target(),
															 input_view,
															 output_view,
															 amp_algorithms::plus<>());
    }

    template <typename T>
    inline void scan_inclusive(const concurrency::accelerator_view& accl_view,
							   const concurrency::array_view<T>& input_view,
							   const concurrency::array_view<T>& output_view)
    {
        _details::scan<_details::scan_default_tile_size,
					   amp_algorithms::scan_mode::inclusive>(accl_view,
															 input_view,
															 output_view,
															 amp_algorithms::plus<>());
    }

    template <typename T>
    inline void scan_inclusive(const concurrency::array_view<T>& input_view,
							   const concurrency::array_view<T>& output_view)
    {
        _details::scan<_details::scan_default_tile_size,
					   amp_algorithms::scan_mode::inclusive>(_details::auto_select_target(),
															 input_view,
															 output_view,
															 amp_algorithms::plus<>());
    }

    //----------------------------------------------------------------------------
    // transform (unary)
    //----------------------------------------------------------------------------

    template<typename T, typename U, int R, typename UnaryFunc>
    inline void transform(const concurrency::accelerator_view& accl_view,
						  const concurrency::array_view<const T, R>& input_view,
						  const concurrency::array_view<U, R>& output_view,
						  UnaryFunc func)
    {
        _details::parallel_for_each(accl_view,
									output_view.extent,
									[=, func = std::move(func)](auto&& idx) restrict(amp) {
            output_view[idx] = func(input_view[idx]);
        });
    }

    template<typename T, typename U, int R, typename UnaryFunc>
    inline void transform(const concurrency::array_view<const T, R>& input_view,
						  const concurrency::array_view<U, R>& output_view,
						  UnaryFunc func)
    {
        ::amp_algorithms::transform(_details::auto_select_target(),
									input_view,
									output_view,
									std::move(func));
    }

    //----------------------------------------------------------------------------
    // transform (binary)
    //----------------------------------------------------------------------------

    template<typename T, typename U, typename V, int R, typename BinaryFunc>
    inline void transform(const concurrency::accelerator_view &accl_view,
						  const concurrency::array_view<const T, R>& input_view1,
						  const concurrency::array_view<const U, R>& input_view2,
						  const concurrency::array_view<V, R>& output_view,
						  BinaryFunc func)
    {
        _details::parallel_for_each(accl_view,
									output_view.extent,
									[=, func = std::move(func)](auto&& idx) restrict(amp) {
            output_view[idx] = func(input_view1[idx], input_view2[idx]);
        });
    }

    template<typename T, typename U, typename V, int R, typename BinaryFunc>
    void transform(const concurrency::array_view<const T, R>& input_view1,
				   const concurrency::array_view<const U, R>& input_view2,
				   const concurrency::array_view<V, R>& output_view,
				   BinaryFunc func)
    {
        ::amp_algorithms::transform(_details::auto_select_target(),
									input_view1,
									input_view2,
									output_view,
									std::move(func));
    }
}	   // namespace amp_algorithms
#endif // _AMP_ALGORITHMS_H_BUMPTZI