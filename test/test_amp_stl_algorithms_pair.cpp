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
// Accessors and copy
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

//----------------------------------------------------------------------------
// Conversion to/from std::pair<>
//----------------------------------------------------------------------------

TEST(stl_pair_tests, stl_pair_conversion_from_std_pair)
{
    std::pair<int, int> y(1, 2);

    amp_stl_algorithms::pair<int, int> x = y;

    ASSERT_EQ(1, x.first);
    ASSERT_EQ(2, x.second);
}

TEST(stl_pair_tests, stl_pair_conversion_from_std_pair_with_casting)
{
    std::pair<int, int> y(1, 2);

    amp_stl_algorithms::pair<float, long> x = y;

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

TEST(stl_pair_tests, stl_pair_conversion_to_std_pair_with_casting)
{
    amp_stl_algorithms::pair<int, int> y(1, 2);

    std::pair<float, long> x = y;

    ASSERT_EQ(1, x.first);
    ASSERT_EQ(2, x.second);
}

//----------------------------------------------------------------------------
// Comparison operator support
//----------------------------------------------------------------------------

typedef std::pair<int, int> std_pair;
typedef amp_stl_algorithms::pair<int, int> amp_pair;

typedef ::testing::Types <
    OperatorTestDefinition<std::equal_to<std_pair>,      amp_algorithms::equal_to<amp_pair>>,
    OperatorTestDefinition<std::not_equal_to<std_pair>,  amp_algorithms::not_equal_to<amp_pair>>,
    OperatorTestDefinition<std::less<std_pair>,          amp_algorithms::less<amp_pair>>,
    OperatorTestDefinition<std::less_equal<std_pair>,    amp_algorithms::less_equal<amp_pair>>,
    OperatorTestDefinition<std::greater<std_pair>,       amp_algorithms::greater<amp_pair>>,
    OperatorTestDefinition<std::greater_equal<std_pair>, amp_algorithms::greater_equal<amp_pair>>
> comparer_types;

std::array<std::pair<std_pair, amp_pair>, 4> pair_comparer_data = {
    std::make_pair(std::make_pair(1, -1), amp_stl_algorithms::make_pair( 2, -1)),
    std::make_pair(std::make_pair(1, -1), amp_stl_algorithms::make_pair( 1,  2)),
    std::make_pair(std::make_pair(3, -1), amp_stl_algorithms::make_pair(-1,  2)),
    std::make_pair(std::make_pair(2,  2), amp_stl_algorithms::make_pair( 2,  2))
};

template <typename T>
class comparer_tests : public ::testing::Test { };
TYPED_TEST_CASE_P(comparer_tests);

TYPED_TEST_P(comparer_tests, test)
{
    compare_binary_operator(TypeParam::stl_type(), TypeParam::amp_type(), cbegin(pair_comparer_data), cend(pair_comparer_data));
}

REGISTER_TYPED_TEST_CASE_P(comparer_tests, test);
INSTANTIATE_TYPED_TEST_CASE_P(stl_pair_tests, comparer_tests, comparer_types);
