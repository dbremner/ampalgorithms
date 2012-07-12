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
 * C++ AMP standard algorithm library.
 *
 * This file contains the C++ AMP standard algorithms
 *---------------------------------------------------------------------------*/
#pragma once

#include <xx_amp_algorithms_impl.h>
#include <amp_iterators.h>
#include <amp_algorithms.h>

namespace amp_stl_algorithms
{
//----------------------------------------------------------------------------
// all_of, any_of, none_of
//----------------------------------------------------------------------------
template<typename ConstRandomAccessIterator,  typename UnaryPredicate>
bool all_of(ConstRandomAccessIterator first, ConstRandomAccessIterator last, UnaryPredicate p );

template<typename ConstRandomAccessIterator, typename UnaryPredicate>
bool any_of(ConstRandomAccessIterator first, ConstRandomAccessIterator last, UnaryPredicate p );

template<typename ConstRandomAccessIterator, typename UnaryPredicate>
bool none_of( ConstRandomAccessIterator first, ConstRandomAccessIterator last, UnaryPredicate p ); 

// non-standard versions which store the result on the accelerator
template<typename ConstRandomAccessIterator,  typename UnaryPredicate, typename OutputIterator>
void all_of(ConstRandomAccessIterator first, ConstRandomAccessIterator last, UnaryPredicate p, OutputIterator dest );

template<typename ConstRandomAccessIterator,  typename UnaryPredicate, typename OutputIterator>
void any_of(ConstRandomAccessIterator first, ConstRandomAccessIterator last, UnaryPredicate p, OutputIterator dest );

template<typename ConstRandomAccessIterator,  typename UnaryPredicate, typename OutputIterator>
void none_of(ConstRandomAccessIterator first, ConstRandomAccessIterator last, UnaryPredicate p, OutputIterator dest );

//----------------------------------------------------------------------------
// for_each
//----------------------------------------------------------------------------
template<typename ConstRandomAccessIterator, typename UnaryFunction>
UnaryFunction for_each( ConstRandomAccessIterator first, ConstRandomAccessIterator last, UnaryFunction f );

// non-standard: no return
template<typename ConstRandomAccessIterator, typename UnaryFunction>
void for_each_no_return( ConstRandomAccessIterator first, ConstRandomAccessIterator last, UnaryFunction f );

//----------------------------------------------------------------------------
// count, count_if
//----------------------------------------------------------------------------
template<typename ConstRandomAccessIterator, typename T>
typename std::iterator_traits<ConstRandomAccessIterator>::difference_type
count( ConstRandomAccessIterator first, ConstRandomAccessIterator last, const T &value ); 

template<typename ConstRandomAccessIterator, typename UnaryPredicate>
typename std::iterator_traits<ConstRandomAccessIterator>::difference_type
count_if( ConstRandomAccessIterator first, ConstRandomAccessIterator last, UnaryPredicate p ); 

//----------------------------------------------------------------------------
// find, find_if
//----------------------------------------------------------------------------
template<typename ConstRandomAccessIterator, typename T>
ConstRandomAccessIterator find( ConstRandomAccessIterator first, ConstRandomAccessIterator last, const T& value );

template<typename ConstRandomAccessIterator, typename UnaryPredicate>
ConstRandomAccessIterator find_if(ConstRandomAccessIterator first, ConstRandomAccessIterator last, UnaryPredicate p );

template<typename RandomAccessIterator, typename T>
void fill( RandomAccessIterator first, RandomAccessIterator last, const T& value );

template<typename RandomAccessIterator, typename Size, typename T>
void fill_n( RandomAccessIterator first, Size count, const T& value );

template<typename ConstRandomAccessIterator,typename RandomAccessIterator, typename UnaryFunction>
RandomAccessIterator transform( ConstRandomAccessIterator first1, 
						        ConstRandomAccessIterator last1, 
						        RandomAccessIterator result,
						        UnaryFunction func);

template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2,typename RandomAccessIterator, typename BinaryFunction>
RandomAccessIterator transform( ConstRandomAccessIterator1 first1, 
						        ConstRandomAccessIterator1 last1,
						        ConstRandomAccessIterator2 first2, 
						        RandomAccessIterator result,
						        BinaryFunction func);

template<typename RandomAccessIterator, typename Generator>
void generate( RandomAccessIterator first, RandomAccessIterator last, Generator g );

template<typename RandomAccessIterator, typename Size, typename Generator>
void generate_n( RandomAccessIterator first, Size count, Generator g );

template<typename RandomAccessIterator>
void sort( RandomAccessIterator first, RandomAccessIterator last );

template<typename RandomAccessIterator, typename Compare>
void sort( RandomAccessIterator first, RandomAccessIterator last, Compare comp ); 

 // TODO: consider supporting the heap functions (is_heap et al)

#ifdef max
#error amp_stl_algorithms encoutered a definition of the macro max
#endif

template<typename T> 
const T& max( const T& a, const T& b ) restrict (cpu,amp);

template<typename T, typename Compare>
const T& max( const T& a, const T& b, Compare comp ) restrict (cpu,amp);

#ifdef max
#error amp_stl_algorithms encoutered a definition of the macro max
#endif

template<typename T> 
const T& min( const T& a, const T& b ) restrict (cpu,amp);

template<typename T, typename Compare>
const T& min( const T& a, const T& b, Compare comp ) restrict (cpu,amp);

template<typename T> 
std::pair<const T&,const T&> minmax( const T& a, const T& b ) restrict (cpu,amp);

template<typename T, typename Compare>
std::pair<const T&,const T&> minmax( const T& a, const T& b, Compare comp ) restrict (cpu,amp);

// TODO: enable initializer list in amp restricted code
//
// template<typename T>
// T max( std::initializer_list<T> ilist) restrict(cpu,amp);
//
// template<typename T, typename Compare>
// T max( std::initializer_list<T> ilist, Compare comp ); 
//
// template<typename T>
// T min( std::initializer_list<T> ilist) restrict(cpu,amp);
//
// template<typename T, typename Compare>
// T min( std::initializer_list<T> ilist, Compare comp ); 
//
// template<typename T>
// std::pair<T,T> minmax( std::initializer_list ilist);
//
// template<typename T, typename Compare>
// std::pair<T,T> minmax( std::initializer_list ilist, Compare comp ); 

template<typename ConstRandomAccessIterator> 
ConstRandomAccessIterator max_element( ConstRandomAccessIterator first, ConstRandomAccessIterator last );

template<typename ConstRandomAccessIterator, typename Compare>
ConstRandomAccessIterator max_element( ConstRandomAccessIterator first, ConstRandomAccessIterator last, Compare comp ); 

template<typename ConstRandomAccessIterator> 
ConstRandomAccessIterator min_element( ConstRandomAccessIterator first, ConstRandomAccessIterator last );

template<typename ConstRandomAccessIterator, typename Compare>
ConstRandomAccessIterator min_element( ConstRandomAccessIterator first, ConstRandomAccessIterator last, Compare comp ); 

template<typename ConstRandomAccessIterator> 
std::pair<ConstRandomAccessIterator,ConstRandomAccessIterator> 
minmax_element( ConstRandomAccessIterator first, ConstRandomAccessIterator last ); 

template<typename ConstRandomAccessIterator, typename Compare>
std::pair<ConstRandomAccessIterator,ConstRandomAccessIterator> 
minmax_element( ConstRandomAccessIterator first, ConstRandomAccessIterator last, Compare comp ); 

// non-standard
template<typename ConstRandomAccessIterator, typename T>
T reduce( ConstRandomAccessIterator first, ConstRandomAccessIterator last, T init );

// non-standard
template<typename ConstRandomAccessIterator, typename T, typename BinaryOperation>
T reduce( ConstRandomAccessIterator first, ConstRandomAccessIterator last, T init, BinaryOperation op ); 

template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2, typename T>
T inner_product( ConstRandomAccessIterator1 first1, 
				 ConstRandomAccessIterator1 last1,
                 ConstRandomAccessIterator2 first2, 
				 T value ); 

template<
    typename ConstRandomAccessIterator1,
    typename ConstRandomAccessIterator2,
    typename T,
    typename BinaryOperation1,
    typename BinaryOperation2>
T inner_product( ConstRandomAccessIterator1 first1, 
                 ConstRandomAccessIterator1 last1,
                 ConstRandomAccessIterator2 first2,
				 T value,
                 BinaryOperation1 op1,
                 BinaryOperation2 op2 ); 

template<typename ConstRandomAccessIterator,typename RandomAccessIterator>
RandomAccessIterator adjacent_difference( ConstRandomAccessIterator first, 
                                          ConstRandomAccessIterator last, 
                                          RandomAccessIterator d_first ); 

template<typename ConstRandomAccessIterator,typename RandomAccessIterator, typename BinaryOperation>
RandomAccessIterator adjacent_difference( ConstRandomAccessIterator first, 
								          ConstRandomAccessIterator last, 
                                          RandomAccessIterator d_first,
                                          BinaryOperation op );

template <typename ConstRandomAccessIterator,typename RandomAccessIterator>
RandomAccessIterator partial_sum( ConstRandomAccessIterator first, 
							      ConstRandomAccessIterator last,
                                  RandomAccessIterator result );

template <typename ConstRandomAccessIterator,typename RandomAccessIterator, typename BinaryOperation>
RandomAccessIterator partial_sum( ConstRandomAccessIterator first, 
							      ConstRandomAccessIterator last,
                                  RandomAccessIterator result, 
							      BinaryOperation binary_op );

}// namespace amp_stl_algorithms

#include <xx_amp_stl_algorithms_impl_inl.h>
