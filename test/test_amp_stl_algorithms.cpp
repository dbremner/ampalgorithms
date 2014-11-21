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

    concurrency::parallel_for_each(input_av.extent, [=](concurrency::index<1> idx) restrict(amp)
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

const std::array<int, 10> adjacent_difference_data[] = { 
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },
    { 0, 1, 2, 3, 5, 5, 6, 7, 8, 9 },
    { 1, 1, 2, 3, 5, 5, 6, 7, 8, 9 },
    { 1, 1, 2, 3, 5, 5, 6, 3, 8, 9 }
};

class adjacent_difference_tests : public ::testing::TestWithParam <std::array<int, 10>> {};

TEST_P(adjacent_difference_tests, test)
{
    std::vector<int> input(begin(GetParam()), end(GetParam()));
    array_view<int> input_av(10, input);
    std::vector<int> output(10, 0);
    array_view<int> output_av(10, output);
    std::vector<int> expected(10, -1); 
        
    std::adjacent_difference(cbegin(input), cend(input), begin(expected));
    
    auto result_last = amp_stl_algorithms::adjacent_difference(begin(input_av), end(input_av), begin(output_av));
    
    ASSERT_EQ(10, std::distance(begin(output_av), result_last));
    ASSERT_TRUE(are_equal(expected, output_av));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, adjacent_difference_tests, ::testing::ValuesIn(adjacent_difference_data));

TEST_F(stl_algorithms_tests, adjacent_difference_with_empty_array)
{
    auto result_last = amp_stl_algorithms::adjacent_difference(begin(input_av), begin(input_av), begin(output_av));
    ASSERT_EQ(0, std::distance(begin(output_av), result_last));

}

TEST_F(stl_algorithms_tests, adjacent_difference_with_single_element_array)
{
    auto result_last = amp_stl_algorithms::adjacent_difference(begin(input_av), begin(input_av) + 1, begin(output_av));

    ASSERT_EQ(0, std::distance(begin(output_av), result_last));
    ASSERT_EQ(-1, output_av[0]);
}

TEST_F(stl_algorithms_tests, adjacent_difference_multi_tile)
{
    const int size = 1024;
    std::vector<int> vec(size);
    generate_data(vec);
    array_view<int> av(size, vec);
    std::vector<int> result(size, 0);
    array_view<int> result_av(size, result);
    std::array<int, size> expected;
    std::fill(begin(expected), end(expected), -1);

    std::adjacent_difference(begin(vec), end(vec), begin(expected));

    auto result_last = amp_stl_algorithms::adjacent_difference(begin(av), end(av), begin(result_av));

    ASSERT_EQ(size, std::distance(begin(result_av), result_last));
    ASSERT_TRUE(are_equal(expected, result_av));
}

//----------------------------------------------------------------------------
// all_of, any_of, none_of
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, none_of_finds_no_values)
{
    bool r = amp_stl_algorithms::none_of(begin(input_av), end(input_av), [](int v) restrict(amp) -> bool { return v > 10; });
    ASSERT_TRUE(r);
}

TEST_F(stl_algorithms_tests, none_of_finds_value)
{
    bool r = amp_stl_algorithms::none_of(begin(input_av), end(input_av), [](int v) restrict(amp) -> bool { return v > 5; });
    ASSERT_FALSE(r);
}

TEST_F(stl_algorithms_tests, any_of_finds_no_values)
{
    bool r = amp_stl_algorithms::any_of(begin(input_av), end(input_av), [] (int v) restrict(amp) -> bool { return v > 10; });
    ASSERT_FALSE(r);
}

TEST_F(stl_algorithms_tests, any_of_finds_value)
{
    bool r = amp_stl_algorithms::any_of(begin(input_av), end(input_av), [](int v) restrict(amp) -> bool { return v > 5; });
    ASSERT_TRUE(r);
}

TEST_F(stl_algorithms_tests, all_of_finds_all_values)
{
    bool r = amp_stl_algorithms::all_of(begin(input_av), end(input_av), [](int v) restrict(amp) -> bool { return v <= 10; });
    ASSERT_TRUE(r);
}

TEST_F(stl_algorithms_tests, all_of_finds_some_values)
{
    bool r = amp_stl_algorithms::all_of(begin(input_av), end(input_av), [](int v) restrict(amp) -> bool { return v > 5; });
    ASSERT_FALSE(r);
}

TEST_F(stl_algorithms_tests, all_of_finds_no_values)
{
    bool r = amp_stl_algorithms::all_of(begin(input_av), end(input_av), [](int v) restrict(amp) -> bool { return v > 10; });
    ASSERT_FALSE(r);
}

//----------------------------------------------------------------------------
// count, count_if
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, count_counts_values)
{
    auto r = amp_stl_algorithms::count(begin(input_av), end(input_av), 2);
    ASSERT_EQ(5, r);
}

TEST_F(stl_algorithms_tests, count_counts_no_values)
{
    auto r = amp_stl_algorithms::count(begin(input_av), end(input_av), 22);
    ASSERT_EQ(0, r);
}

TEST_F(stl_algorithms_tests, count_if_counts_values)
{
    auto r = amp_stl_algorithms::count_if(begin(input_av), end(input_av), [=](const int& v) restrict(amp) { return (v == 2); });
    ASSERT_EQ(5, r);
}

TEST_F(stl_algorithms_tests, count_if_counts_no_values)
{
    auto r = amp_stl_algorithms::count_if(begin(input_av), end(input_av), [=](const int& v) restrict(amp) { return (v == 22); });
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
        equal_av(concurrency::extent<1>(int(input.size())), equal),
        unequal_av(concurrency::extent<1>(int(output.size())), unequal)
    {
        std::copy(cbegin(input), cend(input), begin(equal));
        std::copy(cbegin(input), cend(input), begin(unequal));
        unequal[9] = -1;
    }
};

TEST_F(stl_algorithms_tests_2, equal_true_for_equal_arrays)
{
    bool r = amp_stl_algorithms::equal(begin(input_av), end(input_av), begin(equal_av));
    ASSERT_TRUE(r);
}

TEST_F(stl_algorithms_tests_2, equal_false_for_unequal_arrays)
{
    bool r = amp_stl_algorithms::equal(begin(input_av), end(input_av), begin(unequal_av));
    ASSERT_FALSE(r);
}

TEST_F(stl_algorithms_tests_2, equal_compares_against_first_array)
{
    auto av = input_av.section(0, 9);
    bool r = amp_stl_algorithms::equal(begin(av), end(av), begin(unequal_av));
    ASSERT_TRUE(r);
}

TEST_F(stl_algorithms_tests_2, equal_pred_true_for_equal_arrays)
{
    std::iota(begin(input), end(input), 1);
    std::iota(begin(equal), end(equal), 2);
    auto pred = [=](int& v1, int& v2) restrict(amp) { return ((v1 + 1) == v2); };

    bool r = amp_stl_algorithms::equal(begin(input_av), end(input_av), begin(equal_av), pred);
    
    ASSERT_TRUE(r);
}

TEST_F(stl_algorithms_tests_2, equal_pred_false_for_unequal_arrays)
{
    std::iota(begin(input), end(input), 1);
    std::iota(begin(unequal), end(unequal), 2);
    unequal[3] = 99;
    auto pred = [=](int& v1, int& v2) restrict(amp) { return ((v1 + 1) == v2); };

    bool r = amp_stl_algorithms::equal(begin(input_av), end(input_av), begin(equal_av), pred);
    
    ASSERT_FALSE(r);
}

//----------------------------------------------------------------------------
// for_each, for_each_no_return
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, for_each_no_return)
{
    std::fill(begin(input), end(input), 2);
    int sum = 0;
    array_view<int> av_sum(1, &sum);
    amp_stl_algorithms::for_each_no_return(begin(input_av), end(input_av), [av_sum] (int val) restrict(amp) {
        concurrency::atomic_fetch_add(&av_sum(0), val);
    });
    av_sum.synchronize();
    ASSERT_EQ(std::accumulate(cbegin(input), cend(input), 0, std::plus<int>()), sum);
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
    int expected = std::inner_product(begin(vec1), end(vec1), begin(vec2), 2);

    int result = amp_stl_algorithms::inner_product(begin(av1), end(av1), begin(av2), 2);

    ASSERT_EQ(expected, result);
}

TEST_F(stl_algorithms_tests, inner_product_pred)
{
    std::vector<int> vec1(1024);
    std::fill(begin(vec1), end(vec1), 1);
    array_view<int> av1(1024, vec1);
    std::vector<int> vec2(1024);
    std::fill(begin(vec2), end(vec2), 2);
    array_view<int> av2(1024, vec2);
    int expected = std::inner_product(begin(vec1), end(vec1), begin(vec2), 2, std::plus<int>(), std::plus<int>());

    int result = amp_stl_algorithms::inner_product(begin(av1), end(av1), begin(av2), 2, amp_algorithms::plus<int>(), amp_algorithms::plus<int>());

    ASSERT_EQ(expected, result);
}

//----------------------------------------------------------------------------
// minmax, max_element, min_element, minmax_element
//----------------------------------------------------------------------------

std::array<std::pair<int, int>, 6> minmax_data = {
    std::pair<int, int>(1, 2),
    std::pair<int, int>(100, 100),
    std::pair<int, int>(150, 300),
    std::pair<int, int>(1000, -50),
    std::pair<int, int>(11, 12),
    std::pair<int, int>(-12, 33)
};

// TODO: Should be able to make these tests a bit tidier with better casting support for pair<T, T>
TEST_F(stl_algorithms_tests, minmax)
{
    compare_binary_operator(
        [=](int a, int b)->std::pair<const int, const int> { return std::minmax(a, b); },
        [=](int a, int b)->std::pair<const int, const int> 
        { 
            return amp_stl_algorithms::minmax(a, b); 
        },
        minmax_data);
}

TEST_F(stl_algorithms_tests, minmax_pred)
{
    compare_binary_operator(
        [=](int& a, int& b)->std::pair<const int, const int> { return std::minmax(a, b, std::greater_equal<int>()); },
        [=](int& a, int& b)->std::pair<const int, const int>
        { 
            return amp_stl_algorithms::minmax(a, b, amp_algorithms::greater_equal<int>()); 
        },
        minmax_data);
}

//----------------------------------------------------------------------------
// reduce
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, reduce_sum)
{
    int expected = std::accumulate(begin(input), end(input), 0, std::plus<int>());

    int r = amp_stl_algorithms::reduce(begin(input_av), end(input_av), 0);

    ASSERT_EQ(expected, r);
}

TEST_F(stl_algorithms_tests, reduce_max)
{
    int expected = *std::max_element(begin(input), end(input));

    int r = amp_stl_algorithms::reduce(begin(input_av), end(input_av), 0, [](int a, int b) restrict(cpu, amp) {
        return (a < b) ? b : a;
    });

    ASSERT_EQ(expected, r);
}

//----------------------------------------------------------------------------
// reverse, reverse_copy
//----------------------------------------------------------------------------

class reverse_tests : public ::testing::TestWithParam<int> {};

TEST_P(reverse_tests, test)
{
    std::vector<int> expected(GetParam());
    std::iota(begin(expected), end(expected), 0);
    std::reverse(begin(expected), end(expected));
    std::vector<int> input(GetParam());
    std::iota(begin(input), end(input), 0);
    array_view<int> input_av(int(input.size()), input);

    amp_stl_algorithms::reverse(begin(input_av), end(input_av));

    ASSERT_TRUE(are_equal(expected, input_av));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, reverse_tests, ::testing::Values(1, 1023, 1024));
