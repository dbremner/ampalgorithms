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

#include "amp_algorithms.h"
#include "testtools.h"

using namespace amp_algorithms;
using namespace testtools;

static const int test_tile_size = 256;

class amp_algorithms_scan_tests : public testbase, public ::testing::Test {};

TEST_F(amp_algorithms_scan_tests, details_scan_tile_exclusive)
{
    static const int tile_size = 4;

    std::array<unsigned, 16> input =        {  3,  2,  1,  6,   10, 11, 13,  1,   15, 10,  5, 14,    4, 12,  9,  8 };
    std::array<unsigned, 16> reduce =       {  3,  5,  1, 12,   10, 21, 13, 35,   15, 25,  5, 44,    4, 16,  9, 33 };
    std::array<unsigned, 16> zero_top =     {  3,  5,  1,  0,   10, 21, 13,  0,   15, 25,  5,  0,    4, 16,  9,  0 };
    std::array<unsigned, 16> expected =     {  0,  3,  5,  6,    0, 10, 21, 34,    0, 15, 25, 30,    0,  4, 16, 25 };

    array_view<unsigned> input_av(static_cast<int>(input.size()), input);
    std::array<unsigned, 16> output;
    array_view<unsigned> output_av(static_cast<int>(output.size()), output);
    amp_algorithms::fill(output_av, 0);
    concurrency::tiled_extent<tile_size> compute_domain = input_av.get_extent().tile<4>().pad();

    concurrency::parallel_for_each(compute_domain, [=](concurrency::tiled_index<tile_size> tidx) restrict(amp)
    {
        const int gidx = tidx.global[0];
        const int idx = tidx.local[0];
        tile_static int tile_data[tile_size];
        tile_data[idx] = input_av[gidx];

        amp_algorithms::_details::scan_tile_exclusive<tile_size>(tile_data, tidx, amp_algorithms::plus<int>());

        tidx.barrier.wait();
        output_av[gidx] = tile_data[idx];
    });

    ASSERT_TRUE(are_equal(expected, output_av));
}

TEST_F(amp_algorithms_scan_tests, details_segment_scan_width_2)
{
    // Initial values                     3, 2,   1, 6,   10, 11,   13,  1,   15, 10,    5, 14,     4,  12,     9,   8
    std::array<unsigned, 16> input =    { 0, 3,   5, 6,   12, 22,   33, 47,   62, 77,   87, 92,   106, 118,   127, 135 };
    std::array<unsigned, 16> expected = { 0, 3,   0, 1,    0, 10,    0, 14,    0, 15,    0,  5,     0,  12,     0,   8 };

    array_view<unsigned> input_av(static_cast<int>(input.size()), input);
    std::array<unsigned, 16> output;

    for (int i = 0; i < static_cast<int>(output.size()); ++i)
    {
        output[i] = amp_algorithms::_details::segment_exclusive_scan(input_av, 2, i);
    }

    ASSERT_TRUE(are_equal(expected, output));
}

TEST_F(amp_algorithms_scan_tests, details_segment_scan_width_8)
{
    // Initial values                     3, 2, 1, 6, 10, 11, 13,  1,   15, 10,  5, 14,   4,  12,   9,   8
    std::array<unsigned, 16> input =    { 0, 3, 5, 6, 12, 22, 33, 47,   62, 77, 87, 92, 106, 118, 127, 135 };
    std::array<unsigned, 16> expected = { 0, 3, 5, 6, 12, 22, 33, 47,    0, 15, 25, 30,  44,  56,  65,  73 };

    array_view<unsigned> input_av(static_cast<int>(input.size()), input);
    std::array<unsigned, 16> output;

    for (int i = 0; i < static_cast<int>(output.size()); ++i)
    {
        output[i] = amp_algorithms::_details::segment_exclusive_scan(input_av, 8, i);
    }

    ASSERT_TRUE(are_equal(expected, output));
}

TEST_F(amp_algorithms_scan_tests, details_scan_tile_exclusive_partial)
{
    std::array<unsigned, 7> input =    { 3, 2, 1, 6, 10, 11,  5 };
    std::array<unsigned, 7> expected = { 0, 3, 5, 6, 12, 22, 33 };

    array_view<unsigned> input_av(static_cast<int>(input.size()), input);
    std::array<unsigned, 7> output;
    array_view<unsigned> output_av(static_cast<int>(output.size()), output);
    amp_algorithms::fill(output_av, 0);
    concurrency::tiled_extent<16> compute_domain = input_av.get_extent().tile<16>().pad();

    concurrency::parallel_for_each(compute_domain, [=](concurrency::tiled_index<16> tidx) restrict(amp)
    {
        const int gidx = tidx.global[0];
        const int idx = tidx.local[0];
        tile_static int tile_data[16];
        tile_data[idx] = padded_read(input_av, gidx);

        amp_algorithms::_details::scan_tile_exclusive<16>(tile_data, tidx, amp_algorithms::plus<int>());

        padded_write(output_av, gidx, tile_data[idx]);
    });

    ASSERT_TRUE(are_equal(expected, output_av));
}

TEST_F(amp_algorithms_scan_tests, exclusive_single_tile)
{
    std::vector<int> input(test_tile_size);
    generate_data(input);
    concurrency::array_view<int, 1> input_vw(static_cast<int>(input.size()), input);
    std::vector<int> expected(input.size());
    scan_cpu_exclusive(cbegin(input), cend(input), begin(expected), std::plus<int>());

    scan<test_tile_size, scan_mode::exclusive>(input_vw, input_vw, amp_algorithms::plus<int>());

    input_vw.synchronize();
    ASSERT_TRUE(are_equal(expected, input_vw));
}

TEST_F(amp_algorithms_scan_tests, inclusive_single_tile)
{
    std::vector<int> input(test_tile_size);
    generate_data(input);
    concurrency::array_view<int, 1> input_vw(static_cast<int>(input.size()), input);
    std::vector<int> expected(input.size());
    scan_cpu_inclusive(cbegin(input), cend(input), begin(expected), std::plus<int>());

    scan<test_tile_size, scan_mode::inclusive>(input_vw, input_vw, amp_algorithms::plus<int>());

    input_vw.synchronize();
    ASSERT_TRUE(are_equal(expected, input_vw));
}

TEST_F(amp_algorithms_scan_tests, exclusive_multi_tile)
{
    std::vector<int> input(test_tile_size * 4);
    generate_data(input);
    concurrency::array_view<int, 1> input_vw(static_cast<int>(input.size()), input);
    std::vector<int> expected(input.size());
    scan_cpu_exclusive(cbegin(input), cend(input), begin(expected), std::plus<int>());

    scan<test_tile_size, scan_mode::exclusive>(input_vw, input_vw, amp_algorithms::plus<int>());

    input_vw.synchronize();
    ASSERT_TRUE(are_equal(expected, input_vw));
}

TEST_F(amp_algorithms_scan_tests, inclusive_multi_tile)
{
    std::vector<int> input(test_tile_size * 4);
    generate_data(input);
    std::fill(begin(input), end(input), 1);
    concurrency::array_view<int, 1> input_vw(static_cast<int>(input.size()), input);
    std::vector<int> expected(input.size());
    scan_cpu_inclusive(cbegin(input), cend(input), begin(expected), std::plus<int>());

    scan<test_tile_size, scan_mode::inclusive>(input_vw, input_vw, amp_algorithms::plus<int>());

    input_vw.synchronize();
    ASSERT_TRUE(are_equal(expected, input_vw));
}

TEST_F(amp_algorithms_scan_tests, exclusive_multi_tile_partial)
{
    std::vector<int> input(test_tile_size * 4 + 3);
    generate_data(input);
    concurrency::array_view<int, 1> input_vw(static_cast<int>(input.size()), input);
    std::vector<int> expected(input.size());
    scan_cpu_exclusive(cbegin(input), cend(input), begin(expected), std::plus<int>());

    scan<test_tile_size, scan_mode::exclusive>(input_vw, input_vw, amp_algorithms::plus<int>());

    input_vw.synchronize();
    ASSERT_TRUE(expected == input);
}

TEST_F(amp_algorithms_scan_tests, inclusive_multi_tile_partial)
{
    std::vector<int> input(test_tile_size * 4 + 3);
    generate_data(input);
    concurrency::array_view<int, 1> input_vw(static_cast<int>(input.size()), input);
    std::vector<int> expected(input.size());
    scan_cpu_inclusive(cbegin(input), cend(input), begin(expected), std::plus<int>());

    scan<test_tile_size, scan_mode::inclusive>(input_vw, input_vw, amp_algorithms::plus<int>());

    input_vw.synchronize();
    ASSERT_TRUE(expected == input);
}

TEST_F(amp_algorithms_scan_tests, exclusive_recursive)
{
    std::vector<int> input(test_tile_size * (test_tile_size + 2));
    generate_data(input);
    concurrency::array_view<int, 1> input_vw(static_cast<int>(input.size()), input);
    std::vector<int> expected(input.size());
    scan_cpu_exclusive(cbegin(input), cend(input), begin(expected), std::plus<int>());

    scan<test_tile_size, scan_mode::exclusive>(input_vw, input_vw, amp_algorithms::plus<int>());

    input_vw.synchronize();
    ASSERT_TRUE(expected == input);
}

TEST_F(amp_algorithms_scan_tests, inclusive_recursive)
{
    std::vector<int> input(test_tile_size * (test_tile_size + 2));
    generate_data(input);
    std::fill(begin(input), end(input), 1);
    concurrency::array_view<int, 1> input_vw(static_cast<int>(input.size()), input);
    std::vector<int> expected(input.size());
    scan_cpu_inclusive(cbegin(input), cend(input), begin(expected), std::plus<int>());

    scan<test_tile_size, scan_mode::inclusive>(input_vw, input_vw, amp_algorithms::plus<int>());

    input_vw.synchronize();
    ASSERT_TRUE(expected == input);
}

//----------------------------------------------------------------------------
// Public API Acceptance Tests
//----------------------------------------------------------------------------

typedef ::testing::Types<
    TestDefinition<int,        83, int(scan_mode::inclusive)>,     // Less than one tile.
    TestDefinition<unsigned,   83, int(scan_mode::exclusive)>,
    TestDefinition<float,      83, int(scan_mode::inclusive)>,
    TestDefinition<int,       256, int(scan_mode::inclusive)>,     // Exactly one tile.
    TestDefinition<unsigned,  256, int(scan_mode::exclusive)>,
    TestDefinition<float,     256, int(scan_mode::inclusive)>,
    TestDefinition<int,      1024, int(scan_mode::inclusive)>,     // Several whole tiles.
    TestDefinition<unsigned, 1024, int(scan_mode::exclusive)>,
    TestDefinition<float,    1024, int(scan_mode::inclusive)>,
    TestDefinition<int,      1283, int(scan_mode::inclusive)>,     // Partial tile.
    TestDefinition<unsigned, 1283, int(scan_mode::exclusive)>,
    TestDefinition<float,    1283, int(scan_mode::inclusive)>,
    TestDefinition<int,      7919, int(scan_mode::inclusive)>,     // Lots of tiles and a partial.
    TestDefinition<int,      7919, int(scan_mode::exclusive)>
> scan_acceptance_data;

template <typename T>
class amp_scan_acceptance_tests : public ::testing::Test { };
TYPED_TEST_CASE_P(amp_scan_acceptance_tests);

TYPED_TEST_P(amp_scan_acceptance_tests, test)
{
    typedef TypeParam::value_type T;
    const int size = TypeParam::size;
    static const scan_mode mode = scan_mode(TypeParam::parameter);

    std::vector<T> input(size);
    generate_data(input);
    concurrency::array_view<T, 1> input_vw(static_cast<int>(input.size()), input);
    std::vector<T> expected(input.size());
    scan_cpu<(int)mode>(cbegin(input), cend(input), begin(expected), std::plus<T>());

    scan<test_tile_size, mode>(input_vw, input_vw, amp_algorithms::plus<T>());

    input_vw.synchronize();
    ASSERT_TRUE(expected == input);
}

REGISTER_TYPED_TEST_CASE_P(amp_scan_acceptance_tests, test);
INSTANTIATE_TYPED_TEST_CASE_P(amp_scan_tests, amp_scan_acceptance_tests, scan_acceptance_data);
