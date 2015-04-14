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

//#include "stdafx.h"

#include <amp_algorithms.h>
#include <gtest/gtest.h>

#include "testtools.h"

using namespace concurrency;
using namespace amp_algorithms;
using namespace testtools;

// TODO: Add tests for indexable_view_traits
class amp_algorithms_tests : public stl_algorithms_testbase<13>, public ::testing::Test {};

TEST_F(amp_algorithms_tests, padded_read)
{
    ASSERT_EQ(input[1], padded_read(input_av, concurrency::index<1>(1)));
    ASSERT_EQ(int(), padded_read(input_av, concurrency::index<1>(size + 2)));
}

TEST_F(amp_algorithms_tests, padded_write)
{
    std::fill(begin(input), end(input), 0);

    padded_write(input_av, concurrency::index<1>(1), 11);
    ASSERT_EQ(11, input_av[1]);
    padded_write(input_av, concurrency::index<1>(size + 2), 11);
}

TEST_F(amp_algorithms_tests, generate_int)
{
    std::vector<int> vec(1024);
    array_view<int,1> av(1024, vec);
    av.discard_data();

    amp_algorithms::generate(av, []() restrict(amp) { return 7; });
    av.synchronize();

    for (auto&& e : vec)
    {
        EXPECT_EQ(7, e);
    }
}

TEST_F(amp_algorithms_tests, transform_unary)
{
    constexpr int height = 16;
    constexpr int width = 16;
    constexpr int sz = height * width;

    std::vector<int> vec_in(sz);
    std::fill(begin(vec_in), end(vec_in), 7);
    array_view<const int, 2> av_in(height, width, vec_in);

    std::vector<int> vec_out(sz);
    array_view<int,2> av_out(height, width, vec_out);

    // Test "transform" by doubling the input elements

    amp_algorithms::transform(av_in, av_out, [] (int x) restrict(amp) {
        return 2 * x;
    });
    av_out.synchronize();

    for (auto&& e : vec_out) {
        EXPECT_EQ(2 * 7, e);
    }
}

TEST_F(amp_algorithms_tests, transform_binary)
{
    constexpr int depth = 16;
    constexpr int height = 16;
    constexpr int width = 16;
    constexpr int sz = depth * height * width;

    std::vector<int> vec_in1(sz);
    std::fill(begin(vec_in1), end(vec_in1), 343);
    array_view<const int, 3> av_in1(depth, height, width, vec_in1);

    std::vector<int> vec_in2(sz);
    std::fill(begin(vec_in2), end(vec_in2), 323);
    array_view<const int, 3> av_in2(depth, height, width, vec_in2);

    std::vector<int> vec_out(sz);
    array_view<int, 3> av_out(depth, height, width, vec_out);

    // Test "transform" by adding the two input elements

    amp_algorithms::transform(av_in1, av_in2, av_out, amp_algorithms::plus<>());
    av_out.synchronize();

    for (auto&& e : vec_out) {
        EXPECT_EQ(343 + 323, e);
    }
}

TEST_F(amp_algorithms_tests, fill_int)
{
    std::vector<int> vec(1024);
    array_view<int> av(vec);
    av.discard_data();

    amp_algorithms::fill(av, 7);
    av.synchronize();

    for (auto&& e : vec) {
        EXPECT_EQ(7, e);
    }
}