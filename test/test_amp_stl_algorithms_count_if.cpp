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
* This file contains unit tests for the count_if algorithm.
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
// count_if
//----------------------------------------------------------------------------

class stl_algorithms_count_if_tests : public stl_algorithms_testbase<13>,
								      public ::testing::Test {};

TEST_F(stl_algorithms_count_if_tests, count_if_counts_all_values)
{
	using namespace amp_stl_algorithms;

	static constexpr decltype(input)::value_type x = 1;

	decltype(input) in;
	std::fill_n(std::begin(in), in.size(), x);
	const concurrency::array_view<const decltype(in)::value_type> in_av(in);

	const auto p = [x = x](auto&& y) restrict(cpu, amp) { return x == y; };
	const auto result_expect = std::count_if(std::cbegin(in), std::cend(in), p);
	const auto result_amp = amp_stl_algorithms::count_if(cbegin(in_av), cend(in_av), p);

	ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_count_if_tests, count_if_counts_some_values)
{
	using namespace amp_stl_algorithms;

	for (decltype(input.size()) i = 0; i != input.size(); ++i) {
		const auto p = [x = input_av[i]](auto&& y) restrict(cpu, amp) { return x == y; };
		const auto result_expect = std::count_if(cbegin(input_av), cend(input_av), p);
		const auto result_amp = amp_stl_algorithms::count_if(cbegin(input_av), cend(input_av), p);
		ASSERT_EQ(result_expect, result_amp);
	}
}

TEST_F(stl_algorithms_count_if_tests, count_if_counts_no_values)
{
	using namespace amp_stl_algorithms;

	const auto x = *cbegin(input_av);
	const auto l = std::remove(begin(input_av), end(input_av), x);

	const auto p = [=](auto&& y) restrict(cpu, amp) { return x == y; };
	const auto result_expect = std::count_if(begin(input_av), l, p);
	const auto result_amp = amp_stl_algorithms::count_if(begin(input_av), l, p);
	ASSERT_EQ(0, result_amp);
}

TEST_F(stl_algorithms_count_if_tests, count_if_empty_range)
{
	using namespace amp_stl_algorithms;

	const auto x = *cbegin(input_av);
	const auto p = [=](auto&& y) restrict(cpu, amp) { return x == y; };
	const auto result_expect = std::count_if(cbegin(input_av), cbegin(input_av), p);
	const auto result_amp = amp_stl_algorithms::count_if(cbegin(input_av), cbegin(input_av), p);

	ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_count_if_tests, count_if_single_element_range_counts)
{
	using namespace amp_stl_algorithms;

	const auto p = [x = *cbegin(input_av)](auto&& y) restrict(cpu, amp) { return x == y; };
	const auto result_expect = std::count_if(cbegin(input_av), std::next(cbegin(input_av)), p);
	const auto result_amp = amp_stl_algorithms::count_if(cbegin(input_av),
													     std::next(cbegin(input_av)),
													     p);
	ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_count_if_tests, count_if_single_element_range_does_not_count)
{
	using namespace amp_stl_algorithms;

	const auto x = *cbegin(input_av);
	const auto l = std::remove(begin(input_av), end(input_av), x);

	const auto p = [=](auto&& y) restrict(cpu, amp) { return x == y; };
	const auto result_expect = std::count_if(cbegin(input_av), std::next(cbegin(input_av)), p);
	const auto result_amp = amp_stl_algorithms::count_if(cbegin(input_av),
													     std::next(cbegin(input_av)),
													     p);
	ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_count_if_tests, count_if_counts_all_values_large_range)
{
	using namespace amp_stl_algorithms;

	static constexpr decltype(auto) sz = successor(Execution_parameters::tile_size() *
												   Execution_parameters::maximum_tile_cnt());
	static constexpr int x = 1;

	std::vector<int> in(sz, x);
	const concurrency::array_view<const int> in_av(in);

	const auto p = [x = x](auto&& y) restrict(cpu, amp) { return x == y; };
	const auto result_expect = std::count_if(cbegin(in_av), cend(in_av), p);
	const auto result_amp = amp_stl_algorithms::count_if(cbegin(in_av), cend(in_av), p);

	ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_count_if_tests, count_if_counts_some_values_large_range)
{
	using namespace amp_stl_algorithms;

	static constexpr decltype(auto) sz = successor(Execution_parameters::tile_size() *
												   Execution_parameters::maximum_tile_cnt());
	static constexpr decltype(auto) sample = testtools::compute_sample_size(sz);

	std::vector<int> in(sz);
	std::mt19937_64 g;
	std::uniform_int_distribution<decltype(in)::value_type> d(-half_nonnegative(sample),
															   half_nonnegative(sample) + odd(sample));
	std::generate_n(std::begin(in), sz, [&]() { return d(g); });
	std::sort(std::begin(in), std::end(in));
	std::vector<int> xs;
	std::unique_copy(std::cbegin(in), std::cend(in), std::back_inserter(xs));

	std::random_shuffle(std::begin(in), std::end(in));
	const concurrency::array_view<const int> in_av(in);

	for (auto&& x : xs) {
		const auto p = [=](auto&& y) restrict(cpu, amp) { return x == y; };
		const auto result_expect = std::count_if(cbegin(in_av), cend(in_av), p);
		const auto result_amp = amp_stl_algorithms::count_if(cbegin(in_av), cend(in_av), p);

		ASSERT_EQ(result_expect, result_amp);
	}
}

TEST_F(stl_algorithms_count_if_tests, count_if_counts_no_values_large_range)
{
	using namespace amp_stl_algorithms;

	static constexpr decltype(auto) sz = successor(Execution_parameters::tile_size() *
												   Execution_parameters::maximum_tile_cnt());
	static constexpr int x = 1;

	std::vector<int> in(sz, x);
	const concurrency::array_view<const int> in_av(in);

	const auto p = [x = x](auto&& y) restrict(cpu, amp) { return x != y; };
	const auto result_expect = std::count_if(cbegin(in_av), cend(in_av), p);
	const auto result_amp = amp_stl_algorithms::count_if(cbegin(in_av), cend(in_av), p);

	ASSERT_EQ(result_expect, result_amp);
}