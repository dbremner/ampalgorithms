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

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return  RUN_ALL_TESTS();
}

// TODO: Add tests for indexable_view_traits
class amp_algorithms_tests : public stl_algorithms_testbase<13>, public ::testing::Test {};

TEST_F(amp_algorithms_tests, amp_padded_read)
{
    ASSERT_EQ(input[1], padded_read(input_av, concurrency::index<1>(1)));
    ASSERT_EQ(int(), padded_read(input_av, concurrency::index<1>(size + 2)));
}

TEST_F(amp_algorithms_tests, amp_padded_write)
{
    std::fill(begin(input), end(input), 0);

    padded_write(input_av, concurrency::index<1>(1), 11);
    ASSERT_EQ(11, input_av[1]);
    padded_write(input_av, concurrency::index<1>(size + 2), 11);
}

    //TEST_CLASS(amp_reduce_tests)
    //{

    //    /*
    //    TEST_METHOD_CATEGORY(amp_reduce_double_sum)
    //    {
    //        if (!accelerator().get_supports_double_precision())
    //        {
    //            Assert::Fail(L"Accelerator does not support double precision, skipping test.");
    //        }

    //        double cpu_result, amp_result;

    //        test_reduce<double>(1023 * 1029 * 13, amp_algorithms::plus<double>(), cpu_result, amp_result);

    //       ASSERT_EQ(cpu_result, amp_result, 0.000001);
    //    }
    //    */

    //    TEST_METHOD_CATEGORY(amp_reduce_int_min, "amp")
    //    {
    //        int cpu_result, amp_result;

    //        test_reduce<int>(test_array_size<int>(), amp_algorithms::min<int>(), cpu_result, amp_result);

    //       ASSERT_EQ(cpu_result, amp_result);
    //    }

    //    TEST_METHOD_CATEGORY(amp_reduce_float_max, "amp")
    //    {
    //        float cpu_result, amp_result;

    //        test_reduce<float>(test_array_size<float>(), amp_algorithms::max<float>(), cpu_result, amp_result);

    //       ASSERT_EQ(cpu_result, amp_result);
    //    }

    //private:
    //    template <typename value_type, typename BinaryFunctor>
    //    void test_reduce(int element_count, BinaryFunctor func, value_type& cpu_result, value_type& amp_result)
    //    {          
    //        std::vector<value_type> inVec(element_count);
    //        generate_data(inVec);

    //        array_view<const value_type> inArrView(element_count, inVec);
    //        amp_result = amp_algorithms::reduce(inArrView, func);

    //        // Now compute the result on the CPU for verification
    //        cpu_result = inVec[0];
    //        for (int i = 1; i < element_count; ++i) {
    //            cpu_result = func(cpu_result, inVec[i]);
    //        }
    //    }
    //};

    //TEST_CLASS(amp_functor_view_tests)
    //{

    //    TEST_METHOD_CATEGORY(amp_functor_view_float, "amp")
    //    {
    //        float cpuStdDev, gpuStdDev;

    //        test_functor_view<float>(test_array_size<float>(), cpuStdDev, gpuStdDev);

    //        Assert::IsTrue(compare(gpuStdDev, cpuStdDev));
    //    }

    //    TEST_METHOD_CATEGORY(amp_functor_view_int, "amp")
    //    {
    //        int cpuStdDev, gpuStdDev;

    //        test_functor_view<int>(21, cpuStdDev, gpuStdDev);

    //        Assert::IsTrue(compare(gpuStdDev, cpuStdDev));
    //    }

    //private:
    //    template <typename T>
    //    void test_functor_view(const int element_count, T& cpuStdDev, T& gpuStdDev)
    //    {
    //        std::vector<T> inVec(element_count);
    //        generate_data(inVec);

    //        array_view<const T> inArrView(element_count, inVec);

    //        // The next 4 lines use the functor_view together with the reduce algorithm to obtain the 
    //        // standard deviation of a set of numbers.
    //        T gpuSum = amp_algorithms::reduce(accelerator().create_view(), inArrView, amp_algorithms::plus<T>());
    //        T gpuMean = gpuSum / inArrView.extent.size();

    //        auto funcView = make_indexable_view(inArrView.extent, [inArrView, gpuMean](const concurrency::index<1> &idx) restrict(cpu, amp) {
    //            return ((inArrView(idx) - gpuMean) * (inArrView(idx) - gpuMean));
    //        });

    //        T gpuTotalVariance = amp_algorithms::reduce(funcView, amp_algorithms::plus<T>());
    //        gpuStdDev = static_cast<T>(sqrt(gpuTotalVariance / inArrView.extent.size()));

    //        // Now compute the result on the CPU for verification
    //        T cpuSum = inVec[0];
    //        for (int i = 1; i < element_count; ++i) 
    //        {
    //            cpuSum += inVec[i];
    //        }

    //        T cpuMean = cpuSum / element_count;
    //        T cpuTotalVariance = T(0);
    //        for (auto e : inVec)
    //        {
    //            cpuTotalVariance += ((e - cpuMean) * (e - cpuMean));
    //        }

    //        cpuStdDev = static_cast<T>(sqrt(cpuTotalVariance / element_count));
    //    }
    //};

    //TEST_CLASS(amp_generate_tests)
    //{

    //    TEST_METHOD_CATEGORY(amp_generate_int, "amp")
    //    {
    //        std::vector<int> vec(1024);
    //        array_view<int,1> av(1024, vec);
    //        av.discard_data();

    //        amp_algorithms::generate(av, [] () restrict(amp) {
    //            return 7;
    //        });
    //        av.synchronize();

    //        for (auto e : vec)
    //        {
    //           ASSERT_EQ(7, e);
    //        }
    //    }
    //};

    //TEST_CLASS(amp_transform_tests)
    //{

    //    TEST_METHOD_CATEGORY(amp_transform_unary, "amp")
    //    {
    //        const int height = 16;
    //        const int width = 16;
    //        const int size = height * width;

    //        std::vector<int> vec_in(size);
    //        std::fill(begin(vec_in), end(vec_in), 7);
    //        array_view<const int, 2> av_in(height, width, vec_in);

    //        std::vector<int> vec_out(size);
    //        array_view<int,2> av_out(height, width, vec_out);

    //        // Test "transform" by doubling the input elements

    //        amp_algorithms::transform(av_in, av_out, [] (int x) restrict(amp) {
    //            return 2 * x;
    //        });
    //        av_out.synchronize();

    //        for (auto e : vec_out)
    //        {
    //           ASSERT_EQ(2 * 7, e);
    //        }
    //    }

    //    TEST_METHOD_CATEGORY(amp_transform_binary, "amp")
    //    {
    //        const int depth = 16;
    //        const int height = 16;
    //        const int width = 16;
    //        const int size = depth * height * width;

    //        std::vector<int> vec_in1(size);
    //        std::fill(begin(vec_in1), end(vec_in1), 343);
    //        array_view<const int, 3> av_in1(depth, height, width, vec_in1);

    //        std::vector<int> vec_in2(size);
    //        std::fill(begin(vec_in2), end(vec_in2), 323);
    //        array_view<const int, 3> av_in2(depth, height, width, vec_in2);

    //        std::vector<int> vec_out(size);
    //        array_view<int, 3> av_out(depth, height, width, vec_out);

    //        // Test "transform" by adding the two input elements

    //        amp_algorithms::transform(av_in1, av_in2, av_out, [] (int x1, int x2) restrict(amp) {
    //            return x1 + x2;
    //        });
    //        av_out.synchronize();

    //        for (auto e : vec_out)
    //        {
    //           ASSERT_EQ(343 + 323, e);
    //        }
    //    }
    //};

    //TEST_CLASS(amp_fill_tests)
    //{

    //    TEST_METHOD_CATEGORY(amp_fill_int, "amp")
    //    {
    //        std::vector<int> vec(1024);
    //        array_view<int> av(1024, vec);
    //        av.discard_data();

    //        amp_algorithms::fill(av, 7);
    //        av.synchronize();

    //        for (auto e : vec) 
    //        {
    //           ASSERT_EQ(7, e);
    //        }
    //    }
    //};
