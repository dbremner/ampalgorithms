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

#include <amp_algorithms.h>
#include <amp_stl_algorithms.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace concurrency;
using namespace amp_stl_algorithms;

namespace examples
{
    TEST_CLASS(stl_examples)
    {
        TEST_CLASS_INITIALIZE(initialize_tests)
        {
#if defined(USE_REF)
            bool set_ok = accelerator::set_default(accelerator::direct3d_ref);

            if (!set_ok)
            {
                Logger::WriteMessage("Unable to set default accelerator to REF.");
            }
#endif
        }

        TEST_METHOD(stl_example_1)
        {
            {
                array<float> data(1024 * 1024);
                array_view<float> data_av(data);

                amp_stl_algorithms::iota(begin(data_av), end(data_av), 1.0f);
                auto last = amp_stl_algorithms::remove_if(begin(data_av), end(data_av), 
                    [=](const float& v) restrict(amp) { return int(v) % 2 == 1; });
                float total = amp_stl_algorithms::reduce(begin(data_av), last, 0.0f);

                std::stringstream str;
                str << "AAL: Sum of all even numbers in the input array = " << total;
                Logger::WriteMessage(str.str().c_str());
            }

            {
                std::vector<float> data(1024 * 1024);

                std::iota(std::begin(data), std::end(data), 1.0f);
                auto last = std::remove_if(std::begin(data), std::end(data), 
                    [=](const float& v) { return int(v) % 2 == 1; });
                float total = std::accumulate(begin(data), last, 0.0f);

                std::stringstream str;
                str << "STD: Sum of all even numbers in the input array = " << total;
                Logger::WriteMessage(str.str().c_str());
            }
        }
    };
};// namespace examples
