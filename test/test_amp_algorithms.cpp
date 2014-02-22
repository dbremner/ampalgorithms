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
#include <amp.h>
#include <amp_algorithms.h>
#include "test_amp.h"

// Code coverage is optional and requires VS Premium or Ultimate.
#ifdef CODECOVERAGE
#pragma managed(push, off)
ExcludeFromCodeCoverage(exclude_amp_algorithms_tests, L"amp_algorithms_tests::*");
ExcludeFromCodeCoverage(exclude_test_tools, L"test_tools::*")
ExcludeFromCodeCoverage(exclude_wrl, L"Microsoft::WRL::*")
#pragma managed(pop)
#endif

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace concurrency;
using namespace amp_algorithms;
using namespace test_tools;

// TODO: Add tests for indexable_view_traits

namespace amp_algorithms_details_tests
{
    // This tests an inlined function so don't exclude this from coverage.
    TEST_CLASS(details_tests)
    {
        TEST_METHOD(amp_details_check_hresult)
        {
            try
            {
                amp_algorithms::_details::_check_hresult(E_FAIL, "Failed!");
            }
            catch (runtime_exception& ex)
            {
                Assert::AreEqual(E_FAIL, ex.get_error_code());
                Assert::AreEqual("Failed! 0x80004005.", ex.what());
            }
        }
    };
};

namespace amp_algorithms_tests
{
    // This isn't a test, it's just a convenient way to determine which accelerator tests ran on.
    TEST_CLASS(configuration_tests)
    {
        TEST_CLASS_INITIALIZE(initialize_tests)
        {
            set_default_accelerator();
        }

        TEST_METHOD(amp_accelerator_configuration)
        {
            Logger::WriteMessage("Running amp_algorithms_tests on:");
            Logger::WriteMessage(accelerator().description.c_str());
            Logger::WriteMessage("\n  ");
            Logger::WriteMessage(accelerator().device_path.c_str());
            Logger::WriteMessage("\n");
        }
    };

    TEST_CLASS(padded_read_write_tests)
    {
        TEST_CLASS_INITIALIZE(initialize_tests)
        {
            set_default_accelerator();
        }

        TEST_METHOD(amp_padded_read)
        {
            std::array<int, 10> vec = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
            array_view<int> av(10, vec);

            Assert::AreEqual(1, padded_read(av, concurrency::index<1>(1)));
            Assert::AreEqual(int(), padded_read(av, concurrency::index<1>(20)));
        }

        TEST_METHOD(amp_padded_write)
        {
            std::array<int, 10> vec = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
            array_view<int> av(5, vec);

            padded_write(av, concurrency::index<1>(1), 11);
            Assert::AreEqual(11, av[1]);
            padded_write(av, concurrency::index<1>(1), 11);
            Assert::AreEqual(int(), av[7]);
        }
    };

    TEST_CLASS(operator_tests)
    {
        TEST_CLASS_INITIALIZE(initialize_tests)
        {
            set_default_accelerator();
        }

        TEST_METHOD(amp_arithmetic_operators)
        {
            compare_operators(std::plus<int>(), amp_algorithms::plus<int>());
            compare_operators(std::minus<int>(), amp_algorithms::minus<int>());

            compare_operators(std::multiplies<int>(), amp_algorithms::multiplies<int>());
            compare_operators(std::divides<int>(), amp_algorithms::divides<int>());
            compare_operators(std::modulus<int>(), amp_algorithms::modulus<int>());

            Assert::AreEqual(2, amp_algorithms::negates<int>()(-2));
            Assert::AreEqual(-2, amp_algorithms::negates<int>()(2));
            Assert::AreEqual(0, amp_algorithms::negates<int>()(0));
        }

        TEST_METHOD(amp_comparison_operators)
        {
            // Clear up std::min & std::max overload ambiguity.
            int const & (*min) (int const &, int const &) = std::min<int>;
            int const & (*max) (int const &, int const &) = std::max<int>;

            compare_operators(std::equal_to<int>(), amp_algorithms::equal_to<int>());
            compare_operators(std::not_equal_to<int>(), amp_algorithms::not_equal_to<int>());

            compare_operators(std::greater<int>(), amp_algorithms::greater<int>());
            compare_operators(std::less<int>(), amp_algorithms::less<int>());
            compare_operators(std::greater_equal<int>(), amp_algorithms::greater_equal<int>());
            compare_operators(std::less_equal<int>(), amp_algorithms::less_equal<int>());

            compare_operators(min, amp_algorithms::min<int>());
            compare_operators(max, amp_algorithms::max<int>());
        }

        TEST_METHOD(amp_bitwise_operators)
        {
            compare_operators(std::bit_and<int>(), amp_algorithms::bit_and<int>());
            compare_operators(std::bit_or<int>(), amp_algorithms::bit_or<int>());
            compare_operators(std::bit_xor<int>(), amp_algorithms::bit_xor<int>());
        }

        TEST_METHOD(amp_logical_operators)
        {
            std::array<unsigned, 4> tests = { 0xF0, 0xFF, 0x0A, 0x00 };

            for (auto& v : tests)
            {
                Assert::AreEqual(std::logical_not<unsigned>()(v), amp_algorithms::logical_not<unsigned>()(v));
            }
            compare_logical_operators(std::logical_and<unsigned>(), amp_algorithms::logical_and<unsigned>());
            compare_logical_operators(std::logical_or<unsigned>(), amp_algorithms::logical_or<unsigned>());
        }

        template<typename StlFunc, typename AmpFunc>
        void compare_operators(StlFunc stl_func, AmpFunc amp_func)
        {
            typedef std::pair<int, int> test_pair;

            std::array<test_pair, 5> tests = { 
                test_pair(100, 100), 
                test_pair(150, 300), 
                test_pair(1000, -50), 
                test_pair(11, 12), 
                test_pair(-12, 33) 
            };

            for (auto& p : tests)
            {
                Assert::AreEqual(stl_func(p.first, p.second), amp_func(p.first, p.second));
            }
        }

        template<typename StlFunc, typename AmpFunc>
        void compare_logical_operators(StlFunc stl_func, AmpFunc amp_func)
        {
            typedef std::pair<unsigned int, unsigned int> test_pair;

            std::array<test_pair, 8> tests = { 
                test_pair(0xF, 0xF), 
                test_pair(0xFF, 0x0A), 
                test_pair(0x0A, 0xFF),
                test_pair(0xFF, 0x00),
                test_pair(0x00, 0x00)
            };

            for (auto& p : tests)
            {
                Assert::AreEqual(stl_func(p.first, p.second), amp_func(p.first, p.second));
            }
        }
    };

    TEST_CLASS(reduce_tests)
    {
        TEST_CLASS_INITIALIZE(initialize_tests)
        {
            set_default_accelerator();
        }

        /*
        TEST_METHOD(amp_reduce_double_sum)
        {
            if (!accelerator().get_supports_double_precision())
            {
                Assert::Fail(L"Accelerator does not support double precision, skipping test.");
            }

            double cpu_result, amp_result;

            test_reduce<double>(1023 * 1029 * 13, amp_algorithms::plus<double>(), cpu_result, amp_result);

            Assert::AreEqual(cpu_result, amp_result, 0.000001);
        }
        */

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
        TEST_CLASS_INITIALIZE(initialize_tests)
        {
            set_default_accelerator();
        }

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

    TEST_CLASS(generate_tests)
    {
        TEST_CLASS_INITIALIZE(initialize_tests)
        {
            set_default_accelerator();
        }

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
        TEST_CLASS_INITIALIZE(initialize_tests)
        {
            set_default_accelerator();
        }

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
        TEST_CLASS_INITIALIZE(initialize_tests)
        {
            set_default_accelerator();
        }

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
