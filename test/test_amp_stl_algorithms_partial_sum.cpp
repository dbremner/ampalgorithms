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
* This file contains unit tests for the partial_sum algorithm.
*---------------------------------------------------------------------------*/

#include <gtest/gtest.h>

#include "amp_algorithms_direct3d.h"
#include "amp_iterators.h"
#include "amp_stl_algorithms.h"
#include "testtools.h"

#include <algorithm>
#include <amp.h>
#include <amp_graphics.h>
#include <numeric>
#include <random>

//----------------------------------------------------------------------------
// partial_sum
//----------------------------------------------------------------------------

class stl_algorithms_partial_sum_tests : public stl_algorithms_testbase<13>,
						  	             public ::testing::Test {};

TEST_F(stl_algorithms_partial_sum_tests, partial_sum)
{
	using namespace amp_stl_algorithms;

	for (decltype(input.size()) i = 2; i != input.size() + 1; ++i) {
		decltype(input) out;
		std::fill_n(std::begin(out), out.size(), std::numeric_limits<decltype(input)::value_type>::min());
		concurrency::array_view<decltype(input)::value_type> out_av(out.size());
		concurrency::copy(std::cbegin(out), out_av);

		const auto result_expect = std::partial_sum(std::cbegin(input),
													std::next(std::cbegin(input), i),
													std::begin(out));
		const auto result_amp = amp_stl_algorithms::partial_sum(cbegin(input_av),
																std::next(cbegin(input_av), i),
																begin(out_av));

		ASSERT_EQ(std::distance(std::begin(out), result_expect),
			      std::distance(begin(out_av), result_amp));
		ASSERT_TRUE(std::equal(std::begin(out), result_expect, amp_stl_algorithms::cbegin(out_av)));
	}
}

TEST_F(stl_algorithms_partial_sum_tests, partial_sum_empty_range)
{
	using namespace amp_stl_algorithms;

	decltype(input) out;
	concurrency::array_view<decltype(input)::value_type> out_av(out.size());

	const auto result_expect = std::partial_sum(std::cbegin(input), std::cbegin(input), std::begin(out));
	const auto result_amp = amp_stl_algorithms::partial_sum(cbegin(input_av),
															cbegin(input_av),
															begin(out_av));

	ASSERT_EQ(std::distance(std::begin(out), result_expect),
			  std::distance(amp_stl_algorithms::begin(out_av), result_amp));
}

TEST_F(stl_algorithms_partial_sum_tests, partial_sum_single_element_range)
{
	using namespace amp_stl_algorithms;

	decltype(input) out;
	concurrency::array_view<decltype(input)::value_type> out_av(out.size());

	const auto result_expect = std::partial_sum(std::cbegin(input),
												std::next(std::cbegin(input)),
												std::begin(out));
	const auto result_amp = amp_stl_algorithms::partial_sum(cbegin(input_av),
															std::next(cbegin(input_av)),
															begin(out_av));

	ASSERT_EQ(std::distance(std::begin(out), result_expect),
			  std::distance(amp_stl_algorithms::begin(out_av), result_amp));
	ASSERT_TRUE(std::equal(std::begin(out), result_expect, cbegin(out_av)));
}

TEST_F(stl_algorithms_partial_sum_tests, partial_sum_large_range)
{
	using namespace amp_stl_algorithms;

	static constexpr decltype(auto) sz = successor(Execution_parameters::tile_size() *
												   Execution_parameters::maximum_tile_cnt());/*successor(1 << 26);*/
	static constexpr decltype(auto) sample = ::testtools::compute_sample_size(sz);

	static const int x_min = std::numeric_limits<int>::min() / sz;
	static const int x_max = std::numeric_limits<int>::max() / sz;

	std::mt19937_64 g;
	std::uniform_int_distribution<> d(x_min, x_max);
	std::vector<int> in(sz);
	std::generate_n(begin(in), sz, [&]() { return d(g); });

	const concurrency::array_view<int> in_av(in);

	std::vector<int> out(sz);
	const concurrency::array_view<int> out_av(sz);
	for (decltype(out.size()) i = 0; i != sample; ++i) {
		const auto result_expect = std::partial_sum(std::cbegin(in), std::cend(in), std::begin(out));
		const auto result_amp =  amp_stl_algorithms::partial_sum(cbegin(in_av),
														         cend(in_av),
															     begin(out_av));
		ASSERT_EQ(std::distance(std::begin(out), result_expect),
				  std::distance(begin(out_av), result_amp));
		ASSERT_TRUE(std::equal(std::cbegin(out), std::cend(out), out_av.data()));

		std::random_shuffle(std::begin(in), std::end(in));
		concurrency::copy(std::cbegin(in), in_av);
	}
}

TEST_F(stl_algorithms_partial_sum_tests, partial_sum_large_range_pod)
{
	using namespace amp_stl_algorithms;

	static constexpr decltype(auto) sz = successor(Execution_parameters::tile_size() *
												   Execution_parameters::maximum_tile_cnt());/*successor(1 << 26)*/;
	static constexpr decltype(auto) sample = ::testtools::compute_sample_size(sz);

	static const int x_min = std::numeric_limits<int>::min() / sz;
	static const int x_max = std::numeric_limits<int>::max() / sz;

	using T = concurrency::graphics::int_4;

	std::mt19937_64 g;
	std::uniform_int_distribution<> d(x_min, x_max);
	std::vector<T> in(sz);
	std::generate_n(begin(in), sz, [&]() { return T(d(g), d(g), d(g), d(g)); });

	const concurrency::array_view<T> in_av(in);

	std::vector<T> out(sz);
	const concurrency::array_view<T> out_av(sz);
	for (decltype(out.size()) i = 0; i != sample; ++i) {
		const auto result_expect = std::partial_sum(std::cbegin(in), std::cend(in), std::begin(out));
		const auto result_amp = amp_stl_algorithms::partial_sum(cbegin(in_av),
														        cend(in_av),
															    begin(out_av));

		const auto foo = std::mismatch(std::cbegin(out), std::cend(out), cbegin(out_av));

		ASSERT_EQ(std::distance(std::begin(out), result_expect),
				  std::distance(begin(out_av), result_amp));
		ASSERT_TRUE(std::equal(std::cbegin(out), std::cend(out), cbegin(out_av)));

		std::random_shuffle(std::begin(in), std::end(in));
		concurrency::copy(std::cbegin(in), in_av);
	}
}