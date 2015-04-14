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
* This file contains unit tests for the equal_range algorithm.
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
// equal_range
//----------------------------------------------------------------------------

class stl_algorithms_equal_range_tests : public stl_algorithms_testbase<13>,
								         public ::testing::Test {};

TEST_F(stl_algorithms_equal_range_tests, equal_range_finds)
{
	using namespace amp_stl_algorithms;

	auto in = input; std::sort(std::begin(in), std::end(in));
	const concurrency::array_view<const decltype(in)::value_type> in_av(in);

	for (auto&& key : input) {
		const auto result_expect = std::equal_range(std::cbegin(in), std::cend(in), key);
		const auto result_amp = amp_stl_algorithms::equal_range(cbegin(in_av), cend(in_av), key);

		ASSERT_EQ(std::distance(std::cbegin(in), result_expect.first),
				  std::distance(cbegin(in_av), result_amp.first));
		ASSERT_EQ(std::distance(std::cbegin(in), result_expect.second),
				  std::distance(cbegin(in_av), result_amp.second));
	}
}

TEST_F(stl_algorithms_equal_range_tests, equal_does_not_find_key_is_greater)
{
	using namespace amp_stl_algorithms;

	auto in0 = input; std::sort(std::begin(in0), std::end(in0));
	auto in1 = in0;
	const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

	const auto key = std::numeric_limits<decltype(in0)::value_type>::max();
	const auto result_expect = std::equal_range(std::cbegin(in0), std::cend(in0), key);
	const auto result_amp = amp_stl_algorithms::equal_range(cbegin(in1_av),	cend(in1_av), key);

	ASSERT_EQ(std::distance(std::cbegin(in0), result_expect.first),
			  std::distance(cbegin(in1_av), result_amp.first));
	ASSERT_EQ(std::distance(std::cbegin(in0), result_expect.second),
		      std::distance(cbegin(in1_av), result_amp.second));
}

TEST_F(stl_algorithms_equal_range_tests, equal_does_not_find_key_is_less)
{
	using namespace amp_stl_algorithms;

	auto in0 = input; std::sort(std::begin(in0), std::end(in0));
	auto in1 = in0;
	const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

	const auto key = std::numeric_limits<decltype(in0)::value_type>::min();
	const auto result_expect = std::equal_range(std::cbegin(in0),
												std::cend(in0),
												key);
	const auto result_amp = amp_stl_algorithms::equal_range(cbegin(in1_av),
															cend(in1_av),
															key);

	ASSERT_EQ(std::distance(std::cbegin(in0), result_expect.first),
			  std::distance(cbegin(in1_av), result_amp.first));
	ASSERT_EQ(std::distance(std::cbegin(in0), result_expect.second),
			  std::distance(cbegin(in1_av), result_amp.second));
}

TEST_F(stl_algorithms_equal_range_tests, equal_range_with_empty_range)
{
	using namespace amp_stl_algorithms;

	const auto result_expect = std::equal_range(std::cbegin(input),
												std::cbegin(input),
												input.front());
	const auto result_amp = amp_stl_algorithms::equal_range(cbegin(input_av),
														    cbegin(input_av),
															input.front());

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.first),
			  std::distance(cbegin(input_av), result_amp.first));
	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.second),
			  std::distance(cbegin(input_av), result_amp.second));
}

TEST_F(stl_algorithms_equal_range_tests, equal_range_with_single_element_range_finds)
{
	using namespace amp_stl_algorithms;

	const auto result_expect = std::equal_range(std::cbegin(input),
												std::next(std::cbegin(input)),
												input.front());
	const auto result_amp = amp_stl_algorithms::equal_range(cbegin(input_av),
															std::next(cbegin(input_av)),
															input.front());

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.first),
			  std::distance(cbegin(input_av), result_amp.first));
	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.second),
			  std::distance(cbegin(input_av), result_amp.second));
}

TEST_F(stl_algorithms_equal_range_tests, equal_range_with_single_element_range_does_not_find)
{
	using namespace amp_stl_algorithms;

	const auto key = std::numeric_limits<decltype(input)::value_type>::max();
	const auto result_expect = std::equal_range(std::cbegin(input),
												std::next(std::cbegin(input)),
												key);
	const auto result_amp = amp_stl_algorithms::equal_range(cbegin(input_av),
															std::next(cbegin(input_av)),
															key);

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.first),
			  std::distance(cbegin(input_av), result_amp.first));
	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.second),
			  std::distance(cbegin(input_av), result_amp.second));
}

TEST_F(stl_algorithms_equal_range_tests, equal_range_finds_large_range)
{
	using namespace amp_stl_algorithms;

	static constexpr decltype(auto) sz = successor(Execution_parameters::tile_size() *
												   Execution_parameters::maximum_tile_cnt());
	static constexpr decltype(auto) sample = testtools::compute_sample_size(sz);

	std::vector<int> in(sz);
	std::mt19937_64 g;
	std::uniform_int_distribution<int> d(-half_nonnegative(sample),
										 half_nonnegative(sample) + odd(sample));
	std::generate_n(std::begin(in), in.size(), [&]() { return d(g); });
	std::sort(std::begin(in), std::end(in));

	const concurrency::array_view<const int> in_av(in);

	std::vector<int> unique_x;
	std::unique_copy(std::cbegin(in), std::cend(in), std::back_inserter(unique_x));

	for (auto&& x : unique_x) {
		const auto result_expect = std::equal_range(std::cbegin(in),
												    std::cend(in),
												    std::forward<decltype(x)>(x));
		const auto result_amp = amp_stl_algorithms::equal_range(cbegin(in_av),
															    cend(in_av),
															    std::forward<decltype(x)>(x));

		ASSERT_EQ(std::distance(std::cbegin(in), result_expect.first),
			      std::distance(cbegin(in_av), result_amp.first));
		ASSERT_EQ(std::distance(std::cbegin(in), result_expect.second),
			      std::distance(cbegin(in_av), result_amp.second));
	}
}

TEST_F(stl_algorithms_equal_range_tests, equal_does_not_find_key_is_greater_large_range)
{
	using namespace amp_stl_algorithms;

	static constexpr decltype(auto) sz = successor(Execution_parameters::tile_size() *
												   Execution_parameters::maximum_tile_cnt());
	static constexpr int x = 1;
	static constexpr int superior_x = 2;

	std::vector<int> in(sz, x);
	const concurrency::array_view<const int> in_av(in);

	const auto result_expect = std::equal_range(std::cbegin(in), std::cend(in), superior_x);
	const auto result_amp = amp_stl_algorithms::equal_range(cbegin(in_av), cend(in_av), superior_x);

	ASSERT_EQ(std::distance(std::cbegin(in), result_expect.first),
			  std::distance(cbegin(in_av), result_amp.first));
	ASSERT_EQ(std::distance(std::cbegin(in), result_expect.second),
			  std::distance(cbegin(in_av), result_amp.second));

}

TEST_F(stl_algorithms_equal_range_tests, equal_does_not_find_key_is_less_large_range)
{
	using namespace amp_stl_algorithms;

	static constexpr decltype(auto) sz = successor(Execution_parameters::tile_size() *
												   Execution_parameters::maximum_tile_cnt());
	static constexpr int x = 1;
	static constexpr int inferior_x = 2;

	std::vector<int> in(sz, x);
	const concurrency::array_view<const int> in_av(in);

	const auto result_expect = std::equal_range(std::cbegin(in), std::cend(in), inferior_x);
	const auto result_amp = amp_stl_algorithms::equal_range(cbegin(in_av), cend(in_av), inferior_x);

	ASSERT_EQ(std::distance(std::cbegin(in), result_expect.first),
			  std::distance(cbegin(in_av), result_amp.first));
	ASSERT_EQ(std::distance(std::cbegin(in), result_expect.second),
			  std::distance(cbegin(in_av), result_amp.second));
}