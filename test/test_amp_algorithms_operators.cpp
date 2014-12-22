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

#include "testtools.h"

using namespace concurrency;
using namespace amp_algorithms;
using namespace testtools;

class amp_operator_tests : public testbase, public ::testing::Test {};

//----------------------------------------------------------------------------
// Arithmetic operations
//----------------------------------------------------------------------------

template<typename T>
class test_max
{
public:
    T operator()(const T& a, const T& b) const { return std::max<T>(a, b); }
};

template<typename T>
class test_min
{
public:
    T operator()(const T& a, const T& b) const { return std::min<T>(a, b); }
};

typedef ::testing::Types <

    // Arithmetic operations:
    OperatorTestDefinition<std::plus<int>, amp_algorithms::plus<int>>,
    OperatorTestDefinition<std::minus<int>, amp_algorithms::minus<int>>,
    OperatorTestDefinition<std::multiplies<int>, amp_algorithms::multiplies<int>>,
    OperatorTestDefinition<std::divides<int>, amp_algorithms::divides<int>>,
    OperatorTestDefinition<std::modulus<int>, amp_algorithms::modulus<int>>,

    // Comparison operations:
    OperatorTestDefinition<std::equal_to<int>, amp_algorithms::equal_to<int>>,
    OperatorTestDefinition<std::not_equal_to<int>, amp_algorithms::not_equal_to<int>>,
    OperatorTestDefinition<std::less<int>, amp_algorithms::less<int>>,
    OperatorTestDefinition<std::less_equal<int>, amp_algorithms::less_equal<int>>,
    OperatorTestDefinition<std::greater<int>, amp_algorithms::greater<int>>,
    OperatorTestDefinition<std::greater_equal<int>, amp_algorithms::greater_equal<int>>,
    OperatorTestDefinition<test_min<int>, amp_algorithms::min<int>>,
    OperatorTestDefinition<test_max<int>, amp_algorithms::max<int>>,

    // Logical operations:
    OperatorTestDefinition<std::logical_and<int>, amp_algorithms::logical_and<int>>,
    OperatorTestDefinition<std::logical_or<int>, amp_algorithms::logical_or<int>>,

    // Bitwise operations: 
    OperatorTestDefinition<std::bit_and<int>, amp_algorithms::bit_and<int>>,
    OperatorTestDefinition<std::bit_or<int>, amp_algorithms::bit_or<int>>,
    OperatorTestDefinition<std::bit_xor<int>, amp_algorithms::bit_xor<int>>
> binary_operator_types;

typedef ::testing::Types <
    OperatorTestDefinition<std::negate<int>, amp_algorithms::negate<int>>,
    OperatorTestDefinition<std::logical_not<int>, amp_algorithms::logical_not<int>>,
    OperatorTestDefinition<std::bit_not<int>, amp_algorithms::bit_not<int>>
> unary_operator_types;

std::array<int, 3> arithmetic_unary_operator_data = { 2, 0, -2 };

std::array<std::pair<int, int>, 5> arithmetic_binary_operator_data = {
    std::make_pair(0, 222),
    std::make_pair(101, 101),
    std::make_pair(50, -50),
    std::make_pair(-12, -235),
    std::make_pair(0, 0)
};

template <typename T>
class arithmetic_binary_operator_tests : public ::testing::Test { };
TYPED_TEST_CASE_P(arithmetic_binary_operator_tests);

TYPED_TEST_P(arithmetic_binary_operator_tests, binary_operator_test)
{
    compare_binary_operator(TypeParam::stl_type(), TypeParam::amp_type(), cbegin(arithmetic_binary_operator_data), cend(arithmetic_binary_operator_data));
    
}

REGISTER_TYPED_TEST_CASE_P(arithmetic_binary_operator_tests, binary_operator_test);
INSTANTIATE_TYPED_TEST_CASE_P(amp_operator_tests, arithmetic_binary_operator_tests, binary_operator_types);

template <typename T>
class arithmetic_unary_operator_tests : public ::testing::Test { };
TYPED_TEST_CASE_P(arithmetic_unary_operator_tests);

TYPED_TEST_P(arithmetic_unary_operator_tests, unary_operator_test)
{
    compare_unary_operator(TypeParam::stl_type(), TypeParam::amp_type(), arithmetic_unary_operator_data);
}

REGISTER_TYPED_TEST_CASE_P(arithmetic_unary_operator_tests, unary_operator_test);
INSTANTIATE_TYPED_TEST_CASE_P(amp_operator_tests, arithmetic_unary_operator_tests, unary_operator_types);

//----------------------------------------------------------------------------
// Inversion operations
//----------------------------------------------------------------------------

template<typename T>
class test_func
{
public:
    typedef T argument_type;
    bool operator()(const T& a) const restrict(cpu, amp) { return (a % 2) != 0; }
};

TEST_F(amp_operator_tests, not1)
{
    compare_unary_operator(std::not1(test_func<int>()), amp_algorithms::not1(test_func<int>()), arithmetic_unary_operator_data);
}

TEST_F(amp_operator_tests, not2)
{
    compare_binary_operator(not_equal_to<int>(), amp_algorithms::not2(equal_to<int>()), cbegin(arithmetic_binary_operator_data), cend(arithmetic_binary_operator_data));
}

//----------------------------------------------------------------------------
// Additional bitwise operations with no STL equivalent
//----------------------------------------------------------------------------

TEST_F(amp_operator_tests, static_log2)
{
    ASSERT_EQ(0, static_log2<1>::value);
    ASSERT_EQ(2, static_log2<4>::value);
    ASSERT_EQ(8, static_log2<256>::value);
}

TEST_F(amp_operator_tests, static_is_power_of_two)
{
    ASSERT_FALSE(static_is_power_of_two<0>::value);
    ASSERT_TRUE(static_is_power_of_two<1>::value);
    ASSERT_TRUE(static_is_power_of_two<4>::value);
    ASSERT_FALSE(static_is_power_of_two<5>::value);
    ASSERT_TRUE(static_is_power_of_two<256>::value);
}

TEST_F(amp_operator_tests, is_power_of_two)
{
    ASSERT_FALSE(is_power_of_two(0));
    ASSERT_TRUE(is_power_of_two(1));
    ASSERT_TRUE(is_power_of_two(4));
    ASSERT_FALSE(is_power_of_two(5));
    ASSERT_TRUE(is_power_of_two(256));
}

TEST_F(amp_operator_tests, static_count_bits)
{
    ASSERT_EQ( 4, static_count_bits<0x0F>::value);
    ASSERT_EQ( 8, static_count_bits<0xFF>::value);
    ASSERT_EQ(16, static_count_bits<0xFFFF>::value);
    ASSERT_EQ( 8, (static_count_bits<0xFFFF, amp_algorithms::bit08>::value));
    ASSERT_EQ( 2, static_count_bits<0x0A>::value);
    ASSERT_EQ( 0, static_count_bits<0x00>::value);
}

TEST_F(amp_operator_tests, count_bits)
{
    ASSERT_EQ( 4, count_bits(0x0F));
    ASSERT_EQ( 8, count_bits(0xFF));
    ASSERT_EQ(16, count_bits(0xFFFF));
    ASSERT_EQ(2,  count_bits(0x0A));
    ASSERT_EQ( 0, count_bits(0x00));
}
