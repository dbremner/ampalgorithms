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
* C++ AMP standard algorithms library.
*
* This file contains the iterator classes for C++ AMP containers
*---------------------------------------------------------------------------*/

#pragma once
#ifndef _AMP_ITERATORS_H_BUMPTZI
#define _AMP_ITERATORS_H_BUMPTZI

#include <amp_algorithms.h>
#include <amp_algorithms_type_functions_helpers.h>

#include <amp.h>
#include <iterator>
#include <type_traits>

namespace amp_stl_algorithms
{
    //----------------------------------------------------------------------------
    // array_view_iterator
    //
    // Provides a local iterator for an array_view. Iterators are only comparable
    // for the same array_view (copies of the array_view are shallow, and are OK,
    // i.e., iterators obtained from different copies are comparable).
    // The behavior is undefined on comparing iterators obtained from different
    // array_views.
    //
    // Note: only array_view<T, 1> is supported for now. Ultimately we will become
	// aligned with the array_view proposal that made it into the standard. Work
	// is still underway on getting the conversion between T and const T iterators
	// to work seamlessly and have operators do them transparently.
	// IMPORTANT: these are not tuned for performance (yet). There are some glass
	// jaws, which will be alleviated in the future. Adding move construction and
	// assignment to array_view would also be highly conductive to increased
	// performance.
    //----------------------------------------------------------------------------

	template<typename I, typename T, typename D>
	struct _Iterator_operator_generator : public std::iterator<std::random_access_iterator_tag, T, D> { // This enables (does not impeach) EBCO
		friend bool operator==(const I& x, const I& y) restrict(cpu, amp) { return amp_algorithms::equal_to<I>()(x, y); };
		friend bool operator!=(const I& x, const I& y) restrict(cpu, amp) { return amp_algorithms::not_equal_to<I>()(x, y); };

		friend bool operator<(const I& x, const I& y) restrict(cpu, amp) { return amp_algorithms::less<I>()(x, y); };
		friend bool operator<=(const I& x, const I& y) restrict(cpu, amp) { return amp_algorithms::less_equal<I>()(x, y); };

		friend bool operator>(const I& x, const I& y) restrict(cpu, amp) { return amp_algorithms::greater<I>()(x, y); };
		friend bool operator>=(const I& x, const I& y) restrict(cpu, amp) { return amp_algorithms::greater_equal<I>()(x, y); };

		friend I operator+(const I& it, D n) restrict(cpu, amp) { auto t = it; t += n; return t; };

		friend I operator-(const I& it, D n) restrict(cpu, amp) { return it + (-n); };
		friend I operator-(D n, const I& it) restrict(cpu, amp) { return -(it + (-n)); };
		friend D operator-(const I& it0, const I& it1) restrict(cpu, amp) { return amp_algorithms::minus<I>()(it0, it1); };
	};

	template<typename I, typename T, typename D>
	struct _Array_view_iterator_impl : public _Iterator_operator_generator<I, T, D> {
	public:
		using value_type = std::remove_const_t<T>;

		_Array_view_iterator_impl() restrict(cpu, amp) : av(empty_array_view_factory<T>::create()), idx(-1) {};

		_Array_view_iterator_impl(const _Array_view_iterator_impl&) = default;
		_Array_view_iterator_impl(_Array_view_iterator_impl&&) = default;

		_Array_view_iterator_impl& operator=(const _Array_view_iterator_impl&) = default;
		_Array_view_iterator_impl& operator=(_Array_view_iterator_impl&&) = default;

		~_Array_view_iterator_impl() restrict(cpu, amp) {};

		I& operator++() restrict(cpu, amp) { ++idx; return reinterpret_cast<I&>(*this); };
		I operator++(int) restrict(cpu, amp) { const auto t = *this; ++*this; return reinterpret_cast<const I&>(t); };

		I& operator--() restrict(cpu, amp) { --idx; return reinterpret_cast<I&>(*this); };
		I operator--(int) restrict(cpu, amp) { const auto t = *this; --*this; return reinterpret_cast<const I&>(t); };

		I& operator+=(difference_type n) restrict(cpu, amp) { idx += n; return reinterpret_cast<I&>(*this); };
		I& operator-=(difference_type n) restrict(cpu, amp) { idx -= n; return reinterpret_cast<I&>(*this); };

		reference operator*() const restrict(cpu, amp) { return av[idx]; };
		pointer operator->() const restrict(cpu, amp) { return av.data() + idx; };

		reference operator[](difference_type i) const restrict(cpu, amp) { return av[idx + i]; };
	protected:
		template<typename U>
		explicit _Array_view_iterator_impl(const concurrency::array_view<U>& a, D n) restrict(cpu, amp) : av(a), idx(n) {};

		bool less(const _Array_view_iterator_impl& other) const restrict(cpu, amp) { return idx < other.idx; };
		bool equal(const _Array_view_iterator_impl& other) const restrict(cpu, amp) { return idx == other.idx; };
		difference_type minus(const _Array_view_iterator_impl& other) const restrict(cpu, amp) { return idx - other.idx; };

		concurrency::array_view<T> av;
		difference_type idx;
	};

	namespace _details
	{
		template<typename T>
		struct array_view_iterator_helper;
	}

	//-----------------------------------------------------------------------------
	// Begin const_array_view_iterator
	//-----------------------------------------------------------------------------

	template<typename T> class const_array_view_iterator; // Forward declaration for friendship.

    template<typename T>
    class array_view_iterator : public _Array_view_iterator_impl<array_view_iterator<T>, T, int> {
        friend struct ::amp_stl_algorithms::_details::array_view_iterator_helper<array_view_iterator>;
		friend class const_array_view_iterator<std::remove_const_t<T>>;
		friend class const_array_view_iterator<std::add_const_t<T>>;

		friend struct amp_algorithms::equal_to<array_view_iterator>;
		friend struct amp_algorithms::less<array_view_iterator>;
		friend struct amp_algorithms::minus<array_view_iterator>;
	public:
		array_view_iterator() = default;
		array_view_iterator(const array_view_iterator&) = default;
		array_view_iterator(array_view_iterator&&) = default;

		explicit array_view_iterator(const concurrency::array_view<value_type>& a, difference_type n) restrict(cpu, amp)
			: _Array_view_iterator_impl(a, n)
		{}

		array_view_iterator& operator=(const array_view_iterator&) = default;
		array_view_iterator& operator=(array_view_iterator&&) = default;

		~array_view_iterator() restrict(cpu, amp) {};
    };

    //-----------------------------------------------------------------------------
    // End of array_view_iterator
    //-----------------------------------------------------------------------------

	//-----------------------------------------------------------------------------
	// Begin const_array_view_iterator
	//-----------------------------------------------------------------------------

	template<typename T>
	class const_array_view_iterator : public _Array_view_iterator_impl<const_array_view_iterator<T>, std::add_const_t<T>, int> {
	protected:
		friend struct ::amp_stl_algorithms::_details::array_view_iterator_helper<const_array_view_iterator>;

		friend struct amp_algorithms::equal_to<const_array_view_iterator>;
		friend struct amp_algorithms::less<const_array_view_iterator>;
		friend struct amp_algorithms::minus<const_array_view_iterator>;
	public:
		const_array_view_iterator() = default;
		const_array_view_iterator(const const_array_view_iterator&) = default;
		const_array_view_iterator(const_array_view_iterator&&) = default;

		explicit const_array_view_iterator(const concurrency::array_view<const value_type>& a, difference_type n) restrict(cpu, amp)
			: _Array_view_iterator_impl(a, n)
		{}

		const_array_view_iterator(const array_view_iterator<value_type>& other) restrict(cpu, amp)
			: const_array_view_iterator(other.av, other.idx) {};
		const_array_view_iterator(array_view_iterator<value_type>&& other) restrict(cpu, amp)
			: const_array_view_iterator(move(other.av), move(other.idx)) {};

		const_array_view_iterator(const array_view_iterator<const value_type>& other) restrict(cpu, amp)
			: const_array_view_iterator(other.av, other.idx) {};
		const_array_view_iterator(array_view_iterator<const value_type>&& other) restrict(cpu, amp)
			: const_array_view_iterator(move(other.av), move(other.idx)) {};

		const_array_view_iterator& operator=(const const_array_view_iterator&) = default;
		const_array_view_iterator& operator=(const_array_view_iterator&&) = default;

		const_array_view_iterator& operator=(const array_view_iterator<value_type>& other) restrict(cpu, amp) { av = other.av; idx = other.idx; return *this; };
		const_array_view_iterator& operator=(array_view_iterator<value_type>&& other) restrict(cpu, amp) { av = move(other.av); idx = move(other.idx); return *this; };

		const_array_view_iterator& operator=(const array_view_iterator<const value_type>& other) restrict(cpu, amp) { av = other.av; idx = other.idx; return *this; };
		const_array_view_iterator& operator=(array_view_iterator<const value_type>&& other) restrict(cpu, amp) { av = move(other.av); idx = move(other.idx); return *this; };

		~const_array_view_iterator() restrict(cpu, amp) {};
	};

	//-----------------------------------------------------------------------------
	// End of const_array_view_iterator
	//-----------------------------------------------------------------------------

	//-----------------------------------------------------------------------------
	// Begin reverse_iterator adapter for C++ AMP contexts
	//-----------------------------------------------------------------------------

	template<typename I>
	class reverse_iterator : public _Iterator_operator_generator<reverse_iterator<I>,
																 Value_type<I>,
																 Difference_type<I>> {
		friend struct amp_algorithms::equal_to<reverse_iterator>;
		friend struct amp_algorithms::less<reverse_iterator>;
		friend struct amp_algorithms::minus<reverse_iterator>;
	public:
		using reference = typename std::iterator_traits<I>::reference;
		using pointer = typename std::iterator_traits<I>::pointer;

		reverse_iterator() = default;
		reverse_iterator(const reverse_iterator&) = default;
		reverse_iterator(reverse_iterator&&) = default;

		reverse_iterator(I i) restrict(cpu, amp) : it(amp_stl_algorithms::move(i)) {};

		reverse_iterator& operator=(const reverse_iterator&) = default;
		reverse_iterator& operator=(reverse_iterator&&) = default;

		~reverse_iterator() restrict(cpu, amp) {}; // This should be defaulted.

		I base() const restrict(cpu, amp) { return it; };

		reverse_iterator& operator++() restrict(cpu, amp) { --it; return *this; };
		reverse_iterator operator++(int) restrict(cpu, amp) { const auto t = *this; --*this; return t; };

		reverse_iterator& operator--() restrict(cpu, amp) { ++it; return *this; };
		reverse_iterator operator--(int) restrict(cpu, amp) { const auto t = *this; ++*this; return t; };

		reverse_iterator& operator+=(difference_type n) restrict(cpu, amp) { it -= n; return *this; };
		reverse_iterator& operator-=(difference_type n) restrict(cpu, amp) { it += n; return *this; };

		reference operator*() const restrict(cpu, amp) { auto t = it; return *--t; };
		pointer operator->() const restrict(cpu, amp) { auto t = it; return --it.data(); };

		reference operator[](difference_type i) const restrict(cpu, amp) { return it[-i - 1]; };
	private:
		I it;
	};

	// Helper for making reverse iterators.
	template<typename I>
	inline reverse_iterator<I> make_reverse_iterator(I it) restrict(cpu, amp)
	{
		return amp_stl_algorithms::reverse_iterator<I>(amp_stl_algorithms::move(it));
	}

	//-----------------------------------------------------------------------------
	// End reverse_iterator adapter for C++ AMP contexts
	//-----------------------------------------------------------------------------

    namespace _details
    {
		template<typename I>
        struct array_view_iterator_helper {
            static auto get_base_array_view(const I& it) restrict(cpu, amp) -> decltype(it.av) // Temporary fix for compiler fail with decltype(auto)
            {
                return it.av;
            }
			static decltype(auto) get_offset_array_view(I& it) restrict(cpu, amp)
			{
				return concurrency::array_view<Difference_type<I>>(1, &it.idx);
			}
        };

		template<typename I>
		static inline decltype(auto) create_section(I iter, Difference_type<I> dist)
		{
			auto base_view = array_view_iterator_helper<I>::get_base_array_view(iter);
			return base_view.section(concurrency::index<1>(iter - amp_stl_algorithms::begin(base_view)), concurrency::extent<1>(dist));
		}
    }

    ///////////////////////////////////////////////////////////////////////////////
    // iterator_traits
    //
    // Provides typedefs that programmers could use to define iterators. e.g.,
    //
    // iterator_traits<array_view<float,2>>::const_iterator_type myAvIter;
    //
    // In future release, this should be offered as member typedefs of the
    // respective classes.
    //
    ///////////////////////////////////////////////////////////////////////////////

    template <typename array_type>
    class iterator_traits
    {
        iterator_traits()
        {
            static_assert(false, "This class must be specialized");
        }
    };

    template<typename value_type>
    class iterator_traits<concurrency::array_view<value_type>>
    {
    public:
        typedef array_view_iterator<value_type> iterator_type;
        typedef const_array_view_iterator<value_type> const_iterator_type;
    };

    template<typename value_type>
    class iterator_traits<concurrency::array_view<const value_type>>
    {
    public:
        typedef const_array_view_iterator<value_type> const_iterator_type;
    };

    //----------------------------------------------------------------------------
    // begin and end iterators for array views
    //----------------------------------------------------------------------------

    template<typename T, typename = std::enable_if_t<!std::is_const<T>::value>>
    inline array_view_iterator<T> begin(const concurrency::array_view<T>& av) restrict(cpu,amp)
    {
        return array_view_iterator<T>(av, 0);
    }

	template<typename T, typename = std::enable_if_t<std::is_const<T>::value>>
	inline const_array_view_iterator<std::remove_const_t<T>> begin(const concurrency::array_view<T>& av) restrict(cpu, amp)
	{
		return const_array_view_iterator<std::remove_const_t<T>>(av, 0);
	}

    template<typename T, typename = std::enable_if_t<!std::is_const<T>::value>>
    inline array_view_iterator<T> end(const concurrency::array_view<T>& av) restrict(cpu,amp)
    {
        return array_view_iterator<T>(av, av.get_extent().size());
    }

	template<typename T, typename = std::enable_if_t<std::is_const<T>::value>>
	inline const_array_view_iterator<std::remove_const_t<T>> end(const concurrency::array_view<T>& av) restrict(cpu, amp)
	{
		return const_array_view_iterator<std::remove_const_t<T>>(av, av.get_extent().size());
	}

    //----------------------------------------------------------------------------
    // cbegin and cend iterators for array views
    //----------------------------------------------------------------------------

    template<typename T>
    inline const_array_view_iterator<std::remove_const_t<T>> cbegin(const concurrency::array_view<T>& av) restrict(cpu, amp)
    {
        return const_array_view_iterator<std::remove_const_t<T>>(av, 0);
    }

    template<typename T>
    inline const_array_view_iterator<std::remove_const_t<T>> cend(const concurrency::array_view<T>& av) restrict(cpu, amp)
    {
        return const_array_view_iterator<std::remove_const_t<T>>(av, av.get_extent().size());
    }

	//----------------------------------------------------------------------------
	// reverse constant and non-constant, begin and end iterators for array views
	//----------------------------------------------------------------------------

	template<typename T>
	inline decltype(auto) rbegin(const concurrency::array_view<T>& av) restrict(cpu, amp)
	{
		return ::amp_stl_algorithms::make_reverse_iterator(end(av));
	}

	template<typename T>
	inline decltype(auto) rend(const concurrency::array_view<T>& av) restrict(cpu, amp)
	{
		return ::amp_stl_algorithms::make_reverse_iterator(begin(av) + 1);
	}

	template<typename T>
	inline decltype(auto) crbegin(const concurrency::array_view<T>& av) restrict(cpu, amp)
	{
		return ::amp_stl_algorithms::make_reverse_iterator(cend(av));
	}

	template<typename T>
	inline decltype(auto) crend(const concurrency::array_view<T>& av) restrict(cpu, amp)
	{
		return ::amp_stl_algorithms::make_reverse_iterator(cbegin(av) + 1);
	}

    //----------------------------------------------------------------------------
    // Zip iterator
    //----------------------------------------------------------------------------
}	   // namespace amp_stl_algorithms

namespace amp_algorithms
{
	//-----------------------------------------------------------------------------
	// Specializations for less, equal_to and minus for the iterators.
	// These will eventually be folded into the main namespace (whichever that is).
	//-----------------------------------------------------------------------------

	template<typename T>
	struct equal_to<amp_stl_algorithms::array_view_iterator<T>> {
		bool operator()(const amp_stl_algorithms::array_view_iterator<T>& i0, const amp_stl_algorithms::array_view_iterator<T>& i1) const restrict(cpu, amp)
		{
			return i0.equal(i1);
		}
	};

	template<typename T>
	struct less<amp_stl_algorithms::array_view_iterator<T>> {
		bool operator()(const amp_stl_algorithms::array_view_iterator<T>& i0, const amp_stl_algorithms::array_view_iterator<T>& i1) const restrict(cpu, amp)
		{
			return i0.less(i1);
		}
	};

	template<typename T>
	struct minus<amp_stl_algorithms::array_view_iterator<T>> {
		amp_stl_algorithms::Difference_type<amp_stl_algorithms::array_view_iterator<T>> operator()(const amp_stl_algorithms::array_view_iterator<T>& i0,
																								   const amp_stl_algorithms::array_view_iterator<T>& i1) const restrict(cpu, amp)
		{
			return i0.minus(i1);
		}
	};

	template<typename T>
	struct equal_to<amp_stl_algorithms::const_array_view_iterator<T>> {
		bool operator()(const amp_stl_algorithms::const_array_view_iterator<T>& i0, const amp_stl_algorithms::const_array_view_iterator<T>& i1) const restrict(cpu, amp)
		{
			return i0.equal(i1);
		}
	};

	template<typename T>
	struct less<amp_stl_algorithms::const_array_view_iterator<T>> {
		bool operator()(const amp_stl_algorithms::const_array_view_iterator<T>& i0, const amp_stl_algorithms::const_array_view_iterator<T>& i1) const restrict(cpu, amp)
		{
			return i0.less(i1);
		}
	};

	template<typename T>
	struct minus<amp_stl_algorithms::const_array_view_iterator<T>> {
		amp_stl_algorithms::Difference_type<amp_stl_algorithms::const_array_view_iterator<T>> operator()(const amp_stl_algorithms::const_array_view_iterator<T>& i0,
																										 const amp_stl_algorithms::const_array_view_iterator<T>& i1) const restrict(cpu, amp)
		{
			return i0.minus(i1);
		}
	};

	template<typename I>
	struct equal_to<amp_stl_algorithms::reverse_iterator<I>> {
		bool operator()(const amp_stl_algorithms::reverse_iterator<I>& i0, const amp_stl_algorithms::reverse_iterator<I>& i1) const restrict(cpu, amp)
		{
			return i0.it == i1.it;
		}
	};

	template<typename I>
	struct less<amp_stl_algorithms::reverse_iterator<I>> {
		bool operator()(const amp_stl_algorithms::reverse_iterator<I>& i0, const amp_stl_algorithms::reverse_iterator<I>& i1) const restrict(cpu, amp)
		{
			return i1.it < i0.it;
		}
	};

	template<typename I>
	struct minus<amp_stl_algorithms::reverse_iterator<I>> {
		amp_stl_algorithms::Difference_type<amp_stl_algorithms::reverse_iterator<I>> operator()(const amp_stl_algorithms::reverse_iterator<I>& i0,
																								const amp_stl_algorithms::reverse_iterator<I>& i1) const restrict(cpu, amp)
		{
			return i1.it - i0.it;
		}
	};
}	   // namespace amp_algorithms
#endif // _AMP_ITERATORS_H_BUMPTZI