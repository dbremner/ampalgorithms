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
* This file contains unit tests.
*---------------------------------------------------------------------------*/

#include "stdafx.h"
#include <gtest/gtest.h>

#include <amp_iterators.h>
#include <amp_stl_algorithms.h>
#include "testtools.h"

using namespace concurrency;
using namespace amp_stl_algorithms;
using namespace testtools;

class stl_algorithms_tests : public stl_algorithms_testbase<13>, public ::testing::Test {};

//----------------------------------------------------------------------------
// pair<T1, T2>
//----------------------------------------------------------------------------

TEST(stl_pair_tests, stl_pair_property_accessors)
{
	const amp_stl_algorithms::pair<int, int> res(2, 1);
    amp_stl_algorithms::pair<int, int> input(res.second, res.first);
    const array_view<amp_stl_algorithms::pair<int, int>> input_av(1, &input);

    concurrency::parallel_for_each(input_av.extent, [=](concurrency::index<1> idx) restrict(amp)
    {
        amp_stl_algorithms::swap(input_av[idx].first, input_av[idx].second);
    });

	ASSERT_TRUE(input_av[0] == res);
}

TEST(stl_pair_tests, stl_pair_copy)
{
    amp_stl_algorithms::pair<int, int> input(1, 2);
    auto input_av = array_view<amp_stl_algorithms::pair<int, int>>(1, &input);

    concurrency::parallel_for_each(input_av.extent, [=](concurrency::index<1>) restrict(amp)
    {
        amp_stl_algorithms::pair<int, int> x(3, 4);
        input_av[0] = x;
    });

    ASSERT_EQ(3, input_av[0].first);
    ASSERT_EQ(4, input_av[0].second);
}

TEST(stl_pair_tests, stl_pair_conversion_from_std_pair)
{
    std::pair<int, int> y(1, 2);

    amp_stl_algorithms::pair<int, int> x = y;

    ASSERT_EQ(1, x.first);
    ASSERT_EQ(2, x.second);
}

TEST(stl_pair_tests, stl_pair_conversion_to_std_pair)
{
    amp_stl_algorithms::pair<int, int> y(1, 2);

    std::pair<int, int> x = y;

    ASSERT_EQ(1, x.first);
    ASSERT_EQ(2, x.second);
}

//----------------------------------------------------------------------------
// adjacent_difference
//----------------------------------------------------------------------------

// TODO: simplify here, this needn't be so complicated.
static constexpr std::array<int, 10> adjacent_difference_data[] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },
    { 0, 1, 2, 3, 5, 5, 6, 7, 8, 9 },
    { 1, 1, 2, 3, 5, 5, 6, 7, 8, 9 },
    { 1, 1, 2, 3, 5, 5, 6, 3, 8, 9 }
};

class adjacent_difference_tests : public ::testing::TestWithParam <std::array<int, 10>> {};

TEST_P(adjacent_difference_tests, test)
{
    std::vector<std::remove_reference_t<decltype(GetParam())>::value_type> in(std::cbegin(GetParam()),
																			  std::cend(GetParam()));
	std::vector<std::remove_const_t<decltype(in)::value_type>> expect(in.size());
    array_view<const decltype(in)::value_type> in_av(in);
    array_view<decltype(expect)::value_type> out_av(expect.size());

    const auto result_expect = std::adjacent_difference(std::cbegin(in),
														std::cend(in),
														std::begin(expect));
    const auto result_amp = amp_stl_algorithms::adjacent_difference(cbegin(in_av),
																	cend(in_av),
																	begin(out_av));

    ASSERT_EQ(std::distance(std::begin(expect), result_expect),
			  std::distance(begin(out_av), result_amp));
    ASSERT_TRUE(std::equal(std::begin(expect), result_expect, cbegin(out_av)));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests,
						adjacent_difference_tests,
						::testing::ValuesIn(adjacent_difference_data));

TEST_F(stl_algorithms_tests, adjacent_difference_with_empty_array)
{
	std::vector<decltype(input)::value_type> expect;
	auto result_expect = std::adjacent_difference(std::cbegin(input),
												  std::cbegin(input),
												  std::begin(expect));
    auto result_amp = amp_stl_algorithms::adjacent_difference(cbegin(input_av),
															  cbegin(input_av),
															  begin(output_av));
    ASSERT_EQ(std::distance(std::begin(expect), result_expect),
							std::distance(begin(output_av), result_amp));

}

TEST_F(stl_algorithms_tests, adjacent_difference_with_single_element_array)
{
	std::vector<decltype(input)::value_type> expect(1);
	auto result_expect = std::adjacent_difference(std::cbegin(input),
												  std::next(std::cbegin(input)),
												  std::begin(expect));
    auto result_amp = amp_stl_algorithms::adjacent_difference(cbegin(input_av),
															  std::next(cbegin(input_av)),
															  begin(output_av));

    ASSERT_EQ(std::distance(std::begin(expect), result_expect),
			  std::distance(begin(output_av), result_amp));
    ASSERT_TRUE(std::equal(std::begin(expect), result_expect, cbegin(output_av)));
}

TEST_F(stl_algorithms_tests, adjacent_difference_multi_tile)
{
    static constexpr int sz = successor(Execution_parameters::maximum_tile_cnt()) *
										Execution_parameters::tile_size();
	array_view<int> av(sz);
	std::iota(av.data(), av.data() + av.extent.size(), 0);
    array_view<int> result_av(sz);
    std::vector<int> expect(sz);

    const auto result_expect = std::adjacent_difference(av.data(),
														av.data() + av.extent.size(),
														begin(expect));
    const auto result_last = amp_stl_algorithms::adjacent_difference(cbegin(av),
																	 cend(av),
																	 begin(result_av));

    ASSERT_EQ(std::distance(std::begin(expect), result_expect),
		      std::distance(begin(result_av), result_last));
    ASSERT_TRUE(std::equal(std::cbegin(expect), std::cend(expect), cbegin(result_av)));
}

//----------------------------------------------------------------------------
// for_each, for_each_no_return
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, for_each_no_return)
{
    std::fill(begin(input), end(input), 2);
    int sum = 0;
    array_view<int> av_sum(1, &sum);
    amp_stl_algorithms::for_each_no_return(cbegin(input_av),
										   cend(input_av),
										   [=](auto&& val) restrict(amp) {
        concurrency::atomic_fetch_add(&av_sum[0], forward<decltype(val)>(val));
    });
    av_sum.synchronize();
    ASSERT_EQ(std::accumulate(cbegin(input), cend(input), 0, std::plus<int>()), sum);
}

//----------------------------------------------------------------------------
// includes
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, includes_finds_contiguous_range)
{
	auto in0 = input;
	std::sort(std::begin(in0), std::end(in0));
	const concurrency::array_view<decltype(in0)::value_type> in0_av(in0);

	for (decltype(in0.size()) i = 1; i != in0.size(); ++i) {
		for (decltype(in0.size()) j = 0; j < in0.size() - i; j += i) {
			std::vector<decltype(in0)::value_type> subset(std::cbegin(in0) + j,
														  std::cbegin(in0) + j + i);
			const concurrency::array_view<decltype(in0)::value_type> subset_av(subset);

			const auto result_expect = std::includes(std::cbegin(in0),
													 std::cend(in0),
													 std::cbegin(subset),
													 std::cend(subset));
			const auto result_amp = amp_stl_algorithms::includes(cbegin(in0_av),
																 cend(in0_av),
																 cbegin(subset_av),
																 cend(subset_av));

			ASSERT_EQ(result_expect, result_amp);
		}
	}
}

TEST_F(stl_algorithms_tests, includes_finds_non_contiguous_range)
{
	auto in0 = input;
	std::sort(std::begin(in0), std::end(in0));
	const concurrency::array_view<decltype(in0)::value_type> in0_av(in0);

	for (decltype(in0.size()) i = 2; i != in0.size(); ++i) {
		for (decltype(in0.size()) j = 0; j < in0.size() - i; ++j) {
			std::vector<decltype(in0)::value_type> subset;
			for (auto k = j; k < in0.size(); k += i) subset.push_back(in0[k]);
			const concurrency::array_view<decltype(in0)::value_type> subset_av(subset);

			const auto result_expect = std::includes(std::cbegin(in0),
													 std::cend(in0),
													 std::cbegin(subset),
													 std::cend(subset));
			const auto result_amp = amp_stl_algorithms::includes(cbegin(in0_av),
																 cend(in0_av),
																 cbegin(subset_av),
																 cend(subset_av));

			ASSERT_EQ(result_expect, result_amp);
		}
	}
}

TEST_F(stl_algorithms_tests, includes_does_not_find_empty_intersection)
{
	auto in0 = input;
	std::sort(std::begin(in0), std::end(in0));
	const concurrency::array_view<decltype(in0)::value_type> in0_av(in0);

	for (decltype(in0.size()) i = 1; i != in0.size(); ++i) {
		std::vector<decltype(in0)::value_type> subset(i, std::numeric_limits<decltype(in0)::value_type>::max());
		const concurrency::array_view<decltype(in0)::value_type> subset_av(subset);

		const auto result_expect = std::includes(std::cbegin(in0), std::cend(in0), std::cbegin(subset),
												 std::cend(subset));
		const auto result_amp = amp_stl_algorithms::includes(cbegin(in0_av),
															 cend(in0_av),
															 cbegin(subset_av),
															 cend(subset_av));

		ASSERT_EQ(result_expect, result_amp);
	}
}

TEST_F(stl_algorithms_tests, includes_does_not_find_non_empty_intersection_max)
{
	auto in0 = input;
	std::sort(std::begin(in0), std::end(in0));
	const concurrency::array_view<decltype(in0)::value_type> in0_av(in0);

	for (decltype(in0.size()) i = std::count(std::cbegin(in0), std::cend(in0), in0.back()) + 1; i != in0.size(); ++i) {
		std::vector<decltype(in0)::value_type> subset(i, in0.back());
		const concurrency::array_view<decltype(in0)::value_type> subset_av(subset);

		const auto result_expect = std::includes(std::cbegin(in0),
												 std::cend(in0),
												 std::cbegin(subset),
												 std::cend(subset));
		const auto result_amp = amp_stl_algorithms::includes(cbegin(in0_av),
															 cend(in0_av),
															 cbegin(subset_av),
															 cend(subset_av));

		ASSERT_EQ(result_expect, result_amp);
	}
}

TEST_F(stl_algorithms_tests, includes_does_not_find_non_empty_intersection_min)
{
	auto in0 = input;
	std::sort(std::begin(in0), std::end(in0));
	const concurrency::array_view<decltype(in0)::value_type> in0_av(in0);

	for (decltype(in0.size()) i = std::count(std::cbegin(in0), std::cend(in0), in0.front()) + 1; i != in0.size(); ++i) {
		std::vector<decltype(in0)::value_type> subset(i, in0.front());
		const concurrency::array_view<decltype(in0)::value_type> subset_av(subset);

		const auto result_expect = std::includes(std::cbegin(in0),
												 std::cend(in0),
												 std::cbegin(subset),
												 std::cend(subset));
		const auto result_amp = amp_stl_algorithms::includes(cbegin(in0_av),
															 cend(in0_av),
															 cbegin(subset_av),
															 cend(subset_av));

		ASSERT_EQ(result_expect, result_amp);
	}
}

TEST_F(stl_algorithms_tests, includes_does_not_find_non_empty_intersection_middle)
{
	auto in0 = input;
	std::sort(std::begin(in0), std::end(in0));
	const concurrency::array_view<decltype(in0)::value_type> in0_av(in0);

	for (decltype(in0.size()) i = std::count(std::cbegin(in0), std::cend(in0), in0[in0.size() / 2]) + 1; i != in0.size(); ++i) {
		std::vector<decltype(in0)::value_type> subset(i, in0[in0.size() / 2]);
		const concurrency::array_view<decltype(in0)::value_type> subset_av(subset);

		const auto result_expect = std::includes(std::cbegin(in0),
												 std::cend(in0),
												 std::cbegin(subset),
												 std::cend(subset));
		const auto result_amp = amp_stl_algorithms::includes(cbegin(in0_av),
															 cend(in0_av),
															 cbegin(subset_av),
															 cend(subset_av));

		ASSERT_EQ(result_expect, result_amp);
	}
}
TEST_F(stl_algorithms_tests, includes_with_empty_set)
{
	auto in0 = input; std::sort(std::begin(in0), std::end(in0));
	const concurrency::array_view<decltype(in0)::value_type> in0_av(in0);

	const auto result_expect = std::includes(std::cbegin(in0),
											 std::cbegin(in0),
											 std::cbegin(in0),
											 std::cend(in0));
	const auto result_amp = amp_stl_algorithms::includes(cbegin(in0_av),
														 cbegin(in0_av),
														 cbegin(in0_av),
														 cend(in0_av));

	ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_tests, includes_with_empty_subset)
{
	auto in0 = input; std::sort(std::begin(in0), std::end(in0));
	const concurrency::array_view<decltype(in0)::value_type> in0_av(in0);

	const auto result_expect = std::includes(std::cbegin(in0),
											 std::cend(in0),
											 std::cbegin(in0),
											 std::cbegin(in0));
	const auto result_amp = amp_stl_algorithms::includes(cbegin(in0_av),
														 cend(in0_av),
														 cbegin(in0_av),
														 cbegin(in0_av));

	ASSERT_EQ(result_expect, result_amp);
}

//----------------------------------------------------------------------------
// inner_product
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, inner_product)
{
	static constexpr std::size_t sz = successor(Execution_parameters::maximum_tile_cnt() *
												Execution_parameters::tile_size());
	static constexpr int x0 = 2;

    std::vector<int> vec1(sz, 1);
    array_view<const int> av1(vec1);
    std::vector<int> vec2(sz, 2);
    array_view<const int> av2(vec2);

	const auto expect = std::inner_product(std::cbegin(vec1),
										   std::cend(vec1),
										   std::cbegin(vec2),
										   x0);
    const auto result = amp_stl_algorithms::inner_product(cbegin(av1), cend(av1), cbegin(av2), x0);

    ASSERT_EQ(expect, result);
}

TEST_F(stl_algorithms_tests, inner_product_pred)
{
	static constexpr std::size_t sz = successor(Execution_parameters::maximum_tile_cnt() *
												Execution_parameters::tile_size());;
	static constexpr int x0 = 2;

	std::vector<int> vec1(sz, 1);
    array_view<const int> av1(vec1);
	std::vector<int> vec2(sz, 2);
    array_view<const int> av2(vec2);

	const auto expect = std::inner_product(std::cbegin(vec1),
										   std::cend(vec1),
										   std::cbegin(vec2),
										   x0,
										   std::plus<>(),
										   std::multiplies<>());
    const auto result = amp_stl_algorithms::inner_product(cbegin(av1),
														  cend(av1),
														  cbegin(av2),
														  x0,
														  amp_algorithms::plus<>(),
														  amp_algorithms::multiplies<>());

    ASSERT_EQ(expect, result);
}

//----------------------------------------------------------------------------
// lexicographical_compare
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, lexicographical_compare_1st_range_varies)
{
	using namespace amp_stl_algorithms;

	for (decltype(input.size()) i = 2; i != input.size(); ++i) {
		const auto result_expect = std::lexicographical_compare(std::crbegin(input),
																std::next(std::crbegin(input), i),
																std::cbegin(input),
																std::cend(input));
		const auto result_amp = amp_stl_algorithms::lexicographical_compare(crbegin(input_av),
																			std::next(crbegin(input_av), i),
																			cbegin(input_av),
																			cend(input_av));
		ASSERT_EQ(result_expect, result_amp);
	}
}

TEST_F(stl_algorithms_tests, lexicographical_compare_2nd_range_varies)
{
	for (decltype(input.size()) i = 2; i != input.size(); ++i) {
		const auto result_expect = std::lexicographical_compare(std::cbegin(input),
																std::cend(input),
																std::crbegin(input),
																std::next(std::crbegin(input), i));
		const auto result_amp = amp_stl_algorithms::lexicographical_compare(cbegin(input_av),
																			cend(input_av),
																			crbegin(input_av),
																			std::next(crbegin(input_av), i));
		ASSERT_EQ(result_expect, result_amp);
	}
}

TEST_F(stl_algorithms_tests, lexicographical_compare_single_element_1st_range)
{
	for (decltype(input.size()) i = 1; i != input.size() - 1; ++i) {
		const auto result_expect = std::lexicographical_compare(std::next(std::cbegin(input), i - 1),
																std::next(std::cbegin(input), i),
																std::crbegin(input),
																std::crend(input));
		const auto result_amp = amp_stl_algorithms::lexicographical_compare(std::next(cbegin(input_av), i - 1),
																			std::next(cbegin(input_av), i),
																			crbegin(input_av),
																			crend(input_av));
		ASSERT_EQ(result_expect, result_amp);
	}
}

TEST_F(stl_algorithms_tests, lexicographical_compare_single_element_2nd_range)
{
	for (decltype(input.size()) i = 1; i != input.size() - 1; ++i) {
		const auto result_expect = std::lexicographical_compare(std::crbegin(input),
																std::crend(input),
																std::next(std::cbegin(input), i - 1),
																std::next(std::cbegin(input), i));
		const auto result_amp = amp_stl_algorithms::lexicographical_compare(crbegin(input_av),
																			crend(input_av),
																			std::next(cbegin(input_av), i - 1),
																			std::next(cbegin(input_av), i));
		ASSERT_EQ(result_expect, result_amp);
	}
}

TEST_F(stl_algorithms_tests, lexicographical_compare_single_element_ranges)
{
	const auto result_expect = std::lexicographical_compare(std::cbegin(input),
															std::next(std::cbegin(input)),
															std::cbegin(input),
															std::next(std::cbegin(input)));
	const auto result_amp = amp_stl_algorithms::lexicographical_compare(cbegin(input_av),
																		std::next(cbegin(input_av)),
																		cbegin(input_av),
																		std::next(cbegin(input_av)));
	ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_tests, lexicographical_compare_1st_range_empty)
{
	const auto result_expect = std::lexicographical_compare(std::cbegin(input),
															std::cbegin(input),
															std::cbegin(input),
															std::cend(input));
	const auto result_amp = amp_stl_algorithms::lexicographical_compare(cbegin(input_av),
																		cbegin(input_av),
																		cbegin(input_av),
																		cend(input_av));
	ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_tests, lexicographical_compare_2nd_range_empty)
{
	const auto result_expect = std::lexicographical_compare(std::cbegin(input),
															std::cend(input),
															std::cbegin(input),
															std::cbegin(input));
	const auto result_amp = amp_stl_algorithms::lexicographical_compare(cbegin(input_av),
																		cend(input_av),
																		cbegin(input_av),
																		cbegin(input_av));
	ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_tests, lexicographical_compare_ranges_empty)
{
	const auto result_expect = std::lexicographical_compare(std::cbegin(input),
															std::cbegin(input),
															std::cbegin(input),
															std::cbegin(input));
	const auto result_amp = amp_stl_algorithms::lexicographical_compare(cbegin(input_av),
																		cbegin(input_av),
																		cbegin(input_av),
																		cbegin(input_av));
	ASSERT_EQ(result_expect, result_amp);
}

//----------------------------------------------------------------------------
// lower_bound
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, lower_bound)
{
	auto in = input; std::sort(std::begin(in), std::end(in));
	const concurrency::array_view<const decltype(in)::value_type> in_av(in);

	for (decltype(input.size()) i = 0; i != input.size(); ++i) {
		const auto result_expect = std::lower_bound(std::cbegin(in), std::cend(in), input[i]);
		const auto result_amp = amp_stl_algorithms::lower_bound(cbegin(in_av),
																cend(in_av),
																input[i]);

		ASSERT_EQ(std::distance(std::cbegin(in), result_expect),
				  std::distance(cbegin(in_av), result_amp));
	}
}

TEST_F(stl_algorithms_tests, lower_bound_with_empty_range)
{
	const auto result_expect = std::lower_bound(std::cbegin(input), std::cbegin(input), input.front());
    const auto result_amp = amp_stl_algorithms::lower_bound(cbegin(input_av),
															cbegin(input_av),
															input.front());
    ASSERT_EQ(std::distance(std::cbegin(input), result_expect),
		      std::distance(cbegin(input_av), result_amp));
}

TEST_F(stl_algorithms_tests, lower_bound_with_single_element_range)
{
	const auto result_expect = std::lower_bound(std::cbegin(input),
												std::next(std::cbegin(input)),
												input.front());
	const auto result_amp = amp_stl_algorithms::lower_bound(cbegin(input_av),
															std::next(cbegin(input_av)),
															input.front());
	ASSERT_EQ(std::distance(std::cbegin(input), result_expect),
			  std::distance(cbegin(input_av), result_amp));
}

TEST_F(stl_algorithms_tests, lower_bound_looks_for_global_minimum)
{
	auto in = input; std::sort(std::begin(in), std::end(in));
	const concurrency::array_view<const decltype(in)::value_type> in_av(in);
	const auto x = std::numeric_limits<decltype(in)::value_type>::min();

	const auto result_expect = std::lower_bound(std::cbegin(in), std::cend(in), x);
	const auto result_amp = amp_stl_algorithms::lower_bound(cbegin(in_av), cend(in_av), x);

	ASSERT_EQ(std::distance(std::cbegin(in), result_expect), std::distance(cbegin(in_av), result_amp));
}

TEST_F(stl_algorithms_tests, lower_bound_looks_for_global_maximum)
{
	auto in = input; std::sort(std::begin(in), std::end(in));
	const concurrency::array_view<const decltype(in)::value_type> in_av(in);
	const auto x = std::numeric_limits<decltype(in)::value_type>::max();

	const auto result_expect = std::lower_bound(std::cbegin(in), std::cend(in), x);
	const auto result_amp = amp_stl_algorithms::lower_bound(cbegin(in_av), cend(in_av), x);

	ASSERT_EQ(std::distance(std::cbegin(in), result_expect), std::distance(cbegin(in_av), result_amp));
}

//----------------------------------------------------------------------------
// upper_bound
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, upper_bound)
{
	auto in = input; std::sort(std::begin(in), std::end(in));
	const concurrency::array_view<const decltype(in)::value_type> in_av(in);

	for (decltype(input.size()) i = 0; i != input.size(); ++i) {
		const auto result_expect = std::upper_bound(std::cbegin(in), std::cend(in), input[i]);
		const auto result_amp = amp_stl_algorithms::upper_bound(cbegin(in_av),
																cend(in_av),
																input[i]);

		ASSERT_EQ(std::distance(std::cbegin(in), result_expect),
				  std::distance(cbegin(in_av), result_amp));
	}
}

TEST_F(stl_algorithms_tests, upper_bound_with_empty_range)
{
	const auto result_expect = std::upper_bound(std::cbegin(input), std::cbegin(input), input.front());
	const auto result_amp = amp_stl_algorithms::upper_bound(cbegin(input_av),
															cbegin(input_av),
															input.front());
	ASSERT_EQ(std::distance(std::cbegin(input), result_expect),
			  std::distance(cbegin(input_av), result_amp));
}

TEST_F(stl_algorithms_tests, upper_bound_with_single_element_range)
{
	const auto result_expect = std::upper_bound(std::cbegin(input), std::next(std::cbegin(input)), input.front());
	const auto result_amp = amp_stl_algorithms::upper_bound(cbegin(input_av),
															std::next(cbegin(input_av)),
															input.front());
	ASSERT_EQ(std::distance(std::cbegin(input), result_expect),
			  std::distance(cbegin(input_av), result_amp));
}

TEST_F(stl_algorithms_tests, upper_bound_looks_for_global_minimum)
{
	auto in = input; std::sort(std::begin(in), std::end(in));
	const concurrency::array_view<const decltype(in)::value_type> in_av(in);
	const auto x = std::numeric_limits<decltype(in)::value_type>::min();

	const auto result_expect = std::upper_bound(std::cbegin(in), std::cend(in), x);
	const auto result_amp = amp_stl_algorithms::upper_bound(cbegin(in_av), cend(in_av), x);

	ASSERT_EQ(std::distance(std::cbegin(in), result_expect), std::distance(cbegin(in_av), result_amp));
}

TEST_F(stl_algorithms_tests, upper_bound_looks_for_global_maximum)
{
	auto in = input; std::sort(std::begin(in), std::end(in));
	const concurrency::array_view<const decltype(in)::value_type> in_av(in);
	const auto x = std::numeric_limits<decltype(in)::value_type>::max();

	const auto result_expect = std::upper_bound(std::cbegin(in), std::cend(in), x);
	const auto result_amp = amp_stl_algorithms::upper_bound(cbegin(in_av), cend(in_av), x);

	ASSERT_EQ(std::distance(std::cbegin(in), result_expect), std::distance(cbegin(in_av), result_amp));
}

//----------------------------------------------------------------------------
// inplace_merge
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, inplace_merge_subrange_varies)
{
	using namespace amp_stl_algorithms;

	for (decltype(input.size()) i = 2; i != input.size() - 1; ++i) {
		auto in = input;
		std::sort(std::begin(in), std::next(std::begin(in), i));
		std::sort(std::next(std::begin(in), i), std::end(in));

		const concurrency::array_view<decltype(in)::value_type> in_av(in.size());
		concurrency::copy(std::cbegin(in), in_av);

		std::inplace_merge(std::begin(in), std::next(std::begin(in), i), std::end(in));
		amp_stl_algorithms::inplace_merge(begin(in_av), std::next(begin(in_av), i), end(in_av));

		ASSERT_TRUE(std::equal(std::cbegin(in), std::cend(in), cbegin(in_av)));
	}
}

TEST_F(stl_algorithms_tests, inplace_merge_with_1st_subrange_empty)
{
	std::sort(std::begin(input), std::end(input));
	std::sort(begin(input_av), end(input_av));

	std::inplace_merge(std::begin(input), std::begin(input), std::end(input));
	amp_stl_algorithms::inplace_merge(begin(input_av), begin(input_av), end(input_av));

	ASSERT_TRUE(std::equal(std::cbegin(input), std::cend(input), cbegin(input_av)));
}

TEST_F(stl_algorithms_tests, inplace_merge_with_2nd_subrange_empty)
{
	std::sort(std::begin(input), std::end(input));
	std::sort(begin(input_av), end(input_av));

	std::inplace_merge(std::begin(input), std::end(input), std::end(input));
	amp_stl_algorithms::inplace_merge(begin(input_av), end(input_av), end(input_av));

	ASSERT_TRUE(std::equal(std::cbegin(input), std::cend(input), cbegin(input_av)));
}

TEST_F(stl_algorithms_tests, inplace_merge_with_both_subranges_empty)
{
	std::inplace_merge(std::begin(input), std::begin(input), std::begin(input));
	amp_stl_algorithms::inplace_merge(begin(input_av), begin(input_av), begin(input_av));

	ASSERT_TRUE(std::equal(std::cbegin(input), std::cend(input), cbegin(input_av)));
}

TEST_F(stl_algorithms_tests, inplace_merge_with_single_element_1st_subrange)
{
	std::vector<decltype(input)::value_type> out(std::cbegin(input), std::cend(input));
	std::sort(std::next(std::begin(out)), std::end(out));

	const concurrency::array_view<decltype(out)::value_type> out_av(out.size());
	concurrency::copy(std::cbegin(out), out_av);

	std::inplace_merge(std::begin(out), std::next(std::begin(out)), std::end(out));
	amp_stl_algorithms::inplace_merge(begin(out_av), std::next(begin(out_av)), end(out_av));

	ASSERT_TRUE(std::equal(std::cbegin(out), std::cend(out), cbegin(out_av)));
}

TEST_F(stl_algorithms_tests, inplace_merge_with_single_element_2nd_subrange)
{
	std::vector<decltype(input)::value_type> out(std::cbegin(input), std::cend(input));
	std::sort(std::begin(out), std::prev(std::end(out)));

	const concurrency::array_view<decltype(out)::value_type> out_av(out.size());
	concurrency::copy(std::cbegin(out), out_av);

	std::inplace_merge(std::begin(out), std::prev(std::end(out)), std::end(out));
	amp_stl_algorithms::inplace_merge(begin(out_av), std::prev(end(out_av)), end(out_av));

	ASSERT_TRUE(std::equal(std::cbegin(out), std::cend(out), cbegin(out_av)));
}

TEST_F(stl_algorithms_tests, inplace_merge_with_single_element_subranges)
{
	std::vector<decltype(input)::value_type> out(std::cbegin(input), std::cbegin(input) + 2);
	std::sort(std::rbegin(out), std::rend(out));

	const concurrency::array_view<decltype(out)::value_type> out_av(out.size());
	concurrency::copy(std::cbegin(out), out_av);

	std::inplace_merge(std::begin(out), std::next(std::begin(out)), std::end(out));
	amp_stl_algorithms::inplace_merge(begin(out_av), std::next(begin(out_av)), end(out_av));

	ASSERT_TRUE(std::equal(std::cbegin(out), std::cend(out), cbegin(out_av)));
}

//----------------------------------------------------------------------------
// mismatch
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, mismatch_with_equal_ranges)
{
	const auto result_expect = std::mismatch(std::cbegin(input), std::cend(input), std::cbegin(input));
	const auto result_amp = amp_stl_algorithms::mismatch(cbegin(input_av), cend(input_av), cbegin(input_av));

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.first),
		      std::distance(cbegin(input_av), result_amp.first));
	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.second),
		      std::distance(cbegin(input_av), result_amp.second));
}

TEST_F(stl_algorithms_tests, mismatch_with_unequal_ranges_first_elem)
{
	std::vector<decltype(input)::value_type> in0(std::cbegin(input), std::cend(input));
	in0.front() += 1;
	auto in1 = in0;
	concurrency::array_view<const decltype(input)::value_type> in1_av(in1);
	const auto result_expect = std::mismatch(std::cbegin(input), std::cend(input), std::cbegin(in0));
	const auto result_amp = amp_stl_algorithms::mismatch(cbegin(input_av), cend(input_av), cbegin(in1_av));

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.first),
			  std::distance(cbegin(input_av), result_amp.first));
	ASSERT_EQ(std::distance(std::cbegin(in0), result_expect.second),
		      std::distance(cbegin(in1_av), result_amp.second));
}

TEST_F(stl_algorithms_tests, mismatch_with_unequal_ranges_last_elem)
{
	std::vector<decltype(input)::value_type> in0(std::cbegin(input), std::cend(input));
	in0.back() += 1;
	auto in1 = in0;
	concurrency::array_view<const decltype(input)::value_type> in1_av(in1);
	const auto result_expect = std::mismatch(std::cbegin(input), std::cend(input), std::cbegin(in0));
	const auto result_amp = amp_stl_algorithms::mismatch(cbegin(input_av), cend(input_av), cbegin(in1_av));

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.first),
			  std::distance(cbegin(input_av), result_amp.first));
	ASSERT_EQ(std::distance(std::cbegin(in0), result_expect.second),
			  std::distance(cbegin(in1_av), result_amp.second));
}

TEST_F(stl_algorithms_tests, mismatch_with_unequal_ranges_some_elem)
{
	std::vector<decltype(input)::value_type> in0(std::cbegin(input), std::cend(input));
	in0[in0.size() / 2] += 1;
	auto in1 = in0;
	concurrency::array_view<const decltype(input)::value_type> in1_av(in1);
	const auto result_expect = std::mismatch(std::cbegin(input), std::cend(input), std::cbegin(in0));
	const auto result_amp = amp_stl_algorithms::mismatch(cbegin(input_av), cend(input_av), cbegin(in1_av));

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.first),
			  std::distance(cbegin(input_av), result_amp.first));
	ASSERT_EQ(std::distance(std::cbegin(in0), result_expect.second),
			  std::distance(cbegin(in1_av), result_amp.second));
}

TEST_F(stl_algorithms_tests, mismatch_with_empty_range)
{
	const auto result_expect = std::mismatch(std::cbegin(input), std::cbegin(input), std::cbegin(input));
	const auto result_amp = amp_stl_algorithms::mismatch(cbegin(input_av), cbegin(input_av), cbegin(input_av));

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.first),
			  std::distance(cbegin(input_av), result_amp.first));
	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.second),
			  std::distance(cbegin(input_av), result_amp.second));
}

TEST_F(stl_algorithms_tests, mismatch_with_single_element_range)
{
	const auto result_expect = std::mismatch(std::cbegin(input),
											 std::next(std::cbegin(input)),
											 std::cbegin(input));
	const auto result_amp = amp_stl_algorithms::mismatch(cbegin(input_av),
														 std::next(cbegin(input_av)),
														 cbegin(input_av));

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.first),
			  std::distance(cbegin(input_av), result_amp.first));
	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.second),
			  std::distance(cbegin(input_av), result_amp.second));
}

// TODO: Should be able to make these tests a bit tidier with better casting support for pair<T, T>
//----------------------------------------------------------------------------
// minmax
//----------------------------------------------------------------------------

static constexpr std::array<std::pair<int, int>, 6> minmax_data = {
	std::pair<int, int>(1, 2),
	std::pair<int, int>(100, 100),
	std::pair<int, int>(150, 300),
	std::pair<int, int>(1000, -50),
	std::pair<int, int>(11, 12),
	std::pair<int, int>(-12, 33)
};

TEST_F(stl_algorithms_tests, minmax)
{
    compare_binary_operator(
        [=](auto&& a, auto&& b) { return amp_stl_algorithms::pair<const int,
																  const int>(std::minmax(a, b)); },
        [=](auto&& a, auto&& b) { return amp_stl_algorithms::minmax(a, b); },
        minmax_data);
}

TEST_F(stl_algorithms_tests, minmax_pred)
{
    compare_binary_operator(
        [=](auto&& a, auto&& b) { return amp_stl_algorithms::pair<const int, const int>(std::minmax(a, b, std::greater_equal<>())); },
        [=](auto&& a, auto&& b) { return amp_stl_algorithms::minmax(a, b, amp_algorithms::greater_equal<>()); },
        minmax_data);
}

//----------------------------------------------------------------------------
// partition_point
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, partition_point)
{
	const auto p = [v = input[input.size() / 2]](auto&& x) restrict(cpu, amp) { return x < v; };

	auto in0 = input; std::partition(std::begin(in0), std::end(in0), p);
	auto in1 = in0;
	const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

	const auto result_expect = std::partition_point(std::cbegin(in0), std::cend(in0), p);
	const auto result_amp = amp_stl_algorithms::partition_point(cbegin(in1_av), cend(in1_av), p);

	ASSERT_EQ(std::distance(std::cbegin(in0), result_expect),
			  std::distance(amp_stl_algorithms::cbegin(in1_av), result_amp));
}

TEST_F(stl_algorithms_tests, partition_point_with_empty_range)
{
	const auto p = [v = input[input.size() / 2]](auto&& x) restrict(cpu, amp) { return x < v; };
	const auto result_expect = std::partition_point(std::cbegin(input), std::cbegin(input), p);
	const auto result_amp = amp_stl_algorithms::partition_point(cbegin(input_av), cbegin(input_av), p);

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect),
			  std::distance(cbegin(input_av), result_amp));
}

TEST_F(stl_algorithms_tests, partition_point_with_single_element_range_less)
{
	const auto v = std::numeric_limits<decltype(input)::value_type>::max();
	const auto p = [=](auto&& x) restrict(cpu, amp) { return forward<decltype(x)>(x) < v; };
	const auto result_expect = std::partition_point(std::cbegin(input), std::next(std::cbegin(input)), p);
	const auto result_amp = amp_stl_algorithms::partition_point(cbegin(input_av),
																std::next(cbegin(input_av)),
																p);

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect),
			  std::distance(cbegin(input_av), result_amp));
}

TEST_F(stl_algorithms_tests, partition_point_with_single_element_range_greater)
{
	const auto v = std::numeric_limits<decltype(input)::value_type>::min();
	const auto p = [=](auto&& x) restrict(cpu, amp) { return forward<decltype(x)>(x) < v; };
	const auto result_expect = std::partition_point(std::cbegin(input), std::next(std::cbegin(input)), p);
	const auto result_amp = amp_stl_algorithms::partition_point(cbegin(input_av),
																std::next(cbegin(input_av)),
																p);

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect),
			  std::distance(cbegin(input_av), result_amp));
}

TEST_F(stl_algorithms_tests, partition_point_with_all_less_than)
{
	auto in0 = input;
	auto in1 = input;
	const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

	const auto M = std::numeric_limits<decltype(in1)::value_type>::max();
	const auto p = [=](auto&& x) restrict(cpu, amp) { return x < M; };
	const auto result_expect = std::partition_point(std::cbegin(in0), std::cend(in0), p);
	const auto result_amp = amp_stl_algorithms::partition_point(cbegin(in1_av), cend(in1_av), p);

	ASSERT_EQ(std::distance(std::cbegin(in0), result_expect),
			  std::distance(cbegin(in1_av), result_amp));
}

TEST_F(stl_algorithms_tests, partition_point_with_all_greater_than)
{
	auto in0 = input;
	auto in1 = input;
	const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

	const auto m = std::numeric_limits<decltype(in1)::value_type>::min();
	const auto p = [=](auto&& x) restrict(cpu, amp) { return forward<decltype(x)>(x) < m; };
	const auto result_expect = std::partition_point(std::cbegin(in0), std::cend(in0), p);
	const auto result_amp = amp_stl_algorithms::partition_point(cbegin(in1_av), cend(in1_av), p);

	ASSERT_EQ(std::distance(std::cbegin(in0), result_expect),
			  std::distance(cbegin(in1_av), result_amp));
}

//----------------------------------------------------------------------------
// is_partitioned
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, is_partitioned_true)
{
	auto in0 = input;
	for (decltype(in0.size()) i = 0; i != in0.size(); ++i) {
		const auto p = [v = input[i]](auto&& x) restrict(cpu, amp) { return forward<decltype(x)>(x) < v; };

		std::partition(std::begin(in0), std::end(in0), p);

		const concurrency::array_view<decltype(in0)::value_type> in0_av(in0);
		const auto result_amp = amp_stl_algorithms::is_partitioned(cbegin(in0_av), cend(in0_av), p);

		ASSERT_TRUE(result_amp);
	}
}

TEST_F(stl_algorithms_tests, is_partitioned_false)
{
	const auto p = [v = input[input.size() / 2]](auto&& x) restrict(cpu, amp) { return forward<decltype(x)>(x) < v; };

	auto in0 = input;
	auto p_point = std::partition(std::begin(in0), std::end(in0), p);
	std::rotate(std::begin(in0), p_point, std::end(in0));

	const concurrency::array_view<decltype(in0)::value_type> in0_av(in0);
	const auto result_amp = amp_stl_algorithms::is_partitioned(cbegin(in0_av), cend(in0_av), p);

	ASSERT_FALSE(result_amp);
}

TEST_F(stl_algorithms_tests, is_partitioned_with_empty_range)
{
	const auto p = [v = input[input.size() / 2]](auto&& x) restrict(cpu, amp) { return forward<decltype(x)>(x) < v; };
	const auto result_amp = amp_stl_algorithms::is_partitioned(cbegin(input_av), cbegin(input_av), p);

	ASSERT_TRUE(result_amp);
}

TEST_F(stl_algorithms_tests, is_partitioned_with_single_element_range)
{
	const auto p = [v = input[input.size() / 2]](auto&& x) restrict(cpu, amp) { return forward<decltype(x)>(x) < v; };

	auto in0 = input;
	std::partition(std::begin(in0), std::end(in0), p);

	const concurrency::array_view<decltype(in0)::value_type> in0_av(in0);
	const auto result_amp = amp_stl_algorithms::is_partitioned(cbegin(in0_av), std::next(cbegin(in0_av)), p);

	ASSERT_TRUE(result_amp);
}

//----------------------------------------------------------------------------
// reverse, reverse_copy
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, reverse)
{
	for (decltype(input.size()) i = 2; i != input.size(); ++i) {
		std::vector<decltype(input)::value_type> in0(std::cbegin(input), std::cbegin(input) + i);
		auto in1 = in0;
		const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

		std::reverse(std::begin(in0), std::end(in0));
		amp_stl_algorithms::reverse(amp_stl_algorithms::begin(in1_av), amp_stl_algorithms::end(in1_av));

		ASSERT_TRUE(std::equal(std::cbegin(in0), std::cend(in0), amp_stl_algorithms::cbegin(in1_av)));
	}
}

TEST_F(stl_algorithms_tests, reverse_empty_range)
{
	std::reverse(std::begin(input), std::begin(input));
	amp_stl_algorithms::reverse(amp_stl_algorithms::begin(input_av), amp_stl_algorithms::begin(input_av));

	ASSERT_TRUE(std::equal(std::cbegin(input), std::cend(input), amp_stl_algorithms::cbegin(input_av)));
}

TEST_F(stl_algorithms_tests, reverse_single_element_range)
{
	std::reverse(std::begin(input), std::begin(input) + 1);
	amp_stl_algorithms::reverse(amp_stl_algorithms::begin(input_av), amp_stl_algorithms::begin(input_av) + 1);

	ASSERT_TRUE(std::equal(std::cbegin(input), std::cend(input), amp_stl_algorithms::cbegin(input_av)));
}

TEST_F(stl_algorithms_tests, reverse_copy)
{
	for (decltype(input.size()) i = 2; i != input.size(); ++i) {
		std::vector<decltype(input)::value_type> out0(i);
		auto out1 = out0;
		const concurrency::array_view<decltype(out1)::value_type> out1_av(out1);

		std::reverse_copy(std::cbegin(input), std::next(std::cbegin(input), i), std::begin(out0));
		amp_stl_algorithms::reverse_copy(cbegin(input_av), std::next(cbegin(input_av), i), begin(out1_av));

		ASSERT_TRUE(std::equal(std::cbegin(out0), std::cend(out0), cbegin(out1_av)));
	}
}

TEST_F(stl_algorithms_tests, reverse_copy_empty_range)
{
	std::vector<decltype(input)::value_type> out0(1, decltype(input)::value_type(0));
	auto out1 = out0;
	const concurrency::array_view<decltype(out1)::value_type> out1_av(out1);

	std::reverse_copy(std::cbegin(input), std::cbegin(input), std::begin(out0));
	amp_stl_algorithms::reverse_copy(cbegin(input_av), cbegin(input_av), begin(out1_av));

	ASSERT_TRUE(std::equal(std::cbegin(out0), std::cend(out0), cbegin(out1_av)));
}

TEST_F(stl_algorithms_tests, reverse_copy_single_element_range)
{
	std::vector<decltype(input)::value_type> out0(1);
	auto out1 = out0;
	const concurrency::array_view<decltype(out1)::value_type> out1_av(out1);

	std::reverse_copy(std::cbegin(input), std::next(std::cbegin(input)), std::begin(out0));
	amp_stl_algorithms::reverse_copy(cbegin(input_av), std::next(cbegin(input_av)), begin(out1_av));

	ASSERT_TRUE(std::equal(std::cbegin(out0), std::cend(out0), cbegin(out1_av)));
}

//----------------------------------------------------------------------------
// search
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// partial_sort
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, partial_sort)
{
	for (decltype(input.size()) i = 2; i != input.size() - 1; ++i) {
		auto in0 = input;
		auto in1 = in0;
		const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

		std::partial_sort(std::begin(in0), std::next(std::begin(in0), i), std::end(in0));
		amp_stl_algorithms::partial_sort(begin(in1_av), std::next(begin(in1_av), i), end(in1_av));

		ASSERT_TRUE(std::is_sorted(cbegin(in1_av), std::next(cbegin(in1_av), i)));
		ASSERT_TRUE(std::equal(std::cbegin(in0), std::next(std::cbegin(in0), i), cbegin(in1_av)));
	}
}

TEST_F(stl_algorithms_tests, partial_sort_empty_range)
{
	std::partial_sort(std::begin(input), std::begin(input), std::begin(input));
	amp_stl_algorithms::partial_sort(begin(input_av), begin(input_av), begin(input_av));

	ASSERT_EQ(std::is_sorted(std::cbegin(input), std::cbegin(input)),
			  std::is_sorted(cbegin(input_av), cbegin(input_av)));
	ASSERT_TRUE(std::equal(std::cbegin(input), std::cbegin(input), cbegin(input_av)));
}

TEST_F(stl_algorithms_tests, partial_sort_single_element_range)
{
	std::partial_sort(std::begin(input), std::next(std::begin(input)), std::end(input));
	amp_stl_algorithms::partial_sort(begin(input_av), std::next(begin(input_av)), end(input_av));

	ASSERT_TRUE(std::is_sorted(cbegin(input_av), std::next(cbegin(input_av))));
	ASSERT_TRUE(std::equal(std::cbegin(input), std::next(std::cbegin(input)), cbegin(input_av)));
}


TEST_F(stl_algorithms_tests, partial_sort_sorted_range)
{
	for (decltype(input.size()) i = 2; i != input.size() - 1; ++i) {
		auto in0 = input; std::sort(std::begin(in0), std::end(in0));
		auto in1 = in0;
		const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

		std::partial_sort(std::begin(in0), std::next(std::begin(in0), i), std::end(in0));
		amp_stl_algorithms::partial_sort(begin(in1_av), std::next(begin(in1_av), i), end(in1_av));

		ASSERT_TRUE(std::is_sorted(cbegin(in1_av), std::next(cbegin(in1_av), i)));
		ASSERT_TRUE(std::equal(std::cbegin(in0), std::next(std::cbegin(in0), i), cbegin(in1_av)));
	}
}

TEST_F(stl_algorithms_tests, partial_sort_reverse_sorted_range)
{
	for (decltype(input.size()) i = 2; i != input.size() - 1; ++i) {
		auto in0 = input; std::sort(std::begin(in0), std::end(in0), std::greater<>());
		auto in1 = in0;
		const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

		std::partial_sort(std::begin(in0), std::next(std::begin(in0), i), std::end(in0));
		amp_stl_algorithms::partial_sort(begin(in1_av), std::next(begin(in1_av), i), end(in1_av));

		ASSERT_TRUE(std::is_sorted(cbegin(in1_av), std::next(cbegin(in1_av), i)));
		ASSERT_TRUE(std::equal(std::cbegin(in0), std::next(std::cbegin(in0), i), cbegin(in1_av)));
	}
}

TEST_F(stl_algorithms_tests, partial_sort_concave_range)
{
	for (decltype(input.size()) i = 2; i != input.size() - 1; ++i) {
		auto in0 = input;
		std::sort(std::begin(in0), std::end(in0));
		std::reverse(std::begin(in0) + in0.size() / 2, std::end(in0));
		auto in1 = in0;
		const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

		std::partial_sort(std::begin(in0), std::next(std::begin(in0), i), std::end(in0));
		amp_stl_algorithms::partial_sort(begin(in1_av), std::next(begin(in1_av), i), end(in1_av));

		ASSERT_TRUE(std::is_sorted(cbegin(in1_av), std::next(cbegin(in1_av), i)));
		ASSERT_TRUE(std::equal(std::cbegin(in0), std::next(std::cbegin(in0), i), cbegin(in1_av)));
	}
}

TEST_F(stl_algorithms_tests, partial_sort_convex_range)
{
	for (decltype(input.size()) i = 2; i != input.size() - 1; ++i) {
		auto in0 = input;
		std::sort(std::begin(in0), std::end(in0));
		std::reverse(std::begin(in0), std::begin(in0) + in0.size() / 2);
		auto in1 = in0;
		const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

		std::partial_sort(std::begin(in0), std::next(std::begin(in0), i), std::end(in0));
		amp_stl_algorithms::partial_sort(begin(in1_av), std::next(begin(in1_av), i), end(in1_av));

		ASSERT_TRUE(std::is_sorted(cbegin(in1_av), std::next(cbegin(in1_av), i)));
		ASSERT_TRUE(std::equal(std::cbegin(in0), std::next(std::cbegin(in0), i), cbegin(in1_av)));
	}
}

//----------------------------------------------------------------------------
// unique, unique_copy
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, unique)
{
	for (decltype(input.size()) i = 2; i != input.size(); ++i) {
		std::vector<decltype(input)::value_type> in0(std::cbegin(input), std::cbegin(input) + i);
		auto in1 = in0;
		const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

		const auto result_expect = std::unique(std::begin(in0), std::end(in0));
		const auto result_amp = amp_stl_algorithms::unique(begin(in1_av), end(in1_av));

		ASSERT_EQ(std::distance(std::begin(in0), result_expect),
				  std::distance(begin(in1_av), result_amp));
		std::sort(std::begin(in0), result_expect);
		std::sort(amp_stl_algorithms::begin(in1_av), result_amp);
		ASSERT_TRUE(std::equal(std::begin(in0), result_expect, amp_stl_algorithms::cbegin(in1_av)));
	}
}

TEST_F(stl_algorithms_tests, unique_all_elements_equal)
{
	for (decltype(input.size()) i = 2; i != input.size(); ++i) {
		std::vector<decltype(input)::value_type> in0(i, input.front());
		auto in1 = in0;
		const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

		const auto result_expect = std::unique(std::begin(in0), std::end(in0));
		const auto result_amp = amp_stl_algorithms::unique(begin(in1_av), end(in1_av));

		ASSERT_EQ(std::distance(std::begin(in0), result_expect),
				  std::distance(begin(in1_av), result_amp));
		std::sort(std::begin(in0), result_expect);
		std::sort(begin(in1_av), result_amp);
		ASSERT_TRUE(std::equal(std::begin(in0), result_expect, cbegin(in1_av)));
	}
}

TEST_F(stl_algorithms_tests, unique_empty_range)
{
	const auto result_expect = std::unique(std::begin(input), std::begin(input));
	const auto result_amp = amp_stl_algorithms::unique(begin(input_av), begin(input_av));

	ASSERT_EQ(std::distance(std::begin(input), result_expect),
			  std::distance(begin(input_av), result_amp));
}

TEST_F(stl_algorithms_tests, unique_single_element_range)
{
	const auto result_expect = std::unique(std::begin(input), std::next(std::begin(input)));
	const auto result_amp = amp_stl_algorithms::unique(begin(input_av), std::next(begin(input_av)));

	ASSERT_EQ(std::distance(std::begin(input), result_expect),
			  std::distance(begin(input_av), result_amp));
	ASSERT_TRUE(std::equal(std::begin(input), result_expect, cbegin(input_av)));
}

TEST_F(stl_algorithms_tests, unique_copy)
{
	for (decltype(input.size()) i = 2; i != input.size(); ++i) {
		std::vector<decltype(input)::value_type> out(i);
		const concurrency::array_view<decltype(out)::value_type> out_av(out.size());

		const auto result_expect = std::unique_copy(std::cbegin(input),
												    std::next(std::cbegin(input), i),
													std::begin(out));
		const auto result_amp = amp_stl_algorithms::unique_copy(cbegin(input_av),
																std::next(cbegin(input_av), i),
																begin(out_av));

		ASSERT_EQ(std::distance(std::begin(out), result_expect),
				  std::distance(begin(out_av), result_amp));
		std::sort(std::begin(out), result_expect);
		std::sort(begin(out_av), result_amp);
		ASSERT_TRUE(std::equal(std::begin(out), result_expect, cbegin(out_av)));
	}
}

TEST_F(stl_algorithms_tests, unique_copy_all_elements_equal)
{
	const std::vector<decltype(input)::value_type> in(input.size(), input.front());
	const concurrency::array_view<const decltype(in)::value_type> in_av(in);

	for (decltype(input.size()) i = 2; i != input.size(); ++i) {
		std::vector<decltype(in)::value_type> out(i);
		const concurrency::array_view<decltype(in)::value_type> out_av(out.size());

		const auto result_expect = std::unique_copy(std::cbegin(in),
													std::next(std::cbegin(in), i),
													std::begin(out));
		const auto result_amp = amp_stl_algorithms::unique_copy(cbegin(in_av),
																std::next(cbegin(in_av), i),
																begin(out_av));

		ASSERT_EQ(std::distance(std::begin(out), result_expect),
				  std::distance(begin(out_av), result_amp));
		std::sort(std::begin(out), result_expect);
		std::sort(begin(out_av), result_amp);
		ASSERT_TRUE(std::equal(std::begin(out), result_expect, cbegin(out_av)));
	}
}

TEST_F(stl_algorithms_tests, unique_copy_empty_range)
{
	std::vector<decltype(input)::value_type> out0(1);
	const concurrency::array_view<decltype(out0)::value_type> out1_av(out0.size());

	const auto result_expect = std::unique_copy(std::cbegin(input),
												std::cbegin(input),
												std::begin(out0));
	const auto result_amp = amp_stl_algorithms::unique_copy(cbegin(input_av),
															cbegin(input_av),
															begin(out1_av));

	ASSERT_EQ(std::distance(std::begin(out0), result_expect),
			  std::distance(begin(out1_av), result_amp));
}

TEST_F(stl_algorithms_tests, unique_copy_single_element_range)
{
	std::vector<decltype(input)::value_type> out0(1);
	const concurrency::array_view<decltype(out0)::value_type> out1_av(out0.size());

	const auto result_expect = std::unique_copy(std::cbegin(input),
												std::next(std::cbegin(input)),
												std::begin(out0));
	const auto result_amp = amp_stl_algorithms::unique_copy(cbegin(input_av),
															std::next(cbegin(input_av)),
															begin(out1_av));

	ASSERT_EQ(std::distance(std::begin(out0), result_expect),
			  std::distance(begin(out1_av), result_amp));
	ASSERT_TRUE(std::equal(std::begin(out0), result_expect, cbegin(out1_av)));
}