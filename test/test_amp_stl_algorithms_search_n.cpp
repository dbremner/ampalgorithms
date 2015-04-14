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
* This file contains unit tests for the search_n algorithm.
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
// search_n
//----------------------------------------------------------------------------

class stl_algorithms_search_n_tests : public stl_algorithms_testbase<13>,
						  	          public ::testing::Test {};

TEST_F(stl_algorithms_search_n_tests, search_n)
{
	using namespace amp_stl_algorithms;

	std::sort(begin(input_av), end(input_av));

	std::vector<decltype(input_av)::value_type> unique_x;
	std::unique_copy(cbegin(input_av), cend(input_av), std::back_inserter(unique_x));

	for (auto&& x : unique_x) {
		const auto n = std::count(cbegin(input_av), cend(input_av), forward<decltype(x)>(x));

		const auto result_expect = std::search_n(cbegin(input_av),
											     cend(input_av),
												 n,
												 forward<decltype(x)>(x));
		const auto result_amp = amp_stl_algorithms::search_n(cbegin(input_av),
															 cend(input_av),
															 n,
															 forward<decltype(x)>(x));

		ASSERT_EQ(std::distance(cbegin(input_av), result_expect),
				  std::distance(cbegin(input_av), result_amp));
	};
}

TEST_F(stl_algorithms_search_n_tests, search_n_does_not_find)
{
	using namespace amp_stl_algorithms;

	static constexpr int x = 1;
	static constexpr int not_x = 2;

	const concurrency::array_view<int> in(input.size());
	for (decltype(input.size()) i = 1; i != input.size(); ++i) {
		for (decltype(input.size()) j = 0; j != input.size(); ++j) {
			in[j] = j % i ? x : not_x;
		}

		const auto result_expect = std::search_n(cbegin(in), cend(in), i, x);
		const auto result_amp = amp_stl_algorithms::search_n(cbegin(in), cend(in), i, x);

		ASSERT_EQ(std::distance(cbegin(in), result_expect), std::distance(cbegin(in), result_amp));
	}
}

TEST_F(stl_algorithms_search_n_tests, search_n_with_empty_range)
{
	using namespace amp_stl_algorithms;

	const auto result_expect = std::search_n(std::cbegin(input), std::cbegin(input), 1, input[0]);
	const auto result_amp = amp_stl_algorithms::search_n(cbegin(input_av),
														 cbegin(input_av),
														 1,
														 input[0]);

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect),
			  std::distance(cbegin(input_av), result_amp));
}

TEST_F(stl_algorithms_search_n_tests, search_n_with_single_element_range_finds)
{
	using namespace amp_stl_algorithms;

	const auto result_expect = std::search_n(std::cbegin(input),
											 std::next(std::cbegin(input)),
											 1,
											 input[0]);
	const auto result_amp = amp_stl_algorithms::search_n(cbegin(input_av),
														 std::next(cbegin(input_av)),
														 1,
														 input[0]);

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect),
			  std::distance(cbegin(input_av), result_amp));
}

TEST_F(stl_algorithms_search_n_tests, search_n_with_single_element_range_does_not_find)
{
	using namespace amp_stl_algorithms;

	const auto x = *std::find_if_not(std::cbegin(input),
								     std::cend(input),
									 std::bind1st(std::equal_to<int>(), input[0]));
	const auto result_expect = std::search_n(std::cbegin(input),
											 std::next(std::cbegin(input)),
											 1,
											 x);
	const auto result_amp = amp_stl_algorithms::search_n(cbegin(input_av),
														 std::next(cbegin(input_av)),
														 1,
														 x);

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect),
			  std::distance(cbegin(input_av), result_amp));
}

TEST_F(stl_algorithms_search_n_tests, search_n_large_range)
{
	using namespace amp_stl_algorithms;

	static constexpr decltype(auto) sz = successor(Execution_parameters::tile_size() *
												   Execution_parameters::maximum_tile_cnt());
	static constexpr decltype(auto) sample = testtools::compute_sample_size(sz);
	static constexpr int x_max = sample / 2;

	std::vector<int> in(sz);
	std::mt19937_64 g;
	std::uniform_int_distribution<int> d(-x_max, x_max);
	std::generate_n(std::begin(in), in.size(), [&]() { return d(g); });
	std::sort(std::begin(in), std::end(in));

	const concurrency::array_view<const int> in_av(in);

	std::vector<int> unique_x;
	std::unique_copy(std::cbegin(in), std::cend(in), std::back_inserter(unique_x));

	for (auto&& x : unique_x) {
		const auto cnt = std::count(std::cbegin(in), std::cend(in), forward<decltype(x)>(x));
		const auto result_expect = std::search_n(std::cbegin(in),
												 std::cend(in),
												 cnt,
												 forward<decltype(x)>(x));
		const auto result_amp = amp_stl_algorithms::search_n(cbegin(in_av),
															 cend(in_av),
															 cnt,
															 forward<decltype(x)>(x));

		ASSERT_EQ(std::distance(std::cbegin(in), result_expect),
				  std::distance(cbegin(in_av), result_amp));
	}
}

TEST_F(stl_algorithms_search_n_tests, search_n_all_equals_large_range)
{
	using namespace amp_stl_algorithms;

	static constexpr decltype(auto) sz = successor(Execution_parameters::tile_size() *
												   Execution_parameters::maximum_tile_cnt());
	static constexpr int x = 1;
	static constexpr decltype(auto) sample = testtools::compute_sample_size(sz);

	std::vector<int> in(sz, x);
	const concurrency::array_view<const int> in_av(in);

	std::mt19937_64 g;
	std::uniform_int_distribution<decltype(in.size())> d(1, in.size());

	for (decltype(in.size()) i = 0; i != sample; ++i) {
		const decltype(in.size()) cnt = d(g);
		const auto result_expect = std::search_n(std::cbegin(in), std::cend(in), cnt, x);
		const auto result_amp = amp_stl_algorithms::search_n(cbegin(in_av), cend(in_av), cnt, x);

		ASSERT_EQ(std::distance(std::cbegin(in), result_expect),
				  std::distance(cbegin(in_av), result_amp));
	}
}