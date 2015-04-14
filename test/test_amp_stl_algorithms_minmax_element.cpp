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
* This file contains unit tests for the minmax_element algorithm.
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
// minmax_element
//----------------------------------------------------------------------------

class stl_algorithms_minmax_element_tests : public stl_algorithms_testbase<13>,
						  	                public ::testing::Test {};

TEST_F(stl_algorithms_minmax_element_tests, minmax_element)
{
	using namespace amp_stl_algorithms;

	for (decltype(input_av.extent.size()) i = 0; i != input.size(); ++i) {
		const auto result_expect = std::minmax_element(cbegin(input_av), cend(input_av));
		const auto result_amp = amp_stl_algorithms::minmax_element(cbegin(input_av), cend(input_av));

		ASSERT_EQ(*result_expect.first, *result_amp.first);
		ASSERT_EQ(*result_expect.second, *result_amp.second);

		ASSERT_EQ(std::distance(cbegin(input_av), result_expect.first),
				  std::distance(cbegin(input_av), result_amp.first));
		ASSERT_EQ(std::distance(cbegin(input_av), result_expect.second),
				  std::distance(cbegin(input_av), result_amp.second));

		std::random_shuffle(begin(input_av), end(input_av));
	}
}

TEST_F(stl_algorithms_minmax_element_tests, minmax_element_with_multiple_extrema)
{
	using namespace amp_stl_algorithms;

	const auto it = std::fill_n(begin(input_av),
							    input_av.extent.size() / 3,
								std::numeric_limits<decltype(input_av)::value_type>::min());
	std::fill_n(it,
				input_av.extent.size() / 3,
				std::numeric_limits<decltype(input_av)::value_type>::max());
	std::random_shuffle(begin(input_av), end(input_av));

	for (decltype(input_av.extent.size()) i = 0; i != input.size(); ++i) {
		const auto result_expect = std::minmax_element(cbegin(input_av), cend(input_av));
		const auto result_amp = amp_stl_algorithms::minmax_element(cbegin(input_av), cend(input_av));

		ASSERT_EQ(*result_expect.first, *result_amp.first);
		ASSERT_EQ(*result_expect.second, *result_amp.second);

		ASSERT_EQ(std::distance(cbegin(input_av), result_expect.first),
				  std::distance(cbegin(input_av), result_amp.first));
		ASSERT_EQ(std::distance(cbegin(input_av), result_expect.second),
				  std::distance(cbegin(input_av), result_amp.second));

		std::random_shuffle(begin(input_av), end(input_av));
	}
}

TEST_F(stl_algorithms_minmax_element_tests, minmax_element_empty_range)
{
	using namespace amp_stl_algorithms;

	const auto result_expect = std::minmax_element(cbegin(input_av), cbegin(input_av));
	const auto result_amp = amp_stl_algorithms::minmax_element(cbegin(input_av), cbegin(input_av));

	ASSERT_EQ(std::distance(cbegin(input_av), result_expect.first),
			  std::distance(cbegin(input_av), result_amp.first));
	ASSERT_EQ(std::distance(cbegin(input_av), result_expect.second),
			  std::distance(cbegin(input_av), result_amp.second));
}

TEST_F(stl_algorithms_minmax_element_tests, minmax_element_single_element_range)
{
	using namespace amp_stl_algorithms;

	const auto result_expect = std::minmax_element(cbegin(input_av), std::next(cbegin(input_av)));
	const auto result_amp = amp_stl_algorithms::minmax_element(cbegin(input_av),
														       std::next(cbegin(input_av)));

	ASSERT_EQ(*result_expect.first, *result_amp.first);
	ASSERT_EQ(*result_expect.second, *result_amp.second);

	ASSERT_EQ(std::distance(cbegin(input_av), result_expect.first),
			  std::distance(cbegin(input_av), result_amp.first));
	ASSERT_EQ(std::distance(cbegin(input_av), result_expect.second),
			  std::distance(cbegin(input_av), result_amp.second));
}


TEST_F(stl_algorithms_minmax_element_tests, minmax_element_large_range)
{
	using namespace amp_stl_algorithms;

	static constexpr decltype(auto) sz = successor(Execution_parameters::tile_size() *
												   Execution_parameters::maximum_tile_cnt());
	static constexpr decltype(auto) sample = testtools::compute_sample_size(sz);


	auto in = testtools::generate_data<int>(sz);
	//std::vector<int> in(sz);
//	std::mt19937_64 g;
//	std::uniform_int_distribution<int> d(std::numeric_limits<int>::min(),
										 //std::numeric_limits<int>::max());
	//std::generate_n(std::begin(in), sz, [&]() { return d(g); });
	const concurrency::array_view<int> in_av(sz);

	for (decltype(in_av.extent.size()) i = 0; i != sample; ++i) {
		concurrency::copy(std::cbegin(in), in_av);

		const auto result_expect = std::minmax_element(std::cbegin(in), std::cend(in));
		const auto result_amp = amp_stl_algorithms::minmax_element(cbegin(in_av), cend(in_av));

		ASSERT_EQ(*result_expect.first, *result_amp.first);
		ASSERT_EQ(*result_expect.second, *result_amp.second);

		ASSERT_EQ(std::distance(std::cbegin(in), result_expect.first),
				  std::distance(cbegin(in_av), result_amp.first));
		ASSERT_EQ(std::distance(std::cbegin(in), result_expect.second),
				  std::distance(cbegin(in_av), result_amp.second));

		std::random_shuffle(std::begin(in), std::end(in));
	}
}