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
