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
* This file contains unit tests for the partition algorithm.
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
// partition
//----------------------------------------------------------------------------

class stl_algorithms_partition_tests : public stl_algorithms_testbase<13>,
									   public ::testing::Test {};

TEST_F(stl_algorithms_partition_tests, partition)
{
	using namespace amp_stl_algorithms;

	for (auto&& p_pt : input) {
		auto in0 = input;
		auto in1 = input;
		const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

		const auto p = [=](auto&& x) restrict(cpu, amp) { return x < p_pt; };
		const auto result_expect = std::partition(std::begin(in0), std::end(in0), p);
		const auto result_amp = amp_stl_algorithms::partition(begin(in1_av), end(in1_av), p);

		ASSERT_EQ(std::distance(std::begin(in0), result_expect),
				  std::distance(begin(in1_av), result_amp));
		ASSERT_TRUE(std::is_partitioned(cbegin(in1_av), cend(in1_av), p));

		std::sort(std::begin(in0), result_expect);
		std::sort(result_expect, std::end(in0));
		std::sort(begin(in1_av), result_amp);
		std::sort(result_amp, end(in1_av));
		ASSERT_TRUE(std::equal(std::cbegin(in0), std::cend(in0), cbegin(in1_av)));
	}
}

TEST_F(stl_algorithms_partition_tests, partition_with_empty_range)
{
	using namespace amp_stl_algorithms;

	const auto p = [v = input[input.size() / 2]](auto&& x) restrict(cpu, amp) { return x < v; };
	const auto result_expect = std::partition(std::begin(input), std::begin(input), p);
	const auto result_amp = amp_stl_algorithms::partition(begin(input_av), begin(input_av), p);

	ASSERT_EQ(std::distance(std::begin(input), result_expect),
			  std::distance(begin(input_av), result_amp));
}

TEST_F(stl_algorithms_partition_tests, partition_single_element_range_less)
{
	using namespace amp_stl_algorithms;

	const auto v = std::numeric_limits<decltype(input)::value_type>::max();
	const auto p = [=](auto&& x) restrict(cpu, amp) { return forward<decltype(x)>(x) < v; };
	const auto result_expect = std::partition(std::begin(input), std::next(std::begin(input)), p);
	const auto result_amp = amp_stl_algorithms::partition(begin(input_av),
		std::next(begin(input_av)),
		p);

	ASSERT_EQ(std::distance(std::begin(input), result_expect),
		std::distance(amp_stl_algorithms::begin(input_av), result_amp));
}

TEST_F(stl_algorithms_partition_tests, partition_with_single_element_range_greater)
{
	using namespace amp_stl_algorithms;

	const auto v = std::numeric_limits<decltype(input)::value_type>::min();
	const auto p = [=](auto&& x) restrict(cpu, amp) { return forward<decltype(x)>(x) < v; };
	const auto result_expect = std::partition(std::begin(input), std::next(std::begin(input)), p);
	const auto result_amp = amp_stl_algorithms::partition(begin(input_av),
		std::next(begin(input_av)),
		p);

	ASSERT_EQ(std::distance(std::begin(input), result_expect),
		std::distance(amp_stl_algorithms::begin(input_av),
		result_amp));
}

TEST_F(stl_algorithms_partition_tests, partition_with_all_less_than)
{
	using namespace amp_stl_algorithms;

	auto in0 = input;
	auto in1 = input;
	const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

	const auto v = std::numeric_limits<decltype(in1)::value_type>::max();
	const auto p = [=](auto&& x) restrict(cpu, amp) { return forward<decltype(x)>(x) < v; };
	const auto result_expect = std::partition(std::begin(in0), std::end(in0), p);
	const auto result_amp = amp_stl_algorithms::partition(begin(in1_av), end(in1_av), p);

	ASSERT_EQ(std::distance(std::begin(in0), result_expect),
			  std::distance(begin(in1_av), result_amp));

	std::sort(std::begin(in0), result_expect);
	std::sort(result_expect, std::end(in0));
	std::sort(begin(in1_av), result_amp);
	std::sort(result_amp, end(in1_av));
	ASSERT_TRUE(std::equal(std::cbegin(in0), std::cend(in0), cbegin(in1_av)));
}

TEST_F(stl_algorithms_partition_tests, partition_with_all_greater_than)
{
	using namespace amp_stl_algorithms;

	auto in0 = input;
	auto in1 = input;
	const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

	const auto v = std::numeric_limits<decltype(in1)::value_type>::min();
	const auto p = [=](auto&& x) restrict(cpu, amp) { return forward<decltype(x)>(x) < v; };
	const auto result_expect = std::partition(std::begin(in0), std::end(in0), p);
	const auto result_amp = amp_stl_algorithms::partition(begin(in1_av), end(in1_av), p);

	ASSERT_EQ(std::distance(std::begin(in0), result_expect),
			  std::distance(begin(in1_av), result_amp));

	std::sort(std::begin(in0), result_expect);
	std::sort(result_expect, std::end(in0));
	std::sort(begin(in1_av), result_amp);
	std::sort(result_amp, end(in1_av));
	ASSERT_TRUE(std::equal(std::cbegin(in0), std::cend(in0), cbegin(in1_av)));
}

TEST_F(stl_algorithms_partition_tests, partition_large_range)
{
	using namespace amp_stl_algorithms;

	static constexpr std::size_t sz = successor(Execution_parameters::tile_size() *
		                                        Execution_parameters::maximum_tile_cnt());
	static constexpr std::size_t sample_sz = testtools::compute_sample_size(sz);

	std::vector<int> in(sz);
	std::generate_n(std::begin(in), sz, rand); // rand should suffice here.
	const concurrency::array_view<int> in_av(sz);

	std::mt19937_64 g;
	std::uniform_int_distribution<std::size_t> d(0, predecessor(sz));
	for (std::size_t i = 0; i != sample_sz; ++i) {
		concurrency::copy(std::cbegin(in), in_av);
		const auto piv = in[d(g)];
		const auto p = [=](auto&& x) restrict(cpu, amp) {
			return forward<decltype(x)>(x) < piv;
		};

		const auto result_expect = std::partition(std::begin(in), std::end(in), p);
		const auto result_amp = amp_stl_algorithms::partition(begin(in_av), end(in_av), p);

		ASSERT_EQ(std::distance(std::begin(in), result_expect),
				  std::distance(begin(in_av), result_amp));
		ASSERT_TRUE(std::is_partitioned(cbegin(in_av), cend(in_av), p));

	/*	std::sort(std::begin(in), result_expect);
		std::sort(result_expect, std::end(in));
		std::sort(begin(in_av), result_amp);
		std::sort(result_amp, end(in_av));
		ASSERT_TRUE(std::equal(std::cbegin(in), std::cend(in), cbegin(in_av)));*/

		std::random_shuffle(std::begin(in), std::end(in));
	//	std::cout << '.';
	}
//	std::cin.get();
}

TEST_F(stl_algorithms_partition_tests, partition_large_range_with_pod)
{
	using namespace amp_stl_algorithms;
	using concurrency::graphics::int_4;

	static constexpr std::size_t sz = successor(Execution_parameters::tile_size() *
												Execution_parameters::maximum_tile_cnt());
	static constexpr std::size_t sample_sz = testtools::compute_sample_size(sz);

	std::vector<int_4> in(sz);
	std::generate_n(std::begin(in), sz, []() { return int_4(rand(), rand(), rand(), rand()); });
	const concurrency::array_view<int_4> in_av(sz);

	const auto cmp = [](auto&& x, auto&& y) restrict(cpu, amp) {
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
		concurrency::copy(std::cbegin(in), in_av);

		const auto p = [=, r = in[d(g)]](auto&& x) restrict(cpu, amp) {
			return cmp(forward<decltype(x)>(x), r);
		};

		const auto result_expect = std::partition(std::begin(in), std::end(in), p);
		const auto result_amp = amp_stl_algorithms::partition(begin(in_av), end(in_av), p);

		ASSERT_EQ(std::distance(std::begin(in), result_expect),
			  	  std::distance(begin(in_av), result_amp));
		ASSERT_TRUE(std::is_partitioned(cbegin(in_av), cend(in_av), p));

		std::random_shuffle(std::begin(in), std::end(in));

		//std::sort(std::begin(in), result_expect, cmp);
		//std::sort(result_expect, std::end(in), cmp);
		//std::sort(begin(in_av), result_amp, cmp);
		//std::sort(result_amp, end(in_av), cmp);
		//ASSERT_TRUE(std::equal(std::cbegin(in), std::cend(in), cbegin(in_av)));
	}
}