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
* This file contains unit tests for the nth_element algorithm.
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
// nth_element
//----------------------------------------------------------------------------

class stl_algorithms_nth_element_tests : public stl_algorithms_testbase<13>,
						  	             public ::testing::Test {};

TEST_F(stl_algorithms_nth_element_tests, nth_element)
{
	using namespace amp_stl_algorithms;

	auto in = input;
	const concurrency::array_view<decltype(in)::value_type> in_av(in.size());
	for (decltype(input.size()) nth = 0; nth != input.size(); ++nth) {
		concurrency::copy(std::cbegin(in), in_av);

		std::nth_element(std::begin(in), std::next(std::begin(in), nth), std::end(in));
		amp_stl_algorithms::nth_element(begin(in_av), std::next(begin(in_av), nth), end(in_av));

		ASSERT_TRUE(in[nth] == in_av[nth]);

		std::sort(std::begin(in), std::next(std::begin(in), nth + 1));
		std::sort(begin(in_av), std::next(begin(in_av), nth + 1));

		ASSERT_TRUE(std::equal(std::cbegin(in), std::next(cbegin(in), nth + 1), cbegin(in_av)));

		std::random_shuffle(std::begin(in), std::end(in));
	}
}

TEST_F(stl_algorithms_nth_element_tests, nth_element_empty_range)
{
	using namespace amp_stl_algorithms;

	amp_stl_algorithms::nth_element(begin(input_av), begin(input_av), begin(input_av));
}

TEST_F(stl_algorithms_nth_element_tests, nth_element_single_element_range)
{
	using namespace amp_stl_algorithms;

	std::nth_element(std::begin(input), std::begin(input), std::next(std::begin(input)));
	amp_stl_algorithms::nth_element(begin(input_av), begin(input_av), std::next(begin(input_av)));

	ASSERT_TRUE(*std::cbegin(input) == *cbegin(input_av));
}

TEST_F(stl_algorithms_nth_element_tests, nth_element_large_range)
{
	using namespace amp_stl_algorithms;

	static constexpr decltype(auto) sz = successor(Execution_parameters::tile_size() *
												   Execution_parameters::maximum_tile_cnt());
	static constexpr decltype(auto) sample = testtools::compute_sample_size(sz);

//	std::vector<int> in = testtools::generate_median_of_3_killer<int>(sz);
//	std::generate_n(std::begin(in), sz, rand);
	auto in = testtools::generate_data<int>(sz);
	const concurrency::array_view<int> in_av(sz);

	std::mt19937_64 g;
	std::uniform_int_distribution<decltype(in.size())> d(0, sz - 1);

	for (decltype(in.size()) i = 0; i != sample; ++i) {
		concurrency::copy(std::cbegin(in), in_av);

		const decltype(in.size()) nth = d(g);
		std::nth_element(std::begin(in), std::begin(in) + nth, std::end(in));
		amp_stl_algorithms::nth_element(begin(in_av), begin(in_av) + nth, end(in_av));

		ASSERT_TRUE(in[nth] == in_av[nth]);

		//std::sort(std::begin(in), std::next(std::begin(in), nth + 1));
		//std::sort(begin(in_av), std::next(begin(in_av), nth + 1));

		//ASSERT_TRUE(std::equal(std::cbegin(in), std::next(std::cbegin(in), nth + 1), cbegin(in_av)));

		std::random_shuffle(std::begin(in), std::end(in));
		std::cout << '.';
	}
}

