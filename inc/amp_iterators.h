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

#include <amp.h>
#include <iterator>
#include <type_traits>
#include <xx_amp_stl_algorithms_impl.h>

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
    //----------------------------------------------------------------------------

    namespace _details
    {
        template <typename T>
        struct array_view_iterator_helper;
    }

    template<typename> class array_view_iterator;
    template<typename T> typename array_view_iterator<T>::difference_type operator-(const array_view_iterator<T>&, const array_view_iterator<T>&) restrict(cpu, amp);
    template<typename T> bool operator==(const array_view_iterator<T>&, const array_view_iterator<T>&) restrict(cpu, amp);
    template<typename T> bool operator<(const array_view_iterator<T>&, const array_view_iterator<T>&) restrict(cpu, amp);

    template<typename T>
    class array_view_iterator : public std::iterator<std::random_access_iterator_tag, T, int> {
        friend struct _details::array_view_iterator_helper<array_view_iterator>;
		template<typename> friend class array_view_iterator;

        friend difference_type operator- <>(const array_view_iterator&, const array_view_iterator&) restrict(cpu, amp);
        friend bool operator== <>(const array_view_iterator&, const array_view_iterator&) restrict(cpu, amp);
        friend bool operator< <>(const array_view_iterator&, const array_view_iterator&) restrict(cpu, amp);
	public:
		using value_type = std::remove_const_t<T>;

		array_view_iterator() restrict(cpu, amp) : av(_details::empty_array_view_factory<T>::create()), idx(-1) {};
        array_view_iterator(const array_view_iterator&) = default;
        array_view_iterator(array_view_iterator&&) = default;

        array_view_iterator(const concurrency::array_view<T>& a, difference_type n = 0) restrict(cpu, amp)
            : av(a), idx(n)
        {};

		template<typename U, std::enable_if_t<std::is_const<T>::value && !std::is_const<U>::value && std::is_same<U, std::remove_const_t<T>>::value>* = nullptr>
		array_view_iterator(const array_view_iterator<U>& it) restrict(cpu, amp)
			: av(it.av), idx(it.idx)
		{}

        array_view_iterator& operator=(const array_view_iterator&) = default;
        array_view_iterator& operator=(array_view_iterator&&) = default;

		template<typename U, std::enable_if_t<std::is_const<T>::value && !std::is_const<U>::value && std::is_same<U, std::remove_const_t<T>>::value>* = nullptr>
		array_view_iterator& operator=(const array_view_iterator<U>& it) restrict(cpu, amp)
		{
			av = it.av;
			idx = it.idx;
			return *this;
		}

		~array_view_iterator() restrict(cpu, amp) {}; // This should be defaulted.

        array_view_iterator& operator++() restrict(cpu, amp)
        {
            ++idx;
            return *this;
        }
        array_view_iterator operator++(int) restrict(cpu, amp)
        {
            auto t = *this;
            ++*this;
            return t;
        }

        array_view_iterator& operator--() restrict(cpu, amp)
        {
            --idx;
            return *this;
        }
        array_view_iterator operator--(int) restrict(cpu, amp)
        {
            auto t = *this;
            --*this;
            return t;
        }

        array_view_iterator& operator+=(difference_type n) restrict(cpu, amp)
        {
            idx += n;
            return *this;
        }
        array_view_iterator& operator-=(difference_type n) restrict(cpu, amp)
        {
            *this += -n;
            return *this;
        }

        reference operator*() restrict(cpu, amp) { return av[idx]; };
        const reference operator*() const restrict(cpu, amp) { return av[idx]; };

        pointer operator->() restrict(cpu, amp) { return av.data() + idx; };
        const pointer operator->() const restrict(cpu, amp) { return av.data() + idx; };

        reference operator[](difference_type i) restrict(cpu, amp) { return av[idx + i]; };
        const reference operator[](difference_type i) const restrict(cpu, amp) { return av[idx + i]; };
    private:
        concurrency::array_view<T> av;
        difference_type idx;
    };

    template<typename T>
    inline array_view_iterator<T> operator+(const array_view_iterator<T>& it, typename array_view_iterator<T>::difference_type n) restrict(cpu, amp)
    {
        auto t = it;
        t += n;
        return t;
    }
    template<typename T>
    inline array_view_iterator<T> operator+(typename array_view_iterator<T>::difference_type n, const array_view_iterator<T>& it) restrict(cpu, amp)
    {
        return it + n;
    }

    template<typename T>
    inline array_view_iterator<T> operator-(const array_view_iterator<T>& it, typename array_view_iterator<T>::difference_type n) restrict(cpu, amp)
    {
        auto t = it;
        t -= n;
        return t;
    }

    template<typename T>
    inline typename array_view_iterator<T>::difference_type operator-(const array_view_iterator<T>& it_0, const array_view_iterator<T>& it_1) restrict(cpu, amp)
    {
        return it_0.idx - it_1.idx;
    }

    template<typename T>
    inline bool operator==(const array_view_iterator<T>& it_0, const array_view_iterator<T>& it_1) restrict(cpu, amp)
    {
        return it_0.idx == it_1.idx;
    }
    template<typename T>
    inline bool operator!=(const array_view_iterator<T>& it_0, const array_view_iterator<T>& it_1) restrict(cpu, amp)
    {
        return !(it_0 == it_1);
    }

    template<typename T>
    inline bool operator<(const array_view_iterator<T>& it_0, const array_view_iterator<T>& it_1) restrict(cpu, amp)
    {
        return it_0.idx < it_1.idx;
    }

    template<typename T>
    inline bool operator<=(const array_view_iterator<T>& it_0, const array_view_iterator<T>& it_1) restrict(cpu, amp)
    {
        return !(it_1 < it_0);
    }
    template<typename T>
    inline bool operator>(const array_view_iterator<T>& it_0, const array_view_iterator<T>& it_1) restrict(cpu, amp)
    {
        return !(it_0 <= it_1);
    }
    template<typename T>
    inline bool operator>=(const array_view_iterator<T>& it_0, const array_view_iterator<T>& it_1) restrict(cpu, amp)
    {
        return !(it_0 < it_1);
    }

    //-----------------------------------------------------------------------------
    // End of array_view_iterator
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

    template <typename value_type>
    class iterator_traits<concurrency::array_view<value_type>>
    {
    public:
        typedef array_view_iterator<value_type> iterator_type;
        typedef array_view_iterator<const value_type> const_iterator_type;
    };

    template <typename value_type>
    class iterator_traits<concurrency::array_view<const value_type>>
    {
    public:
        typedef array_view_iterator<value_type> const_iterator_type;
    };

    //----------------------------------------------------------------------------
    // begin and end iterators for array views
    //----------------------------------------------------------------------------

    template<typename T>
    inline array_view_iterator<T> begin(const concurrency::array_view<T>& av) restrict(cpu,amp)
    {
        return array_view_iterator<T>(av, 0);
    }

    template<typename T>
    inline array_view_iterator<T> end(const concurrency::array_view<T>& av) restrict(cpu,amp)
    {
        return array_view_iterator<T>(av, av.get_extent().size());
    }

    //----------------------------------------------------------------------------
    // cbegin and cend iterators for array views
    //----------------------------------------------------------------------------

    template<typename T>
    inline array_view_iterator<const T> cbegin(const concurrency::array_view<T>& av) restrict(cpu, amp)
    {
        return array_view_iterator<const T>(av, 0);
    }

    template<typename T>
    inline array_view_iterator<const T> cend(const concurrency::array_view<T>& av) restrict(cpu, amp)
    {
        return array_view_iterator<const T>(av, av.get_extent().size());
    }

    //----------------------------------------------------------------------------
    // Zip iterator
    //----------------------------------------------------------------------------


} // amp_stl_algorithms