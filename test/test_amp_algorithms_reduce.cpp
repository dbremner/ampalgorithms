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

class amp_reduce_tests : public testbase, public ::testing::Test 
{
public:
    template <typename T>
    void test_functor_view(const int element_count, T& cpuStdDev, T& gpuStdDev)
    {
        std::vector<T> inVec(element_count);
        generate_data(inVec);

        array_view<const T> inArrView(element_count, inVec);

        // The next 4 lines use the functor_view together with the reduce algorithm to obtain the 
        // standard deviation of a set of numbers.
        T gpuSum = amp_algorithms::reduce(accelerator().create_view(), inArrView, amp_algorithms::plus<T>());
        T gpuMean = gpuSum / inArrView.extent.size();

        auto funcView = make_indexable_view(inArrView.extent, [inArrView, gpuMean](const concurrency::index<1> &idx) restrict(cpu, amp) {
            return ((inArrView(idx) - gpuMean) * (inArrView(idx) - gpuMean));
        });

        T gpuTotalVariance = amp_algorithms::reduce(funcView, amp_algorithms::plus<T>());
        gpuStdDev = static_cast<T>(sqrt(gpuTotalVariance / inArrView.extent.size()));

        // Now compute the result on the CPU for verification
        T cpuSum = inVec[0];
        for (int i = 1; i < element_count; ++i)
        {
            cpuSum += inVec[i];
        }

        T cpuMean = cpuSum / element_count;
        T cpuTotalVariance = T(0);
        for (auto e : inVec)
        {
            cpuTotalVariance += ((e - cpuMean) * (e - cpuMean));
        }

        cpuStdDev = static_cast<T>(sqrt(cpuTotalVariance / element_count));
    }
};

TEST_F(amp_reduce_tests, functor_view_float)
{
    float cpuStdDev, gpuStdDev;

    test_functor_view<float>(7919, cpuStdDev, gpuStdDev);

    ASSERT_TRUE(compare(gpuStdDev, cpuStdDev));
}

TEST_F(amp_reduce_tests, functor_view_int)
{
    int cpuStdDev, gpuStdDev;

    test_functor_view<int>(21, cpuStdDev, gpuStdDev);

    ASSERT_TRUE(compare(gpuStdDev, cpuStdDev));
}

//----------------------------------------------------------------------------
// Public API Acceptance Tests
//----------------------------------------------------------------------------

typedef ::testing::Types<
    TestDefinition<int,        83>,     // Less than one tile.
    TestDefinition<unsigned,   83>,
    TestDefinition<float,      83>,
    TestDefinition<int,       256>,     // Exactly one tile.
    TestDefinition<unsigned,  256>,
    TestDefinition<float,     256>,
    TestDefinition<int,      1024>,     // Several whole tiles.
    TestDefinition<unsigned, 1024>,
    TestDefinition<float,    1024>,
    TestDefinition<int,      1283>,     // Partial tile.
    TestDefinition<unsigned, 1283>,
    TestDefinition<float,    1283>,
    TestDefinition<int,      7919>      // Lots of tiles and a partial.
> reduce_acceptance_data;

template <typename T>
class amp_reduce_acceptance_tests : public ::testing::Test { };
TYPED_TEST_CASE_P(amp_reduce_acceptance_tests);

TYPED_TEST_P(amp_reduce_acceptance_tests, test)
{
    typedef TypeParam::value_type T;
    const int size = TypeParam::size;

    std::vector<T> input(size);
    generate_data(input);
    auto expected = std::accumulate(cbegin(input), cend(input), T(), std::plus<T>());
    array_view<const T> input_vw(size, input);

    T r = amp_algorithms::reduce(input_vw, amp_algorithms::plus<T>());

    ASSERT_EQ(expected, r);
}

REGISTER_TYPED_TEST_CASE_P(amp_reduce_acceptance_tests, test);
INSTANTIATE_TYPED_TEST_CASE_P(amp_reduce_tests, amp_reduce_acceptance_tests, reduce_acceptance_data);
