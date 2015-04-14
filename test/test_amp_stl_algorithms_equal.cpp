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
* This file contains unit tests for the equal algorithm.
*---------------------------------------------------------------------------*/

#include "stdafx.h"
#include <gtest/gtest.h>

#include <amp_iterators.h>
#include <amp_stl_algorithms.h>
#include "testtools.h"

#include <algorithm>
#include <amp.h>
#include <amp_graphics.h>
#include <functional>
#include <random>

//----------------------------------------------------------------------------
// equal
//----------------------------------------------------------------------------

class stl_algorithms_equal_tests : public stl_algorithms_testbase<13>,
								   public ::testing::Test {};

TEST_F(stl_algorithms_equal_tests, equal_true_for_equal_ranges)
{
	using namespace amp_stl_algorithms;

	const auto result_expect = std::equal(cbegin(input_av), cend(input_av), cbegin(input_av));
    const auto result_amp = amp_stl_algorithms::equal(cbegin(input_av),
													  cend(input_av),
													  cbegin(input_av));
    ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_equal_tests, equal_false_for_unequal_ranges)
{
	using namespace amp_stl_algorithms;

	static constexpr int x = 1;
	static constexpr int not_x = 0;

	decltype(input) in0; std::fill_n(std::begin(in0), in0.size(), x);
	decltype(input) in1; std::fill_n(std::begin(in1), in1.size(), not_x);

	const concurrency::array_view<const decltype(x)> in0_av(in0);
	const concurrency::array_view<const decltype(not_x)> in1_av(in1);

	const auto result_expect = std::equal(cbegin(in0_av), cend(in0_av), cbegin(in1_av));
    const auto result_amp = amp_stl_algorithms::equal(cbegin(in0_av), cend(in0_av), cbegin(in1_av));

	ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_equal_tests, equal_pred_true_for_equal_ranges)
{
	using namespace amp_stl_algorithms;

    const auto p = [](auto&& x, auto&& y) restrict(cpu, amp) { return !(x < y) && !(y < x); };

	const auto result_expect = std::equal(cbegin(input_av), cend(input_av), cbegin(input_av), p);
	const auto result_amp = amp_stl_algorithms::equal(cbegin(input_av),
													  cend(input_av),
													  cbegin(input_av),
													  p);

    ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_equal_tests, equal_pred_false_for_unequal_ranges)
{
	using namespace amp_stl_algorithms;

	static constexpr int x = 1;
	static constexpr int not_x = 0;

	decltype(input) in0; std::fill_n(std::begin(in0), in0.size(), x);
	decltype(input) in1; std::fill_n(std::begin(in1), in1.size(), not_x);

	const concurrency::array_view<const decltype(x)> in0_av(in0);
	const concurrency::array_view<const decltype(not_x)> in1_av(in1);

	const auto p = [](auto&& x, auto&& y) restrict(cpu, amp) { return !(x < y) && !(y < x); };
	const auto result_expect = std::equal(cbegin(in0_av), cend(in0_av), cbegin(in1_av), p);
	const auto result_amp = amp_stl_algorithms::equal(cbegin(in0_av), cend(in0_av), cbegin(in1_av), p);

	ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_equal_tests, equal_true_for_empty_range)
{
	using namespace amp_stl_algorithms;

	const auto result_expect = std::equal(cbegin(input_av), cbegin(input_av), cbegin(input_av));
    const auto result_amp = amp_stl_algorithms::equal(cbegin(input_av),
													  cbegin(input_av),
													  cbegin(input_av));
    ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_equal_tests, equal_true_for_single_element_range)
{
	using namespace amp_stl_algorithms;

	const auto result_expect = std::equal(cbegin(input_av),
										  std::next(cbegin(input_av)),
										  cbegin(input_av));
    const auto result_amp = amp_stl_algorithms::equal(cbegin(input_av),
													  std::next(cbegin(input_av)),
													  cbegin(input_av));
    ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_equal_tests, equal_false_for_single_element_range)
{
	using namespace amp_stl_algorithms;

	static constexpr int x = 1;
	static constexpr int not_x = 0;

	const concurrency::array_view<const decltype(x)> in0_av(1, &x);
	const concurrency::array_view<const decltype(not_x)> in1_av(1, &not_x);

	const auto result_expect = std::equal(cbegin(in0_av), cend(in0_av), cbegin(in1_av));
	const auto result_amp = amp_stl_algorithms::equal(cbegin(in0_av), cend(in0_av), cbegin(in1_av));

	ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_equal_tests, equal_true_for_large_equal_ranges)
{
	using namespace amp_stl_algorithms;

	static constexpr decltype(auto) sz = successor(Execution_parameters::tile_size() *
												   Execution_parameters::maximum_tile_cnt());
	static constexpr int x = 1;

	std::vector<int> in(sz, x);
	const concurrency::array_view<const int> in_av(in);

	const auto result_expect = std::equal(cbegin(in_av), cend(in_av), cbegin(in_av));
	const auto result_amp = amp_stl_algorithms::equal(cbegin(in_av), cend(in_av), cbegin(in_av));
	ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_equal_tests, equal_false_for_large_unequal_ranges)
{
	using namespace amp_stl_algorithms;

	static constexpr decltype(auto) sz = successor(Execution_parameters::tile_size() *
												   Execution_parameters::maximum_tile_cnt());
	static constexpr decltype(auto) sample = testtools::compute_sample_size(sz);

	const concurrency::array_view<int> in0(sz); std::iota(in0.data(), in0.data() + sz, 0);
	const concurrency::array_view<int> in1(sz); std::iota(in1.data(), in1.data() + sz, -half_nonnegative(sz));

	for (decltype(in0.extent.size()) i = 0; i != sample; ++i) {
		const auto result_expect = std::equal(cbegin(in0), cend(in0), cbegin(in1));
		const auto result_amp = amp_stl_algorithms::equal(cbegin(in0), cend(in0), cbegin(in1));

		ASSERT_EQ(result_expect, result_amp);

		std::random_shuffle(in0.data(), in0.data() + sz);
		std::random_shuffle(in1.data(), in1.data() + sz);
	}
}
