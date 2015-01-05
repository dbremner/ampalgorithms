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
* This file contains unit tests.
*---------------------------------------------------------------------------*/

#include "stdafx.h"
#include <gtest/gtest.h>

#include <amp_stl_algorithms.h>
#include "testtools.h"

using namespace concurrency;
using namespace amp_stl_algorithms;
using namespace testtools;

const std::array<int, 10> is_sorted_sorted_data[] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },
    { 0, 1, 2, 3, 4, 4, 6, 7, 8, 9 }
};
const std::array<int, 10> is_sorted_unsorted_data[] = {
    { 0, 1, 2, 3, 4, -4, 6, 7, 8, 9 },
    { 0, -1, 2, 3, 4, 4, 6, 7, 8, 9 }
};

class stl_algorithms_tests : public stl_algorithms_testbase<13>, public ::testing::Test {};

class is_sorted_sorted_tests : public ::testing::TestWithParam < std::array<int, 10> > {};

TEST_P(is_sorted_sorted_tests, test)
{
    std::vector<int> input(10);
    std::copy(cbegin(GetParam()), cend(GetParam()), begin(input));
    array_view<int> input_av(10, input);

    auto expected = std::distance(cbegin(input), std::is_sorted_until(cbegin(input), cend(input)));
    auto r = std::distance(begin(input_av), amp_stl_algorithms::is_sorted_until(begin(input_av), end(input_av)));
    ASSERT_EQ(expected, r);
    ASSERT_TRUE(amp_stl_algorithms::is_sorted(begin(input_av), end(input_av), amp_algorithms::less_equal<>()));
    ASSERT_TRUE(amp_stl_algorithms::is_sorted(begin(input_av), end(input_av)));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, is_sorted_sorted_tests, ::testing::ValuesIn(is_sorted_sorted_data));

class is_sorted_unsorted_tests : public ::testing::TestWithParam < std::array<int, 10> > {};

TEST_P(is_sorted_unsorted_tests, test)
{
    std::vector<int> input(10);
    std::copy(cbegin(GetParam()), cend(GetParam()), begin(input));
    array_view<int> input_av(10, input);

    ASSERT_FALSE(amp_stl_algorithms::is_sorted(begin(input_av), end(input_av), amp_algorithms::less_equal<int>()));
    ASSERT_FALSE(amp_stl_algorithms::is_sorted(begin(input_av), end(input_av)));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, is_sorted_unsorted_tests, ::testing::ValuesIn(is_sorted_unsorted_data));

class is_sorted_until_sorted_tests : public ::testing::TestWithParam < std::array<int, 10> > {};

TEST_P(is_sorted_until_sorted_tests, test)
{
    std::vector<int> input(10);
    std::copy(cbegin(GetParam()), cend(GetParam()), begin(input));
    array_view<int> input_av(10, input);

    auto expected = std::distance(cbegin(input), std::is_sorted_until(cbegin(input), cend(input)));
    auto r = std::distance(begin(input_av), amp_stl_algorithms::is_sorted_until(begin(input_av), end(input_av)));
    ASSERT_EQ(expected, r);
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, is_sorted_until_sorted_tests, ::testing::ValuesIn(is_sorted_sorted_data));

class is_sorted_until_unsorted_tests : public ::testing::TestWithParam < std::array<int, 10> > {};

TEST_P(is_sorted_until_unsorted_tests, test)
{
    std::vector<int> input(10);
    std::copy(cbegin(GetParam()), cend(GetParam()), begin(input));
    array_view<int> input_av(10, input);

    auto expected = std::distance(cbegin(input), std::is_sorted_until(cbegin(input), cend(input)));
    auto r = std::distance(begin(input_av), amp_stl_algorithms::is_sorted_until(begin(input_av), end(input_av)));
    ASSERT_EQ(expected, r);
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, is_sorted_until_unsorted_tests, ::testing::ValuesIn(is_sorted_sorted_data));
