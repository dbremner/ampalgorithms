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
* This file contains unit tests for the merge algorithm.
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
// merge
//----------------------------------------------------------------------------

class stl_algorithms_merge_tests : public stl_algorithms_testbase<13>,
								   public ::testing::Test {};

TEST_F(stl_algorithms_merge_tests, merge_first_range_varies)
{
	using namespace amp_stl_algorithms;

	auto in0 = input;
	std::sort(std::begin(in0), std::end(in0));
	auto in1 = in0;

	const concurrency::array_view<const decltype(in0)::value_type> in0_av(in0);
	const concurrency::array_view<const decltype(in1)::value_type> in1_av(in1);

	for (decltype(in0.size()) i = 2; i != in0.size(); ++i) {
		std::vector<decltype(in0)::value_type> out0(i + in1.size());
		const concurrency::array_view<decltype(out0)::value_type> out1(out0.size());

		const auto result_expect = std::merge(std::cbegin(in0),
											  std::next(std::cbegin(in0), i),
											  std::cbegin(in1),
											  std::cend(in1),
											  std::begin(out0));
		const auto result_amp = amp_stl_algorithms::merge(cbegin(in0_av),
														  std::next(cbegin(in0_av), i),
														  cbegin(in1_av),
														  cend(in1_av),
														  begin(out1));

		ASSERT_EQ(std::distance(std::begin(out0), result_expect),
				  std::distance(begin(out1), result_amp));
		ASSERT_TRUE(std::equal(std::begin(out0), result_expect, cbegin(out1)));
	}
}

TEST_F(stl_algorithms_merge_tests, merge_second_range_varies)
{
	using namespace amp_stl_algorithms;

	auto in0 = input;
	std::sort(std::begin(in0), std::end(in0));
	auto in1 = in0;

	const concurrency::array_view<const decltype(in0)::value_type> in0_av(in0);
	const concurrency::array_view<const decltype(in1)::value_type> in1_av(in1);

	for (decltype(in0.size()) i = 2; i != in0.size(); ++i) {
		std::vector<decltype(in0)::value_type> out0(i + in1.size());
		const concurrency::array_view<decltype(out0)::value_type> out1(out0.size());

		const auto result_expect = std::merge(std::cbegin(in0),
											  std::cend(in0),
											  std::cbegin(in1),
											  std::next(std::cbegin(in1), i),
											  std::begin(out0));
		const auto result_amp = amp_stl_algorithms::merge(cbegin(in0_av),
														  cend(in0_av),
														  cbegin(in1_av),
														  std::next(cbegin(in1_av), i),
														  begin(out1));

		ASSERT_EQ(std::distance(std::begin(out0), result_expect),
				  std::distance(begin(out1), result_amp));
		ASSERT_TRUE(std::equal(std::begin(out0), result_expect, cbegin(out1)));
	}
}

TEST_F(stl_algorithms_merge_tests, merge_with_1st_range_empty)
{
	using namespace amp_stl_algorithms;

	std::sort(std::begin(input), std::end(input));
	std::sort(begin(input_av), end(input_av));

	decltype(input) out;
	const concurrency::array_view<decltype(out)::value_type> out_av(out.size());

	auto result_expect = std::merge(std::cbegin(input),
									std::cbegin(input),
									std::cbegin(input),
									std::cend(input),
									std::begin(out));
	auto result_amp = amp_stl_algorithms::merge(cbegin(input_av),
												cbegin(input_av),
												cbegin(input_av),
												cend(input_av),
												begin(out_av));

	ASSERT_EQ(std::distance(std::begin(out), result_expect),
			  std::distance(begin(out_av), result_amp));
	ASSERT_TRUE(std::equal(std::begin(out), result_expect, cbegin(out_av)));
}

TEST_F(stl_algorithms_merge_tests, merge_with_2nd_range_empty)
{
	using namespace amp_stl_algorithms;

	std::sort(std::begin(input), std::end(input));
	std::sort(begin(input_av), end(input_av));

	decltype(input) out;
	const concurrency::array_view<decltype(out)::value_type> out_av(out.size());

	const auto result_expect = std::merge(std::cbegin(input),
										  std::cend(input),
										  std::cbegin(input),
										  std::cbegin(input),
										  std::begin(out));
	const auto result_amp = amp_stl_algorithms::merge(cbegin(input_av),
													  cend(input_av),
													  cbegin(input_av),
													  cbegin(input_av),
													  begin(out_av));

	ASSERT_EQ(std::distance(std::begin(out), result_expect),
			  std::distance(begin(out_av), result_amp));
	ASSERT_TRUE(std::equal(std::begin(out), result_expect, cbegin(out_av)));
}

TEST_F(stl_algorithms_merge_tests, merge_with_both_ranges_empty)
{
	using namespace amp_stl_algorithms;

	decltype(input) out;
	const concurrency::array_view<decltype(out)::value_type> out_av(out.size());

	const auto result_expect = std::merge(std::cbegin(input),
										  std::cbegin(input),
										  std::cbegin(input),
										  std::cbegin(input),
										  std::begin(out));
	const auto result_amp = amp_stl_algorithms::merge(cbegin(input_av),
													  cbegin(input_av),
													  cbegin(input_av),
													  cbegin(input_av),
													  begin(out_av));

	ASSERT_EQ(std::distance(std::begin(out), result_expect),
			  std::distance(begin(out_av), result_amp));
}

TEST_F(stl_algorithms_merge_tests, merge_with_single_element_1st_range)
{
	using namespace amp_stl_algorithms;

	std::sort(std::begin(input), std::end(input));
	std::sort(begin(input_av), end(input_av));

	std::vector<decltype(input)::value_type> out(input.size() + 1);
	const concurrency::array_view<decltype(out)::value_type> out_av(out.size());

	const auto result_expect = std::merge(std::cbegin(input),
										  std::next(std::cbegin(input)),
										  std::cbegin(input),
										  std::cend(input),
										  std::begin(out));
	const auto result_amp = amp_stl_algorithms::merge(cbegin(input_av),
													  std::next(cbegin(input_av)),
													  cbegin(input_av),
													  cend(input_av),
													  begin(out_av));

	ASSERT_EQ(std::distance(std::begin(out), result_expect),
			  std::distance(begin(out_av), result_amp));
	ASSERT_TRUE(std::equal(std::begin(out), result_expect, cbegin(out_av)));
}

TEST_F(stl_algorithms_merge_tests, merge_with_single_element_2nd_range)
{
	using namespace amp_stl_algorithms;

	std::sort(std::begin(input), std::end(input));
	std::sort(begin(input_av), end(input_av));

	std::vector<decltype(input)::value_type> out(input.size() + 1);
	const concurrency::array_view<decltype(out)::value_type> out_av(out.size());

	const auto result_expect = std::merge(std::cbegin(input),
										  std::cend(input),
										  std::cbegin(input),
										  std::next(std::cbegin(input)),
										  std::begin(out));
	const auto result_amp = amp_stl_algorithms::merge(cbegin(input_av),
													  cend(input_av),
													  cbegin(input_av),
													  std::next(cbegin(input_av)),
													  begin(out_av));

	ASSERT_EQ(std::distance(std::begin(out), result_expect),
			  std::distance(begin(out_av), result_amp));
	ASSERT_TRUE(std::equal(std::begin(out), result_expect, cbegin(out_av)));
}

TEST_F(stl_algorithms_merge_tests, merge_with_single_element_ranges)
{
	using namespace amp_stl_algorithms;

	std::vector<decltype(input)::value_type> out(2);
	const concurrency::array_view<decltype(out)::value_type> out_av(out.size());

	const auto result_expect = std::merge(std::cbegin(input),
										  std::next(std::cbegin(input)),
										  std::next(std::cbegin(input)),
										  std::next(std::cbegin(input), 2),
										  std::begin(out));
	const auto result_amp = amp_stl_algorithms::merge(cbegin(input_av),
													  std::next(cbegin(input_av)),
													  std::next(cbegin(input_av)),
													  std::next(cbegin(input_av), 2),
													  begin(out_av));

	ASSERT_EQ(std::distance(std::begin(out), result_expect),
			  std::distance(begin(out_av), result_amp));
	ASSERT_TRUE(std::equal(std::begin(out), result_expect, cbegin(out_av)));
}

TEST_F(stl_algorithms_merge_tests, merge_large_range)
{
	using namespace amp_stl_algorithms;

	static constexpr decltype(auto) sz = successor(Execution_parameters::tile_size() *
												   Execution_parameters::maximum_tile_cnt());/*successor(1 << 26);*/
	static constexpr decltype(auto) sample = testtools::compute_sample_size(sz);

	auto in0 = testtools::generate_data<int>(sz);
	auto in1 = testtools::generate_data<int>(sz);

	std::sort(std::begin(in0), std::end(in0));
	std::sort(std::begin(in1), std::end(in1));

	const concurrency::array_view<const int> in0_av(in0);
	const concurrency::array_view<const int> in1_av(in1);

	std::vector<int> out(sz);
	const concurrency::array_view<int> out_av(sz);

	std::mt19937_64 g;
	std::uniform_int_distribution<decltype(in0.size())> d(1, predecessor(in0.size()));
	for (decltype(in0.size()) i = 0; i != sample; ++i) {
		const decltype(in0.size()) dx0 = d(g);
		const decltype(in0.size()) dx1 = sz - dx0;

		const auto result_expect = std::merge(std::cbegin(in0),
											  std::next(std::cbegin(in0), dx0),
											  std::cbegin(in1),
											  std::next(std::cbegin(in1), dx1),
											  std::begin(out));
		const auto result_amp = amp_stl_algorithms::merge(cbegin(in0_av),
			 											  std::next(cbegin(in0_av), dx0),
														  cbegin(in1_av),
														  std::next(cbegin(in1_av), dx1),
														  begin(out_av));

		ASSERT_EQ(std::distance(std::begin(out), result_expect),
				  std::distance(begin(out_av), result_amp));
		ASSERT_TRUE(std::equal(std::cbegin(out), std::cend(out), cbegin(out_av)));
	}
}