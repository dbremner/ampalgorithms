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
* This file contains unit tests for the binary_search algorithm.
*---------------------------------------------------------------------------*/

#include "stdafx.h"
#include <gtest/gtest.h>

#include <amp_iterators.h>
#include <amp_stl_algorithms.h>
#include "testtools.h"

#include <amp.h>
#include <amp_graphics.h>
#include <functional>
#include <random>

//----------------------------------------------------------------------------
// binary_search
//----------------------------------------------------------------------------

class stl_algorithms_binary_search_tests : public stl_algorithms_testbase<13>,
										   public ::testing::Test {};

TEST_F(stl_algorithms_binary_search_tests, binary_search)
{
	using namespace amp_stl_algorithms;
	using namespace concurrency;

	auto in = input; std::sort(std::begin(in), std::end(in));
	const array_view<decltype(in)::value_type> in_av(in);

	for (decltype(input.size()) i = 0; i != input.size(); ++i) {
		const auto result_expect = std::binary_search(std::cbegin(in), std::cend(in), input[i]);
		const auto result_amp = amp_stl_algorithms::binary_search(cbegin(in_av),
																  cend(in_av),
																  input[i]);

		ASSERT_EQ(result_expect, result_amp);
	}
}

TEST_F(stl_algorithms_binary_search_tests, binary_search_does_not_find)
{
	using namespace amp_stl_algorithms;
	using namespace concurrency;

	auto in0 = input; std::sort(std::begin(in0), std::end(in0));
	auto in1 = in0;
	const array_view<decltype(in1)::value_type> in1_av(in1);

	const auto key = std::numeric_limits<decltype(in1)::value_type>::max();
	const auto result_expect = std::binary_search(std::cbegin(in0), std::cend(in0), key);
	const auto result_amp = amp_stl_algorithms::binary_search(cbegin(in1_av), cend(in1_av), key);

	ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_binary_search_tests, binary_search_with_empty_range)
{
	using namespace amp_stl_algorithms;

	const auto result_expect = std::binary_search(std::cbegin(input),
		                                          std::cbegin(input),
		                                          input.back());
	const auto result_amp = amp_stl_algorithms::binary_search(cbegin(input_av),
		                                                      cbegin(input_av),
		                                                      input.back());

	ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_binary_search_tests, binary_search_with_single_element_does_not_find)
{
	using namespace amp_stl_algorithms;
	using namespace concurrency;

	const std::array<int, 1> in{ std::numeric_limits<int>::max() };
	const array_view<const int> in_av(in);
	const auto x = std::numeric_limits<int>::min();

	const auto result_expect = std::binary_search(std::cbegin(in), std::end(in), x);
	const auto result_amp = amp_stl_algorithms::binary_search(cbegin(in_av), cend(in_av), x);

	ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_binary_search_tests, binary_search_with_single_element_finds)
{
	using namespace amp_stl_algorithms;

	const auto result_expect = std::binary_search(std::cbegin(input),
		                                          std::next(std::cbegin(input)),
		                                          input.front());
	const auto result_amp = amp_stl_algorithms::binary_search(cbegin(input_av),
		                                                      std::next(cbegin(input_av)),
		                                                      input.front());

	ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_binary_search_tests, binary_search_with_predicate_finds)
{
	using namespace amp_stl_algorithms;

	static constexpr int length = 100000;
	static constexpr int x = 6000;

	std::vector<int> in(length); std::iota(std::begin(in), std::end(in), 0);
	const concurrency::array_view<const int> in_av(in);

	const auto result_expect = std::binary_search(std::cbegin(in), std::cend(in), x, std::less<int>());
	const auto result_amp = amp_stl_algorithms::binary_search(cbegin(in_av), cend(in_av), x, amp_algorithms::less<int>());

	ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_binary_search_tests, binary_search_large_range)
{
	using namespace amp_stl_algorithms;

	static constexpr std::size_t sz = successor(Execution_parameters::tile_size() *
												Execution_parameters::maximum_tile_cnt());
	static constexpr std::size_t sample_sz = testtools::compute_sample_size(sz);

	std::vector<int> in(sz);
	std::iota(std::begin(in), std::end(in), std::negate<int>()(half_nonnegative(sz)));
	const concurrency::array_view<const int> in_av(in);

	std::mt19937_64 g;
	std::uniform_int_distribution<std::size_t> d(0, predecessor(sz));
	for (std::size_t i = 0; i != sample_sz; ++i) {
		const auto result_expect = std::binary_search(std::cbegin(in), std::cend(in), in[d(g)]);
		const auto result_amp = amp_stl_algorithms::binary_search(cbegin(in_av),
																  cend(in_av),
																  in[d(g)]);

		ASSERT_EQ(result_expect, result_amp);
	}
}

TEST_F(stl_algorithms_binary_search_tests, binary_search_large_range_with_pod)
{
	using namespace amp_stl_algorithms;
	using concurrency::graphics::int_4;

	static constexpr std::size_t sz = successor(Execution_parameters::tile_size() *
										        Execution_parameters::maximum_tile_cnt());
	static constexpr std::size_t sample_sz = testtools::compute_sample_size(sz);

	std::vector<int_4> in(sz); std::iota(std::begin(in), std::end(in), int_4(-half_nonnegative(sz)));
	const concurrency::array_view<const int_4> in_av(in);

	const auto p = [](auto&& x, auto&& y) restrict(cpu, amp) {
		if (x.x < y.x) return true;
		if (y.x < x.x) return false;

		if (x.y < y.y) return true;
		if (y.y < x.y) return false;

		if (x.z < y.z) return true;
		if (y.z < x.z) return false;

		return x.w < y.w;
	};

	std::mt19937_64 g;
	std::uniform_int_distribution<std::size_t> d(0, predecessor(sz));
	for (std::size_t i = 0; i != sample_sz; ++i) {
		const auto result_expect = std::binary_search(std::cbegin(in), std::cend(in), in[d(g)], p);
		const auto result_amp = amp_stl_algorithms::binary_search(cbegin(in_av),
																  cend(in_av),
																  in[d(g)],
																  p);

		ASSERT_EQ(result_expect, result_amp);
	}
}