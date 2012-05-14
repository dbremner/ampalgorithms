/*----------------------------------------------------------------------------
 * Copyright © Microsoft Corp.
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
 * This file contains the test driver
 *---------------------------------------------------------------------------*/
#define NOMINMAX

#include <vector>
#include <algorithm>
#include <iostream>
#include <amp_stl_algorithms.h>

using namespace concurrency;
using namespace amp_stl_algorithms;

// TODO: replace this with a unit test framework

void test_for_each_no_return()
{
	std::vector<int> vec(1024);
	std::fill(vec.begin(), vec.end(), 2);
	array_view<const int> av(1024, vec);
	int sum = 0;
	array_view<int> av_sum(1, &sum);
	amp_stl_algorithms::for_each_no_return(begin(av), end(av), [av_sum] (int val) restrict(amp) {
		atomic_fetch_add(&av_sum(0), val);
	});
	av_sum.synchronize();
	assert(sum == 1024 * 2);
}

void test_find()
{
	static const int numbers[] = {1 , 3, 6, 3, 2, 2 };
	static const int n = sizeof(numbers)/sizeof(numbers[0]);

	array_view<const int> av(extent<1>(n), numbers);
	auto iter = amp_stl_algorithms::find(begin(av), end(av), 3);
	int position = std::distance(begin(av), iter);
	assert(position == 1);

	iter = amp_stl_algorithms::find(begin(av), end(av), 17);
	assert(iter == end(av));
}

void test_none_of()
{
	static const int numbers[] = {1 , 3, 6, 3, 2, 2 };
	static const int n = sizeof(numbers)/sizeof(numbers[0]);

	array_view<const int> av(extent<1>(n), numbers);
	bool r1 = amp_stl_algorithms::none_of(begin(av), end(av), [] (int v) restrict(amp) -> bool { return v>10; });
	assert(r1 == true);
	bool r2 = amp_stl_algorithms::none_of(begin(av), end(av), [] (int v) restrict(amp) -> bool { return v>5; });
	assert(r2 == false);
}

void test_any_of()
{
	static const int numbers[] = {1 , 3, 6, 3, 2, 2 };
	static const int n = sizeof(numbers)/sizeof(numbers[0]);

	array_view<const int> av(extent<1>(n), numbers);
	bool r1 = amp_stl_algorithms::any_of(begin(av), end(av), [] (int v) restrict(amp) -> bool { return v>10; });
	assert(r1 == false);
	bool r2 = amp_stl_algorithms::any_of(begin(av), end(av), [] (int v) restrict(amp) -> bool { return v>5; });
	assert(r2 == true);
}

void test_all_of()
{
	static const int numbers[] = {1 , 3, 6, 3, 2, 2 };
	static const int n = sizeof(numbers)/sizeof(numbers[0]);

	array_view<const int> av(extent<1>(n), numbers);
	bool r1 = amp_stl_algorithms::all_of(begin(av), end(av), [] (int v) restrict(amp) -> bool { return v>10; });
	assert(r1 == false);
	bool r2 = amp_stl_algorithms::all_of(begin(av), end(av), [] (int v) restrict(amp) -> bool { return v>5; });
	assert(r2 == false);
}

void test_count()
{
	static const int numbers[] = {1 , 3, 6, 3, 2, 2, 7, 8, 2, 9, 2, 19, 2};
	static const int n = sizeof(numbers)/sizeof(numbers[0]);
	array_view<const int> av(extent<1>(n), numbers);
	auto r1 = amp_stl_algorithms::count(begin(av), end(av), 2);
	assert(r1 == 5);
	auto r2 = amp_stl_algorithms::count(begin(av), end(av), 17);
	assert(r2 == 0);
}

void test_begin_end_array_view()
{
	std::vector<int> v1(6);
	array_view<int> a1(6, v1);
	auto iter1 = begin(a1);

	array_view<const int> ar1 = a1;
	auto iter2 = begin(ar1);

	auto iter3 = iter1++;
	auto iter4 = ++iter1;
	auto iter5 = iter3 + 7;
	bool res = iter3 < iter4;
	assert(res);
}

void test_random_access_iterator()
{
	std::vector<int> v1(16);
	array_view<int> a1(16, v1);

	// can be default constructed.
	array_view_iterator<int> iter1; 
	auto iter2 = array_view_iterator<double>();

	// can be copy constructed
	array_view_iterator<int> iter3 = begin(a1);
	array_view_iterator<int> iter4(iter3);
	array_view_iterator<int> iter5 = iter4;
	
	// assignment
	iter5 = iter3;

	// equality/inequality comparisons
	bool res = iter3 == iter5;
	assert(res);
	iter3++;
	res = iter3 != iter4;
	assert(res);

	// dereference
	*iter3 = 10;
	assert(a1[1] == 10);
	
	// offset derference operator;
	iter3[2] = 5;
	assert(a1[1 + 2] == 5);

	// increment, decrement, + , -, +=, -=
	auto iter6 = iter3;
	auto iter7 = iter3;
	iter6++;
	iter6 = iter6 + 1;
	iter7 += 2;
	assert(iter6 == iter7);
	--iter6;
	--iter6;
	iter7 = iter7 - 2;
	assert(iter6 == iter7);
	iter7 = iter7 - 1;
	iter6 -= 1;
	assert(iter6 == iter7);

	// <, >, <= >=
	iter6 = iter3;
	iter7 = iter3 + 1;
	assert(iter6 < iter7);
	assert(iter6 <= iter7);
	assert(iter7 > iter6);
	assert(iter7 >= iter6);

	// *i++
	iter6 = begin(a1);
	*iter6 = 3;
	assert(a1[0] == 3);
	int x1 = *iter6++;
	assert(x1 == 3);
	*iter6++ = 7;
	assert(a1[1] == 7);
}

void test_random_access_iterator_in_amp()
{

	std::vector<int> v1(16);
	array_view<int> a1(16, v1);
	std::vector<int> v2(16);
	array_view<int> result(16, v2);
	result.discard_data();
	parallel_for_each(extent<1>(1), [=] (index<1> idx) restrict(amp) {
		int id = 1;

		// can be default constructed.
		array_view_iterator<int> iter1; 
		auto iter2 = array_view_iterator<double>();

		// can be copy constructed
		array_view_iterator<int> iter3 = begin(a1);
		array_view_iterator<int> iter4(iter3);
		array_view_iterator<int> iter5 = iter4;
	
		// assignment
		iter5 = iter3;

		// equality/inequality comparisons
		bool res = iter3 == iter5;
		result[id++] = res;
		iter3++;
		res = iter3 != iter4;
		result[id++] = res;

		// dereference
		*iter3 = 10;
		result[id++] = (a1[1] == 10);
	
		// offset derference operator;
		iter3[2] = 5;
		result[id++] = (a1[1 + 2] == 5);

		// increment, decrement, + , -, +=, -=
		auto iter6 = iter3;
		auto iter7 = iter3;
		iter6++;
		iter6 = iter6 + 1;
		iter7 += 2;
		result[id++] = (iter6 == iter7);
		--iter6;
		--iter6;
		iter7 = iter7 - 2;
		result[id++] = (iter6 == iter7);
		iter7 = iter7 - 1;
		iter6 -= 1;
		result[id++] = (iter6 == iter7);

		// <, >, <= >=
		iter6 = iter3;
		iter7 = iter3 + 1;
		result[id++] = (iter6 < iter7);
		result[id++] = (iter6 <= iter7);
		result[id++] = (iter7 > iter6);
		result[id++] = (iter7 >= iter6);

		// *i++
		iter6 = begin(a1);
		*iter6 = 3;
		result[id++] = (a1[0] == 3);
		int x1 = *iter6++;
		result[id++] = (x1 == 3);
		*iter6++ = 7;
		result[id++] = (a1[1] == 7);
		result[0] = id - 1;
	});
	result.synchronize();
	assert(v2[0] <= (int)v2.size() - 1);
	for (int i = 0; i < v2[0]; i++)
	{
		assert(v2[1 + i] == 1);
	}
}


int main()
{
	test_begin_end_array_view();
	test_random_access_iterator();
	test_random_access_iterator_in_amp();
	test_for_each_no_return();
	test_find();
	test_none_of();
	test_all_of();
	test_any_of();
	test_count();
}