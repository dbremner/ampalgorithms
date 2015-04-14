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
* This file contains a pair class that is compatible with std::pair and usable
* in restrict(amp) contexts.
*---------------------------------------------------------------------------*/

#pragma once
#ifndef _AMP_ALGORITHMS_PAIR_H_BUMPTZI
#define _AMP_ALGORITHMS_PAIR_H_BUMPTZI

#include <amp_algorithms.h>

#include <utility>
#include <type_traits>

namespace amp_stl_algorithms
{
	//----------------------------------------------------------------------------
	// pair<T1, T2>
	//----------------------------------------------------------------------------

	template<typename T1, typename T2>
	struct pair {
		using first_type = T1;
		using second_type = T2;

		static constexpr unsigned int AMP_aligned = sizeof(unsigned int);

		alignas(AMP_aligned) T1 first;
		alignas(AMP_aligned) T2 second;

		/*constexpr*/ pair() = default;

		template<typename U1, typename U2>
		/*constexpr*/ pair(const pair<U1, U2>& other) restrict(cpu, amp)
			: first(other.first), second(other.second)
		{}

		template<typename U1, typename U2>
		/*constexpr*/ pair(pair<U1, U2>&& other) restrict(cpu, amp)
			: first(move(other.first)), second(move(other.second))
		{}

		/*constexpr*/ pair(const pair&) = default;
		/*constexpr*/ pair(pair&&) = default;

		/*constexpr*/ pair(const T1& x, const T2& y) restrict(cpu, amp)
			: first(x), second(y)
		{};

		template<typename U1, typename U2>
		/*constexpr*/ pair(U1&& x, U2&& y) restrict(cpu, amp)
			: first(forward<U1>(x)), second(forward<U2>(y))
		{}

		pair& operator=(const pair&) = default;
		pair& operator=(pair&&) = default;

		template<typename U1, typename U2>
		pair& operator=(const pair<U1, U2>& other) restrict(cpu, amp)
		{
			first = other.first;
			second = other.second;
			return *this;
		}

		template<typename U1, typename U2>
		pair& operator=(pair<U1, U2>&& other) restrict(cpu, amp)
		{
			first = move(other.first);
			second = move(other.second);
			return *this;
		}

		void swap(pair& other) restrict(cpu, amp)
		{
			swap(first, other.first);
			swap(second, other.second);
		}

		// Support interop with std::pair.

		template<typename U1, typename U2>
		pair(const std::pair<U1, U2>& other) restrict(cpu, amp)
			: first(other.first), second(other.second)
		{}

		template<typename U1, typename U2>
		pair(std::pair<U1, U2>&& other) restrict(cpu, amp)
			: first(move(other.first), move(other.second))
		{}

		pair(const std::pair<T1, T2>& other) restrict(cpu, amp)
			: first(other.first), second(other.second)
		{}

		pair(std::pair<T1, T2>&& other) restrict(cpu, amp)
			: first(move(other.first)), second(move(other.second))
		{}

		operator std::pair<T1, T2>() const restrict(cpu)
		{
			return std::make_pair(first, second);
		}
	};

	template<typename T1, typename T2>
	inline constexpr bool operator==(const amp_stl_algorithms::pair<T1, T2>& _Left,
									 const amp_stl_algorithms::pair<T1, T2>& _Right) restrict(cpu, amp)
	{
		return amp_algorithms::equal_to<>()(_Left.first, _Right.first) &&
			   amp_algorithms::equal_to<>()(_Left.second, _Right.second);
	}

	template<typename T1, typename T2>
	inline constexpr bool operator!=(const amp_stl_algorithms::pair<T1, T2>& _Left,
									 const amp_stl_algorithms::pair<T1, T2>& _Right) restrict(cpu, amp)
	{
		return !(_Left == _Right);
	}

	template<typename T1, typename T2>
	inline constexpr bool operator<(const amp_stl_algorithms::pair<T1, T2>& _Left,
									const amp_stl_algorithms::pair<T1, T2>& _Right) restrict(cpu, amp)
	{
		if (amp_algorithms::less<>()(_Left.first, _Right.first)) return true;
		if (amp_algorithms::less<>()(_Right.first, _Left.first)) return false;
		return amp_algorithms::less<>()(_Left.second, _Right.second);
	}

	template<typename T1, typename T2>
	inline constexpr bool operator<=(const amp_stl_algorithms::pair<T1, T2>& _Left,
									 const amp_stl_algorithms::pair<T1, T2>& _Right) restrict(cpu, amp)
	{
		return !(_Right < _Left);
	}

	template<typename T1, typename T2>
	inline constexpr bool operator>(const amp_stl_algorithms::pair<T1, T2>& _Left,
									const amp_stl_algorithms::pair<T1, T2>& _Right) restrict(cpu, amp)
	{
		return !(_Left <= _Right);
	}

	template<typename T1, typename T2>
	inline constexpr bool operator>=(const amp_stl_algorithms::pair<T1, T2>& _Left,
									 const amp_stl_algorithms::pair<T1, T2>& _Right) restrict(cpu, amp)
	{
		return !(_Left < _Right);
	}

	template<typename T1, typename T2>
	void swap(amp_stl_algorithms::pair<T1, T2>& _Left, amp_stl_algorithms::pair<T1, T2>& _Right) restrict(cpu, amp)
	{
		_Left.swap(_Right);
	}

	template<typename T1, typename T2>
	inline constexpr decltype(auto) make_pair(T1&& x, T2&& y) restrict(cpu, amp)
	{
		return amp_stl_algorithms::pair<std::remove_reference_t<std::remove_pointer_t<T1>>,
										std::remove_reference_t<std::remove_pointer_t<T2>>>(forward<T1>(x),
																							forward<T2>(y));
	}
	// Still incomplete!
}	   // namespace amp_stl_algorithms
#endif // _AMP_ALGORITHMS_PAIR_H_BUMPTZI