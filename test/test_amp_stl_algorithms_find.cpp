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

class stl_algorithms_tests : public stl_algorithms_testbase<13>, public ::testing::Test {};

TEST_F(stl_algorithms_tests, find_finds_no_values)
{
    auto iter = amp_stl_algorithms::find(cbegin(input_av), cend(input_av), 17);
    ASSERT_EQ(cend(input_av), iter);
}

TEST_F(stl_algorithms_tests, find_finds_value)
{
    auto iter = amp_stl_algorithms::find(cbegin(input_av), cend(input_av), 3);
    ASSERT_EQ(1, std::distance(cbegin(input_av), iter));
}

TEST_F(stl_algorithms_tests, find_finds_first_value)
{
    auto iter = amp_stl_algorithms::find(cbegin(input_av), cend(input_av), 2);
    ASSERT_EQ(4, std::distance(cbegin(input_av), iter));
}

TEST_F(stl_algorithms_tests, find_if_finds_no_values)
{
    auto iter = amp_stl_algorithms::find_if(cbegin(input_av), cend(input_av), [=](int v) restrict(amp) { return v == 17; });
    ASSERT_EQ(cend(input_av), iter);
}

TEST_F(stl_algorithms_tests, find_if_finds_value)
{
    auto iter = amp_stl_algorithms::find_if(cbegin(input_av), cend(input_av), [=](int v) restrict(amp) { return v == 3; });
    ASSERT_EQ(1, std::distance(cbegin(input_av), iter));
}

TEST_F(stl_algorithms_tests, find_if_finds_first_value)
{
    auto iter = amp_stl_algorithms::find_if(cbegin(input_av), cend(input_av), [=](int v) restrict(amp) { return v == 2; });
    ASSERT_EQ(4, std::distance(cbegin(input_av), iter));
}

TEST_F(stl_algorithms_tests, find_if_not_finds_no_values)
{
    auto iter = amp_stl_algorithms::find_if_not(cbegin(input_av), cend(input_av), [=](int v) restrict(amp) { return v != 17; });
    ASSERT_EQ(cend(input_av), iter);
}

TEST_F(stl_algorithms_tests, find_if_not_finds_value)
{
    auto iter = amp_stl_algorithms::find_if_not(cbegin(input_av), cend(input_av), [=](int v) restrict(amp) { return v != 3; });
    ASSERT_EQ(1, std::distance(cbegin(input_av), iter));
}

TEST_F(stl_algorithms_tests, find_if_not_finds_first_value)
{
    auto iter = amp_stl_algorithms::find_if_not(cbegin(input_av), cend(input_av), [=](int v) restrict(amp) { return v != 2; });
    ASSERT_EQ(4, std::distance(cbegin(input_av), iter));
}

const std::array<int, 10> adjacent_find_data[] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },                   // No adjacent values.
    { 0, 1, 2, 3, 4, 4, 6, 7, 8, 9 },                   // Single adjacent value.
    { 0, 0, 2, 3, 4, 4, 6, 7, 8, 9 }                    // Finds first adjacent pair.
};

class adjacent_find_tests : public ::testing::TestWithParam < std::array<int, 10> > {};

TEST_P(adjacent_find_tests, test)
{
    std::vector<int> input(cbegin(GetParam()), cend(GetParam()));
    array_view<int> input_av(10, input);

    auto expected = std::distance(cbegin(input), std::adjacent_find(cbegin(input), cend(input)));
    auto r = std::distance(begin(input_av), amp_stl_algorithms::adjacent_find(begin(input_av), end(input_av)));

    ASSERT_EQ(expected, r);
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, adjacent_find_tests, ::testing::ValuesIn(adjacent_find_data));
