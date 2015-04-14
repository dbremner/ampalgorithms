/*----------------------------------------------------------------------------
* Copyright � Microsoft Corp.
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
* This file contains unit tests.
*---------------------------------------------------------------------------*/

#include "stdafx.h"
#include <gtest/gtest.h>

#include <amp_stl_algorithms.h>
#include "testtools.h"

#include <amp.h>

TEST(stl_iterator_tests, begin_end_array_view)
{
	using namespace amp_stl_algorithms;

    std::vector<int> v1(6);
    concurrency::array_view<int> a1(6, v1);
    auto iter1 = begin(a1);
	auto itern = end(a1);

    concurrency::array_view<const int> ar1 = a1;
    auto iter2 = begin(ar1);

    auto iter3 = iter1++;
    auto iter4 = ++iter1;
    auto iter5 = iter3 + 7;
    bool res = iter3 < iter4;
    ASSERT_TRUE(res);
}

TEST(stl_iterator_tests, cbegin_cend_array_view)
{
	using namespace amp_stl_algorithms;

	std::vector<int> v1(6);
	concurrency::array_view<int> a1(6, v1);
	auto iter1 = cbegin(a1);
	auto itern = cend(a1);

	concurrency::array_view<const int> ar1 = a1;
	auto iter2 = cbegin(ar1);

	auto iter3 = iter1++;
	auto iter4 = ++iter1;
	auto iter5 = iter3 + 7;
	bool res = iter3 < iter4;
	ASSERT_TRUE(res);
}

TEST(stl_iterator_tests, random_access_iterator_default_ctor)
{
	using namespace amp_stl_algorithms;

    array_view_iterator<int> iter1;
	array_view_iterator<const int> iter2;

	auto iter3 = array_view_iterator<double>();
	auto iter4 = array_view_iterator<const double>();
}

TEST(stl_iterator_tests, random_access_iterator_copy_assignment_comparison)
{
	using namespace amp_stl_algorithms;

    std::vector<int> v1(16);
    concurrency::array_view<int> a1(16, v1);

    // Copy constructor and assignment
    array_view_iterator<int> iter1 = begin(a1);
    array_view_iterator<int> iter2(iter1);
    array_view_iterator<int> iter3 = iter2;

    // Equality/inequality comparisons
    ASSERT_TRUE(begin(a1) == iter1);
    ASSERT_TRUE(begin(a1) == iter2);
    ASSERT_TRUE(begin(a1) == iter3);
    ++iter1;
    ASSERT_FALSE(begin(a1) == iter1);
}

TEST(stl_iterator_tests, mixed_random_access_iterator_copy_assignment_comparison)
{
	using namespace amp_stl_algorithms;

	std::vector<int> v1(16);
	concurrency::array_view<int> a1(16, v1);

	// Copy constructor and assignment
	auto iter1 = cbegin(a1);
	array_view_iterator<int> iter2 = begin(a1);
	const_array_view_iterator<int> iter3 = iter2;

	// Equality/inequality comparisons
	ASSERT_TRUE(begin(a1) == iter1);
	ASSERT_TRUE(begin(a1) == iter2);
	ASSERT_TRUE(begin(a1) == iter3);
	++iter2;
	ASSERT_FALSE(begin(a1) == iter2);
}

TEST(stl_iterator_tests, random_access_iterator_dereference)
{
	using namespace amp_stl_algorithms;

    std::vector<int> v1(16);
    concurrency::array_view<int> a1(16, v1);
    array_view_iterator<int> iter = begin(a1);

    // dereference
    ++iter;
    *iter = 10;
    ASSERT_EQ(10, a1[1]);

    // offset dereference operator
    iter[2] = 5;
    ASSERT_EQ(5, a1[1 + 2]);
}

TEST(stl_iterator_tests, const_random_access_iterator_dereference)
{
	using namespace amp_stl_algorithms;

	std::vector<int> v1(16);
	concurrency::array_view<const int> a1(16, v1);
	auto iter = begin(a1);

	// dereference
	iter++;
	ASSERT_EQ(*iter, a1[1]);

	// offset dereference operator
	iter += 2;
	ASSERT_EQ(*iter, a1[1 + 2]);
}

TEST(stl_iterator_tests, random_access_iterator_increment_decrement)
{
	using namespace amp_stl_algorithms;

    std::vector<int> v1(16);
    concurrency::array_view<int> a1(16, v1);
    array_view_iterator<int> iter1 = begin(a1);
    array_view_iterator<int> iter2 = begin(a1);

    ++iter1;
    iter1 = iter1 + 1;
    iter2 += 2;
    ASSERT_TRUE(iter1 == iter2);

    --iter1;
    --iter1;
    iter2 = iter2 - 2;
    ASSERT_TRUE(iter1 == iter2);

    iter2 = iter2 - 1;
    iter1 -= 1;
    ASSERT_TRUE(iter1 == iter2);
}

TEST(stl_iterator_tests, const_random_access_iterator_increment_decrement)
{
	using namespace amp_stl_algorithms;

	std::vector<int> v1(16);
	concurrency::array_view<int> a1(16, v1);
	auto iter1 = cbegin(a1);
	auto iter2 = cbegin(a1);

	iter1++;
	iter1 = iter1 + 1;
	iter2 += 2;
	ASSERT_TRUE(iter1 == iter2);

	--iter1;
	--iter1;
	iter2 = iter2 - 2;
	ASSERT_TRUE(iter1 == iter2);

	iter2 = iter2 - 1;
	iter1 -= 1;
	ASSERT_TRUE(iter1 == iter2);
}

TEST(stl_iterator_tests, random_access_iterator_equality)
{
	using namespace amp_stl_algorithms;

    std::vector<int> v1(16);
    concurrency::array_view<int> a1(16, v1);
    array_view_iterator<int> iter1 = begin(a1);
    array_view_iterator<int> iter2 = begin(a1) + 1;

    ASSERT_TRUE(iter1 < iter2);
    ASSERT_TRUE(iter1 <= iter2);
    ASSERT_TRUE(iter2 > iter1);
    ASSERT_TRUE(iter2 >= iter1);
}

TEST(stl_iterator_tests, mixed_random_access_iterator_equality)
{
	using namespace amp_stl_algorithms;

	std::vector<int> v1(16);
	concurrency::array_view<int> a1(16, v1);
	auto iter1 = begin(a1);
	auto iter2 = cbegin(a1) + 1;

	ASSERT_TRUE(iter1 < iter2);
	ASSERT_TRUE(iter1 <= iter2);
	ASSERT_TRUE(iter2 > iter1);
	ASSERT_TRUE(iter2 >= iter1);
}

TEST(stl_iterator_tests, random_access_iterator_increment)
{
	using namespace amp_stl_algorithms;

    std::vector<int> v1(16);
    concurrency::array_view<int> a1(16, v1);
    array_view_iterator<int> iter = begin(a1);

    *iter = 3;
    ASSERT_EQ(3, a1[0]);
    int x1 = *iter++;
    ASSERT_EQ(3, x1);
    *iter++ = 7;
    ASSERT_EQ(7, a1[1]);
}

TEST(stl_iterator_tests, random_access_iterator_in_amp)
{
	using namespace amp_stl_algorithms;

    std::vector<int> v1(16);
    concurrency::array_view<int> a1(16, v1);
    std::vector<int> v2(16);
    concurrency::array_view<int> result(16, v2);
    result.discard_data();
    parallel_for_each(concurrency::extent<1>(1), [=](concurrency::index<1>) restrict(amp) {
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
        ++iter3;
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
        ++iter6;
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
    ASSERT_TRUE(v2[0] <= (int)v2.size() - 1);
    for (int i = 0; i < v2[0]; i++)
    {
        ASSERT_EQ(1, v2[1 + i]);
    }
}

TEST(stl_iterator_tests, mixed_random_access_iterator_in_amp)
{
	using namespace amp_stl_algorithms;

	std::vector<int> v1(16);
	concurrency::array_view<int> a1(16, v1);
	std::vector<int> v2(16);
	concurrency::array_view<int> result(16, v2);
	result.discard_data();
	parallel_for_each(concurrency::extent<1>(1), [=](concurrency::index<1>) restrict(amp) {
		int id = 1;

		// can be default constructed.
		array_view_iterator<const int> iter1;
		auto iter2 = array_view_iterator<const double>();

		// can be copy constructed
		array_view_iterator<int> iter3 = begin(a1);
		const_array_view_iterator<int> iter4(iter3);
		const_array_view_iterator<int> iter5 = iter4;

		// assignment
		iter5 = iter3;

		// equality/inequality comparisons
		bool res = iter3 == iter5;
		result[id++] = res;
		++iter3;
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
		iter7 = iter3;
		const_array_view_iterator<int> iter8 = iter3 + 1;
		result[id++] = (iter7 < iter8);
		result[id++] = (iter7 <= iter8);
		result[id++] = (iter8 > iter7);
		result[id++] = (iter8 >= iter7);

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
	ASSERT_TRUE(v2[0] <= (int) v2.size() - 1);
	for (int i = 0; i < v2[0]; i++)
	{
		ASSERT_EQ(1, v2[1 + i]);
	}
}
