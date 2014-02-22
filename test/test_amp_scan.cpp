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

namespace ScanTests
{
    std::wstring Msg(std::vector<int>& expected, std::vector<int>& actual, size_t width = 8)
    {
        std::wostringstream msg;
        msg << ContainerWidth(width) << L"[" << expected << L"] != [" << actual << L"]" << std::endl;
        return msg.str();
    }

    TEST_CLASS(ScanOptimizedTests)
    {
    public:
        TEST_METHOD(scan_exclusiveTests_Simple_One_Tile)
        {
            std::vector<int> input(8, 1);
            std::vector<int> result(input.size());
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 0);

            scan_exclusive<4>(begin(input), end(input), result.begin());
            
            Assert::IsTrue(expected == result, Msg(expected, result).c_str());
        }

        TEST_METHOD(scan_exclusiveTests_Simple_Two_Tiles)
        {
            std::vector<int> input(16, 1);
            std::vector<int> result(input.size());
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 0);

            scan_exclusive<4>(begin(input), end(input), result.begin());
            
            Assert::IsTrue(expected == result, Msg(expected, result, 16).c_str());
        }

        TEST_METHOD(scan_exclusiveTests_Sequential_One_Tile)
        {
            std::array<int, 8> input =      {  1, 2,  3,  4,  5,  6,  7,  8 };
            std::vector<int> result(input.size());
            //std::array<int, 8> expected = { +1, 3, +3, 10, +5, 11, +7, 36 }; // Up sweep only
            //std::array<int, 8> expected = { +1, 3, +3, 10, +5, 11, +7,  0 }; // Down sweep depth = 0
            //std::array<int, 8> expected = { +1, 3, +3,  0, +5, 11, +7, 10 }; // Down sweep depth = 1
            //std::array<int, 8> expected = { +1, 0, +3,  3, +5, 10, +7, 21 }; // Down sweep depth = 2
            std::array<int, 8> expected =   {  0, 1,  3,  6, 10, 15, 21, 28 }; // Final Result

            scan_exclusive<4>(begin(input), end(input), result.begin());
            
            std::vector<int> exp(begin(expected), end(expected));
            Assert::IsTrue(exp == result, Msg(exp, result).c_str());
        }

        TEST_METHOD(scan_inclusiveTests_Simple_One_Tile)
        {
            std::vector<int> input(8, 1);
            std::vector<int> result(input.size());
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 1);

            scan_inclusive<4>(begin(input), end(input), result.begin());
            
            Assert::IsTrue(expected == result, Msg(expected, result).c_str());
        }

        TEST_METHOD(scan_inclusiveTests_Simple_Two_Tiles)
        {
            std::vector<int> input(16, 1);
            std::vector<int> result(input.size());
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 1);

            scan_inclusive<4>(begin(input), end(input), result.begin());
            
            Assert::IsTrue(expected == result, Msg(expected, result, 16).c_str());
        }

        TEST_METHOD(scan_inclusiveTests_Complex_One_Tile)
        {
            std::array<int, 8> input =    { 1, 3,  6,  2,  7,  9,  0,  5 };
            std::vector<int> result(input.size());
            std::array<int, 8> expected = { 1, 4, 10, 12, 19, 28, 28, 33 };

            scan_inclusive<4>(begin(input), end(input), result.begin());
            
            std::vector<int> exp(begin(expected), end(expected));
            Assert::IsTrue(exp == result, Msg(exp, result, 8).c_str());
        }

        TEST_METHOD(scan_exclusiveTests_Sequential_Two_Tiles)
        {
            // Use REF accelerator for a warp width of 4.
            //concurrency::accelerator::set_default(concurrency::accelerator::direct3d_ref);
            std::array<int, 16> input =    {  1, 2,  3,  4, 5,  6,  7,  8,  9, 10, 11, 12, 13, 14 ,15 ,16 };
            std::vector<int> result(input.size());
            //std::array<int, 16> expected =   {  1, 3, 3, 10,  5, 11,  7, 26, 9, 19, 11, 42 }; // Up sweep only
            //std::array<int, 16> expected = {  1, 3, 3,  0,  5, 11,  7,  0, 9, 19, 11,  0 }; // Down sweep depth = 0
            //std::array<int, 8> expected =  {  1, 0, 3,  3,  5,  0,  7, 11 }; // Down sweep depth = 1
            //std::array<int, 8> expected =  {  0, 1, 3,  6,  0,  5, 11, 18 }; // Down sweep depth = 2
            //std::array<int, 16> expected = {  0, 1, 3,  6,  0,  5, 11, 18,  0,  9, 19, 30 };  // Down sweep depth = 2
            std::array<int, 16> expected =   {  0, 1, 3,  6, 10, 15, 21, 28, 36, 45, 55, 66, 78, 91, 105, 120 }; // Final Result

            scan_exclusive<4>(begin(input), end(input), result.begin());
            
            std::vector<int> exp(begin(expected), end(expected));
            Assert::IsTrue(exp == result, Msg(exp, result, 16).c_str());
        }

        TEST_METHOD(scan_inclusiveTests_Complex_Two_Tiles)
        {
            std::array<int, 8> input =    { 1, 3,  6,  2,  7,  9,  0,  5 };
            std::vector<int> result(input.size());
            std::array<int, 8> expected = { 1, 4, 10, 12, 19, 28, 28, 33 };

            scan_inclusive<2>(begin(input), end(input), result.begin());
            
            std::vector<int> exp(begin(expected), end(expected));
            Assert::IsTrue(exp == result, Msg(exp, result).c_str());
        }

        TEST_METHOD(scan_exclusiveTests_Large)
        {
            std::vector<int> input(4096, 1);
            std::vector<int> result(input.size());
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 0);
            // Does not work for tiles sizes greater than 32. Relying on warp sync.
            scan_exclusive<256>(begin(input), end(input), result.begin());
            
            Assert::IsTrue(expected == result, Msg(expected, result, 24).c_str());
        }

        TEST_METHOD(scan_inclusiveTests_Large)
        {
            std::vector<int> input(4096, 1);
            std::vector<int> result(input.size());
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 1);
            // Does not work for tiles sizes greater than 32. Relying on warp sync.
            scan_inclusive<256>(begin(input), end(input), result.begin());
            
            Assert::IsTrue(expected == result, Msg(expected, result, 24).c_str());
        }

        TEST_METHOD(scan_exclusiveTests_Simple_Overlapped_Tiles)
        {
            std::vector<int> input(10, 1);
            std::vector<int> result(input.size());
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 0);

            scan_exclusive<4>(begin(input), end(input), result.begin());
            
            Assert::IsTrue(expected == result, Msg(expected, result, 16).c_str());
        }
    };
}
