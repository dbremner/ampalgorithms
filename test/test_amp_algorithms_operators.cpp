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

#include "stdafx.h"

#include <amp_algorithms.h>
#include <gtest\gtest.h>

#include "testtools.h"

using namespace concurrency;
using namespace amp_algorithms;
using namespace testtools;

class amp_operator_tests : public testbase, public ::testing::Test {};

//----------------------------------------------------------------------------
// Arithmetic operations
//----------------------------------------------------------------------------

std::array<std::pair<int, int>, 6> arithmetic_operator_data = {
    std::pair<int, int>(1, 2),
    std::pair<int, int>(100, 100),
    std::pair<int, int>(150, 300),
    std::pair<int, int>(11, -50),
    std::pair<int, int>(11, 12),
    std::pair<int, int>(-12, 33)
};

TEST_F(amp_operator_tests, plus)
{
    compare_binary_operator(std::plus<int>(), amp_algorithms::plus<>(), arithmetic_operator_data);
}

TEST_F(amp_operator_tests, minus)
{
    compare_binary_operator(std::minus<int>(), amp_algorithms::minus<>(), arithmetic_operator_data);
}

TEST_F(amp_operator_tests, multiplies)
{
    compare_binary_operator(std::multiplies<int>(), amp_algorithms::multiplies<>(), arithmetic_operator_data);
}

TEST_F(amp_operator_tests, divides)
{
    compare_binary_operator(std::divides<int>(), amp_algorithms::divides<>(), arithmetic_operator_data);
}

TEST_F(amp_operator_tests, modulus)
{
    compare_binary_operator(std::modulus<int>(), amp_algorithms::modulus<>(), arithmetic_operator_data);
}

std::array<int, 3> negate_operator_data = { 2, 0, -2 };

TEST_F(amp_operator_tests, negate)
{
    compare_unary_operator(std::negate<int>(), amp_algorithms::negate<>(), negate_operator_data);
}

//----------------------------------------------------------------------------
// Additional arithmetic operations with no STL equivalents
//----------------------------------------------------------------------------

TEST_F(amp_operator_tests, static_log2)
{
    ASSERT_EQ(0, static_log2(1));
    ASSERT_EQ(2, static_log2(4));
    ASSERT_EQ(8, static_log2(256));
}

TEST_F(amp_operator_tests, static_is_power_of_two)
{
    ASSERT_FALSE(static_is_power_of_two(0));
    ASSERT_TRUE(static_is_power_of_two(1));
    ASSERT_TRUE(static_is_power_of_two(4));
    ASSERT_FALSE(static_is_power_of_two(5));
    ASSERT_TRUE(static_is_power_of_two(256));
}

TEST_F(amp_operator_tests, is_power_of_two)
{
    ASSERT_FALSE(is_power_of_two(0));
    ASSERT_TRUE(is_power_of_two(1));
    ASSERT_TRUE(is_power_of_two(4));
    ASSERT_FALSE(is_power_of_two(5));
    ASSERT_TRUE(is_power_of_two(256));
}

//----------------------------------------------------------------------------
// Comparison operations
//----------------------------------------------------------------------------

TEST_F(amp_operator_tests, equal_to)
{
    compare_binary_operator(std::equal_to<int>(), amp_algorithms::equal_to<>(), arithmetic_operator_data);
}

TEST_F(amp_operator_tests, not_equal_to)
{
    compare_binary_operator(std::not_equal_to<int>(), amp_algorithms::not_equal_to<>(), arithmetic_operator_data);
}

TEST_F(amp_operator_tests, greater)
{
    compare_binary_operator(std::greater<int>(), amp_algorithms::greater<>(), arithmetic_operator_data);
}

TEST_F(amp_operator_tests, greater_equal)
{
    compare_binary_operator(std::greater_equal<int>(), amp_algorithms::greater_equal<>(), arithmetic_operator_data);
}

TEST_F(amp_operator_tests, less)
{
    compare_binary_operator(std::less<int>(), amp_algorithms::less<>(), arithmetic_operator_data);
}

TEST_F(amp_operator_tests, less_equal)
{
    compare_binary_operator(std::less_equal<int>(), amp_algorithms::less_equal<>(), arithmetic_operator_data);
}

std::array<std::pair<unsigned, unsigned>, 8> logical_operator_data = {
    std::pair<unsigned, unsigned>(0xF, 0xF),
    std::pair<unsigned, unsigned>(0xFF, 0x0A),
    std::pair<unsigned, unsigned>(0x0A, 0xFF),
    std::pair<unsigned, unsigned>(0xFF, 0x00),
    std::pair<unsigned, unsigned>(0x00, 0x00)
};

//----------------------------------------------------------------------------
// Bitwise operations
//----------------------------------------------------------------------------

std::array<int, 4> bit_not_operator_data = { 0xF0, 0xFF, 0x00, 0x0A };

TEST_F(amp_operator_tests, bit_and)
{
    compare_binary_operator(std::bit_and<int>(), amp_algorithms::bit_and<int>(), logical_operator_data);
}

TEST_F(amp_operator_tests, bit_or)
{
    compare_binary_operator(std::bit_or<int>(), amp_algorithms::bit_or<int>(), logical_operator_data);
}

TEST_F(amp_operator_tests, bit_xor)
{
    compare_binary_operator(std::bit_xor<int>(), amp_algorithms::bit_xor<int>(), logical_operator_data);
}

TEST_F(amp_operator_tests, bit_not)
{
    compare_unary_operator(std::bit_not<int>(), amp_algorithms::bit_not<int>(), bit_not_operator_data);
}

//----------------------------------------------------------------------------
// Additional bitwise operations with no STL equivalent
//----------------------------------------------------------------------------

TEST_F(amp_operator_tests, static_count_bits)
{
    ASSERT_EQ( 4, static_count_bits(0x0F));
    ASSERT_EQ( 8, static_count_bits(0xFF));
    ASSERT_EQ(16, static_count_bits(0xFFFF));
    ASSERT_EQ( 8, static_count_bits(0xFFFF, amp_algorithms::bit08));
    ASSERT_EQ( 2, static_count_bits(0x0A));
    ASSERT_EQ( 0, static_count_bits(0x00));
}

TEST_F(amp_operator_tests, count_bits)
{
    ASSERT_EQ( 4, count_bits(0x0F));
    ASSERT_EQ( 8, count_bits(0xFF));
    ASSERT_EQ(16, count_bits(0xFFFF));
    ASSERT_EQ(2,  count_bits(0x0A));
    ASSERT_EQ( 0, count_bits(0x00));
}

//----------------------------------------------------------------------------
// Logical operations
//----------------------------------------------------------------------------

TEST_F(amp_operator_tests, logical_not)
{
    compare_unary_operator(std::logical_not<int>(), amp_algorithms::logical_not<int>(), bit_not_operator_data);
}

TEST_F(amp_operator_tests, logical_and)
{
    compare_binary_operator(std::logical_and<int>(), amp_algorithms::logical_and<int>(), logical_operator_data);
}

TEST_F(amp_operator_tests, logical_or)
{
    compare_binary_operator(std::logical_or<int>(), amp_algorithms::logical_or<int>(), logical_operator_data);
}

template<typename T>
class is_odd {
public:
    typedef T argument_type;

    constexpr bool operator()(const T& a) restrict(cpu, amp) { return (a % 2) != 0; }
};

TEST_F(amp_operator_tests, not1)
{
    compare_unary_operator(std::not1(is_odd<int>()), amp_algorithms::not1(is_odd<int>()), negate_operator_data);
}

TEST_F(amp_operator_tests, not2)
{
    compare_binary_operator(std::not2(equal_to<int>()), amp_algorithms::not2(equal_to<int>()), arithmetic_operator_data);
}