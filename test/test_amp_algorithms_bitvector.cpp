/*----------------------------------------------------------------------------
* Copyright © Microsoft Corp.
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

#include <amp_algorithms.h>
#include <gtest\gtest.h>

#include "testtools.h"

using namespace concurrency;
using namespace amp_algorithms;
using namespace testtools;

class amp_algorithms_tests : public stl_algorithms_testbase<13>, public ::testing::Test {};

template <typename T>
class custom_segment
{
private:
    int m_index;

public:
    custom_segment(int index) : m_index(index) { }

    bool operator()(const T &i) const
    {
        return (i == m_index);
    }
};

TEST_F(amp_algorithms_tests, bitvector_uniform_initialization)
{
    auto v = bitvector(1327);
    v.initialize(uniform_segments<int>(2));

    ASSERT_EQ(42, v.data.size());
    ASSERT_TRUE(std::all_of(cbegin(v.data), cend(v.data) - 1, [=](unsigned v) { return v == 0x55555555; }));
    ASSERT_EQ(0x5555, v.data[41]);
}

TEST_F(amp_algorithms_tests, bitvector_custom_initialization)
{
    auto v = bitvector(1327);
    v.initialize(custom_segment<int>(797));

    ASSERT_EQ(0x20000000, v.data[24]);
    v.data[24] = 0;
    ASSERT_TRUE(std::all_of(cbegin(v.data), cend(v.data), [=](unsigned v) { return v == 0; }));
}

TEST_F(amp_algorithms_tests, bit_vector_is_bit_set_forwards)
{
    auto v = bitvector(1327);
    v.initialize(custom_segment<int>(797));

    ASSERT_FALSE(v.is_bit_set(796));
    ASSERT_TRUE(v.is_bit_set(797));
    ASSERT_FALSE(v.is_bit_set(798));
}

TEST_F(amp_algorithms_tests, bit_vector_is_bit_set_backwards)
{
    auto v = bitvector(1327);
    v.initialize(custom_segment<int>(797));

    ASSERT_FALSE(v.is_bit_set(795, scan_direction::backward));
    ASSERT_TRUE(v.is_bit_set(796, scan_direction::backward));
    ASSERT_FALSE(v.is_bit_set(797, scan_direction::backward));
}