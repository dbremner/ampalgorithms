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
    amp_stl_algorithms::pair<int, int> input(1, 2);
    array_view<amp_stl_algorithms::pair<int, int>> input_av(1, &input);

    concurrency::parallel_for_each(input_av.extent, [=](concurrency::index<1> idx) restrict(amp)
    {
        amp_stl_algorithms::swap(input_av[idx].first, input_av[idx].second);
    });

    ASSERT_EQ(2, input_av[0].first);
    ASSERT_EQ(1, input_av[0].second);
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

static constexpr std::array<int, 10> adjacent_difference_data[] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },
    { 0, 1, 2, 3, 5, 5, 6, 7, 8, 9 },
    { 1, 1, 2, 3, 5, 5, 6, 7, 8, 9 },
    { 1, 1, 2, 3, 5, 5, 6, 3, 8, 9 }
};

class adjacent_difference_tests : public ::testing::TestWithParam <std::array<int, 10>> {};

TEST_P(adjacent_difference_tests, test)
{
    std::vector<std::remove_reference_t<decltype(GetParam())>::value_type> input(cbegin(GetParam()), cend(GetParam()));
	std::vector<std::remove_const_t<decltype(input)::value_type>> expect(10);
    array_view<decltype(input)::value_type> input_av(input);
    array_view<decltype(expect)::value_type> output_av(input.size());


    auto result_expect = std::adjacent_difference(std::cbegin(input), std::cend(input), std::begin(expect));
    auto result_last = amp_stl_algorithms::adjacent_difference(cbegin(input_av), cend(input_av), begin(output_av));

    ASSERT_EQ(std::distance(std::begin(expect), result_expect), std::distance(begin(output_av), result_last));
    ASSERT_TRUE(are_equal(expect, output_av));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, adjacent_difference_tests, ::testing::ValuesIn(adjacent_difference_data));

TEST_F(stl_algorithms_tests, adjacent_difference_with_empty_array)
{
	std::vector<decltype(input)::value_type> expect;
	auto result_expect = std::adjacent_difference(std::cbegin(input), std::cbegin(input), std::begin(expect));
    auto result_last = amp_stl_algorithms::adjacent_difference(cbegin(input_av), cbegin(input_av), begin(output_av));
    ASSERT_EQ(std::distance(std::begin(expect), result_expect), std::distance(begin(output_av), result_last));

}

TEST_F(stl_algorithms_tests, adjacent_difference_with_single_element_array)
{
	std::vector<decltype(input)::value_type> expect(1);
	auto result_expect = std::adjacent_difference(std::cbegin(input), std::cbegin(input) + 1u, std::begin(expect));
    auto result_last = amp_stl_algorithms::adjacent_difference(cbegin(input_av), cbegin(input_av) + 1, begin(output_av));

    ASSERT_EQ(std::distance(std::begin(expect), result_expect), std::distance(begin(output_av), result_last));
    ASSERT_EQ(expect.front(), output_av[0]);
}

TEST_F(stl_algorithms_tests, adjacent_difference_multi_tile)
{
    static constexpr int sz = 1024;
    std::vector<int> vec(sz);
    generate_data(vec);
    array_view<int> av(vec);
    std::vector<int> result(sz, 0);
    array_view<int> result_av(result);
    std::array<int, sz> expect;

    std::adjacent_difference(cbegin(vec), cend(vec), begin(expect));

    auto result_last = amp_stl_algorithms::adjacent_difference(cbegin(av), cend(av), begin(result_av));
    ASSERT_EQ(sz, std::distance(begin(result_av), result_last));
    ASSERT_TRUE(are_equal(expect, result_av));
}

//----------------------------------------------------------------------------
// all_of, any_of, none_of
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, none_of_finds_no_values)
{
    bool r = amp_stl_algorithms::none_of(cbegin(input_av), cend(input_av), [](auto&& v) restrict(amp) { return v > 10; });
    ASSERT_TRUE(r);
}

TEST_F(stl_algorithms_tests, none_of_finds_value)
{
    bool r = amp_stl_algorithms::none_of(cbegin(input_av), cend(input_av), [](auto&& v) restrict(amp) { return v > 5; });
    ASSERT_FALSE(r);
}

TEST_F(stl_algorithms_tests, any_of_finds_no_values)
{
    bool r = amp_stl_algorithms::any_of(cbegin(input_av), cend(input_av), [] (auto&& v) restrict(amp) { return v > 10; });
    ASSERT_FALSE(r);
}

TEST_F(stl_algorithms_tests, any_of_finds_value)
{
    bool r = amp_stl_algorithms::any_of(cbegin(input_av), cend(input_av), [](auto&& v) restrict(amp) { return v > 5; });
    ASSERT_TRUE(r);
}

TEST_F(stl_algorithms_tests, all_of_finds_all_values)
{
    bool r = amp_stl_algorithms::all_of(cbegin(input_av), cend(input_av), [](auto&& v) restrict(amp) { return v <= 10; });
    ASSERT_TRUE(r);
}

TEST_F(stl_algorithms_tests, all_of_finds_some_values)
{
    bool r = amp_stl_algorithms::all_of(cbegin(input_av), cend(input_av), [](auto&& v) restrict(amp) { return v > 5; });
    ASSERT_FALSE(r);
}

TEST_F(stl_algorithms_tests, all_of_finds_no_values)
{
    bool r = amp_stl_algorithms::all_of(cbegin(input_av), cend(input_av), [](int v) restrict(amp) { return v > 10; });
    ASSERT_FALSE(r);
}

//----------------------------------------------------------------------------
// binary_search
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, binary_search)
{
	auto in0 = input; std::sort(std::begin(in0), std::end(in0));
	auto in1 = in0;
	const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

	auto result_expect = std::binary_search(std::cbegin(in0), std::cend(in0), input.back());
	auto result_amp = amp_stl_algorithms::binary_search(amp_stl_algorithms::cbegin(in1_av), amp_stl_algorithms::cend(in1_av), input.back());

	ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_tests, binary_search_does_not_find)
{
	auto in0 = input; std::sort(std::begin(in0), std::end(in0));
	auto in1 = in0;
	const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

	const auto key = std::numeric_limits<decltype(in1)::value_type>::max();
	auto result_expect = std::binary_search(std::cbegin(in0), std::cend(in0), key);
	auto result_amp = amp_stl_algorithms::binary_search(amp_stl_algorithms::cbegin(in1_av), amp_stl_algorithms::cend(in1_av), key);

	ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_tests, binary_search_with_empty_range)
{
	auto result_expect = std::binary_search(std::cbegin(input), std::cbegin(input), input.back());
	auto result_amp = amp_stl_algorithms::binary_search(amp_stl_algorithms::cbegin(input_av), amp_stl_algorithms::cbegin(input_av), input.back());

	ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_tests, binary_search_with_single_element_does_not_find)
{
	const auto key = std::numeric_limits<decltype(input)::value_type>::max();
	auto result_expect = std::binary_search(std::cbegin(input), std::cbegin(input) + 1, key);
	auto result_amp = amp_stl_algorithms::binary_search(amp_stl_algorithms::cbegin(input_av), amp_stl_algorithms::cbegin(input_av) + 1, key);

	ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_tests, binary_search_with_single_element_finds)
{
	auto result_expect = std::binary_search(std::cbegin(input), std::cbegin(input) + 1, input.front());
	auto result_amp = amp_stl_algorithms::binary_search(amp_stl_algorithms::cbegin(input_av), amp_stl_algorithms::cbegin(input_av) + 1, input.front());

	ASSERT_EQ(result_expect, result_amp);
}

//----------------------------------------------------------------------------
// count, count_if
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, count_counts_values)
{
    auto r = amp_stl_algorithms::count(cbegin(input_av), cend(input_av), 2);
    ASSERT_EQ(5, r);
}

TEST_F(stl_algorithms_tests, count_counts_no_values)
{
    auto r = amp_stl_algorithms::count(cbegin(input_av), cend(input_av), 22);
    ASSERT_EQ(0, r);
}

TEST_F(stl_algorithms_tests, count_if_counts_values)
{
    auto r = amp_stl_algorithms::count_if(cbegin(input_av), cend(input_av), [=](auto&& v) restrict(amp) { return (v == 2); });
    ASSERT_EQ(5, r);
}

TEST_F(stl_algorithms_tests, count_if_counts_no_values)
{
    auto r = amp_stl_algorithms::count_if(cbegin(input_av), cend(input_av), [=](auto&& v) restrict(amp) { return (v == 22); });
    ASSERT_EQ(0, r);
}

//----------------------------------------------------------------------------
// equal
//----------------------------------------------------------------------------

class stl_algorithms_tests_2 : public stl_algorithms_tests
{
protected:
    std::array<int, 13> equal;
    array_view<int> equal_av;
    std::array<int, 13> unequal;
    array_view<int> unequal_av;

    stl_algorithms_tests_2() :
        equal_av(concurrency::extent<1>(static_cast<int>(input.size())), equal),
        unequal_av(concurrency::extent<1>(static_cast<int>(output.size())), unequal)
    {
        std::copy(cbegin(input), cend(input), begin(equal));
        std::copy(cbegin(input), cend(input), begin(unequal));
        unequal[9] = -1;
    }
};

TEST_F(stl_algorithms_tests_2, equal_true_for_equal_arrays)
{
    bool r = amp_stl_algorithms::equal(cbegin(input_av), cend(input_av), cbegin(equal_av));
    ASSERT_TRUE(r);
}

TEST_F(stl_algorithms_tests_2, equal_false_for_unequal_arrays)
{
    bool r = amp_stl_algorithms::equal(cbegin(input_av), cend(input_av), cbegin(unequal_av));
    ASSERT_FALSE(r);
}

TEST_F(stl_algorithms_tests_2, equal_compares_against_first_array)
{
    auto av = input_av.section(0, 9);
    bool r = amp_stl_algorithms::equal(cbegin(av), cend(av), cbegin(unequal_av));
    ASSERT_TRUE(r);
}

TEST_F(stl_algorithms_tests_2, equal_pred_true_for_equal_arrays)
{
    std::iota(begin(input), end(input), 1);
    std::iota(begin(equal), end(equal), 2);
    auto pred = [](auto&& v1, auto&& v2) restrict(amp) { return ((v1 + 1) == v2); };

    bool r = amp_stl_algorithms::equal(cbegin(input_av), cend(input_av), cbegin(equal_av), pred);

    ASSERT_TRUE(r);
}

TEST_F(stl_algorithms_tests_2, equal_pred_false_for_unequal_arrays)
{
    std::iota(begin(input), end(input), 1);
    std::iota(begin(unequal), end(unequal), 2);
    unequal[3] = 99;
    auto pred = [](auto&& v1, auto&& v2) restrict(amp) { return ((v1 + 1) == v2); };

    bool r = amp_stl_algorithms::equal(cbegin(input_av), cend(input_av), cbegin(equal_av), pred);

    ASSERT_FALSE(r);
}

//----------------------------------------------------------------------------
// equal_range
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, equal_range_finds)
{
	auto in0 = input; std::sort(std::begin(in0), std::end(in0));
	auto in1 = in0;
	const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

	for (auto&& key : input) {
		const auto result_expect = std::equal_range(std::cbegin(in0), std::cend(in0), key);
		const auto result_amp = amp_stl_algorithms::equal_range(amp_stl_algorithms::cbegin(in1_av), amp_stl_algorithms::cend(in1_av), key);

		ASSERT_EQ(result_expect.first - std::cbegin(in0), result_amp.first - amp_stl_algorithms::cbegin(in1_av));
		ASSERT_EQ(result_expect.second - std::cbegin(in0), result_amp.second - amp_stl_algorithms::cbegin(in1_av));
	}
}

TEST_F(stl_algorithms_tests, equal_does_not_find_key_is_greater)
{
	auto in0 = input; std::sort(std::begin(in0), std::end(in0));
	auto in1 = in0;
	const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

	const auto key = std::numeric_limits<decltype(in0)::value_type>::max();
	const auto result_expect = std::equal_range(std::cbegin(in0), std::cend(in0), key);
	const auto result_amp = amp_stl_algorithms::equal_range(amp_stl_algorithms::cbegin(in1_av), amp_stl_algorithms::cend(in1_av), key);

	ASSERT_EQ(result_expect.first - std::cbegin(in0), result_amp.first - amp_stl_algorithms::cbegin(in1_av));
	ASSERT_EQ(result_expect.second - std::cbegin(in0), result_amp.second - amp_stl_algorithms::cbegin(in1_av));
}

TEST_F(stl_algorithms_tests, equal_does_not_find_key_is_less)
{
	auto in0 = input; std::sort(std::begin(in0), std::end(in0));
	auto in1 = in0;
	const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

	const auto key = std::numeric_limits<decltype(in0)::value_type>::min();
	const auto result_expect = std::equal_range(std::cbegin(in0), std::cend(in0), key);
	const auto result_amp = amp_stl_algorithms::equal_range(amp_stl_algorithms::cbegin(in1_av), amp_stl_algorithms::cend(in1_av), key);

	ASSERT_EQ(result_expect.first - std::cbegin(in0), result_amp.first - amp_stl_algorithms::cbegin(in1_av));
	ASSERT_EQ(result_expect.second - std::cbegin(in0), result_amp.second - amp_stl_algorithms::cbegin(in1_av));
}

TEST_F(stl_algorithms_tests, equal_range_with_empty_range)
{
	const auto result_expect = std::equal_range(std::cbegin(input), std::cbegin(input), input.front());
	const auto result_amp = amp_stl_algorithms::equal_range(amp_stl_algorithms::cbegin(input_av), amp_stl_algorithms::cbegin(input_av), input.front());

	ASSERT_EQ(result_expect.first - std::cbegin(input), result_amp.first - amp_stl_algorithms::cbegin(input_av));
	ASSERT_EQ(result_expect.second - std::cbegin(input), result_amp.second - amp_stl_algorithms::cbegin(input_av));
}

TEST_F(stl_algorithms_tests, equal_range_with_single_element_range_finds)
{
	const auto result_expect = std::equal_range(std::cbegin(input), std::cbegin(input) + 1, input.front());
	const auto result_amp = amp_stl_algorithms::equal_range(amp_stl_algorithms::cbegin(input_av), amp_stl_algorithms::cbegin(input_av) + 1, input.front());

	ASSERT_EQ(result_expect.first - std::cbegin(input), result_amp.first - amp_stl_algorithms::cbegin(input_av));
	ASSERT_EQ(result_expect.second - std::cbegin(input), result_amp.second - amp_stl_algorithms::cbegin(input_av));
}

TEST_F(stl_algorithms_tests, equal_range_with_single_element_range_does_not_find)
{
	const auto key = std::numeric_limits<decltype(input)::value_type>::max();
	const auto result_expect = std::equal_range(std::cbegin(input), std::cbegin(input) + 1, key);
	const auto result_amp = amp_stl_algorithms::equal_range(amp_stl_algorithms::cbegin(input_av), amp_stl_algorithms::cbegin(input_av) + 1, key);

	ASSERT_EQ(result_expect.first - std::cbegin(input), result_amp.first - amp_stl_algorithms::cbegin(input_av));
	ASSERT_EQ(result_expect.second - std::cbegin(input), result_amp.second - amp_stl_algorithms::cbegin(input_av));
}
//----------------------------------------------------------------------------
// for_each, for_each_no_return
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, for_each_no_return)
{
    std::fill(begin(input), end(input), 2);
    int sum = 0;
    array_view<int> av_sum(1, &sum);
    amp_stl_algorithms::for_each_no_return(cbegin(input_av), cend(input_av), [=](auto&& val) restrict(amp) {
        concurrency::atomic_fetch_add(&av_sum[{0}], val);
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
			std::vector<decltype(in0)::value_type> subset(std::cbegin(in0) + j, std::cbegin(in0) + j + i);
			const concurrency::array_view<decltype(in0)::value_type> subset_av(subset);

			const auto result_expect = std::includes(std::cbegin(in0), std::cend(in0), std::cbegin(subset), std::cend(subset));
			const auto result_amp = amp_stl_algorithms::includes(amp_stl_algorithms::cbegin(in0_av), amp_stl_algorithms::cend(in0_av), amp_stl_algorithms::cbegin(subset_av), amp_stl_algorithms::cend(subset_av));

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

			const auto result_expect = std::includes(std::cbegin(in0), std::cend(in0), std::cbegin(subset), std::cend(subset));
			const auto result_amp = amp_stl_algorithms::includes(amp_stl_algorithms::cbegin(in0_av), amp_stl_algorithms::cend(in0_av), amp_stl_algorithms::cbegin(subset_av), amp_stl_algorithms::cend(subset_av));

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

		const auto result_expect = std::includes(std::cbegin(in0), std::cend(in0), std::cbegin(subset), std::cend(subset));
		const auto result_amp = amp_stl_algorithms::includes(amp_stl_algorithms::cbegin(in0_av), amp_stl_algorithms::cend(in0_av), amp_stl_algorithms::cbegin(subset_av), amp_stl_algorithms::cend(subset_av));

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

		const auto result_expect = std::includes(std::cbegin(in0), std::cend(in0), std::cbegin(subset), std::cend(subset));
		const auto result_amp = amp_stl_algorithms::includes(amp_stl_algorithms::cbegin(in0_av), amp_stl_algorithms::cend(in0_av), amp_stl_algorithms::cbegin(subset_av), amp_stl_algorithms::cend(subset_av));

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

		const auto result_expect = std::includes(std::cbegin(in0), std::cend(in0), std::cbegin(subset), std::cend(subset));
		const auto result_amp = amp_stl_algorithms::includes(amp_stl_algorithms::cbegin(in0_av), amp_stl_algorithms::cend(in0_av), amp_stl_algorithms::cbegin(subset_av), amp_stl_algorithms::cend(subset_av));

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

		const auto result_expect = std::includes(std::cbegin(in0), std::cend(in0), std::cbegin(subset), std::cend(subset));
		const auto result_amp = amp_stl_algorithms::includes(amp_stl_algorithms::cbegin(in0_av), amp_stl_algorithms::cend(in0_av), amp_stl_algorithms::cbegin(subset_av), amp_stl_algorithms::cend(subset_av));

		ASSERT_EQ(result_expect, result_amp);
	}
}
TEST_F(stl_algorithms_tests, includes_with_empty_set)
{
	auto in0 = input; std::sort(std::begin(in0), std::end(in0));
	const concurrency::array_view<decltype(in0)::value_type> in0_av(in0);

	const auto result_expect = std::includes(std::cbegin(in0), std::cbegin(in0), std::cbegin(in0), std::cend(in0));
	const auto result_amp = amp_stl_algorithms::includes(amp_stl_algorithms::cbegin(in0_av), amp_stl_algorithms::cbegin(in0_av), amp_stl_algorithms::cbegin(in0_av), amp_stl_algorithms::cend(in0_av));

	ASSERT_EQ(result_expect, result_amp);
}

TEST_F(stl_algorithms_tests, includes_with_empty_subset)
{
	auto in0 = input; std::sort(std::begin(in0), std::end(in0));
	const concurrency::array_view<decltype(in0)::value_type> in0_av(in0);

	const auto result_expect = std::includes(std::cbegin(in0), std::cend(in0), std::cbegin(in0), std::cbegin(in0));
	const auto result_amp = amp_stl_algorithms::includes(amp_stl_algorithms::cbegin(in0_av), amp_stl_algorithms::cend(in0_av), amp_stl_algorithms::cbegin(in0_av), amp_stl_algorithms::cbegin(in0_av));

	ASSERT_EQ(result_expect, result_amp);
}
//----------------------------------------------------------------------------
// inner_product
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, inner_product)
{
    std::vector<int> vec1(1024);
    std::fill(begin(vec1), end(vec1), 1);
    array_view<int> av1(1024, vec1);
    std::vector<int> vec2(1024);
    std::fill(begin(vec2), end(vec2), 2);
    array_view<int> av2(1024, vec2);
    int expect = std::inner_product(cbegin(vec1), cend(vec1), cbegin(vec2), 2);

    int result = amp_stl_algorithms::inner_product(cbegin(av1), cend(av1), cbegin(av2), 2);

    ASSERT_EQ(expect, result);
}

TEST_F(stl_algorithms_tests, inner_product_pred)
{
    std::vector<int> vec1(1024);
    std::fill(begin(vec1), end(vec1), 1);
    array_view<int> av1(1024, vec1);
    std::vector<int> vec2(1024);
    std::fill(begin(vec2), end(vec2), 2);
    array_view<int> av2(1024, vec2);
    int expect = std::inner_product(cbegin(vec1), cend(vec1), cbegin(vec2), 2, std::plus<int>(), std::plus<int>());

    int result = amp_stl_algorithms::inner_product(cbegin(av1), cend(av1), cbegin(av2), 2, amp_algorithms::plus<>(), amp_algorithms::plus<>());

    ASSERT_EQ(expect, result);
}

//----------------------------------------------------------------------------
//lower_bound, upper_bound
//----------------------------------------------------------------------------

static constexpr std::vector<unsigned int> l_b_data[] = {
	{ 1 },					 // Single element range.
    { 1, 2, 3, 4, 5, 6, 7 }, // Typical range.
    { 0, 0, 0, 0, 0, 0, 0 }, // All equal, less than tested, should return last.
    { 3, 4, 5, 6, 7, 8, 9 }, // All equal, greater than tested, should return first.
    { 2, 2, 2, 2, 2, 2, 2 },  // All equal to tested, should return first.
	{ 1, 2, 3, 4, 5, 6, 7, 8 }, // The following test the same, but with even sized range
	{ 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 3, 4, 5, 6, 7, 8, 9, 10 },
	{ 2, 2, 2, 2, 2, 2, 2, 2 }
};

class lower_bound_tests : public ::testing::TestWithParam<std::vector<unsigned int>> {};

TEST_P(lower_bound_tests, test)
{
    std::vector<unsigned int> input(std::cbegin(GetParam()), std::cend(GetParam()));
    concurrency::array_view<unsigned int> input_av(input);

    auto expected_iter = std::lower_bound(cbegin(input), cend(input), 2u);
    auto result_iter = amp_stl_algorithms::lower_bound(cbegin(input_av), cend(input_av), 2u);

    ASSERT_EQ(std::distance(std::cbegin(input), expected_iter), std::distance(cbegin(input_av), result_iter));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, lower_bound_tests, ::testing::ValuesIn(l_b_data));

TEST_F(stl_algorithms_tests, lower_bound_with_empty_array)
{
    auto result_iter = amp_stl_algorithms::lower_bound(cbegin(input_av), cbegin(input_av), 42);
    ASSERT_EQ(0, std::distance(cbegin(input_av), result_iter));
}

class upper_bound_tests : public ::testing::TestWithParam<std::vector<unsigned int>> {};

TEST_P(upper_bound_tests, test)
{
	std::vector<unsigned int> input(std::cbegin(GetParam()), std::cend(GetParam()));
	concurrency::array_view<unsigned int> input_av(input);

	auto expected_iter = std::upper_bound(cbegin(input), cend(input), 2u);
	auto result_iter = amp_stl_algorithms::upper_bound(cbegin(input_av), cend(input_av), 2u);

	ASSERT_EQ(std::distance(std::cbegin(input), expected_iter), std::distance(cbegin(input_av), result_iter));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, upper_bound_tests, ::testing::ValuesIn(l_b_data));

TEST_F(stl_algorithms_tests, upper_bound_with_empty_array)
{
	auto result_iter = amp_stl_algorithms::upper_bound(cbegin(input_av), cbegin(input_av), 42);
	ASSERT_EQ(0, std::distance(cbegin(input_av), result_iter));
}

//----------------------------------------------------------------------------
// mismatch
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, mismatch_with_equal_ranges)
{
	const auto result_expect = std::mismatch(std::cbegin(input), std::cend(input), std::cbegin(input));
	const auto result_amp = amp_stl_algorithms::mismatch(cbegin(input_av), cend(input_av), cbegin(input_av));

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.first), std::distance(cbegin(input_av), result_amp.first));
	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.second), std::distance(cbegin(input_av), result_amp.second));
}

TEST_F(stl_algorithms_tests, mismatch_with_unequal_ranges_first_elem)
{
	std::vector<decltype(input)::value_type> in0(std::cbegin(input), std::cend(input));
	in0.front() += 1;
	auto in1 = in0;
	concurrency::array_view<const decltype(input)::value_type> in1_av(in1);
	const auto result_expect = std::mismatch(std::cbegin(input), std::cend(input), std::cbegin(in0));
	const auto result_amp = amp_stl_algorithms::mismatch(cbegin(input_av), cend(input_av), cbegin(in1_av));

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.first), std::distance(cbegin(input_av), result_amp.first));
	ASSERT_EQ(std::distance(std::cbegin(in0), result_expect.second), std::distance(cbegin(in1_av), result_amp.second));
}

TEST_F(stl_algorithms_tests, mismatch_with_unequal_ranges_last_elem)
{
	std::vector<decltype(input)::value_type> in0(std::cbegin(input), std::cend(input));
	in0.back() += 1;
	auto in1 = in0;
	concurrency::array_view<const decltype(input)::value_type> in1_av(in1);
	const auto result_expect = std::mismatch(std::cbegin(input), std::cend(input), std::cbegin(in0));
	const auto result_amp = amp_stl_algorithms::mismatch(cbegin(input_av), cend(input_av), cbegin(in1_av));

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.first), std::distance(cbegin(input_av), result_amp.first));
	ASSERT_EQ(std::distance(std::cbegin(in0), result_expect.second), std::distance(cbegin(in1_av), result_amp.second));
}

TEST_F(stl_algorithms_tests, mismatch_with_unequal_ranges_some_elem)
{
	std::vector<decltype(input)::value_type> in0(std::cbegin(input), std::cend(input));
	in0[in0.size() / 2] += 1;
	auto in1 = in0;
	concurrency::array_view<const decltype(input)::value_type> in1_av(in1);
	const auto result_expect = std::mismatch(std::cbegin(input), std::cend(input), std::cbegin(in0));
	const auto result_amp = amp_stl_algorithms::mismatch(cbegin(input_av), cend(input_av), cbegin(in1_av));

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.first), std::distance(cbegin(input_av), result_amp.first));
	ASSERT_EQ(std::distance(std::cbegin(in0), result_expect.second), std::distance(cbegin(in1_av), result_amp.second));
}

TEST_F(stl_algorithms_tests, mismatch_with_empty_range)
{
	const auto result_expect = std::mismatch(std::cbegin(input), std::cbegin(input), std::cbegin(input));
	const auto result_amp = amp_stl_algorithms::mismatch(cbegin(input_av), cbegin(input_av), cbegin(input_av));

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.first), std::distance(cbegin(input_av), result_amp.first));
	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.second), std::distance(cbegin(input_av), result_amp.second));
}

TEST_F(stl_algorithms_tests, mismatch_with_single_element_range)
{
	const auto result_expect = std::mismatch(std::cbegin(input), std::next(std::cbegin(input)), std::cbegin(input));
	const auto result_amp = amp_stl_algorithms::mismatch(cbegin(input_av), std::next(cbegin(input_av)), cbegin(input_av));

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.first), std::distance(cbegin(input_av), result_amp.first));
	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.second), std::distance(cbegin(input_av), result_amp.second));
}

//----------------------------------------------------------------------------
// max_element, min_element, minmax, minmax_element
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, max_element)
{
	auto result_expect = std::max_element(std::cbegin(input), std::cend(input));
	auto result_amp = amp_stl_algorithms::max_element(amp_stl_algorithms::cbegin(input_av), amp_stl_algorithms::cend(input_av));

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect), std::distance(amp_stl_algorithms::cbegin(input_av), result_amp));
	ASSERT_EQ(*result_expect, *result_amp);
}

TEST_F(stl_algorithms_tests, max_element_with_multiple_maxima)
{
	auto in0 = input;
	std::fill(std::begin(in0), std::begin(in0) + in0.size() / 3, std::numeric_limits<Value_type<decltype(in0)>>::max());
	std::random_shuffle(std::begin(in0), std::end(in0));
	const concurrency::array_view<Value_type<decltype(in0)>> in0_av(in0);

	auto result_expect = std::max_element(std::cbegin(in0), std::cend(in0));
	auto result_amp = amp_stl_algorithms::max_element(amp_stl_algorithms::cbegin(in0_av), amp_stl_algorithms::cend(in0_av));

	ASSERT_EQ(std::distance(std::cbegin(in0), result_expect), std::distance(amp_stl_algorithms::cbegin(in0_av), result_amp));
	ASSERT_EQ(*result_expect, *result_amp);
}

TEST_F(stl_algorithms_tests, max_element_empty_range)
{
	auto result_expect = std::max_element(std::cbegin(input), std::cbegin(input));
	auto result_amp = amp_stl_algorithms::max_element(amp_stl_algorithms::cbegin(input_av), amp_stl_algorithms::cbegin(input_av));

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect), std::distance(amp_stl_algorithms::cbegin(input_av), result_amp));
	ASSERT_EQ(*result_expect, *result_amp);
}

TEST_F(stl_algorithms_tests, max_element_single_element_range)
{
	auto result_expect = std::max_element(std::cbegin(input), std::cbegin(input) + 1);
	auto result_amp = amp_stl_algorithms::max_element(amp_stl_algorithms::cbegin(input_av), amp_stl_algorithms::cbegin(input_av) + 1);

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect), std::distance(amp_stl_algorithms::cbegin(input_av), result_amp));
	ASSERT_EQ(*result_expect, *result_amp);
}

static constexpr std::array<std::pair<int, int>, 6> minmax_data = {
    std::pair<int, int>(1, 2),
    std::pair<int, int>(100, 100),
    std::pair<int, int>(150, 300),
    std::pair<int, int>(1000, -50),
    std::pair<int, int>(11, 12),
    std::pair<int, int>(-12, 33)
};

TEST_F(stl_algorithms_tests, min_element)
{
	auto result_expect = std::min_element(std::cbegin(input), std::cend(input));
	auto result_amp = amp_stl_algorithms::min_element(amp_stl_algorithms::cbegin(input_av), amp_stl_algorithms::cend(input_av));

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect), std::distance(amp_stl_algorithms::cbegin(input_av), result_amp));
	ASSERT_EQ(*result_expect, *result_amp);
}

TEST_F(stl_algorithms_tests, min_element_with_multiple_minima)
{
	auto in0 = input;
	std::fill(std::begin(in0), std::begin(in0) + input.size() / 3, std::numeric_limits<Value_type<decltype(in0)>>::min());
	std::random_shuffle(std::begin(in0), std::end(in0));
	const concurrency::array_view<Value_type<decltype(in0)>> in0_av(in0);

	auto result_expect = std::min_element(std::cbegin(in0), std::cend(in0));
	auto result_amp = amp_stl_algorithms::min_element(amp_stl_algorithms::cbegin(in0_av), amp_stl_algorithms::cend(in0_av));

	ASSERT_EQ(std::distance(std::cbegin(in0), result_expect), std::distance(amp_stl_algorithms::cbegin(in0_av), result_amp));
	ASSERT_EQ(*result_expect, *result_amp);
}

TEST_F(stl_algorithms_tests, min_element_empty_range)
{
	auto result_expect = std::min_element(std::cbegin(input), std::cbegin(input));
	auto result_amp = amp_stl_algorithms::min_element(amp_stl_algorithms::cbegin(input_av), amp_stl_algorithms::cbegin(input_av));

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect), std::distance(amp_stl_algorithms::cbegin(input_av), result_amp));
	ASSERT_EQ(*result_expect, *result_amp);
}

TEST_F(stl_algorithms_tests, min_element_single_element_range)
{
	auto result_expect = std::min_element(std::cbegin(input), std::cbegin(input) + 1);
	auto result_amp = amp_stl_algorithms::min_element(amp_stl_algorithms::cbegin(input_av), amp_stl_algorithms::cbegin(input_av) + 1);

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect), std::distance(amp_stl_algorithms::cbegin(input_av), result_amp));
	ASSERT_EQ(*result_expect, *result_amp);
}

// TODO: Should be able to make these tests a bit tidier with better casting support for pair<T, T>
TEST_F(stl_algorithms_tests, minmax)
{
    compare_binary_operator(
        [=](auto&& a, auto&& b) { return amp_stl_algorithms::pair<const int, const int>(std::minmax(a, b)); },
        [=](auto&& a, auto&& b) { return amp_stl_algorithms::minmax(a, b); },
        minmax_data);
}

TEST_F(stl_algorithms_tests, minmax_pred)
{
    compare_binary_operator(
        [=](auto&& a, auto&& b) { return amp_stl_algorithms::pair<const int, const int>(std::minmax(a, b, std::greater_equal<int>())); },
        [=](auto&& a, auto&& b) { return amp_stl_algorithms::minmax(a, b, amp_algorithms::greater_equal<>()); },
        minmax_data);
}

TEST_F(stl_algorithms_tests, minmax_element)
{
	auto result_expect = std::minmax_element(std::cbegin(input), std::cend(input));
	auto result_amp = amp_stl_algorithms::minmax_element(amp_stl_algorithms::cbegin(input_av), amp_stl_algorithms::cend(input_av));

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.first), std::distance(amp_stl_algorithms::cbegin(input_av), result_amp.first));
	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.second), std::distance(amp_stl_algorithms::cbegin(input_av), result_amp.second));
	ASSERT_EQ(*result_expect.first, *result_amp.first);
	ASSERT_EQ(*result_expect.second, *result_amp.second);
}

TEST_F(stl_algorithms_tests, minmax_element_with_multiple_extrema)
{
	auto in0 = input;
	std::fill(std::begin(in0), std::begin(in0) + in0.size() / 3, std::numeric_limits<Value_type<decltype(in0)>>::min());
	std::fill(std::begin(in0) + in0.size() / 3, std::begin(in0) + 2 * in0.size() / 3, std::numeric_limits<Value_type<decltype(in0)>>::max());
	std::random_shuffle(std::begin(in0), std::end(in0));
	const concurrency::array_view<Value_type<decltype(in0)>> in0_av(in0);

	auto result_expect = std::minmax_element(std::cbegin(in0), std::cend(in0));
	auto result_amp = amp_stl_algorithms::minmax_element(amp_stl_algorithms::cbegin(in0_av), amp_stl_algorithms::cend(in0_av));

	ASSERT_EQ(std::distance(std::cbegin(in0), result_expect.first), std::distance(amp_stl_algorithms::cbegin(in0_av), result_amp.first));
	ASSERT_EQ(std::distance(std::cbegin(in0), result_expect.second), std::distance(amp_stl_algorithms::cbegin(in0_av), result_amp.second));
	ASSERT_EQ(*result_expect.first, *result_amp.first);
	ASSERT_EQ(*result_expect.second, *result_amp.second);
}

TEST_F(stl_algorithms_tests, minmax_element_empty_range)
{
	auto result_expect = std::minmax_element(std::cbegin(input), std::cbegin(input));
	auto result_amp = amp_stl_algorithms::minmax_element(amp_stl_algorithms::cbegin(input_av), amp_stl_algorithms::cbegin(input_av));

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.first), std::distance(amp_stl_algorithms::cbegin(input_av), result_amp.first));
	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.second), std::distance(amp_stl_algorithms::cbegin(input_av), result_amp.second));
}

TEST_F(stl_algorithms_tests, minmax_element_single_element_range)
{
	auto result_expect = std::minmax_element(std::cbegin(input), std::cbegin(input) + 1);
	auto result_amp = amp_stl_algorithms::minmax_element(amp_stl_algorithms::cbegin(input_av), amp_stl_algorithms::cbegin(input_av) + 1);

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.first), std::distance(amp_stl_algorithms::cbegin(input_av), result_amp.first));
	ASSERT_EQ(std::distance(std::cbegin(input), result_expect.second), std::distance(amp_stl_algorithms::cbegin(input_av), result_amp.second));
	ASSERT_EQ(*result_expect.first, *result_amp.first);
	ASSERT_EQ(*result_expect.second, *result_amp.second);
}

//----------------------------------------------------------------------------
// partial_sum
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, partial_sum)
{
	decltype(input) out;
	concurrency::array_view<decltype(input)::value_type> out_amp(out.size());
	for (auto i = 2; i != out.size() + 1; ++i) {
		const auto result_expect = std::partial_sum(std::cbegin(input), std::cbegin(input) + i, std::begin(out));
		const auto result_amp = amp_stl_algorithms::partial_sum(amp_stl_algorithms::cbegin(input_av),
																amp_stl_algorithms::cbegin(input_av) + i,
																amp_stl_algorithms::begin(out_amp));

		ASSERT_EQ(std::distance(std::begin(out), result_expect), std::distance(amp_stl_algorithms::begin(out_amp), result_amp));
		ASSERT_TRUE(std::equal(std::begin(out), result_expect, amp_stl_algorithms::cbegin(out_amp)));
	}
}

TEST_F(stl_algorithms_tests, partial_sum_empty_range)
{
	decltype(input) out;
	concurrency::array_view<decltype(input)::value_type> out_amp(out.size());

	const auto result_expect = std::partial_sum(std::cbegin(input), std::cbegin(input), std::begin(out));
	const auto result_amp = amp_stl_algorithms::partial_sum(amp_stl_algorithms::cbegin(input_av),
															amp_stl_algorithms::cbegin(input_av), amp_stl_algorithms::begin(out_amp));

	ASSERT_EQ(std::distance(std::begin(out), result_expect), std::distance(amp_stl_algorithms::begin(out_amp), result_amp));
}

TEST_F(stl_algorithms_tests, partial_sum_single_element_range)
{
	decltype(input) out;
	concurrency::array_view<decltype(input)::value_type> out_amp(out.size());

	const auto result_expect = std::partial_sum(std::cbegin(input), std::next(std::cbegin(input)), std::begin(out));
	const auto result_amp = amp_stl_algorithms::partial_sum(amp_stl_algorithms::cbegin(input_av),
															std::next(amp_stl_algorithms::cbegin(input_av)), amp_stl_algorithms::begin(out_amp));

	ASSERT_EQ(std::distance(std::begin(out), result_expect), std::distance(amp_stl_algorithms::begin(out_amp), result_amp));
	ASSERT_TRUE(std::equal(std::begin(out), result_expect, amp_stl_algorithms::cbegin(out_amp)));
}

//----------------------------------------------------------------------------
// partition
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, partition)
{
	auto in0 = input;
	auto in1 = input;
	const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

	const auto p = [v = input[input.size() / 2]](auto&& x) restrict(cpu, amp) { return x < v; };
	const auto result_expect = std::partition(std::begin(in0), std::end(in0), p);
	const auto result_amp = amp_stl_algorithms::partition(begin(in1_av), end(in1_av), p);

	ASSERT_EQ(std::distance(std::begin(in0), result_expect), std::distance(begin(in1_av), result_amp));
	ASSERT_TRUE(std::is_partitioned(cbegin(in1_av), cend(in1_av), p));

	std::sort(std::begin(in0), result_expect);
	std::sort(result_expect, std::end(in0));
	std::sort(begin(in1_av), result_amp);
	std::sort(result_amp, end(in1_av));
	ASSERT_TRUE(std::equal(std::cbegin(in0), std::cend(in0), cbegin(in1_av)));
}

TEST_F(stl_algorithms_tests, partition_with_empty_range)
{
	const auto p = [v = input[input.size() / 2]](auto&& x) restrict(cpu, amp) { return x < v; };
	const auto result_expect = std::partition(std::begin(input), std::begin(input), p);
	const auto result_amp = amp_stl_algorithms::partition(begin(input_av), begin(input_av), p);

	ASSERT_EQ(std::distance(std::begin(input), result_expect), std::distance(begin(input_av), result_amp));
}

TEST_F(stl_algorithms_tests, partition_single_element_range_less)
{
	const auto p = [v = std::numeric_limits<decltype(input)::value_type>::max()](auto&& x) restrict(cpu, amp) { return x < v; };
	const auto result_expect = std::partition(std::begin(input), std::begin(input) + 1, p);
	const auto result_amp = amp_stl_algorithms::partition(amp_stl_algorithms::begin(input_av), amp_stl_algorithms::begin(input_av) + 1, p);

	ASSERT_EQ(std::distance(std::begin(input), result_expect), std::distance(amp_stl_algorithms::begin(input_av), result_amp));
}

TEST_F(stl_algorithms_tests, partition_with_single_element_range_greater)
{
	const auto p = [v = std::numeric_limits<decltype(input)::value_type>::min()](auto&& x) restrict(cpu, amp) { return x < v; };
	const auto result_expect = std::partition(std::begin(input), std::begin(input) + 1, p);
	const auto result_amp = amp_stl_algorithms::partition(amp_stl_algorithms::begin(input_av), amp_stl_algorithms::begin(input_av) + 1, p);

	ASSERT_EQ(std::distance(std::begin(input), result_expect), std::distance(amp_stl_algorithms::begin(input_av), result_amp));
}

TEST_F(stl_algorithms_tests, partition_with_all_less_than)
{
	auto in0 = input;
	auto in1 = input;
	const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

	const auto p = [M = std::numeric_limits<decltype(in1)::value_type>::max()](auto&& x) restrict(cpu, amp) { return x < M; };
	const auto result_expect = std::partition(std::begin(in0), std::end(in0), p);
	const auto result_amp = amp_stl_algorithms::partition(begin(in1_av), end(in1_av), p);

	ASSERT_EQ(std::distance(std::begin(in0), result_expect), std::distance(begin(in1_av), result_amp));

	std::sort(std::begin(in0), result_expect);
	std::sort(result_expect, std::end(in0));
	std::sort(begin(in1_av), result_amp);
	std::sort(result_amp, end(in1_av));
	ASSERT_TRUE(std::equal(std::cbegin(in0), std::cend(in0), cbegin(in1_av)));
}

TEST_F(stl_algorithms_tests, partition_with_all_greater_than)
{
	auto in0 = input;
	auto in1 = input;
	const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

	const auto p = [m = std::numeric_limits<decltype(in1)::value_type>::min()](auto&& x) restrict(cpu, amp) { return x < m; };
	const auto result_expect = std::partition(std::begin(in0), std::end(in0), p);
	const auto result_amp = amp_stl_algorithms::partition(begin(in1_av), end(in1_av), p);

	ASSERT_EQ(std::distance(std::begin(in0), result_expect), std::distance(begin(in1_av), result_amp));

	std::sort(std::begin(in0), result_expect);
	std::sort(result_expect, std::end(in0));
	std::sort(begin(in1_av), result_amp);
	std::sort(result_amp, end(in1_av));
	ASSERT_TRUE(std::equal(std::cbegin(in0), std::cend(in0), cbegin(in1_av)));
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
	const auto result_amp = amp_stl_algorithms::partition_point(amp_stl_algorithms::cbegin(in1_av), amp_stl_algorithms::cend(in1_av), p);

	ASSERT_EQ(std::distance(std::cbegin(in0), result_expect), std::distance(amp_stl_algorithms::cbegin(in1_av), result_amp));
}

TEST_F(stl_algorithms_tests, partition_point_with_empty_range)
{
	const auto p = [v = input[input.size() / 2]](auto&& x) restrict(cpu, amp) { return x < v; };
	const auto result_expect = std::partition_point(std::cbegin(input), std::cbegin(input), p);
	const auto result_amp = amp_stl_algorithms::partition_point(amp_stl_algorithms::cbegin(input_av), amp_stl_algorithms::cbegin(input_av), p);

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect), std::distance(amp_stl_algorithms::cbegin(input_av), result_amp));
}

TEST_F(stl_algorithms_tests, partition_point_with_single_element_range_less)
{
	const auto p = [v = std::numeric_limits<decltype(input)::value_type>::max()](auto&& x) restrict(cpu, amp) { return x < v; };
	const auto result_expect = std::partition_point(std::cbegin(input), std::cbegin(input) + 1, p);
	const auto result_amp = amp_stl_algorithms::partition_point(amp_stl_algorithms::cbegin(input_av), amp_stl_algorithms::cbegin(input_av) + 1, p);

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect), std::distance(amp_stl_algorithms::cbegin(input_av), result_amp));
}

TEST_F(stl_algorithms_tests, partition_point_with_single_element_range_greater)
{
	const auto p = [v = std::numeric_limits<decltype(input)::value_type>::min()](auto&& x) restrict(cpu, amp) { return x < v; };
	const auto result_expect = std::partition_point(std::cbegin(input), std::cbegin(input) + 1, p);
	const auto result_amp = amp_stl_algorithms::partition_point(amp_stl_algorithms::cbegin(input_av), amp_stl_algorithms::cbegin(input_av) + 1, p);

	ASSERT_EQ(std::distance(std::cbegin(input), result_expect), std::distance(amp_stl_algorithms::cbegin(input_av), result_amp));
}

TEST_F(stl_algorithms_tests, partition_point_with_all_less_than)
{
	auto in0 = input;
	auto in1 = input;
	const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

	const auto p = [M = std::numeric_limits<decltype(in1)::value_type>::max()](auto&& x) restrict(cpu, amp) { return x < M; };
	const auto result_expect = std::partition_point(std::cbegin(in0), std::cend(in0), p);
	const auto result_amp = amp_stl_algorithms::partition_point(amp_stl_algorithms::cbegin(in1_av), amp_stl_algorithms::cend(in1_av), p);

	ASSERT_EQ(std::distance(std::cbegin(in0), result_expect), std::distance(amp_stl_algorithms::cbegin(in1_av), result_amp));
}

TEST_F(stl_algorithms_tests, partition_point_with_all_greater_than)
{
	auto in0 = input;
	auto in1 = input;
	const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

	const auto p = [m = std::numeric_limits<decltype(in1)::value_type>::min()](auto&& x) restrict(cpu, amp) { return x < m; };
	const auto result_expect = std::partition_point(std::cbegin(in0), std::cend(in0), p);
	const auto result_amp = amp_stl_algorithms::partition_point(amp_stl_algorithms::cbegin(in1_av), amp_stl_algorithms::cend(in1_av), p);

	ASSERT_EQ(std::distance(std::cbegin(in0), result_expect), std::distance(amp_stl_algorithms::cbegin(in1_av), result_amp));
}

//----------------------------------------------------------------------------
// partition_point
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, is_partitioned_true)
{
	auto in0 = input;
	for (decltype(in0.size()) i = 0; i != in0.size(); ++i) {
		const auto p = [v = input[i]](auto&& x) restrict(cpu, amp) { return x < v; };

		std::partition(std::begin(in0), std::end(in0), p);
		const concurrency::array_view<decltype(in0)::value_type> in0_av(in0);

		const auto result_amp = amp_stl_algorithms::is_partitioned(amp_stl_algorithms::cbegin(in0_av), amp_stl_algorithms::cend(in0_av), p);

		ASSERT_TRUE(result_amp);
	}
}

TEST_F(stl_algorithms_tests, is_partitioned_false)
{
	const auto p = [v = input[input.size() / 2]](auto&& x) restrict(cpu, amp) { return x < v; };

	auto in0 = input;
	auto p_point = std::partition(std::begin(in0), std::end(in0), p);
	std::rotate(std::begin(in0), p_point, std::end(in0));

	const concurrency::array_view<decltype(in0)::value_type> in0_av(in0);
	const auto result_amp = amp_stl_algorithms::is_partitioned(amp_stl_algorithms::cbegin(in0_av), amp_stl_algorithms::cend(in0_av), p);

	ASSERT_FALSE(result_amp);
}

TEST_F(stl_algorithms_tests, is_partitioned_with_empty_range)
{
	const auto p = [v = input[input.size() / 2]](auto&& x) restrict(cpu, amp) { return x < v; };
	const auto result_amp = amp_stl_algorithms::is_partitioned(amp_stl_algorithms::cbegin(input_av), amp_stl_algorithms::cbegin(input_av), p);

	ASSERT_TRUE(result_amp);
}

TEST_F(stl_algorithms_tests, is_partitioned_with_single_element_range)
{
	const auto p = [v = input[input.size() / 2]](auto&& x) restrict(cpu, amp) { return x < v; };

	auto in0 = input; std::partition(std::begin(in0), std::end(in0), p);
	const concurrency::array_view<decltype(in0)::value_type> in0_av(in0);

	const auto result_amp = amp_stl_algorithms::is_partitioned(amp_stl_algorithms::cbegin(in0_av), amp_stl_algorithms::cbegin(in0_av) + 1, p);

	ASSERT_TRUE(result_amp);
}

//----------------------------------------------------------------------------
// reduce
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, reduce_sum)
{
    int expect = std::accumulate(cbegin(input), cend(input), 0, std::plus<int>());

    int r = amp_stl_algorithms::reduce(cbegin(input_av), cend(input_av), 0, amp_algorithms::plus<>());

    ASSERT_EQ(expect, r);
}

TEST_F(stl_algorithms_tests, reduce_max)
{
    int expect = *std::max_element(cbegin(input), cend(input));

    int r = amp_stl_algorithms::reduce(begin(input_av), end(input_av), 0, amp_algorithms::max<>());

    ASSERT_EQ(expect, r);
}


//----------------------------------------------------------------------------
// reverse, reverse_copy
//----------------------------------------------------------------------------

class reverse_tests : public ::testing::TestWithParam<int> {};

TEST_P(reverse_tests, test)
{
    std::vector<int> expect(GetParam());
    std::iota(begin(expect), end(expect), 0);
    std::reverse(begin(expect), end(expect));
    std::vector<int> input(GetParam());
    std::iota(begin(input), end(input), 0);
    array_view<int> input_av(input);

    amp_stl_algorithms::reverse(begin(input_av), end(input_av));

    ASSERT_TRUE(are_equal(expect, input_av));
}

//----------------------------------------------------------------------------
// sort
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, sort)
{
	auto in0 = input;
	auto in1 = input;
	const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

	std::sort(std::begin(in0), std::end(in0));
	amp_stl_algorithms::sort(begin(in1_av), end(in1_av));

	ASSERT_TRUE(std::is_sorted(cbegin(in1_av), cend(in1_av)));
	ASSERT_TRUE(std::equal(std::cbegin(in0), std::cend(in0), cbegin(in1_av)));
}

TEST_F(stl_algorithms_tests, sort_empty_range)
{
	std::sort(std::begin(input), std::begin(input));
	amp_stl_algorithms::sort(begin(input_av), begin(input_av));

	ASSERT_EQ(std::is_sorted(std::cbegin(input), std::cbegin(input)), std::is_sorted(cbegin(input_av), cbegin(input_av)));
	ASSERT_TRUE(std::equal(std::cbegin(input), std::cbegin(input), cbegin(input_av)));
}

TEST_F(stl_algorithms_tests, sort_single_element_range)
{
	std::sort(std::begin(input), std::begin(input) + 1);
	amp_stl_algorithms::sort(begin(input_av), begin(input_av) + 1);

	ASSERT_TRUE(std::is_sorted(cbegin(input_av), cbegin(input_av) + 1));
	ASSERT_TRUE(std::equal(std::cbegin(input), std::cbegin(input) + 1, cbegin(input_av)));
}


TEST_F(stl_algorithms_tests, sort_sorted_range)
{
	auto in0 = input; std::sort(std::begin(in0), std::end(in0));
	auto in1 = input; std::sort(std::begin(in1), std::end(in1));
	const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

	std::sort(std::begin(in0), std::end(in0));
	amp_stl_algorithms::sort(begin(in1_av), end(in1_av));

	ASSERT_TRUE(std::is_sorted(cbegin(in1_av), cend(in1_av)));
	ASSERT_TRUE(std::equal(std::cbegin(in0), std::cend(in0), cbegin(in1_av)));
}

TEST_F(stl_algorithms_tests, sort_reverse_sorted_range)
{
	auto in0 = input; std::sort(std::begin(in0), std::end(in0), std::greater<>());
	auto in1 = input;
	const concurrency::array_view<decltype(in1)::value_type> in1_av(in1);

	std::sort(std::begin(in0), std::end(in0));
	amp_stl_algorithms::sort(begin(in1_av), end(in1_av));

	ASSERT_TRUE(std::is_sorted(cbegin(in1_av), cend(in1_av)));
	ASSERT_TRUE(std::equal(std::cbegin(in0), std::cend(in0), cbegin(in1_av)));
}

TEST_F(stl_algorithms_tests, sort_concave_range)
{
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

TEST_F(stl_algorithms_tests, sort_convex_range)
{
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