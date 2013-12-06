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

#include <amp_stl_algorithms.h>
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
namespace test_tools_tests
{
    TEST_CLASS(equality_tests)
    {
        TEST_METHOD(test_array_view_equality)
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
};