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
// reduce
//----------------------------------------------------------------------------

class stl_algorithms_reduce_tests : public stl_algorithms_testbase<13>,
						  	        public ::testing::Test {};

TEST_F(stl_algorithms_reduce_tests, reduce_sum)
{
	using namespace amp_stl_algorithms;

    const auto result_expect = std::accumulate(cbegin(input_av),
											   cend(input_av),
											   decltype(input_av)::value_type(0),
											   std::plus<>());
    const auto result_amp = amp_stl_algorithms::reduce(cbegin(input_av),
													   cend(input_av),
													   decltype(input_av)::value_type(0),
													   amp_algorithms::plus<>());

    ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_reduce_tests, reduce_max)
{
	using namespace amp_stl_algorithms;

	const auto result_expect = std::accumulate(cbegin(input_av),
											   cend(input_av),
											   std::numeric_limits<decltype(input_av)::value_type>::min(),
											   [](auto&& x, auto&& y) {
		return std::max(std::forward<decltype(x)>(x), std::forward<decltype(y)>(y));
	});
	const auto result_amp = amp_stl_algorithms::reduce(cbegin(input_av),
													   cend(input_av),
													   std::numeric_limits<decltype(input_av)::value_type>::min(),
													   [](auto&& x, auto&& y) restrict(cpu, amp) {
		return amp_stl_algorithms::max(forward<decltype(x)>(x), forward<decltype(y)>(y));
	});

    ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_reduce_tests, reduce_empty_range)
{
	using namespace amp_stl_algorithms;

	const auto result_expect = std::accumulate(cbegin(input_av),
											   cbegin(input_av),
											   decltype(input_av)::value_type(0),
											   std::plus<>());
	const auto result_amp = amp_stl_algorithms::reduce(cbegin(input_av),
													   cbegin(input_av),
													   decltype(input_av)::value_type(0),
													   amp_algorithms::plus<>());

    ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_reduce_tests, reduce_single_element_range)
{
	using namespace amp_stl_algorithms;

	const auto result_expect = std::accumulate(cbegin(input_av),
											   std::next(cbegin(input_av)),
											   decltype(input_av)::value_type(0),
											   std::plus<>());
	const auto result_amp = amp_stl_algorithms::reduce(cbegin(input_av),
													   std::next(cbegin(input_av)),
													   decltype(input_av)::value_type(0),
													   amp_algorithms::plus<>());

	ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_reduce_tests, reduce_large_range)
{
	using namespace amp_stl_algorithms;

	static constexpr decltype(auto) sz = successor(Execution_parameters::tile_size() *
												   Execution_parameters::maximum_tile_cnt());/*successor(1 << 26)*/;

	static const int x_min = std::numeric_limits<int>::min() / sz;
	static const int x_max = std::numeric_limits<int>::max() / sz;

	std::mt19937_64 g;
	std::uniform_int_distribution<> d(x_min, x_max);
	const concurrency::array_view<int> in(sz);
	std::generate_n(begin(in), sz, [&]() { return d(g); });

	const auto result_expect = concurrency::parallel_reduce(cbegin(in), cend(in), 0, std::plus<>());
	const auto result_amp = amp_stl_algorithms::reduce(cbegin(in), cend(in), 0, amp_algorithms::plus<>());

	ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_reduce_tests, reduce_large_range_pod)
{
	using namespace amp_stl_algorithms;

	static constexpr decltype(auto) sz = successor(Execution_parameters::tile_size() *
												   Execution_parameters::maximum_tile_cnt());/*successor(1 << 25)*/;

	static const int x_min = std::numeric_limits<int>::min() / sz;
	static const int x_max = std::numeric_limits<int>::max() / sz;

	std::mt19937_64 g;
	std::uniform_int_distribution<> d;
	const concurrency::array_view<concurrency::graphics::int_4> in(sz);
	std::generate_n(begin(in), sz, [&]() { return concurrency::graphics::int_4(d(g), d(g), d(g), d(g)); });

	const auto result_expect = concurrency::parallel_reduce(cbegin(in),
															cend(in),
															concurrency::graphics::int_4(0, 0, 0, 0));
	const auto result_amp = amp_stl_algorithms::reduce(cbegin(in),
													   cend(in),
													   concurrency::graphics::int_4(0, 0, 0, 0),
													   amp_algorithms::plus<>());

	ASSERT_EQ(result_expect, result_amp);
}