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
* This file contains the unit tests for sort.
*---------------------------------------------------------------------------*/
#include "stdafx.h"

#include <amp_algorithms.h>
#include "test_amp.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace concurrency;
using namespace amp_algorithms;
using namespace test_tools;

namespace amp_algorithms_tests
{
    TEST_CLASS(sort_tests)
    {
        TEST_CLASS_INITIALIZE(initialize_tests)
        {
            set_default_accelerator();
        }

        TEST_METHOD(amp_sort_radix)
        {
            std::vector<int> input(1024);
            generate_data(input);

            std::vector<int> expected(input.size());
            std::copy(begin(input), end(input), begin(expected));
            std::sort(begin(expected), end(expected));

            array_view<int> input_av(int(input.size()), input);

            radix_sort(input_av, 4);

            //Assert::IsTrue(are_equal(expected, input_av));
        }
    };
}; // namespace amp_algorithms_tests

// TODO: Finish make_array_view, assuming we really need it.
template< typename ConstRandomAccessIterator >
void make_array_view( ConstRandomAccessIterator first, ConstRandomAccessIterator last )
{
    return array_view(std::distance(first, last), first);
}
