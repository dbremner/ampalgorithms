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
* This file contains the C++ AMP standard library algorithms
*---------------------------------------------------------------------------*/

#pragma once
#ifndef _AMP_STL_ALGORITHMS_H_BUMPTZI
#define _AMP_STL_ALGORITHMS_H_BUMPTZI

#include <xx_amp_stl_algorithms_impl.h>

#include <type_traits>
#include <utility>

// TODO: Get the tests, header and internal implementations into the same logical order.
// TODO_NOT_IMPLEMENTED: consider supporting the heap functions (is_heap etc)

namespace amp_stl_algorithms
{
    //----------------------------------------------------------------------------
    // adjacent_difference
    //----------------------------------------------------------------------------

    template<typename ConstRandomAccessIterator,typename RandomAccessIterator>
    RandomAccessIterator adjacent_difference(ConstRandomAccessIterator first,
											 ConstRandomAccessIterator last,
											 RandomAccessIterator dest_first)
	{
		return _adjacent_difference(first, last, dest_first);
	}

    template<typename ConstRandomAccessIterator,typename RandomAccessIterator, typename BinaryOperation>
	RandomAccessIterator adjacent_difference(ConstRandomAccessIterator first,
											 ConstRandomAccessIterator last,
											 RandomAccessIterator dest_first,
											 BinaryOperation op)
	{
		return _adjacent_difference(first, last, dest_first, std::move(op));
	}

    //----------------------------------------------------------------------------
    // all_of, any_of, none_of
    //----------------------------------------------------------------------------

    template<typename ConstRandomAccessIterator,  typename UnaryPredicate>
	bool all_of(ConstRandomAccessIterator first, ConstRandomAccessIterator last, UnaryPredicate p)
	{
		return _all_of(first, last, std::move(p));
	}

    template<typename ConstRandomAccessIterator, typename UnaryPredicate>
	bool any_of(ConstRandomAccessIterator first, ConstRandomAccessIterator last, UnaryPredicate p)
	{
		return _any_of(first, last, std::move(p));
	}

    template<typename ConstRandomAccessIterator, typename UnaryPredicate>
	bool none_of(ConstRandomAccessIterator first, ConstRandomAccessIterator last, UnaryPredicate p)
	{
		return _none_of(first, last, std::move(p));
	}

    // non-standard versions which store the result on the accelerator
	// TODO: keeping results on the accelerator in general needs a more structured approach
	// - we should revisit this.
 /*   template<typename ConstRandomAccessIterator, typename OutputIterator, typename UnaryPredicate>
	void all_of(ConstRandomAccessIterator first, ConstRandomAccessIterator last,
				OutputIterator dest_first, UnaryPredicate p)
	{
		any_of_impl(first, last, dest_first, [p = std::move(p)](auto&& x) { return !p(x); });
	}

    template<typename ConstRandomAccessIterator, typename OutputIterator, typename UnaryPredicate>
	void any_of(ConstRandomAccessIterator first, ConstRandomAccessIterator last,
				OutputIterator dest_first, UnaryPredicate p)
	{
		any_of_impl(first, last, dest_first, std::move(p));
	}

    template<typename ConstRandomAccessIterator, typename OutputIterator, typename UnaryPredicate>
	void none_of(ConstRandomAccessIterator first, ConstRandomAccessIterator last,
				 OutputIterator dest_first, UnaryPredicate p)
	{
		any_of_impl(first, last, dest_first)
	}*/

    //----------------------------------------------------------------------------
    // copy, copy_if, copy_n, copy_backward
    //----------------------------------------------------------------------------

    template<typename ConstRandomAccessIterator, typename RandomAccessIterator>
	RandomAccessIterator copy(ConstRandomAccessIterator first,
							  ConstRandomAccessIterator last,
							  RandomAccessIterator dest_first)
	{
		return _copy(first, last, dest_first);
	}

    template<typename ConstRandomAccessIterator, typename RandomAccessIterator, typename UnaryPredicate>
	RandomAccessIterator copy_if(ConstRandomAccessIterator first,
								 ConstRandomAccessIterator last,
								 RandomAccessIterator dest_first,
								 UnaryPredicate p)
	{
		return _copy_if(first, last, dest_first, std::move(p));
	}

    template<typename ConstRandomAccessIterator, typename Size, typename RandomAccessIterator>
	RandomAccessIterator copy_n(ConstRandomAccessIterator first, Size count, RandomAccessIterator result)
	{
		return _copy_n(first, std::move(count), result);
	}

    // TODO_NOT_IMPLEMENTED: copy_backward, does copy_backward really make any sense in a
	// data-parallel context? Yes it does, consider backward overlap.
    template<typename ConstRandomAccessIterator, typename RandomAccessIterator>
    RandomAccessIterator copy_backward(ConstRandomAccessIterator first,
									   ConstRandomAccessIterator last,
									   RandomAccessIterator d_last);

    //----------------------------------------------------------------------------
    // count, count_if
    //----------------------------------------------------------------------------

    template<typename ConstRandomAccessIterator, typename T>
	Difference_type<ConstRandomAccessIterator> count(ConstRandomAccessIterator first,
													 ConstRandomAccessIterator last,
													 const T& value)
	{
		return _count(first, last, value);
	}

    template<typename ConstRandomAccessIterator, typename UnaryPredicate>
	Difference_type<ConstRandomAccessIterator> count_if(ConstRandomAccessIterator first,
														ConstRandomAccessIterator last,
														UnaryPredicate p)
	{
		return _count_if(first, last, std::move(p));
	}

    //----------------------------------------------------------------------------
    // equal, equal_range
    //----------------------------------------------------------------------------

    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2>
	bool equal(ConstRandomAccessIterator1 first1,
			   ConstRandomAccessIterator1 last1,
			   ConstRandomAccessIterator2 first2)
	{
		return _equal(first1, last1, first2);
	}

    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2, typename BinaryPredicate>
	bool equal(ConstRandomAccessIterator1 first1,
			   ConstRandomAccessIterator1 last1,
			   ConstRandomAccessIterator2 first2,
			   BinaryPredicate p)
	{
		return _equal(first1, last1, first2, std::move(p));
	}

    template<typename ConstRandomAccessIterator, typename T>
	std::pair<ConstRandomAccessIterator, ConstRandomAccessIterator> equal_range(ConstRandomAccessIterator first,
																				ConstRandomAccessIterator last,
																				const T& value)
	{
		return _equal_range(first, last, value);
	}

    template<typename ConstRandomAccessIterator, typename T, typename Compare>
	std::pair<ConstRandomAccessIterator, ConstRandomAccessIterator> equal_range(ConstRandomAccessIterator first,
																				ConstRandomAccessIterator last,
																				const T& value,
																				Compare comp)
	{
		return _equal_range(first, last, value, std::move(comp));
	}

    //----------------------------------------------------------------------------
    // fill, fill_n
    //----------------------------------------------------------------------------

    template<typename RandomAccessIterator, typename T>
	void fill(RandomAccessIterator first, RandomAccessIterator last, const T& value)
	{
		_fill(first, last, value);
	}

    template<typename RandomAccessIterator, typename Size, typename T>
	RandomAccessIterator fill_n(RandomAccessIterator first, Size count, const T& value)
	{
		return _fill_n(first, count, value);
	}

    //----------------------------------------------------------------------------
    // find, find_if, find_if_not, find_end, find_first_of, adjacent_find
    //----------------------------------------------------------------------------

    template<typename ConstRandomAccessIterator, typename T>
	ConstRandomAccessIterator find(ConstRandomAccessIterator first,
								   ConstRandomAccessIterator last,
								   const T& value)
	{
		return _find(first, last, value);
	}

    template<typename ConstRandomAccessIterator, typename UnaryPredicate>
	ConstRandomAccessIterator find_if(ConstRandomAccessIterator first,
									  ConstRandomAccessIterator last,
									  UnaryPredicate p)
	{
		return _find_if(first, last, std::move(p));
	}

    template<typename ConstRandomAccessIterator, typename UnaryPredicate>
	ConstRandomAccessIterator find_if_not(ConstRandomAccessIterator first,
										  ConstRandomAccessIterator last,
										  UnaryPredicate p)
	{
		return _find_if_not(first, last, std::move(p));
	}

    // TODO_NOT_IMPLEMENTED: find_end
    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2>
    ConstRandomAccessIterator1 find_end(ConstRandomAccessIterator1 first1,
										ConstRandomAccessIterator1 last1,
										ConstRandomAccessIterator2 first2,
										ConstRandomAccessIterator2 last2);

    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2, typename Predicate>
    ConstRandomAccessIterator1 find_end(ConstRandomAccessIterator1 first1,
										ConstRandomAccessIterator1 last1,
										ConstRandomAccessIterator2 first2,
										ConstRandomAccessIterator2 last2,
										Predicate p);

    // TODO_NOT_IMPLEMENTED: find_first_of
    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2>
    ConstRandomAccessIterator1 find_first_of(ConstRandomAccessIterator1 first1,
											 ConstRandomAccessIterator1 last1,
											 ConstRandomAccessIterator2 first2,
											 ConstRandomAccessIterator2 last2);

    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2, typename Predicate>
    ConstRandomAccessIterator1 find_first_of(ConstRandomAccessIterator1 first1,
											 ConstRandomAccessIterator1 last1,
											 ConstRandomAccessIterator2 first2,
											 ConstRandomAccessIterator2 last2,
											 Predicate p);

    template<typename ConstRandomAccessIterator>
	ConstRandomAccessIterator adjacent_find(ConstRandomAccessIterator first, ConstRandomAccessIterator last)
	{
		return _adjacent_find(first, last);
	}

    template<typename ConstRandomAccessIterator, typename Predicate>
	ConstRandomAccessIterator adjacent_find(ConstRandomAccessIterator first,
											ConstRandomAccessIterator last,
											Predicate p)
	{
		return _adjacent_find(first, last, std::move(p));
	}

    //----------------------------------------------------------------------------
    // for_each, for_each_no_return
    //----------------------------------------------------------------------------

    template<typename ConstRandomAccessIterator, typename UnaryFunction>
	UnaryFunction for_each(ConstRandomAccessIterator first, ConstRandomAccessIterator last, UnaryFunction f)
	{
		return _for_each(first, last, std::move(f));
	}

    // non-standard: no return
    template<typename ConstRandomAccessIterator, typename UnaryFunction>
	void for_each_no_return(ConstRandomAccessIterator first, ConstRandomAccessIterator last, UnaryFunction f)
	{
		_for_each_no_return(first, last, std::move(f));
	}

    //----------------------------------------------------------------------------
    // generate, generate_n
    //----------------------------------------------------------------------------

    template<typename RandomAccessIterator, typename Generator>
	void generate(RandomAccessIterator first, RandomAccessIterator last, Generator g)
	{
		_generate(first, last, std::move(g));
	}

    template<typename RandomAccessIterator, typename Size, typename Generator>
	RandomAccessIterator generate_n(RandomAccessIterator first, Size count, Generator g)
	{
		return _generate_n(first, std::move(count), std::move(g));
	}

    //----------------------------------------------------------------------------
    // includes
    //----------------------------------------------------------------------------

    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2>
	bool includes(ConstRandomAccessIterator1 first1,
				  ConstRandomAccessIterator1 last1,
				  ConstRandomAccessIterator2 first2,
				  ConstRandomAccessIterator2 last2)
	{
		return _includes(first1, last1, first2, last2);
	}

    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2, typename Compare>
	bool includes(ConstRandomAccessIterator1 first1,
				  ConstRandomAccessIterator1 last1,
				  ConstRandomAccessIterator2 first2,
				  ConstRandomAccessIterator2 last2,
				  Compare comp)
	{
		return _includes(first1, last1, first2, last2, std::move(comp));
	}

    //----------------------------------------------------------------------------
    // inner_product
    //----------------------------------------------------------------------------

    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2, typename T>
	T inner_product(ConstRandomAccessIterator1 first1,
					ConstRandomAccessIterator1 last1,
					ConstRandomAccessIterator2 first2,
					T value)
	{
		return _inner_product(first1, last1, first2, std::move(value));
	}

    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2, typename T,
			 typename BinaryOperation1, typename BinaryOperation2>
	T inner_product(ConstRandomAccessIterator1 first1,
					ConstRandomAccessIterator1 last1,
					ConstRandomAccessIterator2 first2,
					T value,
					BinaryOperation1 op1,
					BinaryOperation2 op2)
	{
		return _inner_product(first1, last1, first2, std::move(value), std::move(op1), std::move(op2));
	}

    //----------------------------------------------------------------------------
    // iota
    //----------------------------------------------------------------------------

    template<typename RandomAccessIterator, typename T>
	void iota(RandomAccessIterator first, RandomAccessIterator last, T value)
	{
		_iota(first, last, std::move(value));
	}

    //----------------------------------------------------------------------------
    // lexographical_compare
    //----------------------------------------------------------------------------

    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2>
	bool lexicographical_compare(ConstRandomAccessIterator1 first1,
								 ConstRandomAccessIterator1 last1,
								 ConstRandomAccessIterator2 first2,
								 ConstRandomAccessIterator2 last2)
	{
		return _lexicographical_compare(first1, last1, first2, last2);
	}

    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2, typename Compare>
	bool lexicographical_compare(ConstRandomAccessIterator1 first1,
								 ConstRandomAccessIterator1 last1,
								 ConstRandomAccessIterator2 first2,
								 ConstRandomAccessIterator2 last2,
								 Compare comp)
	{
		return _lexicographical_compare(first1, last1, first2, last2, std::move(comp));
	}

	//----------------------------------------------------------------------------
	// is_permutation, next_permutation, prev_permutation
	//----------------------------------------------------------------------------

    // TODO_NOT_IMPLEMENTED: is_permutation, next_permutation, prev_permutation
	template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2, typename BinaryPredicate>
	bool is_permutation(ConstRandomAccessIterator1 first1,
						ConstRandomAccessIterator1 last1,
						ConstRandomAccessIterator2 first2,
						BinaryPredicate p);

	template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2>
	bool is_permutation(ConstRandomAccessIterator1 first1,
						ConstRandomAccessIterator1 last1,
						ConstRandomAccessIterator2 first2);

    //----------------------------------------------------------------------------
    // lower_bound, upper_bound
    //----------------------------------------------------------------------------

    template<typename ConstRandomAccessIterator, typename T>
	ConstRandomAccessIterator lower_bound(ConstRandomAccessIterator first,
										  ConstRandomAccessIterator last,
										  const T& value)
	{
		return _lower_bound(first, last, value);
	}

    template<typename ConstRandomAccessIterator, typename T, typename Compare>
	ConstRandomAccessIterator lower_bound(ConstRandomAccessIterator first,
										  ConstRandomAccessIterator last,
										  const T& value,
										  Compare comp)
	{
		return _lower_bound(first, last, value, std::move(comp));
	}

    template<typename ConstRandomAccessIterator, typename T>
	ConstRandomAccessIterator upper_bound(ConstRandomAccessIterator first,
										  ConstRandomAccessIterator last,
										  const T& value)
	{
		return _upper_bound(first, last, value);
	}

    template<typename ConstRandomAccessIterator, typename T, typename Compare>
	ConstRandomAccessIterator upper_bound(ConstRandomAccessIterator first,
										  ConstRandomAccessIterator last,
										  const T& value,
										  Compare comp)
	{
		return _upper_bound(first, last, value, std::move(comp));
	}

    //----------------------------------------------------------------------------
    // merge, inplace_merge
    //----------------------------------------------------------------------------

    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2, typename RandomAccessIterator>
	RandomAccessIterator merge(ConstRandomAccessIterator1 first1,
							   ConstRandomAccessIterator1 last1,
							   ConstRandomAccessIterator2 first2,
							   ConstRandomAccessIterator2 last2,
							   RandomAccessIterator dest_first)
	{
		return _merge(first1, last1, first2, last2, dest_first);
	}

	template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2,
			 typename RandomAccessIterator, typename Compare>
	RandomAccessIterator merge(ConstRandomAccessIterator1 first1,
							   ConstRandomAccessIterator1 last1,
							   ConstRandomAccessIterator2 first2,
							   ConstRandomAccessIterator2 last2,
							   RandomAccessIterator dest_first,
							   Compare comp)
	{
		return _merge(first1, last1, first2, last2, dest_first, std::move(comp));
	}

    template<typename RandomAccessIterator>
	void inplace_merge(RandomAccessIterator first, RandomAccessIterator middle, RandomAccessIterator last)
	{
		_inplace_merge(first, middle, last);
	}

    template<typename RandomAccessIterator, typename Compare>
	void inplace_merge(RandomAccessIterator first,
					   RandomAccessIterator middle,
					   RandomAccessIterator last,
					   Compare comp)
	{
		_inplace_merge(first, middle, last, std::move(comp));
	}

    //----------------------------------------------------------------------------
    // minmax, max_element, min_element, minmax_element
    //----------------------------------------------------------------------------

	template<typename T>
	/*constexpr*/ const T& max(const T& a, const T& b) restrict(cpu, amp)
	{
		return _max(a, b);
	}

	template<typename T, typename Compare>
	/*constexpr*/ const T& max(const T& a, const T& b, Compare comp) restrict(cpu, amp)
	{
		return _max(a, b, move(comp));
	}

	template<typename T>
	/*constexpr*/ const T& min(const T& a, const T& b) restrict(cpu, amp)
	{
		return _min(a, b);
	}

	template<typename T, typename Compare>
	/*constexpr*/ const T& min(const T& a, const T& b, Compare comp) restrict(cpu, amp)
	{
		return _min(a, b, move(comp));
	}

	template<typename T>
	/*constexpr*/ amp_stl_algorithms::pair<const T, const T> minmax(const T& a, const T& b) restrict(cpu, amp)
	{
		return _minmax(a, b);
	}

	template <typename T, typename Compare>
	/*constexpr*/ amp_stl_algorithms::pair<const T, const T> minmax(const T& a, const T& b, Compare comp) restrict(cpu, amp)
	{
		return _minmax(a, b, move(comp));
	}

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
	ConstRandomAccessIterator max_element(ConstRandomAccessIterator first, ConstRandomAccessIterator last)
	{
		return _max_element(first, last);
	}

    template<typename ConstRandomAccessIterator, typename Compare>
	ConstRandomAccessIterator max_element(ConstRandomAccessIterator first,
										  ConstRandomAccessIterator last,
										  Compare comp)
	{
		return _max_element(first, last, std::move(comp));
	}

    template<typename ConstRandomAccessIterator>
	ConstRandomAccessIterator min_element(ConstRandomAccessIterator first, ConstRandomAccessIterator last)
	{
		return _min_element(first, last);
	}

    template<typename ConstRandomAccessIterator, typename Compare>
	ConstRandomAccessIterator min_element(ConstRandomAccessIterator first,
										  ConstRandomAccessIterator last,
										  Compare comp)
	{
		return _min_element(first, last, std::move(comp));
	}

    template<typename ConstRandomAccessIterator>
	std::pair<ConstRandomAccessIterator, ConstRandomAccessIterator> minmax_element(ConstRandomAccessIterator first,
																				   ConstRandomAccessIterator last)
	{
		return _minmax_element(first, last);
	}

    template<typename ConstRandomAccessIterator, typename Compare>
	std::pair<ConstRandomAccessIterator, ConstRandomAccessIterator> minmax_element(ConstRandomAccessIterator first,
																				   ConstRandomAccessIterator last,
																				   Compare comp)
	{
		return _minmax_element(first, last, std::move(comp));
	}

    //----------------------------------------------------------------------------
    // mismatch
    //----------------------------------------------------------------------------

    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2>
	std::pair<ConstRandomAccessIterator1, ConstRandomAccessIterator2> mismatch(ConstRandomAccessIterator1 first1,
																			   ConstRandomAccessIterator1 last1,
																			   ConstRandomAccessIterator2 first2)
	{
		return _mismatch(first1, last1, first2);
	}

    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2, typename BinaryPredicate>
	std::pair<ConstRandomAccessIterator1, ConstRandomAccessIterator2> mismatch(ConstRandomAccessIterator1 first1,
																			   ConstRandomAccessIterator1 last1,
																			   ConstRandomAccessIterator2 first2,
																			   BinaryPredicate p)
	{
		return _mismatch(first1, last1, first2, std::move(p));
	}

    //----------------------------------------------------------------------------
    // move, move_backward
    //----------------------------------------------------------------------------

    template<typename ConstRandomAccessIterator, typename RandomAccessIterator>
	RandomAccessIterator move(ConstRandomAccessIterator first,
							  ConstRandomAccessIterator last,
							  RandomAccessIterator dest_first)
	{	// TODO: add unit test for move, requires adding a move constructor counting ADT.
		return _move(first, last, dest_first);
	}

    // TODO_NOT_IMPLEMENTED: move_backward, does move_backward really make any sense
	// in a data-parallel context?
    template<typename ConstRandomAccessIterator, typename RandomAccessIterator>
    RandomAccessIterator move_backward(ConstRandomAccessIterator first,
									   ConstRandomAccessIterator last,
									   RandomAccessIterator dest_last);

    //----------------------------------------------------------------------------
    // nth_element
    //----------------------------------------------------------------------------

    template<typename RandomAccessIterator>
	void nth_element(RandomAccessIterator first, RandomAccessIterator nth, RandomAccessIterator last)
	{
		_nth_element(first, nth, last);
	}

    template<typename RandomAccessIterator, typename Compare>
	void nth_element(RandomAccessIterator first,
					 RandomAccessIterator nth,
					 RandomAccessIterator last,
					 Compare comp)
	{
		_nth_element(first, nth, last, std::move(comp));
	}

	//----------------------------------------------------------------------------
	// partial_sum
	//----------------------------------------------------------------------------

	template<typename ConstRandomAccessIterator, typename RandomAccessIterator>
	RandomAccessIterator partial_sum(ConstRandomAccessIterator first,
									 ConstRandomAccessIterator last,
									 RandomAccessIterator dest_first)
	{
		return _partial_sum(first, last, dest_first);
	}

	template<typename ConstRandomAccessIterator, typename RandomAccessIterator, typename BinaryOperation>
	RandomAccessIterator partial_sum(ConstRandomAccessIterator first,
									 ConstRandomAccessIterator last,
									 RandomAccessIterator dest_first,
									 BinaryOperation binary_op)
	{
		return _partial_sum(first, last, dest_first, binary_op);
	}

    //----------------------------------------------------------------------------
    // partition, stable_partition, partition_point, is_partitioned
    //----------------------------------------------------------------------------

    template<typename ConstRandomAccessIterator, typename UnaryPredicate>
	bool is_partitioned(ConstRandomAccessIterator first, ConstRandomAccessIterator last, UnaryPredicate p)
	{
		return _is_partitioned(first, last, std::move(p));
	}

    template<typename RandomAccessIterator, typename UnaryPredicate>
	RandomAccessIterator partition(RandomAccessIterator first, RandomAccessIterator last, UnaryPredicate p)
	{
		return _partition(first, last, std::move(p));
	}

    // TODO_NOT_IMPLEMENTED: stable_partition
    template<typename RandomAccessIterator, typename UnaryPredicate>
    RandomAccessIterator stable_partition(RandomAccessIterator first,
										  RandomAccessIterator last,
										  UnaryPredicate p);

    template<typename ConstRandomAccessIterator, typename UnaryPredicate>
	ConstRandomAccessIterator partition_point(ConstRandomAccessIterator first,
											  ConstRandomAccessIterator last,
											  UnaryPredicate p)
	{
		return _partition_point(first, last, std::move(p));
	}

    //----------------------------------------------------------------------------
    // reduce
    //----------------------------------------------------------------------------

	// NOTE: not in the standard
	template<typename ConstRandomAccessIterator, typename T, typename BinaryOperation>
	T reduce(ConstRandomAccessIterator first,
			 ConstRandomAccessIterator last,
			 const T& identity_element,
			 BinaryOperation op)
	{
		return _reduce(first, last, identity_element, std::move(op));
	}

	//----------------------------------------------------------------------------
	// exclusive_scan, inclusive_scan, inplace_exclusive_scan, inplace_inclusive_scan
	//----------------------------------------------------------------------------

	// NOTE: not in the standard
	template<typename ConstRandomAccessIterator, typename RandomAccessIterator, typename T>
	std::pair<RandomAccessIterator, RandomAccessIterator> exclusive_scan(ConstRandomAccessIterator first,
																		 ConstRandomAccessIterator last,
																		 RandomAccessIterator dest_first,
																		 const T& identity_element)
	{
		return _exclusive_scan(first, last, dest_first, identity_element);
	}

	template<typename ConstRandomAccessIterator, typename RandomAccessIterator, typename T, typename BinaryOperation>
	std::pair<RandomAccessIterator, RandomAccessIterator> exclusive_scan(ConstRandomAccessIterator first,
																		 ConstRandomAccessIterator last,
																		 RandomAccessIterator dest_first,
																		 const T& identity_element,
																		 BinaryOperation binary_op)
	{
		return _exclusive_scan(first, last, dest_first, identity_element, std::move(binary_op));
	}

	template<typename RandomAccessIterator, typename T>
	std::pair<RandomAccessIterator, RandomAccessIterator> inplace_exclusive_scan(RandomAccessIterator first,
																				 RandomAccessIterator last,
																				 const T& identity_element)
	{
		return _inplace_exclusive_scan(first, last, identity_element);
	}

	template<typename RandomAccessIterator, typename T, typename BinaryOperation>
	std::pair<RandomAccessIterator, RandomAccessIterator> inplace_exclusive_scan(RandomAccessIterator first,
																				 RandomAccessIterator last,
																				 const T& identity_element,
																				 BinaryOperation binary_op)
	{
		return _inplace_exclusive_scan(first, last, identity_element, std::move(binary_op));
	}

	template<typename ConstRandomAccessIterator, typename RandomAccessIterator, typename T>
	std::pair<RandomAccessIterator, RandomAccessIterator> inclusive_scan(ConstRandomAccessIterator first,
																		 ConstRandomAccessIterator last,
																		 RandomAccessIterator dest_first,
																		 const T& identity_element)
	{
		return _inclusive_scan(first, last, dest_first, identity_element);
	}

	template<typename ConstRandomAccessIterator, typename RandomAccessIterator, typename T, typename BinaryOperation>
	std::pair<RandomAccessIterator, RandomAccessIterator> inclusive_scan(ConstRandomAccessIterator first,
																		 ConstRandomAccessIterator last,
																		 RandomAccessIterator dest_first,
																		 const T& identity_element,
																		 BinaryOperation binary_op)
	{
		return _inclusive_scan(first, last, dest_first, identity_element, std::move(binary_op));
	}

	template<typename RandomAccessIterator, typename T>
	std::pair<RandomAccessIterator, RandomAccessIterator> inplace_inclusive_scan(RandomAccessIterator first,
																				 RandomAccessIterator last,
																				 const T& identity_element)
	{
		return _inplace_inclusive_scan(first, last, identity_element);
	}

	template<typename RandomAccessIterator, typename T, typename BinaryOperation>
	std::pair<RandomAccessIterator, RandomAccessIterator> inplace_inclusive_scan(RandomAccessIterator first,
																				 RandomAccessIterator last,
																				 const T& identity_element,
																				 BinaryOperation binary_op)
	{
		return _inplace_inclusive_scan(first, last, identity_element, std::move(binary_op));
	}

	//----------------------------------------------------------------------------
	// exclusive_segmented_scan, inclusive_segmented_scan,
	// inplace_exclusive_segmented_scan, inplace_inclusive_segmented_scan
	//----------------------------------------------------------------------------

	// TODO_NOT_IMPLEMENTED: segmented scans

	//----------------------------------------------------------------------------
    // remove, remove_if, remove_copy, remove_copy_if
    //----------------------------------------------------------------------------

    template<typename RandomAccessIterator, typename T>
	RandomAccessIterator remove(RandomAccessIterator first, RandomAccessIterator last, const T& value)
	{
		return _remove(first, last, value);
	}

    template<typename RandomAccessIterator, typename UnaryPredicate>
	RandomAccessIterator remove_if(RandomAccessIterator first, RandomAccessIterator last, UnaryPredicate p)
	{
		return _remove_if(first, last, std::move(p));
	}

    template<typename ConstRandomAccessIterator,typename RandomAccessIterator, typename T>
	RandomAccessIterator remove_copy(ConstRandomAccessIterator first,
									 ConstRandomAccessIterator last,
									 RandomAccessIterator dest_first,
									 const T& value)
	{
		return _remove_copy(first, last, dest_first, value);
	}

    template<typename ConstRandomAccessIterator,typename RandomAccessIterator, typename UnaryPredicate>
	RandomAccessIterator remove_copy_if(ConstRandomAccessIterator first,
										ConstRandomAccessIterator last,
										RandomAccessIterator dest_first,
										UnaryPredicate p)
	{
		return _remove_copy_if(first, last, dest_first, std::move(p));
	}

    //----------------------------------------------------------------------------
    // replace, replace_if, replace_copy, replace_copy_if
    //----------------------------------------------------------------------------

    template<typename RandomAccessIterator, typename T>
	void replace(RandomAccessIterator first, RandomAccessIterator last, const T& old_value, const T& new_value)
	{
		_replace(first, last, old_value, new_value);
	}

    template<typename RandomAccessIterator, typename UnaryPredicate, typename T>
	void replace_if(RandomAccessIterator first, RandomAccessIterator last, UnaryPredicate p, const T& new_value)
	{
		_replace_if(first, last, std::move(p), new_value);
	}

    template<typename ConstRandomAccessIterator,typename RandomAccessIterator, typename T>
	RandomAccessIterator replace_copy(ConstRandomAccessIterator first,
									  ConstRandomAccessIterator last,
									  RandomAccessIterator dest_first,
									  const T& old_value,
									  const T& new_value)
	{
		return _replace_copy(first, last, dest_first, old_value, new_value);
	}

    template<typename ConstRandomAccessIterator,typename RandomAccessIterator, typename UnaryPredicate, typename T>
	RandomAccessIterator replace_copy_if(ConstRandomAccessIterator first,
										 ConstRandomAccessIterator last,
										 RandomAccessIterator dest_first,
										 UnaryPredicate p,
										 const T& new_value)
	{
		return _replace_copy_if(first, last, dest_first, std::move(p), new_value);
	}

    //----------------------------------------------------------------------------
    // reverse, reverse_copy
    //----------------------------------------------------------------------------

    template<typename RandomAccessIterator>
	void reverse(RandomAccessIterator first, RandomAccessIterator last)
	{
		_reverse(first, last);
	}

    template<typename ConstRandomAccessIterator, typename RandomAccessIterator>
	RandomAccessIterator reverse_copy(ConstRandomAccessIterator first,
									  ConstRandomAccessIterator last,
									  RandomAccessIterator dest_first)
	{
		return _reverse_copy(first, last, dest_first);
	}

    //----------------------------------------------------------------------------
    // rotate, rotate_copy
    //----------------------------------------------------------------------------

    template<typename RandomAccessIterator>
	RandomAccessIterator rotate(RandomAccessIterator first, RandomAccessIterator middle, RandomAccessIterator last)
	{
		return _rotate(first, middle, last);
	}

    template<typename ConstRandomAccessIterator,typename RandomAccessIterator>
	RandomAccessIterator rotate_copy(ConstRandomAccessIterator first,
									 ConstRandomAccessIterator middle,
									 ConstRandomAccessIterator last,
									 RandomAccessIterator dest_first)
	{
		return _rotate_copy(first, middle, last, dest_first);
	}

    //----------------------------------------------------------------------------
    // search, search_n, binary_search
    //----------------------------------------------------------------------------

    // TODO_NOT_IMPLEMENTED: search
    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2>
    ConstRandomAccessIterator1 search(ConstRandomAccessIterator1 first1,
									  ConstRandomAccessIterator1 last1,
									  ConstRandomAccessIterator2 first2,
									  ConstRandomAccessIterator2 last2);

    // NOT IMPLEMENTED
    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2, typename BinaryPredicate>
    ConstRandomAccessIterator1 search(ConstRandomAccessIterator1 first1,
									  ConstRandomAccessIterator1 last1,
									  ConstRandomAccessIterator2 first2,
									  ConstRandomAccessIterator2 last2,
									  BinaryPredicate p);


    template<typename ConstRandomAccessIterator, typename Size, typename T>
	ConstRandomAccessIterator search_n(ConstRandomAccessIterator first,
									   ConstRandomAccessIterator last,
									   Size count,
									   const T& value)
	{
		return _search_n(first, last, std::move(count), value);
	}


    template<typename ConstRandomAccessIterator, typename Size, typename T, typename BinaryPredicate>
	ConstRandomAccessIterator search_n(ConstRandomAccessIterator first,
									   ConstRandomAccessIterator last,
									   Size count,
									   const T& value,
									   BinaryPredicate p)
	{
		return _search_n(first, last, std::move(count), value, std::move(p));
	}

    template<typename ConstRandomAccessIterator, typename T>
	bool binary_search(ConstRandomAccessIterator first, ConstRandomAccessIterator last, const T& value)
	{
		return _binary_search(first, last, value);
	}

    template<typename ConstRandomAccessIterator, typename T, typename Compare>
	bool binary_search(ConstRandomAccessIterator first,
					   ConstRandomAccessIterator last,
					   const T& value,
					   Compare comp)
	{
		return _binary_search(first, last, value, std::move(comp));
	}

    //----------------------------------------------------------------------------
    // set_difference, set_intersection, set_symmetric_difference, set_union
    //----------------------------------------------------------------------------

	// TODO_NOT_IMPLEMENTED: set_*
    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2,typename RandomAccessIterator>
    RandomAccessIterator set_difference(ConstRandomAccessIterator1 first1,
										ConstRandomAccessIterator1 last1,
										ConstRandomAccessIterator2 first2,
										ConstRandomAccessIterator2 last2,
										RandomAccessIterator d_first);

    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2,
			 typename RandomAccessIterator, typename Compare>
    RandomAccessIterator set_difference(ConstRandomAccessIterator1 first1,
										ConstRandomAccessIterator1 last1,
										ConstRandomAccessIterator2 first2,
										ConstRandomAccessIterator2 last2,
										RandomAccessIterator dest_first,
										Compare comp);

    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2,typename RandomAccessIterator>
    RandomAccessIterator set_intersection(ConstRandomAccessIterator1 first1,
										  ConstRandomAccessIterator1 last1,
										  ConstRandomAccessIterator2 first2,
										  ConstRandomAccessIterator2 last2,
										  RandomAccessIterator dest_first);

    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2,
			 typename RandomAccessIterator, typename Compare>
    RandomAccessIterator set_intersection(ConstRandomAccessIterator1 first1,
										  ConstRandomAccessIterator1 last1,
										  ConstRandomAccessIterator2 first2,
										  ConstRandomAccessIterator2 last2,
										  RandomAccessIterator dest_first,
										  Compare comp);

    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2,typename RandomAccessIterator>
    RandomAccessIterator set_symmetric_difference(ConstRandomAccessIterator1 first1,
												  ConstRandomAccessIterator1 last1,
												  ConstRandomAccessIterator2 first2,
												  ConstRandomAccessIterator2 last2,
												  RandomAccessIterator dest_first);

    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2,
			 typename RandomAccessIterator, typename Compare>
    RandomAccessIterator set_symmetric_difference(ConstRandomAccessIterator1 first1,
												  ConstRandomAccessIterator1 last1,
												  ConstRandomAccessIterator2 first2,
												  ConstRandomAccessIterator2 last2,
												  RandomAccessIterator dest_first,
												  Compare comp);

    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2,typename RandomAccessIterator>
    RandomAccessIterator set_union(ConstRandomAccessIterator1 first1,
								   ConstRandomAccessIterator1 last1,
								   ConstRandomAccessIterator2 first2,
								   ConstRandomAccessIterator2 last2,
								   RandomAccessIterator dest_first);

    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2,
			 typename RandomAccessIterator, typename Compare>
    RandomAccessIterator set_union(ConstRandomAccessIterator1 first1,
								   ConstRandomAccessIterator1 last1,
								   ConstRandomAccessIterator2 first2,
								   ConstRandomAccessIterator2 last2,
								   RandomAccessIterator d_first,
								   Compare comp);

    //----------------------------------------------------------------------------
    // shuffle, random_shuffle
    //----------------------------------------------------------------------------

    // TODO_NOT_IMPLEMENTED: random_shuffle
    template<typename RandomAccessIterator>
    void random_shuffle(RandomAccessIterator first, RandomAccessIterator last);

    // NOT IMPLEMENTED
    template<typename RandomAccessIterator, typename RandomNumberGenerator>
    void random_shuffle(RandomAccessIterator first, RandomAccessIterator last, RandomNumberGenerator r);

    // NOT IMPLEMENTED
    template<typename RandomAccessIterator, typename RandomNumberGenerator>
    void random_shuffle(RandomAccessIterator first, RandomAccessIterator last, RandomNumberGenerator r);

    // TODO_NOT_IMPLEMENTED: shuffle
    template<typename RandomAccessIterator, typename UniformRandomNumberGenerator>
    void shuffle(RandomAccessIterator first, RandomAccessIterator last, UniformRandomNumberGenerator g);

    //----------------------------------------------------------------------------
    // sort, partial_sort, partial_sort_copy, stable_sort, is_sorted, is_sorted_until
    //----------------------------------------------------------------------------

    template<typename ConstRandomAccessIterator>
	bool is_sorted(ConstRandomAccessIterator first, ConstRandomAccessIterator last)
	{
		return _is_sorted(first, last);
	}

    template<typename ConstRandomAccessIterator, typename Compare>
	bool is_sorted(ConstRandomAccessIterator first, ConstRandomAccessIterator last, Compare comp)
	{
		return _is_sorted(first, last, std::move(comp));
	}

    template<typename ConstRandomAccessIterator>
	ConstRandomAccessIterator is_sorted_until(ConstRandomAccessIterator first, ConstRandomAccessIterator last)
	{
		return _is_sorted_until(first, last);
	}

    template<typename ConstRandomAccessIterator, typename Compare>
	ConstRandomAccessIterator is_sorted_until(ConstRandomAccessIterator first,
											  ConstRandomAccessIterator last,
											  Compare comp)
	{
		return _is_sorted_until(first, last, std::move(comp));
	}

    template<typename RandomAccessIterator>
	void sort(RandomAccessIterator first, RandomAccessIterator last)
	{
		_sort(first, last);
	}

    template<typename RandomAccessIterator, typename Compare>
	void sort(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
	{
		_sort(first, last, std::move(comp));
	}

    template<typename RandomAccessIterator>
	void partial_sort(RandomAccessIterator first, RandomAccessIterator middle, RandomAccessIterator last)
	{
		_partial_sort(first, middle, last);
	}

    template<typename RandomAccessIterator, typename Compare>
	void partial_sort(RandomAccessIterator first,
					  RandomAccessIterator middle,
					  RandomAccessIterator last,
					  Compare comp)
	{
		_partial_sort(first, middle, last, std::move(comp));
	}

    // TODO_NOT_IMPLEMENTED: partial_sort_copy - implement after heap functions.
    template<typename ConstRandomAccessIterator,typename RandomAccessIterator>
    RandomAccessIterator partial_sort_copy(ConstRandomAccessIterator first,
										   ConstRandomAccessIterator last,
										   RandomAccessIterator d_first,
										   RandomAccessIterator d_last);

    // NOT IMPLEMENTED - see above
    template<typename ConstRandomAccessIterator,typename RandomAccessIterator, typename Compare>
    RandomAccessIterator partial_sort_copy(ConstRandomAccessIterator first,
										   ConstRandomAccessIterator last,
										   RandomAccessIterator d_first,
										   RandomAccessIterator d_last,
										   Compare comp);

    // TODO_NOT_IMPLEMENTED: stable_sort
    template<typename RandomAccessIterator>
    void stable_sort(RandomAccessIterator first, RandomAccessIterator last);

    // NOT IMPLEMENTED
    template<typename RandomAccessIterator, typename Compare>
    void stable_sort(RandomAccessIterator first, RandomAccessIterator last, Compare comp);

    //----------------------------------------------------------------------------
    // swap, swap_ranges, iter_swap
    //----------------------------------------------------------------------------

	template<typename T>
	void swap(T& a, T& b) restrict(cpu, amp)
	{
		_swap(a, b);
	}

    template<typename T, int N>
	void swap(T (&a)[N], T(&b)[N]) restrict(cpu, amp)
	{
		_swap(a, b);
	}

    template<typename RandomAccessIterator1, typename RandomAccessIterator2>
	RandomAccessIterator2 swap_ranges(RandomAccessIterator1 first1,
									  RandomAccessIterator1 last1,
									  RandomAccessIterator2 first2)
	{
		return _swap_ranges(first1, last1, first2);
	}

    template<typename RandomAccessIterator1, typename RandomAccessIterator2>
	void iter_swap(RandomAccessIterator1 a, RandomAccessIterator2 b) restrict(cpu, amp)
	{
		_iter_swap(a, b);
	}

    //----------------------------------------------------------------------------
    // transform
    //----------------------------------------------------------------------------

    template<typename ConstRandomAccessIterator,typename RandomAccessIterator, typename UnaryFunction>
	RandomAccessIterator transform(ConstRandomAccessIterator first1,
								   ConstRandomAccessIterator last1,
								   RandomAccessIterator dest_first,
								   UnaryFunction unary_op)
	{
		return _transform(first1, last1, dest_first, std::move(unary_op));
	}

    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2,
			 typename RandomAccessIterator, typename BinaryFunction>
	RandomAccessIterator transform(ConstRandomAccessIterator1 first1,
								   ConstRandomAccessIterator1 last1,
								   ConstRandomAccessIterator2 first2,
								   RandomAccessIterator dest_first,
								   BinaryFunction binary_op)
	{
		return _transform(first1, last1, first2, dest_first, std::move(binary_op));
	}

    //----------------------------------------------------------------------------
    // unique, unique_copy
    //----------------------------------------------------------------------------

    template<typename ConstRandomAccessIterator>
	ConstRandomAccessIterator unique(ConstRandomAccessIterator first, ConstRandomAccessIterator last)
	{
		return _unique(first, last);
	}

    template<typename ConstRandomAccessIterator, typename BinaryPredicate>
	ConstRandomAccessIterator unique(ConstRandomAccessIterator first,
									 ConstRandomAccessIterator last,
									 BinaryPredicate p)
	{
		return _unique(first, last, std::move(p));
	}

    template<typename ConstRandomAccessIterator, typename RandomAccessIterator>
	RandomAccessIterator unique_copy(ConstRandomAccessIterator first,
									 ConstRandomAccessIterator last,
									 RandomAccessIterator dest_first)
	{
		return _unique_copy(first, last, dest_first);
	}

    template<typename ConstRandomAccessIterator,typename RandomAccessIterator, typename BinaryPredicate>
	RandomAccessIterator unique_copy(ConstRandomAccessIterator first,
									 ConstRandomAccessIterator last,
									 RandomAccessIterator dest_first,
									 BinaryPredicate p)
	{
		return _unique_copy(first, last, dest_first, std::move(p));
	}
}	   // namespace amp_stl_algorithms
#endif // _AMP_STL_ALGORITHMS_H_BUMPTZI