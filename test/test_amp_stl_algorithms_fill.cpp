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
    int sz = static_cast<int>(input.size() / 2);
    std::fill_n(begin(input), sz, 3);
    auto iter = amp_stl_algorithms::fill_n(begin(input_av), sz, 3);

    ASSERT_EQ(expected.size() / 2, std::distance(begin(output_av), iter));
    ASSERT_TRUE(are_equal(expected, output_av));
}

TEST_F(stl_algorithms_tests, fill_n_for_zero_elements)
{
    int sz = static_cast<int>(input.size() / 2);
    std::fill_n(begin(input), sz, 3);
    auto iter = amp_stl_algorithms::fill_n(begin(input_av), 0, 3);

    ASSERT_EQ(0, std::distance(begin(output_av), iter));
    ASSERT_TRUE(are_equal(expected, output_av));
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

    auto iter = amp_stl_algorithms::generate_n(begin(output_av), output_av.extent.size() / 2, []() restrict(amp) { return 7; });

    ASSERT_EQ(expected.size() / 2, std::distance(begin(output_av), iter));
    ASSERT_TRUE(are_equal(expected, output_av));
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
