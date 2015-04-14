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
* C++ AMP standard algorithms library.
*
* This file contains unit tests.
*---------------------------------------------------------------------------*/

#include "stdafx.h"

#include <gtest\gtest.h>

#include "amp_algorithms.h"
#include "amp_algorithms_execution_parameters.h"
#include "amp_stl_algorithms.h"
#include "testtools.h"

using namespace amp_algorithms;
using namespace testtools;

static constexpr decltype(auto) test_tile_size = amp_stl_algorithms::
												 Execution_parameters::tile_size();

class amp_algorithms_scan_tests : public testbase, public ::testing::Test {};

TEST_F(amp_algorithms_scan_tests, details_scan_tile_exclusive)
{
    static constexpr int tile_size = 4;

    std::array<unsigned, 16> input =        {  3,  2,  1,  6,   10, 11, 13,  1,   15, 10,  5, 14,    4, 12,  9,  8 };
//    std::array<unsigned, 16> reduce =       {  3,  5,  1, 12,   10, 21, 13, 35,   15, 25,  5, 44,    4, 16,  9, 33 };
//    std::array<unsigned, 16> zero_top =     {  3,  5,  1,  0,   10, 21, 13,  0,   15, 25,  5,  0,    4, 16,  9,  0 };
    std::array<unsigned, 16> expected =     {  0,  3,  5,  6,    0, 10, 21, 34,    0, 15, 25, 30,    0,  4, 16, 25 };

    array_view<unsigned> input_av(static_cast<int>(input.size()), input);
    std::array<unsigned, 16> output;
    array_view<unsigned> output_av(static_cast<int>(output.size()), output);
    amp_algorithms::fill(output_av, 0u);
    const auto compute_domain = input_av.get_extent().tile<tile_size>().pad();

    concurrency::parallel_for_each(compute_domain, [=](auto&& tidx) restrict(amp)
    {
        const int gidx = tidx.global[0];
        const int idx = tidx.local[0];
        tile_static int tile_data[tidx.tile_dim0];
        tile_data[idx] = input_av[gidx];

        amp_algorithms::_details::scan_tile_exclusive<tidx.tile_dim0>(tile_data,
																	  tidx,
																	  amp_algorithms::plus<>());

        tidx.barrier.wait_with_tile_static_memory_fence();
        output_av[gidx] = tile_data[idx];
    });

    ASSERT_TRUE(std::equal(std::cbegin(expected),
						   std::cend(expected),
						   amp_stl_algorithms::cbegin(output_av)));
}

TEST_F(amp_algorithms_scan_tests, details_segment_scan_width_2)
{
    // Initial values                     3, 2,   1, 6,   10, 11,   13,  1,   15, 10,    5, 14,     4,  12,     9,   8
    std::array<unsigned, 16> input =    { 0, 3,   5, 6,   12, 22,   33, 47,   62, 77,   87, 92,   106, 118,   127, 135 };
    std::array<unsigned, 16> expected = { 0, 3,   0, 1,    0, 10,    0, 14,    0, 15,    0,  5,     0,  12,     0,   8 };

    array_view<unsigned> input_av(input.size(), input);
    std::array<unsigned, 16> output;

	int i = 0;
	std::generate_n(std::begin(output), output.size(), [&]() {
		return amp_algorithms::_details::segment_exclusive_scan(input_av, 2, i++);
	});

    ASSERT_TRUE(std::equal(std::cbegin(expected),
						   std::cend(expected),
						   std::cbegin(output)));
}

TEST_F(amp_algorithms_scan_tests, details_segment_scan_width_8)
{
    // Initial values                     3, 2, 1, 6, 10, 11, 13,  1,   15, 10,  5, 14,   4,  12,   9,   8
    std::array<unsigned, 16> input =    { 0, 3, 5, 6, 12, 22, 33, 47,   62, 77, 87, 92, 106, 118, 127, 135 };
    std::array<unsigned, 16> expected = { 0, 3, 5, 6, 12, 22, 33, 47,    0, 15, 25, 30,  44,  56,  65,  73 };

    array_view<unsigned> input_av(input.size(), input);
    std::array<unsigned, 16> output;

	int i = 0;
	std::generate_n(std::begin(output), output.size(), [&]() {
		return amp_algorithms::_details::segment_exclusive_scan(input_av, 8, i++);
	});

    ASSERT_TRUE(std::equal(std::cbegin(expected),
						   std::cend(expected),
						   std::cbegin(output)));
}

TEST_F(amp_algorithms_scan_tests, details_scan_tile_exclusive_partial)
{
    std::array<unsigned, 7> input =    { 3, 2, 1, 6, 10, 11,  5 };
    std::array<unsigned, 7> expected = { 0, 3, 5, 6, 12, 22, 33 };

    array_view<unsigned> input_av(static_cast<int>(input.size()), input);
    std::array<unsigned, 7> output;
    array_view<unsigned> output_av(static_cast<int>(output.size()), output);
    amp_algorithms::fill(output_av, 0u);
    const auto compute_domain = input_av.get_extent().tile<16>().pad();

    concurrency::parallel_for_each(compute_domain, [=](concurrency::tiled_index<16> tidx) restrict(amp)
    {
        const int gidx = tidx.global[0];
        const int idx = tidx.local[0];
        tile_static int tile_data[16];
        tile_data[idx] = input_av[gidx];

        amp_algorithms::_details::scan_tile_exclusive<16>(tile_data, tidx, amp_algorithms::plus<>());

        output_av[gidx] = tile_data[idx];
    });

    ASSERT_TRUE(std::equal(std::cbegin(expected),
						   std::cend(expected),
						   amp_stl_algorithms::cbegin(output_av)));
}

TEST_F(amp_algorithms_scan_tests, exclusive_multi_tile)
{
	using namespace amp_stl_algorithms;

    std::vector<int> input(test_tile_size * 4);
	std::iota(std::begin(input), std::end(input), 1);
    concurrency::array_view<int> input_vw(input);
    std::vector<int> expected(input.size());

	scan_cpu_exclusive(cbegin(input), cend(input), begin(expected), std::plus<>());
	inplace_exclusive_scan(begin(input_vw), end(input_vw), 0, amp_algorithms::plus<>());

    ASSERT_TRUE(std::equal(std::cbegin(expected), std::cend(expected), cbegin(input_vw)));
}

TEST_F(amp_algorithms_scan_tests, exclusive_multi_tile_partial)
{
	using namespace amp_stl_algorithms;

    std::vector<int> input(generate_data<int>(test_tile_size * 4 + predecessor(test_tile_size)));
    concurrency::array_view<int> input_vw(input);
    std::vector<int> expected(input.size());

	scan_cpu_exclusive(cbegin(input), cend(input), begin(expected), std::plus<>());
	inplace_exclusive_scan(begin(input_vw), end(input_vw), 0, amp_algorithms::plus<>());

    ASSERT_TRUE(std::equal(std::cbegin(expected), std::cend(expected), cbegin(input_vw)));
}

TEST_F(amp_algorithms_scan_tests, exclusive_recursive_scan)
{
	using namespace amp_stl_algorithms;

    std::vector<int> input(successor(Execution_parameters::maximum_tile_cnt()) * test_tile_size, 1);
    concurrency::array_view<int> input_vw(input);
    std::vector<int> expected(input.size());

	scan_cpu_exclusive(std::cbegin(input), std::cend(input), std::begin(expected), std::plus<>());
    inplace_exclusive_scan(begin(input_vw), end(input_vw), 0, amp_algorithms::plus<>());

	*input_vw.data();
    ASSERT_TRUE(std::equal(std::cbegin(expected), std::cend(expected), cbegin(input_vw)));
}

TEST_F(amp_algorithms_scan_tests, exclusive_large_data)
{
	using namespace amp_stl_algorithms;

    std::vector<int> input(generate_data<int>(twice(Execution_parameters::maximum_tile_cnt()) * test_tile_size));
    concurrency::array_view<int> input_vw(input);
    std::vector<int> expected(input.size());

    scan_cpu_exclusive(std::cbegin(input), std::cend(input), std::begin(expected), std::plus<>());
	inplace_exclusive_scan(begin(input_vw), end(input_vw), 0, amp_algorithms::plus<>());

	ASSERT_TRUE(std::equal(std::cbegin(expected), std::cend(expected), cbegin(input_vw)));
}

TEST_F(amp_algorithms_scan_tests, inclusive_multi_tile_plus)
{
	using namespace amp_stl_algorithms;

	auto input = ::testtools::generate_data<int>(test_tile_size * 4);
    concurrency::array_view<int> input_vw(input);
    std::vector<int> expected(input.size());

	scan_cpu_inclusive(std::cbegin(input), std::cend(input), std::begin(expected), std::plus<>());
    inplace_inclusive_scan(begin(input_vw), end(input_vw), 0, amp_algorithms::plus<>());

    ASSERT_TRUE(std::equal(std::cbegin(expected), std::cend(expected), cbegin(input_vw)));
}

TEST_F(amp_algorithms_scan_tests, inclusive_multi_tile_partial)
{
	using namespace amp_stl_algorithms;

	auto input = generate_data<int>(test_tile_size * 4 + predecessor(test_tile_size));
	concurrency::array_view<int> input_vw(input);
	std::vector<int> expected(input.size());

	scan_cpu_inclusive(std::cbegin(input), std::cend(input), std::begin(expected), std::plus<>());
	inplace_inclusive_scan(begin(input_vw), end(input_vw), 0, amp_algorithms::plus<>());

	ASSERT_TRUE(std::equal(std::cbegin(expected), std::cend(expected), cbegin(input_vw)));
}

TEST_F(amp_algorithms_scan_tests, inclusive_recursive_scan)
{
	using namespace amp_stl_algorithms;

	std::vector<int> input(generate_data<int>(successor(Execution_parameters::maximum_tile_cnt()) * test_tile_size));
	concurrency::array_view<int> input_vw(input);
	std::vector<int> expected(input.size());

	scan_cpu_inclusive(std::cbegin(input), std::cend(input), std::begin(expected), std::plus<>());
	inplace_inclusive_scan(begin(input_vw), end(input_vw), 0, amp_algorithms::plus<>());

	ASSERT_TRUE(std::equal(std::cbegin(expected), std::cend(expected), cbegin(input_vw)));
}

TEST_F(amp_algorithms_scan_tests, inclusive_large_data)
{
	using namespace amp_stl_algorithms;

	std::vector<int> input(generate_data<int>(twice(Execution_parameters::maximum_tile_cnt()) * test_tile_size));
	concurrency::array_view<int> input_vw(input);
	std::vector<int> expected(input.size());

	scan_cpu_inclusive(cbegin(input), cend(input), begin(expected), std::plus<int>());
	inplace_inclusive_scan(begin(input_vw), end(input_vw), 0, amp_algorithms::plus<>());

	ASSERT_TRUE(std::equal(std::cbegin(expected), std::cend(expected), cbegin(input_vw)));
}