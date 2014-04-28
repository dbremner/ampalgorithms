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
#include "testtools.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace concurrency;
using namespace amp_algorithms;
using namespace amp_algorithms::_details;
using namespace testtools;

namespace amp_algorithms_tests
{
    TEST_CLASS(amp_sort_tests)
    {
        TEST_CLASS_INITIALIZE(initialize_tests)
        {
            set_default_accelerator();
        }

        TEST_METHOD(aaa_radix_key_value_tests)
        {
            std::array<std::pair<int, int>, 16> theories =
            { 
                std::make_pair( 3, 3),
                std::make_pair( 2, 2),
                std::make_pair( 1, 1),
                std::make_pair( 6, 2),
                std::make_pair(10, 2),
                std::make_pair(11, 3),
                std::make_pair(13, 1),
                std::make_pair(16, 0),
                std::make_pair(15, 3),
                std::make_pair(10, 2),
                std::make_pair( 5, 1),
                std::make_pair(14, 2),
                std::make_pair( 4, 0),
                std::make_pair(12, 0),
                std::make_pair( 7, 3),
                std::make_pair( 8, 0)
            };

            for (auto t : theories)
            {
                Assert::AreEqual(t.second, radix_key_value<int, 2>(t.first, 0));
            }
        }

        //TEST_METHOD(aaa_amp_sort_radix)
        //{
        //    std::array<unsigned, 16> input = {  3,  2,  1,  6, 10, 11, 13, 16, 15, 10,  5, 14,  4, 12,  9,  8 };
        //    //                                  3   2   1   2   2   3   1   0   3   2   1   2   0   0   1   0
        //    //std::array<unsigned, 4> expected = { 3, 2, 1, 6 };
        //    std::array<int, 4> expected = { 4, 4, 5, 3 };
        //    //std::array<unsigned, 4> expected = { 0, 4, 9, 12 };
        //    array_view<unsigned> input_av(int(input.size()), input);

        //    //radix_sort(input_av);
        //    
        //    //histogram_tile<unsigned, 2, 8>(input_av, 0);

        //    Assert::IsTrue(are_equal(expected, input_av.section(0, 4)));
        //}
    };
}; // namespace amp_algorithms_tests

// TODO: Finish make_array_view, assuming we really need it.

template< typename ConstRandomAccessIterator >
void make_array_view( ConstRandomAccessIterator first, ConstRandomAccessIterator last )
{
    return array_view(std::distance(first, last), first);
}
