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

const std::array<int, 13> copy_data[] = {
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 1 },
    { 3, 1, 0, 2, 3, 0, 0, 4, 0, 1, 0, 6, 7 }
};

class stl_algorithms_tests : public stl_algorithms_testbase<13>, public ::testing::Test {};


//----------------------------------------------------------------------------
// copy, copy_if, copy_n
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, copy)
{
    std::iota(begin(input), end(input), 1);

    auto iter = amp_stl_algorithms::copy(begin(input_av), end(input_av), begin(output_av));

    ASSERT_TRUE(are_equal(input, output_av));
}

class copy_if_tests : public stl_algorithms_testbase<13>, public ::testing::TestWithParam<std::array<int, 13>> {};

TEST_P(copy_if_tests, test)
{
    std::copy(cbegin(GetParam()), cend(GetParam()), begin(input));
    auto expected_iter = std::copy_if(begin(input), end(input), begin(expected), greater_than<int>());
    auto expected_size = std::distance(begin(expected), expected_iter);

    auto iter = amp_stl_algorithms::copy_if(begin(input_av), end(input_av), begin(output_av), greater_than<int>());

    ASSERT_EQ(expected_size, std::distance(begin(output_av), iter));
    ASSERT_TRUE(are_equal(expected, output_av, expected_size));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, copy_if_tests, ::testing::ValuesIn(copy_data));

TEST_F(stl_algorithms_tests, copy_n)
{
    int size = int(input.size() / 2);
    std::copy_n(cbegin(input), size, begin(expected));
    auto iter = amp_stl_algorithms::copy_n(begin(input_av), size, begin(output_av));

    ASSERT_EQ(size, std::distance(begin(output_av), iter));
    ASSERT_TRUE(are_equal(expected, output_av));
}

//----------------------------------------------------------------------------
// rotate_copy
//----------------------------------------------------------------------------

class rotate_copy_tests : public ::testing::TestWithParam < std::pair<int, int> > {};

TEST_P(rotate_copy_tests, test)
{
    int size = GetParam().first;
    int middle_offset = GetParam().second;
    std::vector<int> vec(size);
    std::iota(begin(vec), end(vec), 0);
    array_view<int> av(int(vec.size()), vec);

    std::vector<int> result(size, 0);
    concurrency::array_view<int> result_av(size, result);
    std::vector<int> expected_result(size, 0);
    auto expected_end = std::rotate_copy(begin(vec), begin(vec) + middle_offset, end(vec), begin(expected_result));

    auto result_end = amp_stl_algorithms::rotate_copy(begin(av), begin(av) + middle_offset, end(av), begin(result_av));

    //ASSERT_TRUE(are_equal(expected_result, result_av));
    ASSERT_EQ((size_t)std::distance(begin(expected_result), expected_end), (size_t)std::distance(begin(av), result_end));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, rotate_copy_tests, ::testing::Values(
    std::make_pair<int, int>(1, 0),
    std::make_pair<int, int>(1023, 200),
    std::make_pair<int, int>(1024, 713))
    );
