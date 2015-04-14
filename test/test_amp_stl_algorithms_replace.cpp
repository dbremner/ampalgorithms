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

const std::array<int, 13> replace_data[] = {
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 1 },
    { 3, 1, 0, 2, 3, 0, 0, 4, 0, 1, 0, 6, 7 }
};

class stl_algorithms_tests : public stl_algorithms_testbase<13>, public ::testing::Test {};

class replace_tests : public stl_algorithms_testbase<13>, public ::testing::TestWithParam < std::array<int, 13> > {};

TEST_P(replace_tests, test)
{
    std::copy(begin(GetParam()), end(GetParam()), begin(expected));
    std::replace(begin(expected), end(expected), 1, -1);
    std::copy(cbegin(GetParam()), cend(GetParam()), begin(input));

    amp_stl_algorithms::replace(begin(input_av), end(input_av), 1, -1);

    ASSERT_TRUE(are_equal(expected, input_av));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, replace_tests, ::testing::ValuesIn(replace_data));

class replace_if_tests : public stl_algorithms_testbase<13>, public ::testing::TestWithParam < std::array<int, 13> > {};

TEST_P(replace_if_tests, test)
{
    std::copy(begin(GetParam()), end(GetParam()), begin(expected));
    std::replace_if(begin(expected), end(expected), greater_than<int>(), -1);
    std::copy(cbegin(GetParam()), cend(GetParam()), begin(input));

    amp_stl_algorithms::replace_if(begin(input_av), end(input_av), greater_than<int>(), -1);

    ASSERT_TRUE(are_equal(expected, input_av));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, replace_if_tests, ::testing::ValuesIn(replace_data));

class replace_copy_tests : public stl_algorithms_testbase<13>, public ::testing::TestWithParam < std::array<int, 13> > {};

TEST_F(stl_algorithms_tests, replace_copy)
{
    auto expected_r = std::replace_copy(begin(input), end(input), begin(expected), 2, -2);

    auto r = amp_stl_algorithms::replace_copy(begin(input_av), end(input_av), begin(output_av), 2, -2);

    ASSERT_TRUE(are_equal(expected, output_av));
    ASSERT_EQ(std::distance(begin(expected), expected_r), std::distance(begin(output_av), r));
}

TEST_P(replace_copy_tests, test)
{
    std::array<int, 13> expected_output;
    std::fill(begin(expected_output), end(expected_output), -1);
    std::copy(begin(GetParam()), end(GetParam()), begin(expected));
    auto expected_iter = std::replace_copy(begin(expected), end(expected), begin(expected_output), 1, -1);
    auto expected_size = std::distance(begin(expected_output), expected_iter);
    std::copy(cbegin(GetParam()), cend(GetParam()), begin(input));

    auto iter = amp_stl_algorithms::replace_copy(begin(input_av), end(input_av), begin(output_av), 1, -1);

    //ASSERT_EQ(expected_size, std::distance(begin(output_av), iter));
    ASSERT_TRUE(are_equal(expected, input_av));
    ASSERT_TRUE(are_equal(expected_output, output_av, expected_size - 1));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, replace_copy_tests, ::testing::ValuesIn(replace_data));

class replace_copy_if_tests : public stl_algorithms_testbase<13>, public ::testing::TestWithParam < std::array<int, 13> > {};

TEST_P(replace_copy_if_tests, test)
{
    std::array<int, 13> expected_output;
    std::fill(begin(expected_output), end(expected_output), -1);
    std::copy(begin(GetParam()), end(GetParam()), begin(expected));
    auto expected_iter = std::replace_copy(begin(expected), end(expected), begin(expected_output), 1, -1);
    auto expected_size = std::distance(begin(expected_output), expected_iter);
    std::copy(cbegin(GetParam()), cend(GetParam()), begin(input));

    auto iter = amp_stl_algorithms::replace_copy(begin(input_av), end(input_av), begin(output_av), 1, -1);

    ASSERT_EQ(expected_size, std::distance(begin(output_av), iter));
    ASSERT_TRUE(are_equal(expected, input_av));
    ASSERT_TRUE(are_equal(expected_output, output_av, expected_size - 1));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, replace_copy_if_tests, ::testing::ValuesIn(replace_data));
