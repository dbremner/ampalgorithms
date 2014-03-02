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
* This file contains the unit tests.
*---------------------------------------------------------------------------*/

#include "stdafx.h"
#include "amp_scan.h"
#include "amp_stl_algorithms.h"
#include "test_amp.h"

// Code coverage is optional and requires VS Premium or Ultimate.
#ifdef CODECOVERAGE
#pragma managed(push, off)
ExcludeFromCodeCoverage(exclude_test_tool_tests, L"test_tools_tests::*");
#pragma managed(pop)
#endif

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace concurrency;
using namespace amp_stl_algorithms;
using namespace test_tools;

// TODO: Filename should match namespace
namespace testtools_tests
{
    TEST_CLASS(testtools_equality)
    {
        TEST_METHOD(testtools_array_view_equality)
        {
            const int size = 10;
            std::vector<int> a(size, 0);
            std::vector<int> b(size, 0);
            array_view<int> b_av(size, b);

            Assert::IsTrue(are_equal(a, b_av));
            Assert::IsTrue(are_equal(b_av, a));

            b_av[6] = 2;

            Assert::IsFalse(are_equal(a, b_av));
            Assert::IsFalse(are_equal(b_av, a));

            b_av = b_av.section(0, 5);

            Assert::IsFalse(are_equal(a, b_av));
            Assert::IsFalse(are_equal(b_av, a));

            a.resize(5);

            Assert::IsTrue(are_equal(a, b_av));
            Assert::IsTrue(are_equal(b_av, a));
        }
    };

    std::wstring Msg(std::vector<int>& expected, std::vector<int>& actual, size_t width = 32)
    {
        std::wostringstream msg;
        msg << container_width(50) << L"[" << expected << L"] != [" << actual << L"]" << std::endl;
        return msg.str();
    }

    TEST_CLASS(testtools_sequential_scan)
    {
        template <scan_mode _Mode, typename _BinaryOp, typename InIt, typename OutIt>
        inline void scan_sequential(InIt first, InIt last, OutIt dest_first, const _BinaryOp& op)
        {
            typedef InIt::value_type T;

            auto result = (_Mode == amp_algorithms::scan_mode::exclusive) ? T() : *first;
            *dest_first = result;
            std::transform(first + 1, last, dest_first + 1, [=, &result](const T& v)
            {
                result += v;
                return result;
            });
        }

        TEST_METHOD(testtools_sequential_exclusive_scan)
        {
            std::vector<int> input(1024, 1);
            std::vector<int> result(input.size(), -1);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 0);

            scan_sequential<amp_algorithms::scan_mode::exclusive>(begin(input), end(input), result.begin(), amp_algorithms::plus<int>());

            Assert::IsTrue(expected == result, Msg(expected, result).c_str());
        }

        TEST_METHOD(testtools_sequential_inclusive_scan)
        {
            std::vector<int> input(1024, 1);
            std::vector<int> result(input.size(), -1);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 1);

            scan_sequential<amp_algorithms::scan_mode::inclusive>(begin(input), end(input), result.begin(), amp_algorithms::plus<int>());

            Assert::IsTrue(expected == result, Msg(expected, result).c_str());
        }

    };
};
