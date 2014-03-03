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
* C++ AMP standard algorithms library.
*
* This file contains the helpers classes in amp_algorithms::_details namespace
*---------------------------------------------------------------------------*/

#include "stdafx.h"

#include "test_amp.h"
#include "amp_scan.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace amp_algorithms;
using namespace test_tools;

namespace amp_algorithms_tests
{
    std::wstring Msg(std::vector<int>& expected, std::vector<int>& actual, size_t width = 32)
    {
        std::wostringstream msg;
        msg << container_width(50) << L"[" << expected << L"] != [" << actual << L"]" << std::endl;
        return msg.str();
    }

    TEST_CLASS(scan_tests)
    {
        //TEST_CLASS_INITIALIZE(initialize_tests)
        //{
        //    set_default_accelerator();
        //}

        TEST_METHOD(amp_scan_exclusive_single_warp)
        {
            std::vector<int> input(32, 1);
            std::vector<int> result(input.size(), -1);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 0);

            scan_exclusive_new<32>(begin(input), end(input), begin(result));

            Assert::IsTrue(expected == result, Msg(expected, result).c_str());
        }

        TEST_METHOD(amp_scan_inclusive_single_warp)
        {
            std::vector<int> input(32, 1);
            std::vector<int> result(input.size(), -1);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 1);

            scan_inclusive_new<32>(begin(input), end(input), begin(result));

            Assert::IsTrue(expected == result, Msg(expected, result).c_str());
        }

        TEST_METHOD(amp_scan_exclusive_multi_warp)
        {
            std::vector<int> input(64, 1);
            std::vector<int> result(input.size(), -1);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 0);

            scan_exclusive_new<64>(begin(input), end(input), begin(result));

            Assert::IsTrue(expected == result, Msg(expected, result).c_str());
        }

        TEST_METHOD(amp_scan_inclusive_multi_warp)
        {
            std::vector<int> input(64, 1);
            std::vector<int> result(input.size(), -1);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 1);

            scan_inclusive_new<64>(begin(input), end(input), begin(result));

            Assert::IsTrue(expected == result, Msg(expected, result).c_str());
        }

        TEST_METHOD(amp_scan_exclusive_multi_tile)
        {
            std::vector<int> input(128, 1);
            std::vector<int> result(input.size(), -1);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 0);

            scan_exclusive_new<32>(begin(input), end(input), begin(result));

            Assert::IsTrue(expected == result, Msg(expected, result).c_str());
        }

        TEST_METHOD(amp_scan_inclusive_multi_tile)
        {
            std::vector<int> input(128, 1);
            std::vector<int> result(input.size(), -1);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 1);

            scan_inclusive_new<32>(begin(input), end(input), begin(result));

            Assert::IsTrue(expected == result, Msg(expected, result).c_str());
        }

        TEST_METHOD(amp_scan_exclusive_multi_warp_multi_tile)
        {
            std::vector<int> input(1024, 1);
            std::vector<int> result(input.size(), -1);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 0);

            scan_exclusive_new<128>(begin(input), end(input), begin(result));

            Assert::IsTrue(expected == result, Msg(expected, result).c_str());
        }

        TEST_METHOD(amp_scan_inclusive_multi_warp_multi_tile)
        {
            std::vector<int> input(1024, 1);
            std::vector<int> result(input.size(), -1);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 1);

            scan_inclusive_new<128>(begin(input), end(input), begin(result));

            Assert::IsTrue(expected == result, Msg(expected, result).c_str());
        }

        TEST_METHOD(amp_scan_exclusive_incomplete_warp)
        {
            std::vector<int> input(34, 1);
            std::vector<int> result(input.size(), -1);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 0);

            scan_exclusive_new<32>(begin(input), end(input), begin(result));

            Assert::IsTrue(expected == result, Msg(expected, result, 36).c_str());
        }

        TEST_METHOD(amp_scan_inclusive_incomplete_warp)
        {
            std::vector<int> input(34, 1);
            std::vector<int> result(input.size(), -1);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 1);

            scan_inclusive_new<32>(begin(input), end(input), begin(result));

            Assert::IsTrue(expected == result, Msg(expected, result, 36).c_str());
        }

        TEST_METHOD(amp_scan_inclusive_overlapped_input_and_output)
        {
            std::vector<int> input(1024);
            generate_data(input);
            std::vector<int> expected(input.size());
            scan_sequential_inclusive(begin(input), end(input), begin(expected));

            scan_inclusive_new<128>(begin(input), end(input), begin(input));

            Assert::IsTrue(expected == input, Msg(expected, input).c_str());
        }

        TEST_METHOD(amp_scan_exclusive)
        {
            std::vector<int> input(1024);
            generate_data(input);
            std::vector<int> result(input.size(), -1);
            std::vector<int> expected(input.size());
            scan_sequential_exclusive(begin(input), end(input), begin(expected));

            scan_exclusive_new<128>(begin(input), end(input), begin(result));

            Assert::IsTrue(expected == result, Msg(expected, result).c_str());
        }

        TEST_METHOD(amp_scan_inclusive)
        {
            std::vector<int> input(1024);
            generate_data(input);
            std::vector<int> result(input.size(), -1);
            std::vector<int> expected(input.size());
            scan_sequential_inclusive(begin(input), end(input), begin(expected));

            scan_inclusive_new<128>(begin(input), end(input), begin(result));

            Assert::IsTrue(expected == result, Msg(expected, result).c_str());
        }

    };
}
