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
* This file contains a tuple ADT that is a subset of the std::tuple one,
* usable in restrict(amp) contexts.
*---------------------------------------------------------------------------*/

#pragma once
#ifndef _AMP_ALGORITHMS_TUPLE_H_BUMPTZI
#define _AMP_ALGORITHMS_TUPLE_H_BUMPTZI

namespace amp_stl_algorithms
{
	//----------------------------------------------------------------------------
	//  tuple<... T>
	//----------------------------------------------------------------------------

	// http://mitchnull.blogspot.com/2012/06/c11-tuple-implementation-details-part-1.html
	// TODO_NOT_IMPLEMENTED: Tuple<...T>
	/*
	template <typename... T>
	class tuple;

	namespace _details
	{

	}

	template <typename... T>
	class tuple {
	public:
	tuple();

	explicit tuple(const T&...);

	template <typename... U>
	explicit tuple(U&&...);

	tuple(const tuple&);

	tuple(tuple&&);

	template <typename... U>
	tuple(const tuple<U...>&);

	template <typename... U>
	tuple(tuple<U...>&&);

	tuple& operator=(const tuple&);

	tuple& operator=(tuple&&);

	template <typename... U>
	tuple& operator=(const tuple<U...>&);

	template <typename... U>
	tuple& operator=(tuple<U...>&&);

	void swap(tuple&);
	};

	template <typename... T> typename tuple_size<tuple<T...>>;

	template <size_t I, typename... T> typename tuple_element<I, tuple<T...>>;

	// element access:

	template <size_t I, typename... T>
	typename tuple_element<I, tuple<T...>>::type&
	get(tuple<T...>&);

	template <size_t I, typename... T>
	typename tuple_element<I, tuple<T...>>::type const&
	get(const tuple<T...>&);

	template <size_t I, typename... T>
	typename tuple_element<I, tuple<T...>>::type&&
	get(tuple<T...>&&);

	// relational operators:

	template<typename... T, typename... U>
	bool operator==(const tuple<T...>&, const tuple<U...>&);

	template<typename... T, typename... U>
	bool operator<(const tuple<T...>&, const tuple<U...>&);

	template<typename... T, typename... U>
	bool operator!=(const tuple<T...>&, const tuple<U...>&);

	template<typename... T, typename... U>
	bool operator>(const tuple<T...>&, const tuple<U...>&);

	template<typename... T, typename... U>
	bool operator<=(const tuple<T...>&, const tuple<U...>&);

	template<typename... T, typename... U>
	bool operator>=(const tuple<T...>&, const tuple<U...>&);

	template <typename... Types>
	void swap(tuple<Types...>& x, tuple<Types...>& y);
	*/
}	   // namespace amp_stl_algorithms
#endif // _AMP_ALGORITHMS_TUPLE_H_BUMPTZI