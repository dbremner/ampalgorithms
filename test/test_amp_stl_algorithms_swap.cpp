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

TEST_F(stl_algorithms_tests, swap_cpu)
{
    int a = 1;
    int b = 2;

    amp_stl_algorithms::swap(a, b);

    ASSERT_EQ(2, a);
    ASSERT_EQ(1, b);
}

TEST_F(stl_algorithms_tests, swap_amp)
{
    std::vector<int> vec(2);
    std::iota(begin(vec), end(vec), 1);
    array_view<int> av(2, vec);

    parallel_for_each(concurrency::extent<1>(1), [=](auto&& idx) restrict(amp) {
        amp_stl_algorithms::swap(av[idx], av[idx + 1]);
    });

    ASSERT_EQ(2, av[0]);
    ASSERT_EQ(1, av[1]);
}

TEST_F(stl_algorithms_tests, swap_n_cpu)
{
    int arr1[size];
    int arr2[size];
    std::iota(arr1, arr1 + size, 0);
    std::iota(arr2, arr2 + size, -9);

    amp_stl_algorithms::swap<int, size>(arr1, arr2);

    for (int i = 0; i < size; ++i)
    {
        EXPECT_EQ(i, arr2[i]);
        EXPECT_EQ((-9 + i), arr1[i]);
    }
}

TEST_F(stl_algorithms_tests, swap_n_amp)
{
    static constexpr std::array<int, 10> exp = { 6, 7, 8, 9, 10, 1, 2, 3, 4, 5 };

    std::vector<int> vec(10);
    std::iota(begin(vec), end(vec), 1);
    array_view<int> av(10, vec);

    parallel_for_each(concurrency::extent<1>(5).tile<5>(), [=](auto&& tidx) restrict(amp) {
        tile_static int arr1[5];
        tile_static int arr2[5];

        arr1[tidx.local[0]] = av[tidx.local[0]];
        arr2[tidx.local[0]] = av[tidx.local[0] + 5];

        tidx.barrier.wait_with_tile_static_memory_fence();

        if (tidx.tile_origin == tidx.global) {
            amp_stl_algorithms::swap(arr1, arr2);
        }

        tidx.barrier.wait_with_tile_static_memory_fence();

        av[tidx.local[0]] = arr1[tidx.local[0]];
        av[tidx.local[0] + 5] = arr2[tidx.local[0]];
    });
    ASSERT_TRUE(are_equal(exp, av));
}

TEST_F(stl_algorithms_tests, swap_ranges)
{
    auto block_size = size / 6;
    std::copy(std::cbegin(input), std::cend(input), std::begin(expected));
    std::swap_ranges(std::begin(expected) + block_size, std::begin(expected) + block_size * 2,
					 std::begin(expected) + block_size * 4);

    auto expected_end = amp_stl_algorithms::swap_ranges(amp_stl_algorithms::begin(input_av) + block_size,
														amp_stl_algorithms::begin(input_av) + block_size * 2,
														amp_stl_algorithms::begin(input_av) + block_size * 4);

    ASSERT_TRUE(are_equal(expected, input_av));
}

TEST_F(stl_algorithms_tests, swap_iter)
{
    std::vector<int> vec(2);
    std::iota(begin(vec), end(vec), 1);
    array_view<int> av(2, vec);

    parallel_for_each(concurrency::extent<1>(1), [=](auto&&) restrict(amp) {
        amp_stl_algorithms::iter_swap(begin(av), begin(av) + 1);
    });

    ASSERT_EQ(2, av[0]);
    ASSERT_EQ(1, av[1]);
}
