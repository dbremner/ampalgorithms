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
#include <amp_stl_algorithms.h>
#include <CppUnitTest.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace concurrency;
using namespace amp_stl_algorithms;

//  Define these namespaces and classes to pick up poorly specified namespaces and types in library code.
//  This makes the test code more like a real library client which may define conflicting namespaces etc.
namespace details { };
namespace direct3d { };
namespace fast_math { };
namespace graphics { };
namespace precise_math { };
class extent { };

namespace tests
{
    TEST_CLASS(stl_algorithms_tests)
    {
        TEST_METHOD(stl_for_each_no_return)
        {
            std::vector<int> vec(1024);
            std::fill(vec.begin(), vec.end(), 2);
            array_view<const int> av(1024, vec);
            int sum = 0;
            array_view<int> av_sum(1, &sum);
            amp_stl_algorithms::for_each_no_return(begin(av), end(av), [av_sum] (int val) restrict(amp) {
                atomic_fetch_add(&av_sum(0), val);
            });
            av_sum.synchronize();
            Assert::AreEqual(1024 * 2, sum);
        }

        TEST_METHOD(stl_find)
        {
            static const int numbers[] = {1 , 3, 6, 3, 2, 2 };
            static const int n = sizeof(numbers)/sizeof(numbers[0]);

            array_view<const int> av(concurrency::extent<1>(n), numbers);
            auto iter = amp_stl_algorithms::find(begin(av), end(av), 3);
            int position = std::distance(begin(av), iter);
            Assert::AreEqual(1, position);

            iter = amp_stl_algorithms::find(begin(av), end(av), 17);
            Assert::IsTrue(end(av) == iter);
        }

        TEST_METHOD(stl_none_of)
        {
            static const int numbers[] = {1 , 3, 6, 3, 2, 2 };
            static const int n = sizeof(numbers)/sizeof(numbers[0]);

            array_view<const int> av(concurrency::extent<1>(n), numbers);
            bool r1 = amp_stl_algorithms::none_of(begin(av), end(av), [] (int v) restrict(amp) -> bool { return v>10; });
            Assert::IsTrue(r1);
            bool r2 = amp_stl_algorithms::none_of(begin(av), end(av), [] (int v) restrict(amp) -> bool { return v>5; });
            Assert::IsFalse(r2);
        }

        TEST_METHOD(stl_any_of)
        {
            static const int numbers[] = {1 , 3, 6, 3, 2, 2 };
            static const int n = sizeof(numbers)/sizeof(numbers[0]);

            array_view<const int> av(concurrency::extent<1>(n), numbers);
            bool r1 = amp_stl_algorithms::any_of(begin(av), end(av), [] (int v) restrict(amp) -> bool { return v>10; });
            Assert::IsFalse(r1);
            bool r2 = amp_stl_algorithms::any_of(begin(av), end(av), [] (int v) restrict(amp) -> bool { return v>5; });
            Assert::IsTrue(r2);
        }

        TEST_METHOD(stl_all_of)
        {
            static const int numbers[] = {1 , 3, 6, 3, 2, 2 };
            static const int n = sizeof(numbers)/sizeof(numbers[0]);

            array_view<const int> av(concurrency::extent<1>(n), numbers);
            bool r1 = amp_stl_algorithms::all_of(begin(av), end(av), [] (int v) restrict(amp) -> bool { return v>10; });
            Assert::IsFalse(r1);
            bool r2 = amp_stl_algorithms::all_of(begin(av), end(av), [] (int v) restrict(amp) -> bool { return v>5; });
            Assert::IsFalse(r2);
        }

        TEST_METHOD(stl_count)
        {
            static const int numbers[] = {1 , 3, 6, 3, 2, 2, 7, 8, 2, 9, 2, 19, 2};
            static const int n = sizeof(numbers)/sizeof(numbers[0]);
            array_view<const int> av(concurrency::extent<1>(n), numbers);
            auto r1 = amp_stl_algorithms::count(begin(av), end(av), 2);
            Assert::AreEqual(5, r1);
            auto r2 = amp_stl_algorithms::count(begin(av), end(av), 17);
            Assert::AreEqual(0, r2);
        }

        TEST_METHOD(stl_begin_end_array_view)
        {
            std::vector<int> v1(6);
            array_view<int> a1(6, v1);
            auto iter1 = begin(a1);

            array_view<const int> ar1 = a1;
            auto iter2 = begin(ar1);

            auto iter3 = iter1++;
            auto iter4 = ++iter1;
            auto iter5 = iter3 + 7;
            bool res = iter3 < iter4;
            Assert::IsTrue(res);
        }

        // TODO: Break this up into smaller tests?

        TEST_METHOD(stl_random_access_iterator)
        {
            std::vector<int> v1(16);
            array_view<int> a1(16, v1);

            // can be default constructed.
            array_view_iterator<int> iter1; 
            auto iter2 = array_view_iterator<double>();

            // can be copy constructed
            array_view_iterator<int> iter3 = begin(a1);
            array_view_iterator<int> iter4(iter3);
            array_view_iterator<int> iter5 = iter4;

            // assignment
            iter5 = iter3;

            // equality/inequality comparisons
            bool res = iter3 == iter5;
            Assert::IsTrue(res);
            iter3++;
            res = iter3 != iter4;
            Assert::IsTrue(res);

            // dereference
            *iter3 = 10;
            Assert::AreEqual(10, a1[1]);

            // offset derference operator;
            iter3[2] = 5;
            Assert::AreEqual(5, a1[1 + 2]);

            // increment, decrement, + , -, +=, -=
            auto iter6 = iter3;
            auto iter7 = iter3;
            iter6++;
            iter6 = iter6 + 1;
            iter7 += 2;
            Assert::IsTrue(iter6 == iter7);
            --iter6;
            --iter6;
            iter7 = iter7 - 2;
            Assert::IsTrue(iter6 == iter7);
            iter7 = iter7 - 1;
            iter6 -= 1;
            Assert::IsTrue(iter6 == iter7);

            // <, >, <= >=
            iter6 = iter3;
            iter7 = iter3 + 1;
            Assert::IsTrue(iter6 < iter7);
            Assert::IsTrue(iter6 <= iter7);
            Assert::IsTrue(iter7 > iter6);
            Assert::IsTrue(iter7 >= iter6);

            // *i++
            iter6 = begin(a1);
            *iter6 = 3;
            Assert::AreEqual(3, a1[0]);
            int x1 = *iter6++;
            Assert::AreEqual(3, x1);
            *iter6++ = 7;
            Assert::AreEqual(7, a1[1]);
        }

        // TODO: Break this up into smaller tests?

        TEST_METHOD(stl_random_access_iterator_in_amp)
        {
            int id = 1;
            std::vector<int> v1(16);
            array_view<int> a1(16, v1);
            std::vector<int> v2(16);
            array_view<int> result(16, v2);
            result.discard_data();
            parallel_for_each(concurrency::extent<1>(1), [=] (index<1> idx) restrict(amp) {
                int id = 1;

                // can be default constructed.
                array_view_iterator<int> iter1; 
                auto iter2 = array_view_iterator<double>();

                // can be copy constructed
                array_view_iterator<int> iter3 = begin(a1);
                array_view_iterator<int> iter4(iter3);
                array_view_iterator<int> iter5 = iter4;

                // assignment
                iter5 = iter3;

                // equality/inequality comparisons
                bool res = iter3 == iter5;
                result[id++] = res;
                iter3++;
                res = iter3 != iter4;
                result[id++] = res;

                // dereference
                *iter3 = 10;
                result[id++] = (a1[1] == 10);

                // offset derference operator;
                iter3[2] = 5;
                result[id++] = (a1[1 + 2] == 5);

                // increment, decrement, + , -, +=, -=
                auto iter6 = iter3;
                auto iter7 = iter3;
                iter6++;
                iter6 = iter6 + 1;
                iter7 += 2;
                result[id++] = (iter6 == iter7);
                --iter6;
                --iter6;
                iter7 = iter7 - 2;
                result[id++] = (iter6 == iter7);
                iter7 = iter7 - 1;
                iter6 -= 1;
                result[id++] = (iter6 == iter7);

                // <, >, <= >=
                iter6 = iter3;
                iter7 = iter3 + 1;
                result[id++] = (iter6 < iter7);
                result[id++] = (iter6 <= iter7);
                result[id++] = (iter7 > iter6);
                result[id++] = (iter7 >= iter6);

                // *i++
                iter6 = begin(a1);
                *iter6 = 3;
                result[id++] = (a1[0] == 3);
                int x1 = *iter6++;
                result[id++] = (x1 == 3);
                *iter6++ = 7;
                result[id++] = (a1[1] == 7);
                result[0] = id - 1;
            });
            result.synchronize();
            Assert::IsTrue(v2[0] <= (int)v2.size() - 1);
            for (int i = 0; i < v2[0]; i++)
            {
                Assert::AreEqual(1, v2[1 + i]);
            }
        }

        TEST_METHOD(stl_generate)
        {
            std::vector<int> vec(1024);

            // Generate using an array_view over the vector. Requires explicit synchronize.
            array_view<int> av(1024, vec);
            av.discard_data();

            amp_stl_algorithms::generate(begin(av), end(av), [] () restrict(amp) {
                return 7;
            });
            av.synchronize();

            for (auto element : vec)
            {
                Assert::AreEqual(7, element);
            }
        }

        TEST_METHOD(stl_generate_n)
        {
            std::vector<int> vec(1024);
            array_view<int> av(1024, vec);
            av.discard_data();

            amp_stl_algorithms::generate_n(begin(av), av.extent.size(), [] () restrict(amp) {
                return 616;
            });
            av.synchronize();

            for (auto element : vec)
            {
                Assert::AreEqual(616, element);
            }
        }

        TEST_METHOD(stl_unary_transform)
        {
            const int size = 1024;
            std::vector<int> vec_in(size);
            std::fill(begin(vec_in), end(vec_in), 7);
            array_view<const int> av_in(size, vec_in);

            std::vector<int> vec_out(size);
            array_view<int> av_out(size, vec_out);

            // Test "transform" by doubling the input elements

            amp_stl_algorithms::transform(begin(av_in), end(av_in), begin(av_out), [] (int x) restrict(amp) {
                return 2 * x;
            });
            av_out.synchronize();

            for (auto element : vec_out)
            {
                Assert::AreEqual(2 * 7, element);
            }
        }

        TEST_METHOD(stl_binary_transform)
        {
            const int size = 1024;

            std::vector<int> vec_in1(size);
            std::fill(begin(vec_in1), end(vec_in1), 343);
            array_view<const int> av_in1(size, vec_in1);

            std::vector<int> vec_in2(size);
            std::fill(begin(vec_in2), end(vec_in2), 323);
            array_view<const int> av_in2(size, vec_in2);

            std::vector<int> vec_out(size);
            array_view<int> av_out(size, vec_out);

            // Test "transform" by adding the two input elements

            amp_stl_algorithms::transform(begin(av_in1), end(av_in1), begin(av_in2), begin(av_out), [] (int x1, int x2) restrict(amp) {
                return x1 + x2;
            });
            av_out.synchronize();

            for (auto element : vec_out)
            {
                Assert::AreEqual(343 + 323, element);
            }
        }

        TEST_METHOD(stl_fill)
        {
            std::vector<int> vec(1024);

            // Fill using an array_view iterator
            array_view<int> av(1024, vec);
            av.discard_data();

            amp_stl_algorithms::fill(begin(av), end(av), 7);
            av.synchronize();

            for (auto element : vec)
            {
                Assert::AreEqual(7, element);
            }
        }

        TEST_METHOD(stl_fill_n)
        {
            std::vector<int> vec(1024);
            array_view<int> av(1024, vec);
            av.discard_data();

            amp_stl_algorithms::fill_n(begin(av), av.extent.size(), 616);
            av.synchronize();

            for (auto element : vec)
            {
                Assert::AreEqual(616, element);
            }
        }

        TEST_METHOD(stl_reduce_sum)
        {
            static const int numbers[] = {1, 3, 6, 3, 2, 2, 7, 8, 2, 9, 2, 19, 2};
            static const int n = sizeof(numbers)/sizeof(numbers[0]);
            array_view<const int> av(concurrency::extent<1>(n), numbers);
            auto result = amp_stl_algorithms::reduce(begin(av), end(av), 0);
            Assert::AreEqual(66, result);
        }

        TEST_METHOD(stl_reduce_max)
        {
            static const int numbers[] = {1, 3, 6, 3, 2, 2, 7, 8, 2, 9, 2, 19, 2};
            static const int n = sizeof(numbers)/sizeof(numbers[0]);
            array_view<const int> av(concurrency::extent<1>(n), numbers);
            auto result = amp_stl_algorithms::reduce(begin(av), end(av), 0, [](int a, int b) restrict(cpu, amp) {
                return (a < b) ? b : a;
            });
            Assert::AreEqual(19, result);
        }
    };
};