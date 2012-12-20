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
#include <CppUnitTest.h>

#include "test_amp.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace concurrency;
using namespace amp_algorithms;

// TODO: Add tests for indexable_view_traits

namespace amp_algorithms_tests
{
    TEST_CLASS(reduce_tests)
    {
        TEST_METHOD(amp_reduce_double_sum)
        {
            double cpu_result, amp_result;

            test_reduce<double>(1023 * 1029 * 13, amp_algorithms::sum<double>(), cpu_result, amp_result);

            Assert::AreEqual(cpu_result, amp_result, 0.000001);
        }

        TEST_METHOD(amp_reduce_int_min)
        {
            int cpu_result, amp_result;

            test_reduce<int>(1023 * 1029 * 13, amp_algorithms::min<int>(), cpu_result, amp_result);

            Assert::AreEqual(cpu_result, amp_result);
        }

        TEST_METHOD(amp_reduce_float_max)
        {
            float cpu_result, amp_result;

            test_reduce<float>(1023 * 1029 * 5, amp_algorithms::max<float>(), cpu_result, amp_result);

            Assert::AreEqual(cpu_result, amp_result);
        }

    private:
        template <typename value_type, typename BinaryFunctor>
        void test_reduce(int element_count, BinaryFunctor func, value_type& cpu_result, value_type& amp_result)
        {
            std::vector<value_type> inVec(element_count);
            generate_data(inVec);

            array_view<const value_type> inArrView(element_count, inVec);
            amp_result = amp_algorithms::reduce(inArrView, func);

            // Now compute the result on the CPU for verification
            cpu_result = inVec[0];
            for (int i = 1; i < element_count; ++i) {
                cpu_result = func(cpu_result, inVec[i]);
            }
        }
    };

    TEST_CLASS(functor_view_tests)
    {
        TEST_METHOD(amp_functor_view_float)
        {
            float cpuStdDev, gpuStdDev;

            test_functor_view<float>(1023 * 31, cpuStdDev, gpuStdDev);

            Assert::IsTrue(compare(gpuStdDev, cpuStdDev));
        }

        TEST_METHOD(amp_functor_view_int)
        {
            int cpuStdDev, gpuStdDev;

            test_functor_view<int>(21, cpuStdDev, gpuStdDev);

            Assert::IsTrue(compare(gpuStdDev, cpuStdDev));
        }

    private:
        template <typename T>
        void test_functor_view(const int element_count, T& cpuStdDev, T& gpuStdDev)
        {
            std::vector<T> inVec(element_count);
            generate_data(inVec);

            array_view<const T> inArrView(element_count, inVec);

            // The next 4 lines use the functor_view together with the reduce algorithm to obtain the 
            // standard deviation of a set of numbers.
            T gpuSum = amp_algorithms::reduce(accelerator().create_view(), inArrView, amp_algorithms::sum<T>());
            T gpuMean = gpuSum / inArrView.extent.size();

            auto funcView = make_indexable_view(inArrView.extent, [inArrView, gpuMean](const concurrency::index<1> &idx) restrict(cpu, amp) {
                return ((inArrView(idx) - gpuMean) * (inArrView(idx) - gpuMean));
            });

            T gpuTotalVariance = amp_algorithms::reduce(funcView, amp_algorithms::sum<T>());
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

    TEST_CLASS(generate_tests)
    {
        TEST_METHOD(amp_generate_int)
        {
            std::vector<int> vec(1024);
            array_view<int,1> av(1024, vec);
            av.discard_data();

            amp_algorithms::generate(av, [] () restrict(amp) {
                return 7;
            });
            av.synchronize();

            for (auto e : vec)
            {
                Assert::AreEqual(7, e);
            }
        }
    };

    TEST_CLASS(transform_tests)
    {
        TEST_METHOD(amp_transform_unary)
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

            for (auto e : vec_out)
            {
                Assert::AreEqual(2 * 7, e);
            }
        }

        TEST_METHOD(amp_transform_binary)
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

            for (auto e : vec_out)
            {
                Assert::AreEqual(343 + 323, e);
            }
        }
    };

    TEST_CLASS(fill_tests)
    {
        TEST_METHOD(amp_fill_int)
        {
            std::vector<int> vec(1024);
            array_view<int> av(1024, vec);
            av.discard_data();

            amp_algorithms::fill(av, 7);
            av.synchronize();

            for (auto e : vec) 
            {
                Assert::AreEqual(7, e);
            }
        }
    };
};
