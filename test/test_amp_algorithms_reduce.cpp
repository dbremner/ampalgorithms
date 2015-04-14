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
#include <gtest\gtest.h>

#include "testtools.h"

using namespace concurrency;
using namespace amp_algorithms;
using namespace testtools;

class amp_reduce_tests : public testbase, public ::testing::Test {
public:
    template<typename T, typename BinaryFunctor>
    void test_reduce(int element_count, const T& identity_element, BinaryFunctor func, T& cpu_result, T& amp_result)
    {
		using namespace amp_stl_algorithms;

        std::vector<T> inVec(generate_data<T>(element_count));

        array_view<const T> inArrView(element_count, inVec);
        amp_result = reduce(cbegin(inArrView), cend(inArrView), identity_element, func);

        // Now compute the result on the CPU for verification
        cpu_result = std::accumulate(std::cbegin(inVec), std::cend(inVec), identity_element, func);
    }

    template <typename T>
    void test_functor_view(int element_count, T& cpuStdDev, T& gpuStdDev)
    {
		using namespace amp_stl_algorithms;

        std::vector<T> inVec(generate_data<T>(element_count));
        array_view<const T> inArrView(element_count, inVec);

        // The next 4 lines use the functor_view together with the reduce
		// algorithm to obtain the standard deviation of a set of numbers.
        T gpuSum = amp_algorithms::reduce(inArrView, amp_algorithms::plus<>());
        T gpuMean = gpuSum / inArrView.extent.size();

        auto funcView = make_indexable_view(inArrView.extent, [=](auto&& idx) restrict(cpu, amp) {
            return ((inArrView(idx) - gpuMean) * (inArrView(idx) - gpuMean));
        });

        T gpuTotalVariance = amp_algorithms::reduce(funcView, amp_algorithms::plus<>());
        gpuStdDev = static_cast<T>(std::sqrt(gpuTotalVariance / static_cast<T>(element_count)));

        // Now compute the result on the CPU for verification
        T cpuSum = std::accumulate(std::cbegin(inVec), std::cend(inVec), T(0));

        T cpuMean = cpuSum / element_count;
		T cpuTotalVariance = std::accumulate(std::cbegin(inVec), std::cend(inVec), T(0), [=](auto&& x,
																							 auto&& y) {
			return x + ((y - cpuMean) * (y - cpuMean));
		});

		cpuStdDev = static_cast<T>(std::sqrt(cpuTotalVariance / static_cast<T>(element_count)));
    }
};

TEST_F(amp_reduce_tests, reduce_int_min)
{
	using namespace amp_stl_algorithms;

    int cpu_result, amp_result;

	test_reduce<int>(test_array_size<int>(),
				     std::numeric_limits<int>::max(),
					 [](auto&& x, auto&& y) restrict(cpu, amp) { return min(forward<decltype(x)>(x),
																			forward<decltype(y)>(y)); },
					 cpu_result,
					 amp_result);

    ASSERT_EQ(cpu_result, amp_result);
}

TEST_F(amp_reduce_tests, reduce_float_max)
{
	using namespace amp_stl_algorithms;

    float cpu_result, amp_result;

    test_reduce<float>(test_array_size<float>(),
					   std::numeric_limits<float>::min(),
					   [](auto&& x, auto&& y) restrict(cpu, amp) { return max(forward<decltype(x)>(x),
																			  forward<decltype(y)>(y)); },
					   cpu_result,
					   amp_result);

    ASSERT_TRUE(testtools::compare(cpu_result, amp_result)); // Revisit this.
}

TEST_F(amp_reduce_tests, functor_view_float)
{
    float cpuStdDev, gpuStdDev;

    test_functor_view<float>(test_array_size<float>(), cpuStdDev, gpuStdDev);

    ASSERT_TRUE(compare(gpuStdDev, cpuStdDev));
}

TEST_F(amp_reduce_tests, functor_view_int)
{
    int cpuStdDev, gpuStdDev;

    test_functor_view<int>(21, cpuStdDev, gpuStdDev);

    ASSERT_TRUE(compare(gpuStdDev, cpuStdDev));
}