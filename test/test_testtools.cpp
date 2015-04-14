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
* This file contains unit tests.
*---------------------------------------------------------------------------*/

#include "stdafx.h"

#include <gtest\gtest.h>

#include "amp_stl_algorithms.h"
#include "testtools.h"

using namespace concurrency;
using namespace amp_stl_algorithms;
using namespace testtools;

class testtools_tests : public testbase, public ::testing::Test {};

TEST_F(testtools_tests, cpu_exclusive_scan_plus)
{
    std::array<int, 16> input =    { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
    std::vector<int> result(input.size(), -1);
    std::array<int, 16> expected = { 0, 1, 3, 6, 10, 15, 21, 28, 36, 45, 55, 66, 78, 91, 105, 120 };

    scan_cpu_exclusive(begin(input), end(input), result.begin(), std::plus<int>());

    std::vector<int> exp(begin(expected), end(expected));
    ASSERT_TRUE(exp == result);
}

TEST_F(testtools_tests, cpu_exclusive_scan_multiplies)
{
    std::array<int, 6> input = { 1, 2, 3, 4, 5, 6 };
    std::vector<int> result(input.size(), -1);
    std::array<int, 6> expected = {0, 0, 0, 0, 0, 0 };

    scan_cpu_exclusive(begin(input), end(input), result.begin(), std::multiplies<int>());

    std::vector<int> exp(begin(expected), end(expected));
    ASSERT_TRUE(exp == result);
}

TEST_F(testtools_tests, cpu_inclusive_scan_plus)
{
    std::array<int, 16> input = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
    std::vector<int> result(input.size(), -1);
    std::array<int, 16> expected = { 1, 3, 6, 10, 15, 21, 28, 36, 45, 55, 66, 78, 91, 105, 120, 136 };

    scan_cpu_inclusive(begin(input), end(input), result.begin(), std::plus<int>());

    std::vector<int> exp(begin(expected), end(expected));
    ASSERT_TRUE(exp == result);
}

TEST_F(testtools_tests, cpu_inclusive_scan_multiplies)
{
    std::array<int, 6> input =    { 1,  2,  3,   4,   5,   6 };
    std::vector<int> result(input.size(), -1);
    std::array<int, 6> expected = { 1,  2,  6,  24,  120, 720 };

    scan_cpu_inclusive(begin(input), end(input), result.begin(), std::multiplies<int>());

    std::vector<int> exp(begin(expected), end(expected));
    ASSERT_TRUE(exp == result);
}

TEST_F(testtools_tests, ostream_insertion_std_vector)
{
    auto data = std::vector<int>(5);
    std::iota(begin(data), end(data), 1);
    std::ostringstream stream;
    stream << data;
    ASSERT_STREQ("1,2,3,4,", stream.str().c_str());
}

TEST_F(testtools_tests, ostream_insertion_std_array)
{
    auto data = std::array<int, 5>();
    std::iota(begin(data), end(data), 1);
    std::ostringstream stream;
    stream << data;
    ASSERT_STREQ("1,2,3,4,", stream.str().c_str());
}

TEST_F(testtools_tests, ostream_insertion_amp_array_view)
{
    auto data = std::vector<int>(5);
    std::iota(begin(data), end(data), 1);
    auto data_vw = concurrency::array_view<int, 1>(5, data.data());
    std::ostringstream stream;
    stream << data_vw;
    ASSERT_STREQ("1,2,3,4,", stream.str().c_str());
}

TEST_F(testtools_tests, ostream_insertion_amp_array)
{
    auto data = std::vector<int>(5);
    std::iota(begin(data), end(data), 1);
    auto data_arr = concurrency::array<int, 1>(5);
    copy(begin(data), end(data), data_arr);
    std::ostringstream stream;
    stream << data_arr;
    ASSERT_STREQ("1,2,3,4,", stream.str().c_str());
}

TEST_F(testtools_tests, ostream_insertion_container_width_limited)
{
    auto data = std::vector<int>(5);
    std::iota(begin(data), end(data), 1);
    std::ostringstream stream;
    stream << container_width(2) << data << container_width(3) << " " << data;
    ASSERT_STREQ("1,2, 1,2,3,", stream.str().c_str());
}
