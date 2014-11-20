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

const std::array<int, 13> remove_if_data[] = {
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 1 },
    { 3, 1, 0, 2, 3, 0, 0, 4, 0, 1, 0, 6, 7 }
};

class remove_tests : public stl_algorithms_testbase<13>, public ::testing::TestWithParam < std::array<int, 13> > {};

TEST_P(remove_tests, test)
{
    std::copy(begin(GetParam()), end(GetParam()), begin(expected));
    auto expected_iter = std::remove(begin(expected), end(expected), 1);
    auto expected_size = std::distance(begin(expected), expected_iter);
    std::copy(cbegin(GetParam()), cend(GetParam()), begin(input));

    auto iter = amp_stl_algorithms::remove(begin(input_av), end(input_av), 1);

    ASSERT_EQ(expected_size, std::distance(begin(input_av), iter));
    ASSERT_TRUE(are_equal(expected, input_av, expected_size));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, remove_tests, ::testing::ValuesIn(remove_if_data));

class remove_if_tests : public stl_algorithms_testbase<13>, public ::testing::TestWithParam<std::array<int, 13>> {};

TEST_P(remove_if_tests, test)
{
    std::copy(begin(GetParam()), end(GetParam()), begin(expected));
    auto expected_iter = std::remove_if(begin(expected), end(expected), greater_than<int>());
    auto expected_size = std::distance(begin(expected), expected_iter);
    std::copy(cbegin(GetParam()), cend(GetParam()), begin(input));

    auto iter = amp_stl_algorithms::remove_if(begin(input_av), end(input_av), greater_than<int>());

    ASSERT_EQ(std::distance(begin(input_av), iter), std::distance(begin(expected), expected_iter));
    ASSERT_TRUE(are_equal(expected, input_av, expected_size));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, remove_if_tests, ::testing::ValuesIn(remove_if_data));

class remove_copy_tests : public stl_algorithms_testbase<13>, public ::testing::TestWithParam <std::array<int, 13>> {};

TEST_P(remove_copy_tests, test)
{
    std::array<int, size> expected_output;
    std::fill(begin(expected_output), end(expected_output), -1);
    std::copy(begin(GetParam()), end(GetParam()), begin(expected));
    auto expected_iter = std::remove_copy(begin(expected), end(expected), begin(expected_output), 1);
    std::copy(cbegin(GetParam()), cend(GetParam()), begin(input));
    auto expected_size = std::distance(begin(expected_output), expected_iter);

    auto iter = amp_stl_algorithms::remove_copy(begin(input_av), end(input_av), begin(output_av), 1);

    ASSERT_EQ(expected_size, std::distance(begin(input_av), iter));
    ASSERT_TRUE(are_equal(expected, input_av, size - expected_size));
    ASSERT_TRUE(are_equal(expected_output, output_av, expected_size));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, remove_copy_tests, ::testing::ValuesIn(remove_if_data));

class remove_copy_if_tests : public stl_algorithms_testbase<13>, public ::testing::TestWithParam < std::array<int, 13> > {};

TEST_P(remove_copy_if_tests, test)
{
    std::array<int, size> expected_output;
    std::fill(begin(expected_output), end(expected_output), -1);
    std::copy(begin(GetParam()), end(GetParam()), begin(expected));
    auto expected_iter = std::remove_copy_if(begin(expected), end(expected), begin(expected_output), greater_than<int>());
    std::copy(cbegin(GetParam()), cend(GetParam()), begin(input));
    auto expected_size = std::distance(begin(expected_output), expected_iter);

    auto iter = amp_stl_algorithms::remove_copy_if(begin(input_av), end(input_av), begin(output_av), greater_than<int>());

    ASSERT_EQ(expected_size, std::distance(begin(input_av), iter));
    ASSERT_TRUE(are_equal(expected, input_av, size - expected_size));
    ASSERT_TRUE(are_equal(expected_output, output_av, expected_size));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, remove_copy_if_tests, ::testing::ValuesIn(remove_if_data));
