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
* This file contains unit tests for the transform algorithm.
*---------------------------------------------------------------------------*/

#include <gtest/gtest.h>

#include "amp_stl_algorithms.h"
#include "testtools.h"

#include <algorithm>
#include <iterator>
#include <functional>
#include <ppl.h>

//----------------------------------------------------------------------------
// transform
//----------------------------------------------------------------------------

class stl_algorithms_transform_tests : public stl_algorithms_testbase<13>,
									   public ::testing::Test {};

TEST_F(stl_algorithms_transform_tests, unary_transform)
{
	using namespace amp_stl_algorithms;

	for (decltype(input.size()) i = 2; i != input.size(); ++i) {
		decltype(input) out;
		const concurrency::array_view<decltype(input)::value_type> out_av(out.size());

		const auto result_expect = std::transform(cbegin(input_av),
												  std::next(cbegin(input_av), i),
												  std::begin(out),
												  std::negate<>());
		const auto result_amp = amp_stl_algorithms::transform(cbegin(input_av),
															  std::next(cbegin(input_av), i),
															  begin(out_av),
															  amp_algorithms::negate<>());
		ASSERT_EQ(std::distance(std::begin(out), result_expect),
				  std::distance(begin(out_av), result_amp));
		ASSERT_TRUE(std::equal(std::begin(out), result_expect, cbegin(out_av)));
	}
}

TEST_F(stl_algorithms_transform_tests, unary_transform_empty_range)
{
	using namespace amp_stl_algorithms;

	decltype(input) out;
	const concurrency::array_view<decltype(input)::value_type> out_av(out.size());

	const auto result_expect = std::transform(cbegin(input_av),
											  cbegin(input_av),
											  std::begin(out),
											  std::negate<>());
	const auto result_amp = amp_stl_algorithms::transform(cbegin(input_av),
														  cbegin(input_av),
														  begin(out_av),
														  amp_algorithms::negate<>());
	ASSERT_EQ(std::distance(std::begin(out), result_expect),
			  std::distance(begin(out_av), result_amp));
}

TEST_F(stl_algorithms_transform_tests, unary_transform_single_element_range)
{
	using namespace amp_stl_algorithms;

	decltype(input) out;
	const concurrency::array_view<decltype(input)::value_type> out_av(out.size());

	const auto result_expect = std::transform(cbegin(input_av),
											  std::next(cbegin(input_av)),
											  std::begin(out),
											  std::negate<>());
	const auto result_amp = amp_stl_algorithms::transform(cbegin(input_av),
														  std::next(cbegin(input_av)),
														  begin(out_av),
														  amp_algorithms::negate<>());

	ASSERT_EQ(std::distance(std::begin(out), result_expect),
			  std::distance(begin(out_av), result_amp));
	ASSERT_TRUE(std::equal(std::begin(out), result_expect, cbegin(out_av)));
}

TEST_F(stl_algorithms_transform_tests, unary_transform_large_range)
{
	using namespace amp_stl_algorithms;

	static constexpr decltype(auto) sz = successor(Execution_parameters::tile_size() *
												   Execution_parameters::maximum_tile_cnt());/*successor(1 << 26);*/
	static constexpr decltype(auto) sample = testtools::compute_sample_size(sz);

	const concurrency::array_view<int> in(sz);
	std::generate_n(in.data(), sz, rand);

	std::vector<int> out(sz);
	const concurrency::array_view<int> out_av(sz);

	for (decltype(in.extent.size()) i = 0; i != sample; ++i) {
		const auto result_expect = concurrency::parallel_transform(cbegin(in),
																   cend(in),
												                   std::begin(out),
												                   std::negate<>());
		const auto result_amp = amp_stl_algorithms::transform(cbegin(in),
															  cend(in),
															  begin(out_av),
															  amp_algorithms::negate<>());
		ASSERT_EQ(std::distance(std::begin(out), result_expect),
				  std::distance(begin(out_av), result_amp));
		ASSERT_TRUE(std::equal(std::begin(out), result_expect, cbegin(out_av)));

		std::random_shuffle(in.data(), in.data() + sz);
	}
}

TEST_F(stl_algorithms_transform_tests, unary_transform_large_range_pod)
{
	using namespace amp_stl_algorithms;

	static constexpr decltype(auto) sz = successor(Execution_parameters::tile_size() *
												   Execution_parameters::maximum_tile_cnt());/*successor(1 << 26);*/
	static constexpr decltype(auto) sample = testtools::compute_sample_size(sz);

	using T = concurrency::graphics::int_4;
	const concurrency::array_view<T> in(sz);
	std::generate_n(in.data(), sz, []() { return T(rand(), rand(), rand(), rand()); });
	std::vector<T> out(sz);
	const concurrency::array_view<T> out_av(sz);

	for (decltype(in.extent.size()) i = 0; i != sample; ++i) {
		const auto result_expect = concurrency::parallel_transform(cbegin(in),
																   cend(in),
												                   std::begin(out),
												                   std::negate<>());
		const auto result_amp = amp_stl_algorithms::transform(cbegin(in),
															  cend(in),
															  begin(out_av),
															  [](auto&& x) restrict(amp) { return -x; }); // Bug workaround.
		ASSERT_EQ(std::distance(std::begin(out), result_expect),
				  std::distance(begin(out_av), result_amp));
		ASSERT_TRUE(std::equal(std::begin(out), result_expect, cbegin(out_av)));

		std::random_shuffle(in.data(), in.data() + sz);
	}
}

TEST_F(stl_algorithms_transform_tests, binary_transform)
{
 	using namespace amp_stl_algorithms;

	for (decltype(input.size()) i = 2; i != input.size(); ++i) {
		decltype(input) out;
		const concurrency::array_view<decltype(input)::value_type> out_av(out.size());

		const auto result_expect = std::transform(cbegin(input_av),
												  std::next(cbegin(input_av), i),
												  cbegin(input_av),
												  std::begin(out),
												  std::minus<>());
		const auto result_amp = amp_stl_algorithms::transform(cbegin(input_av),
															  std::next(cbegin(input_av), i),
															  cbegin(input_av),
															  begin(out_av),
															  amp_algorithms::minus<>());
		ASSERT_EQ(std::distance(std::begin(out), result_expect),
				  std::distance(begin(out_av), result_amp));
	}
}
