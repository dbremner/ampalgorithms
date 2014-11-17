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

int main(int argc, char **argv) 
{
    ::testing::InitGoogleTest(&argc, argv);
    return  RUN_ALL_TESTS();
}

template <typename T>
class greater_than
{
private:
    T m_value;
public:
    greater_than(const T& value = 0) : m_value(value) {}

    T operator()(const T &v) const restrict(cpu, amp)
    {
        return (v > m_value) ? 1 : 0;
    }
};

const std::array<int, 13> remove_if_data[] = {
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 1 },
    { 3, 1, 0, 2, 3, 0, 0, 4, 0, 1, 0, 6, 7 }
};

class stl_algorithms_tests : public stl_algorithms_testbase<13>, public ::testing::Test {};
// TODO: Get the tests, header and internal implementations into the same logical order.

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
// copy, copy_if, copy_n, copy_backward
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, copy)
{
    std::iota(begin(input), end(input), 1);

    auto iter = amp_stl_algorithms::copy(begin(input_av), end(input_av), begin(output_av));

    ASSERT_TRUE(are_equal(input, output_av));
}

class copy_if_tests : public stl_algorithms_testbase<13>, public ::testing::TestWithParam<std::array<int, 13>> {};

TEST_P(copy_if_tests, test)
{
    std::copy(cbegin(GetParam()), cend(GetParam()), begin(input));
    auto expected_iter = std::copy_if(begin(input), end(input), begin(expected), greater_than<int>());
    auto expected_size = std::distance(begin(expected), expected_iter);

    auto iter = amp_stl_algorithms::copy_if(begin(input_av), end(input_av), begin(output_av), greater_than<int>());

    ASSERT_EQ(expected_size, std::distance(begin(output_av), iter));
    ASSERT_TRUE(are_equal(expected, output_av, expected_size));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, copy_if_tests, ::testing::ValuesIn(remove_if_data));

TEST_F(stl_algorithms_tests, copy_n)
{
    int size = int(input.size() / 2);
    std::copy_n(cbegin(input), size, begin(expected));
    auto iter = amp_stl_algorithms::copy_n(begin(input_av), size, begin(output_av));

    ASSERT_EQ(size, std::distance(begin(output_av), iter));
    ASSERT_TRUE(are_equal(expected, output_av));
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
// fill, fill_n
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, fill)
{
    std::fill(begin(expected), end(expected), 7);

    amp_stl_algorithms::fill(begin(output_av), end(output_av), 7);

    ASSERT_TRUE(are_equal(expected, output_av));
}

TEST_F(stl_algorithms_tests, fill_n)
{
    int size = int(input.size() / 2);
    std::fill_n(begin(input), size, 3);
    amp_stl_algorithms::fill_n(begin(input_av), size, 3);

    //ASSERT_EQ(size, std::distance(begin(output_av), iter));
    ASSERT_TRUE(are_equal(expected, output_av));
}

//----------------------------------------------------------------------------
// find, find_if, find_if_not, find_end, find_first_of, adjacent_find
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, find_finds_no_values)
{
    auto iter = amp_stl_algorithms::find(begin(input_av), end(input_av), 17);
    ASSERT_EQ(end(input_av), iter);
}

TEST_F(stl_algorithms_tests, find_finds_value)
{
    auto iter = amp_stl_algorithms::find(begin(input_av), end(input_av), 3);
    ASSERT_EQ(1, std::distance(begin(input_av), iter));
}

TEST_F(stl_algorithms_tests, find_finds_first_value)
{
    auto iter = amp_stl_algorithms::find(begin(input_av), end(input_av), 2);
    ASSERT_EQ(4, std::distance(begin(input_av), iter));
}

TEST_F(stl_algorithms_tests, find_if_finds_no_values)
{
    auto iter = amp_stl_algorithms::find_if(begin(input_av), end(input_av), [=](int v) restrict(amp) { return v == 17; });
    ASSERT_EQ(end(input_av), iter);
}

TEST_F(stl_algorithms_tests, find_if_finds_value)
{
    auto iter = amp_stl_algorithms::find_if(begin(input_av), end(input_av), [=](int v) restrict(amp) { return v == 3; });
    ASSERT_EQ(1, std::distance(begin(input_av), iter));
}

TEST_F(stl_algorithms_tests, find_if_finds_first_value)
{
    auto iter = amp_stl_algorithms::find_if(begin(input_av), end(input_av), [=](int v) restrict(amp) { return v == 2; });
    ASSERT_EQ(4, std::distance(begin(input_av), iter));
}

TEST_F(stl_algorithms_tests, find_if_not_finds_no_values)
{
    auto iter = amp_stl_algorithms::find_if_not(begin(input_av), end(input_av), [=](int v) restrict(amp) { return v != 17; });
    ASSERT_EQ(end(input_av), iter);
}

TEST_F(stl_algorithms_tests, find_if_not_finds_value)
{
    auto iter = amp_stl_algorithms::find_if_not(begin(input_av), end(input_av), [=](int v) restrict(amp) { return v != 3; });
    ASSERT_EQ(1, std::distance(begin(input_av), iter));
}

TEST_F(stl_algorithms_tests, find_if_not_finds_first_value)
{
    auto iter = amp_stl_algorithms::find_if_not(begin(input_av), end(input_av), [=](int v) restrict(amp) { return v != 2; });
    ASSERT_EQ(4, std::distance(begin(input_av), iter));
}

const std::array<int, 10> adjacent_find_data[] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },                   // No adjacent values.
    { 0, 1, 2, 3, 4, 4, 6, 7, 8, 9 },                   // Single adjacent value.
    { 0, 0, 2, 3, 4, 4, 6, 7, 8, 9 }                    // Finds first adjacent pair.
};

class adjacent_find_tests : public ::testing::TestWithParam < std::array<int, 10> > {};

TEST_P(adjacent_find_tests, test)
{
    std::vector<int> input(cbegin(GetParam()), cend(GetParam()));
    array_view<int> input_av(10, input);

    auto expected = std::distance(cbegin(input), std::adjacent_find(cbegin(input), cend(input)));
    auto r = std::distance(begin(input_av), amp_stl_algorithms::adjacent_find(begin(input_av), end(input_av)));

    ASSERT_EQ(expected, r);
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, adjacent_find_tests, ::testing::ValuesIn(adjacent_find_data));

//----------------------------------------------------------------------------
// for_each, for_each_no_return
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, for_each_no_return)
{
    std::fill(input.begin(), input.end(), 2);
    int sum = 0;
    array_view<int> av_sum(1, &sum);
    amp_stl_algorithms::for_each_no_return(begin(input_av), end(input_av), [av_sum] (int val) restrict(amp) {
        concurrency::atomic_fetch_add(&av_sum(0), val);
    });
    av_sum.synchronize();
    ASSERT_EQ(std::accumulate(cbegin(input), cend(input), 0, std::plus<int>()), sum);
}

//----------------------------------------------------------------------------
// generate, generate_n
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, generate)
{
    std::generate(begin(expected), end(expected), [=]() { return 7; });

    amp_stl_algorithms::generate(begin(output_av), end(output_av), []() restrict(amp) { return 7; });

    ASSERT_TRUE(are_equal(expected, output_av));
}

TEST_F(stl_algorithms_tests, generate_n)
{
    std::fill(begin(expected), end(expected), -1);
    std::generate_n(begin(expected), expected.size() / 2, [=]() { return 7; });

    amp_stl_algorithms::generate_n(begin(output_av), output_av.extent.size() / 2, []() restrict(amp) { return 7; });

    ASSERT_TRUE(are_equal(expected, output_av));
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
// iota
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, iota)
{
    std::iota(begin(expected), end(expected), 2);

    amp_stl_algorithms::iota(begin(input_av), end(input_av), 2);

    ASSERT_TRUE(are_equal(expected, input_av));
}

//----------------------------------------------------------------------------
// minmax, max_element, min_element, minmax_element
//----------------------------------------------------------------------------

// TODO: Should be able to make these tests a bit tidier with better casting support for pair<T, T>
TEST_F(stl_algorithms_tests, stl_minmax)
{
    compare_binary_operators(
        [=](int a, int b)->std::pair<const int, const int> { return std::minmax(a, b); },
        [=](int a, int b)->std::pair<const int, const int> 
    { 
        return amp_stl_algorithms::minmax(a, b); 
    });
}

TEST_F(stl_algorithms_tests, stl_minmax_pred)
{
    //std::pair<const int&, const int&>(*minmax) (const int&, const int&) = std::minmax<int>;

    compare_operators(
        [=](int& a, int& b)->std::pair<const int, const int> { return std::minmax(a, b, std::greater_equal<int>()); },
        [=](int& a, int& b)->std::pair<const int, const int>
    { 
        return amp_stl_algorithms::minmax(a, b, amp_algorithms::greater_equal<int>()); 
    });
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
// remove, remove_if, remove_copy, remove_copy_if
//----------------------------------------------------------------------------

class remove_tests : public stl_algorithms_testbase<13>, public ::testing::TestWithParam < std::array<int, 13> > {};

TEST_P(remove_tests, test)
{
    std::copy(begin(GetParam()), end(GetParam()), begin(expected));
    auto expected_iter = std::remove(begin(expected), end(expected), 1);
    auto expected_size = std::distance(begin(expected), expected_iter);
    std::copy(cbegin(GetParam()), cend(GetParam()), begin(input));

    auto iter = amp_stl_algorithms::remove(begin(input_av), end(input_av), 1);

    ASSERT_EQ(expected_size, std::distance(begin(input_av), iter));
    ASSERT_TRUE(are_equal(expected, input_av, expected_size));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, remove_tests, ::testing::ValuesIn(remove_if_data));

class remove_if_tests : public stl_algorithms_testbase<13>, public ::testing::TestWithParam<std::array<int, 13>> {};

TEST_P(remove_if_tests, test)
{
    std::copy(begin(GetParam()), end(GetParam()), begin(expected));
    auto expected_iter = std::remove_if(begin(expected), end(expected), greater_than<int>());
    auto expected_size = std::distance(begin(expected), expected_iter);
    std::copy(cbegin(GetParam()), cend(GetParam()), begin(input));

    auto iter = amp_stl_algorithms::remove_if(begin(input_av), end(input_av), greater_than<int>());

    ASSERT_EQ(std::distance(begin(input_av), iter), std::distance(begin(expected), expected_iter));
    ASSERT_TRUE(are_equal(expected, input_av, expected_size));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, remove_if_tests, ::testing::ValuesIn(remove_if_data));

class remove_copy_tests : public stl_algorithms_testbase<13>, public ::testing::TestWithParam <std::array<int, 13>> {};

TEST_P(remove_copy_tests, test)
{
    std::array<int, size> expected_output;
    std::fill(begin(expected_output), end(expected_output), -1);
    std::copy(begin(GetParam()), end(GetParam()), begin(expected));
    auto expected_iter = std::remove_copy(begin(expected), end(expected), begin(expected_output), 1);
    std::copy(cbegin(GetParam()), cend(GetParam()), begin(input));
    auto expected_size = std::distance(begin(expected_output), expected_iter);

    auto iter = amp_stl_algorithms::remove_copy(begin(input_av), end(input_av), begin(output_av), 1);

    ASSERT_EQ(expected_size, std::distance(begin(input_av), iter));
    ASSERT_TRUE(are_equal(expected, input_av, size - expected_size));
    ASSERT_TRUE(are_equal(expected_output, output_av, expected_size));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, remove_copy_tests, ::testing::ValuesIn(remove_if_data));

class remove_copy_if_tests : public stl_algorithms_testbase<13>, public ::testing::TestWithParam < std::array<int, 13> > {};

TEST_P(remove_copy_if_tests, test)
{
    std::array<int, size> expected_output;
    std::fill(begin(expected_output), end(expected_output), -1);
    std::copy(begin(GetParam()), end(GetParam()), begin(expected));
    auto expected_iter = std::remove_copy_if(begin(expected), end(expected), begin(expected_output), greater_than<int>());
    std::copy(cbegin(GetParam()), cend(GetParam()), begin(input));
    auto expected_size = std::distance(begin(expected_output), expected_iter);

    auto iter = amp_stl_algorithms::remove_copy_if(begin(input_av), end(input_av), begin(output_av), greater_than<int>());

    ASSERT_EQ(expected_size, std::distance(begin(input_av), iter));
    ASSERT_TRUE(are_equal(expected, input_av, size - expected_size));
    ASSERT_TRUE(are_equal(expected_output, output_av, expected_size));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, remove_copy_if_tests, ::testing::ValuesIn(remove_if_data));

//----------------------------------------------------------------------------
// replace, replace_if, replace_copy, replace_copy_if
//----------------------------------------------------------------------------

class replace_tests : public stl_algorithms_testbase<13>, public ::testing::TestWithParam < std::array<int, 13> > {};

TEST_P(replace_tests, test)
{
    std::copy(begin(GetParam()), end(GetParam()), begin(expected));
    std::replace(begin(expected), end(expected), 1, -1);
    std::copy(cbegin(GetParam()), cend(GetParam()), begin(input));

    amp_stl_algorithms::replace(begin(input_av), end(input_av), 1, -1);

    ASSERT_TRUE(are_equal(expected, input_av));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, replace_tests, ::testing::ValuesIn(remove_if_data));

class replace_if_tests : public stl_algorithms_testbase<13>, public ::testing::TestWithParam < std::array<int, 13> > {};

TEST_P(replace_if_tests, test)
{
    std::copy(begin(GetParam()), end(GetParam()), begin(expected));
    std::replace_if(begin(expected), end(expected), greater_than<int>(), -1);
    std::copy(cbegin(GetParam()), cend(GetParam()), begin(input));

    amp_stl_algorithms::replace_if(begin(input_av), end(input_av), greater_than<int>(), -1);

    ASSERT_TRUE(are_equal(expected, input_av));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, replace_if_tests, ::testing::ValuesIn(remove_if_data));

TEST_F(stl_algorithms_tests, replace_copy)
{
    auto expected_r = std::replace_copy(begin(input), end(input), begin(expected), 2, -2);

    auto r = amp_stl_algorithms::replace_copy(begin(input_av), end(input_av), begin(output_av), 2, -2);

    ASSERT_TRUE(are_equal(expected, output_av));
    ASSERT_EQ(std::distance(begin(expected), expected_r), std::distance(begin(output_av), r));
}

class replace_copy_tests : public stl_algorithms_testbase<13>, public ::testing::TestWithParam < std::array<int, 13> > {};

TEST_P(replace_copy_tests, test)
{
    std::array<int, 13> expected_output;
    std::fill(begin(expected_output), end(expected_output), -1);
    std::copy(begin(GetParam()), end(GetParam()), begin(expected));
    auto expected_iter = std::replace_copy(begin(expected), end(expected), begin(expected_output), 1, -1);
    auto expected_size = std::distance(begin(expected_output), expected_iter);
    std::copy(cbegin(GetParam()), cend(GetParam()), begin(input));

    auto iter = amp_stl_algorithms::replace_copy(begin(input_av), end(input_av), begin(output_av), 1, -1);

    //ASSERT_EQ(expected_size, std::distance(begin(output_av), iter));
    ASSERT_TRUE(are_equal(expected, input_av));
    ASSERT_TRUE(are_equal(expected_output, output_av, expected_size - 1));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, replace_copy_tests, ::testing::ValuesIn(remove_if_data));

class replace_copy_if_tests : public stl_algorithms_testbase<13>, public ::testing::TestWithParam < std::array<int, 13> > {};

TEST_P(replace_copy_if_tests, test)
{
    std::array<int, 13> expected_output;
    std::fill(begin(expected_output), end(expected_output), -1);
    std::copy(begin(GetParam()), end(GetParam()), begin(expected));
    auto expected_iter = std::replace_copy(begin(expected), end(expected), begin(expected_output), 1, -1);
    auto expected_size = std::distance(begin(expected_output), expected_iter);
    std::copy(cbegin(GetParam()), cend(GetParam()), begin(input));

    auto iter = amp_stl_algorithms::replace_copy(begin(input_av), end(input_av), begin(output_av), 1, -1);

    //ASSERT_EQ(expected_size, std::distance(begin(output_av), iter));
    ASSERT_TRUE(are_equal(expected, input_av));
    ASSERT_TRUE(are_equal(expected_output, output_av, expected_size - 1));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, replace_copy_if_tests, ::testing::ValuesIn(remove_if_data));

//----------------------------------------------------------------------------
// reverse, reverse_copy
//----------------------------------------------------------------------------

class stl_reverse_tests : public ::testing::TestWithParam<int> {};

TEST_P(stl_reverse_tests, test)
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

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, stl_reverse_tests, ::testing::Values(1, 1023, 1024));

//----------------------------------------------------------------------------
// rotate_copy
//----------------------------------------------------------------------------

class rotate_copy_tests : public ::testing::TestWithParam<std::pair<int, int>> {};

TEST_P(rotate_copy_tests, test)
{
    int size = GetParam().first;
    int middle_offset = GetParam().second;
    std::vector<int> vec(size);
    std::iota(begin(vec), end(vec), 0);
    array_view<int> av(int(vec.size()), vec);
    
    std::vector<int> result(size, 0);
    concurrency::array_view<int> result_av(size, result);
    std::vector<int> expected_result(size, 0);
    auto expected_end = std::rotate_copy(begin(vec), begin(vec) + middle_offset, end(vec), begin(expected_result));
    
    auto result_end = amp_stl_algorithms::rotate_copy(begin(av), begin(av) + middle_offset, end(av), begin(result_av));
    
    //ASSERT_TRUE(are_equal(expected_result, result_av));
    ASSERT_EQ((size_t)std::distance(begin(expected_result), expected_end), (size_t)std::distance(begin(av), result_end));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, rotate_copy_tests, ::testing::Values(
    std::make_pair<int, int>(1, 0),
    std::make_pair<int, int>(1023, 200),
    std::make_pair<int, int>(1024, 713))
);

//----------------------------------------------------------------------------
// sort, partial_sort, partial_sort_copy, stable_sort, is_sorted, is_sorted_until
//----------------------------------------------------------------------------

const std::array<int, 10> is_sorted_sorted_data[] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },
    { 0, 1, 2, 3, 4, 4, 6, 7, 8, 9 }
};
const std::array<int, 10> is_sorted_unsorted_data[] = {
    { 0, 1, 2, 3, 4, -4, 6, 7, 8, 9 },
    { 0, -1, 2, 3, 4, 4, 6, 7, 8, 9 }
};

class is_sorted_sorted_tests : public ::testing::TestWithParam < std::array<int, 10> > {};

TEST_P(is_sorted_sorted_tests, test)
{
    std::vector<int> input(10);
    std::copy(cbegin(GetParam()), cend(GetParam()), begin(input));
    array_view<int> input_av(10, input);

    auto expected = std::distance(cbegin(input), std::is_sorted_until(cbegin(input), cend(input)));
    auto r = std::distance(begin(input_av), amp_stl_algorithms::is_sorted_until(begin(input_av), end(input_av)));
    ASSERT_EQ(expected, r);
    ASSERT_TRUE(amp_stl_algorithms::is_sorted(begin(input_av), end(input_av), amp_algorithms::less_equal<int>()));
    ASSERT_TRUE(amp_stl_algorithms::is_sorted(begin(input_av), end(input_av)));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, is_sorted_sorted_tests, ::testing::ValuesIn(is_sorted_sorted_data));

class is_sorted_unsorted_tests : public ::testing::TestWithParam < std::array<int, 10> > {};

TEST_P(is_sorted_unsorted_tests, test)
{
    std::vector<int> input(10);
    std::copy(cbegin(GetParam()), cend(GetParam()), begin(input));
    array_view<int> input_av(10, input);

    ASSERT_FALSE(amp_stl_algorithms::is_sorted(begin(input_av), end(input_av), amp_algorithms::less_equal<int>()));
    ASSERT_FALSE(amp_stl_algorithms::is_sorted(begin(input_av), end(input_av)));
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, is_sorted_unsorted_tests, ::testing::ValuesIn(is_sorted_unsorted_data));

class is_sorted_until_sorted_tests : public ::testing::TestWithParam < std::array<int, 10> > {};

TEST_P(is_sorted_until_sorted_tests, test)
{
    std::vector<int> input(10);
    std::copy(cbegin(GetParam()), cend(GetParam()), begin(input));
    array_view<int> input_av(10, input);

    auto expected = std::distance(cbegin(input), std::is_sorted_until(cbegin(input), cend(input)));
    auto r = std::distance(begin(input_av), amp_stl_algorithms::is_sorted_until(begin(input_av), end(input_av)));
    ASSERT_EQ(expected, r);
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, is_sorted_until_sorted_tests, ::testing::ValuesIn(is_sorted_sorted_data));

class is_sorted_until_unsorted_tests : public ::testing::TestWithParam < std::array<int, 10> > {};

TEST_P(is_sorted_until_unsorted_tests, test)
{
    std::vector<int> input(10);
    std::copy(cbegin(GetParam()), cend(GetParam()), begin(input));
    array_view<int> input_av(10, input);

    auto expected = std::distance(cbegin(input), std::is_sorted_until(cbegin(input), cend(input)));
    auto r = std::distance(begin(input_av), amp_stl_algorithms::is_sorted_until(begin(input_av), end(input_av)));
    ASSERT_EQ(expected, r);
}

INSTANTIATE_TEST_CASE_P(stl_algorithms_tests, is_sorted_until_unsorted_tests, ::testing::ValuesIn(is_sorted_sorted_data));

//----------------------------------------------------------------------------
// swap, swap<T, N>, swap_ranges, iter_swap
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, swap_cpu)
{
    int a = 1;
    int b = 2;
    
    amp_stl_algorithms::swap(a, b);
    
    ASSERT_EQ(2, a);
    ASSERT_EQ(1, b);
}

TEST_F(stl_algorithms_tests, swap_amp)
{
    std::vector<int> vec(2);
    std::iota(begin(vec), end(vec), 1);
    array_view<int> av(2, vec);
    
    parallel_for_each(concurrency::extent<1>(1), [=](concurrency::index<1> idx) restrict(amp)
    {
        amp_stl_algorithms::swap(av[idx], av[idx + 1]);
    });
    
    ASSERT_EQ(2, av[0]);
    ASSERT_EQ(1, av[1]);
}

TEST_F(stl_algorithms_tests, swap_n_cpu)
{
    int arr1[size];
    int arr2[size];
    std::iota(arr1, arr1 + size, 0);
    std::iota(arr2, arr2 + size, -9);

    amp_stl_algorithms::swap<int, size>(arr1, arr2);

    for (int i = 0; i < size; ++i)
    {
        EXPECT_EQ(i, arr2[i]);
        EXPECT_EQ((-9 + i), arr1[i]);
    }
}

TEST_F(stl_algorithms_tests, swap_n_amp)
{
    std::array<int, 10> expected = { 6, 7, 8, 9, 10, 1, 2, 3, 4, 5 };

    std::vector<int> vec(10);
    std::iota(begin(vec), end(vec), 1);
    array_view<int> av(10, vec);

    parallel_for_each(concurrency::tiled_extent<5>(concurrency::extent<1>(5)),
        [=](concurrency::tiled_index<5> tidx) restrict(amp)
    {
        tile_static int arr1[5];
        tile_static int arr2[5];

        int idx = tidx.global[0];
        int i = tidx.local[0];

        arr1[i] = av[i];
        arr2[i] = av[i + 5];

        tidx.barrier.wait();

        if (i == 0)
        {
            amp_stl_algorithms::swap<int, 5>(arr1, arr2);
        }

        tidx.barrier.wait();

        av[i] = arr1[i];
        av[i + 5] = arr2[i];

        tidx.barrier.wait();
    });
    ASSERT_TRUE(are_equal(expected, av));
}

TEST_F(stl_algorithms_tests, swap_ranges)
{
    auto block_size = size / 6;
    std::copy(cbegin(input), cend(input), begin(expected));
    std::swap_ranges(begin(expected) + block_size, begin(expected) + block_size * 2, begin(expected) + block_size * 4);

    auto expected_end = amp_stl_algorithms::swap_ranges(begin(input_av) + block_size, begin(input_av) + block_size * 2, begin(input_av) + block_size * 4);

    ASSERT_TRUE(are_equal(expected, input_av));
}

TEST_F(stl_algorithms_tests, swap_iter)
{
    std::vector<int> vec(2);
    std::iota(begin(vec), end(vec), 1);
    array_view<int> av(2, vec);

    parallel_for_each(concurrency::extent<1>(1), [=](concurrency::index<1> idx) restrict(amp)
    {
        amp_stl_algorithms::iter_swap(begin(av), begin(av) + 1);
    });

    ASSERT_EQ(2, av[0]);
    ASSERT_EQ(1, av[1]);
}

//----------------------------------------------------------------------------
// transform
//----------------------------------------------------------------------------

TEST_F(stl_algorithms_tests, unary_transform)
{
    std::iota(begin(input), end(input), 7);
    std::transform(cbegin(input), cend(input), begin(expected), [](int x) { return x * 2; });

    amp_stl_algorithms::transform(begin(input_av), end(input_av), begin(output_av), [] (int x) restrict(amp) 
    {
        return 2 * x;
    });

    ASSERT_TRUE(are_equal(expected, output_av));
}

TEST_F(stl_algorithms_tests, binary_transform)
{
    std::iota(begin(input), end(input), 99);
    std::vector<int> input2(size);
    std::iota(begin(input2), end(input2), 0);
    array_view<const int> input2_av(size, input2);
    std::transform(cbegin(input), cend(input), cbegin(input2), begin(expected), std::plus<int>());

    amp_stl_algorithms::transform(begin(input_av), end(input_av), begin(input2_av), begin(output_av), [] (int x1, int x2) restrict(amp) 
    {
        return x1 + x2;
    });

    ASSERT_TRUE(are_equal(expected, output_av));
}
