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
* This file contains unit tests for the sort algorithm.
*---------------------------------------------------------------------------*/

#include <gtest/gtest.h>

#include "amp_iterators.h"
#include "amp_stl_algorithms.h"
#include "testtools.h"

#include <algorithm>
#include <amp.h>
#include <amp_graphics.h>
#include <ppl.h>
#include <random>

//----------------------------------------------------------------------------
// sort
//----------------------------------------------------------------------------

class stl_algorithms_sort_tests : public stl_algorithms_testbase<13>,
						  	      public ::testing::Test {};

TEST_F(stl_algorithms_sort_tests, sort)
{
	using namespace amp_stl_algorithms;

	for (decltype(input.size()) i = 2; i != input.size(); ++i) {
		std::vector<decltype(input)::value_type> in0(std::cbegin(input), std::cbegin(input) + i);
		auto in1 = in0;
		const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

		std::sort(std::begin(in0), std::end(in0));
		amp_stl_algorithms::sort(begin(in1_av), end(in1_av));

		ASSERT_TRUE(std::is_sorted(cbegin(in1_av), cend(in1_av)));
		ASSERT_TRUE(std::equal(std::cbegin(in0), std::cend(in0), cbegin(in1_av)));
	}
}

TEST_F(stl_algorithms_sort_tests, sort_empty_range)
{
	using namespace amp_stl_algorithms;

	std::sort(std::begin(input), std::begin(input));
	amp_stl_algorithms::sort(begin(input_av), begin(input_av));

	ASSERT_EQ(std::is_sorted(std::cbegin(input), std::cbegin(input)),
			  std::is_sorted(cbegin(input_av), cbegin(input_av)));
	ASSERT_TRUE(std::equal(std::cbegin(input), std::cbegin(input), cbegin(input_av)));
}

TEST_F(stl_algorithms_sort_tests, sort_single_element_range)
{
	using namespace amp_stl_algorithms;

	std::sort(std::begin(input), std::begin(input) + 1);
	amp_stl_algorithms::sort(begin(input_av), begin(input_av) + 1);

	ASSERT_TRUE(std::is_sorted(cbegin(input_av), cbegin(input_av) + 1));
	ASSERT_TRUE(std::equal(std::cbegin(input), std::cbegin(input) + 1, cbegin(input_av)));
}


TEST_F(stl_algorithms_sort_tests, sort_sorted_range)
{
	using namespace amp_stl_algorithms;

	auto in0 = input; std::sort(std::begin(in0), std::end(in0));
	auto in1 = in0;
	const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

	std::sort(std::begin(in0), std::end(in0));
	amp_stl_algorithms::sort(begin(in1_av), end(in1_av));

	ASSERT_TRUE(std::is_sorted(cbegin(in1_av), cend(in1_av)));
	ASSERT_TRUE(std::equal(std::cbegin(in0), std::cend(in0), cbegin(in1_av)));
}

TEST_F(stl_algorithms_sort_tests, sort_reverse_sorted_range)
{
	using namespace amp_stl_algorithms;

	auto in0 = input; std::sort(std::begin(in0), std::end(in0), std::greater<>());
	auto in1 = in0;
	const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

	std::sort(std::begin(in0), std::end(in0));
	amp_stl_algorithms::sort(begin(in1_av), end(in1_av));

	ASSERT_TRUE(std::is_sorted(cbegin(in1_av), cend(in1_av)));
	ASSERT_TRUE(std::equal(std::cbegin(in0), std::cend(in0), cbegin(in1_av)));
}

TEST_F(stl_algorithms_sort_tests, sort_concave_range)
{
	using namespace amp_stl_algorithms;

	auto in0 = input;
	std::sort(std::begin(in0), std::end(in0));
	std::reverse(std::begin(in0) + in0.size() / 2, std::end(in0));
	auto in1 = in0;
	const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

	std::sort(std::begin(in0), std::end(in0));
	amp_stl_algorithms::sort(begin(in1_av), end(in1_av));

	ASSERT_TRUE(std::is_sorted(cbegin(in1_av), cend(in1_av)));
	ASSERT_TRUE(std::equal(std::cbegin(in0), std::cend(in0), cbegin(in1_av)));
}

TEST_F(stl_algorithms_sort_tests, sort_convex_range)
{
	using namespace amp_stl_algorithms;

	auto in0 = input;
	std::sort(std::begin(in0), std::end(in0));
	std::reverse(std::begin(in0), std::begin(in0) + in0.size() / 2);
	auto in1 = in0;
	const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

	std::sort(std::begin(in0), std::end(in0));
	amp_stl_algorithms::sort(begin(in1_av), end(in1_av));

	ASSERT_TRUE(std::is_sorted(cbegin(in1_av), cend(in1_av)));
	ASSERT_TRUE(std::equal(std::cbegin(in0), std::cend(in0), cbegin(in1_av)));
}

TEST_F(stl_algorithms_sort_tests, sort_large_range)
{
	using namespace amp_stl_algorithms;

	static constexpr decltype(auto) sz = successor(Execution_parameters::tile_size() *
												   Execution_parameters::maximum_tile_cnt());/*successor(1 << 26);*/
	static constexpr decltype(auto) sample = testtools::compute_sample_size(sz);

	auto in = testtools::generate_data<int>(sz);
	const concurrency::array_view<int> in_av(in.size());

	for (decltype(in.size()) i = 0; i != sample; ++i) {
		concurrency::copy(std::cbegin(in), in_av);

		concurrency::parallel_sort(std::begin(in), std::end(in));
		amp_stl_algorithms::sort(begin(in_av), end(in_av));

		ASSERT_TRUE(std::is_sorted(cbegin(in_av), cend(in_av)));
		ASSERT_TRUE(std::equal(std::cbegin(in), std::cend(in), cbegin(in_av)));

		std::random_shuffle(std::begin(in), std::end(in));
	}
}

TEST_F(stl_algorithms_sort_tests, sort_large_range_pod)
{
	using namespace amp_stl_algorithms;

	static constexpr decltype(auto) sz = successor(Execution_parameters::tile_size() *
												   Execution_parameters::maximum_tile_cnt());
	static constexpr decltype(auto) sample = testtools::compute_sample_size(sz);

	using T = concurrency::graphics::int_4;
	std::vector<T> in(sz);
	std::generate_n(std::begin(in), sz, []() { return T(rand(), rand(), rand(), rand()); });
	const concurrency::array_view<T> in_av(sz);

	const auto cmp = [](auto&& x, auto&& y) restrict(cpu, amp) { // Lexicographical ordering.
		if (x.x < y.x) return true;
		if (y.x < x.x) return false;

		if (x.y < y.y) return true;
		if (y.y < x.y) return false;

		if (x.z < y.z) return true;
		if (y.z < x.z) return false;
		return x.w < y.w;
	};
	for (decltype(in.size()) i = 0; i != sample; ++i) {
		concurrency::copy(std::cbegin(in), in_av);

		concurrency::parallel_sort(std::begin(in), std::end(in), cmp);
		amp_stl_algorithms::sort(begin(in_av), end(in_av), cmp);

		ASSERT_TRUE(std::is_sorted(cbegin(in_av), cend(in_av), cmp));
		ASSERT_TRUE(std::equal(std::cbegin(in), std::cend(in), cbegin(in_av)));

		std::random_shuffle(std::begin(in), std::end(in));
	}
}