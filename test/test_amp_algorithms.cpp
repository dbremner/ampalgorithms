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
 * This file contains the test driver
 *---------------------------------------------------------------------------*/
#define NOMINMAX

#include <vector>
#include <algorithm>
#include <iostream>
#include <amp_algorithms.h>

using namespace concurrency;
using namespace amp_algorithms;

// TODO: replace this with a unit test framework

// TODO: Add tests for indexable_view_traits

template <typename value_type, typename BinaryFunctor>
void test_reduce(int element_count, BinaryFunctor func, const char *testName)
{
    std::vector<value_type> inVec(element_count);
    // Fill the vector with random values
    for (int i = 0; i < element_count; ++i) {
        inVec[i] = (value_type)rand();
        if ((i % 4) == 0) {
            inVec[i] = -inVec[i];
        }
    }

    array_view<const value_type> inArrView(element_count, inVec);
    value_type amp_result = amp_algorithms::reduce(inArrView, func);

    // Now compute the result on the CPU for verification
    value_type cpu_result = inVec[0];
    for (int i = 1; i < element_count; ++i) {
        cpu_result = func(cpu_result, inVec[i]);
    }

	// TODO: The results must be tested with some tolerance
    if (amp_result != cpu_result) {
        std::cout << testName << ": Failed. Expected: " << cpu_result << ", Actual: " << amp_result << std::endl;
    }
    else {
        std::cout << testName << ": Passed. Expected: " << cpu_result << ", Actual: " << amp_result << std::endl;
    }
}

template <typename T>
void test_functor_view()
{
    const int element_count = 1023 * 31;
    std::vector<T> inVec(element_count);
    // Fill the vector with random values
    for (int i = 0; i < element_count; ++i) {
        inVec[i] = (T)rand();
        if ((i % 4) == 0) {
            inVec[i] = -inVec[i];
        }
    }

    array_view<const T> inArrView(element_count, inVec);

    // The next 4 lines use the functor_view together with the reduce algorithm to obtain the 
    // standard deviation of a set of numbers.
    T gpuSum = amp_algorithms::reduce(accelerator().create_view(), inArrView, amp_algorithms::sum<T>());
    T gpuMean = gpuSum / inArrView.extent.size();

    auto funcView = create_indexable_view(inArrView.extent, [inArrView, gpuMean](const concurrency::index<1> &idx) restrict(cpu, amp) {
        return ((inArrView(idx) - gpuMean) * (inArrView(idx) - gpuMean));
    });

    T gpuTotalVariance = amp_algorithms::reduce(funcView, amp_algorithms::sum<T>());
    T gpuStdDev = sqrt(gpuTotalVariance / inArrView.extent.size());

    // Now compute the result on the CPU for verification
    T cpuSum = 0.0;
    for (int i = 0; i < element_count; ++i) {
        cpuSum += inVec[i];
    }

    T cpuMean = cpuSum / element_count;
    T cpuTotalVariance = 0.0;
    for (int i = 0; i < element_count; ++i) {
        cpuTotalVariance += ((inVec[i] - cpuMean) * (inVec[i] - cpuMean));
    }

    T cpuStdDev = sqrt(cpuTotalVariance / element_count);

	// TODO: The results must be tested with some tolerance
    if (gpuStdDev != cpuStdDev) {
        std::cout << "test_functor_view: Failed. Expected: " << cpuStdDev << ", Actual: " << gpuStdDev << std::endl;
    }
    else {
        std::cout << "test_functor_view: Passed. Expected: " << cpuStdDev << ", Actual: " << gpuStdDev << std::endl;
    }
}

void test_generate()
{
	std::vector<int> vec(1024);
	array_view<int,1> av(1024, vec);
	av.discard_data();

	amp_algorithms::generate(av, [] () restrict(amp) {
		return 7;
	});
	av.synchronize();

	std::for_each(begin(vec), end(vec), [] (int element) {
		assert(element == 7);
	});
}

void test_unary_transform()
{
	const int height = 16;
	const int width = 16;
	const int size = height * width;

	std::vector<int> vec_in(size);
	std::fill(begin(vec_in), end(vec_in), 7);
	array_view<const int, 2> av_in(height, width, vec_in);

	std::vector<int> vec_out(size);
	array_view<int,2> av_out(height, width, vec_out);

	// Test "transform" by doubling the input elements

	amp_algorithms::transform(av_in, av_out, [] (int x) restrict(amp) {
		return 2 * x;
	});
	av_out.synchronize();

	std::for_each(begin(vec_out), end(vec_out), [] (int element) {
		assert(element == 2*7);
	});
}

void test_binary_transform()
{
	const int depth = 16;
	const int height = 16;
	const int width = 16;
	const int size = depth * height * width;

	std::vector<int> vec_in1(size);
	std::fill(begin(vec_in1), end(vec_in1), 343);
	array_view<const int, 3> av_in1(depth, height, width, vec_in1);

	std::vector<int> vec_in2(size);
	std::fill(begin(vec_in2), end(vec_in2), 323);
	array_view<const int, 3> av_in2(depth, height, width, vec_in2);

	std::vector<int> vec_out(size);
	array_view<int, 3> av_out(depth, height, width, vec_out);

	// Test "transform" by adding the two input elements

	amp_algorithms::transform(av_in1, av_in2, av_out, [] (int x1, int x2) restrict(amp) {
		return x1 + x2;
	});
	av_out.synchronize();

	std::for_each(begin(vec_out), end(vec_out), [] (int element) {
		assert(element == 343 + 323);
	});
}

void test_fill()
{
	std::vector<int> vec(1024);
	array_view<int> av(1024, vec);
	av.discard_data();

	amp_algorithms::fill(av, 7);
	av.synchronize();

	std::for_each(begin(vec), end(vec), [] (int element) {
		assert(element == 7);
	});
}



int main()
{
    //test_indexable_view_traits();
    test_reduce<double>(1023 * 1029 * 13, amp_algorithms::sum<double>(), "Test sum reduction double");
    test_reduce<int>(1023 * 1029 * 13, amp_algorithms::min<int>(), "Test min reduction int");
    test_reduce<float>(1023 * 1029 * 5, amp_algorithms::max<float>(), "Test max reduction float");

    test_functor_view<float>();

	test_generate();
	test_unary_transform();
	test_binary_transform();
	test_fill();

    return 0;
}
