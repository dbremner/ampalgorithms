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

std::array<std::pair<int, int>, 6> arithmetic_operator_data = {
    std::pair<int, int>(1, 2),
    std::pair<int, int>(100, 100),
    std::pair<int, int>(150, 300),
    std::pair<int, int>(1000, -50),
    std::pair<int, int>(11, 12),
    std::pair<int, int>(-12, 33)
};

std::array<int, 3> negate_operator_data = { 2, 0, -2 };

TEST_F(amp_operator_tests, plus)
{
    compare_binary_operator(std::plus<int>(), amp_algorithms::plus<int>(), arithmetic_operator_data);
}

TEST_F(amp_operator_tests, minus)
{
    compare_binary_operator(std::minus<int>(), amp_algorithms::minus<int>(), arithmetic_operator_data);
}

TEST_F(amp_operator_tests, multiplies)
{
    compare_binary_operator(std::multiplies<int>(), amp_algorithms::multiplies<int>(), arithmetic_operator_data);
}

TEST_F(amp_operator_tests, divides)
{
    compare_binary_operator(std::divides<int>(), amp_algorithms::divides<int>(), arithmetic_operator_data);
}

TEST_F(amp_operator_tests, modulus)
{
    compare_binary_operator(std::modulus<int>(), amp_algorithms::modulus<int>(), arithmetic_operator_data);
}

TEST_F(amp_operator_tests, negate)
{
    compare_unary_operator(std::negate<int>(), amp_algorithms::negate<int>(), negate_operator_data);
}

TEST_F(amp_operator_tests, equal_to)
{
    compare_binary_operator(std::equal_to<int>(), amp_algorithms::equal_to<int>(), arithmetic_operator_data);
}

TEST_F(amp_operator_tests, not_equal_to)
{
    compare_binary_operator(std::not_equal_to<int>(), amp_algorithms::not_equal_to<int>(), arithmetic_operator_data);
}

TEST_F(amp_operator_tests, greater)
{
    compare_binary_operator(std::greater<int>(), amp_algorithms::greater<int>(), arithmetic_operator_data);
}

TEST_F(amp_operator_tests, greater_equal)
{
    compare_binary_operator(std::greater_equal<int>(), amp_algorithms::greater_equal<int>(), arithmetic_operator_data);
}

TEST_F(amp_operator_tests, less)
{
    compare_binary_operator(std::less<int>(), amp_algorithms::less<int>(), arithmetic_operator_data);
}

TEST_F(amp_operator_tests, less_equal)
{
    compare_binary_operator(std::less_equal<int>(), amp_algorithms::less_equal<int>(), arithmetic_operator_data);
}

TEST_F(amp_operator_tests, max)
{
    int const & (*max) (int const &, int const &) = std::max<int>;
    compare_binary_operator(max, amp_algorithms::max<int>(), arithmetic_operator_data);
}

TEST_F(amp_operator_tests, min)
{
    int const & (*min) (int const &, int const &) = std::min<int>;
    compare_binary_operator(min, amp_algorithms::min<int>(), arithmetic_operator_data);
}

std::array<std::pair<unsigned, unsigned>, 8> logical_operator_data = {
    std::pair<unsigned, unsigned>(0xF, 0xF),
    std::pair<unsigned, unsigned>(0xFF, 0x0A),
    std::pair<unsigned, unsigned>(0x0A, 0xFF),
    std::pair<unsigned, unsigned>(0xFF, 0x00),
    std::pair<unsigned, unsigned>(0x00, 0x00)
};

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
