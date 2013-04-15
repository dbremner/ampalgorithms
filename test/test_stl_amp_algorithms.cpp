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

#include <amp_stl_algorithms.h>
#include "test_amp.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace concurrency;
using namespace amp_stl_algorithms;

namespace tests
{
	// This isn't a test, it's just a convenient way to determine which accelerator tests ran on.
	TEST_CLASS(configuration_tests)
	{
		TEST_CLASS_INITIALIZE(initialize_tests)
		{
			set_default_accelerator();
		}

		TEST_METHOD(stl_accelerator_configuration)
		{
			Logger::WriteMessage("Running stl_algorithms_tests on:");
			Logger::WriteMessage(accelerator().description.c_str());
			Logger::WriteMessage("\n  ");
			Logger::WriteMessage(accelerator().device_path.c_str());
			Logger::WriteMessage("\n");
		}
	};
	
	// TODO: Get the tests, header and internal implementations into the same logical order.

    TEST_CLASS(stl_algorithms_tests)
    {
		TEST_CLASS_INITIALIZE(initialize_tests)
		{
			set_default_accelerator();
		}

        TEST_METHOD(stl_for_each_no_return)
        {
            std::vector<int> vec(1024);
            std::fill(vec.begin(), vec.end(), 2);
            array_view<const int> av(1024, vec);
            int sum = 0;
            array_view<int> av_sum(1, &sum);
            amp_stl_algorithms::for_each_no_return(begin(av), end(av), [av_sum] (int val) restrict(amp) {
                concurrency::atomic_fetch_add(&av_sum(0), val);
            });
            av_sum.synchronize();
            Assert::AreEqual(1024 * 2, sum);
        }

        //----------------------------------------------------------------------------
        // find, find_if, find_if_not, find_end, find_first_of, adjacent_find
        //----------------------------------------------------------------------------

        TEST_METHOD(stl_find)
        {
            static const int numbers[] = { 1, 3, 6, 3, 2, 2 };
            static const int n = sizeof(numbers)/sizeof(numbers[0]);

            array_view<const int> av(concurrency::extent<1>(n), numbers);
            auto iter = amp_stl_algorithms::find(begin(av), end(av), 3);
            int position = std::distance(begin(av), iter);
            Assert::AreEqual(1, position);

            iter = amp_stl_algorithms::find(begin(av), end(av), 17);
            Assert::IsTrue(end(av) == iter);
        }

        TEST_METHOD(stl_find_if)
        {
            static const int numbers[] = { 1, 3, 6, 3, 2, 2 };
            static const int n = sizeof(numbers)/sizeof(numbers[0]);

            array_view<const int> av(concurrency::extent<1>(n), numbers);
            auto iter = amp_stl_algorithms::find_if(begin(av), end(av), [=](int v) restrict(amp) { return v == 3; });
            int position = std::distance(begin(av), iter);
            Assert::AreEqual(1, position);

            iter = amp_stl_algorithms::find_if(begin(av), end(av), [=](int v) restrict(amp) { return v == 17; });
            Assert::IsTrue(end(av) == iter);
        }

        TEST_METHOD(stl_find_if_not)
        {
            static const int numbers[] = { 1, 3, 6, 3, 2, 2 };
            static const int n = sizeof(numbers)/sizeof(numbers[0]);

            array_view<const int> av(concurrency::extent<1>(n), numbers);
            auto iter = amp_stl_algorithms::find_if_not(begin(av), end(av), [=](int v) restrict(amp) { return v != 3; });
            int position = std::distance(begin(av), iter);
            Assert::AreEqual(1, position);

            iter = amp_stl_algorithms::find_if_not(begin(av), end(av), [=](int v) restrict(amp) { return v != 17; });
            Assert::IsTrue(end(av) == iter);
        }

        //----------------------------------------------------------------------------
        // all_of, any_of, none_of
        //----------------------------------------------------------------------------

        TEST_METHOD(stl_none_of)
        {
            static const int numbers[] = { 1, 3, 6, 3, 2, 2 };
            static const int n = sizeof(numbers)/sizeof(numbers[0]);

            array_view<const int> av(concurrency::extent<1>(n), numbers);
            bool r1 = amp_stl_algorithms::none_of(begin(av), end(av), [] (int v) restrict(amp) -> bool { return v>10; });
            Assert::IsTrue(r1);
            bool r2 = amp_stl_algorithms::none_of(begin(av), end(av), [] (int v) restrict(amp) -> bool { return v>5; });
            Assert::IsFalse(r2);
        }

        TEST_METHOD(stl_any_of)
        {
            static const int numbers[] = { 1, 3, 6, 3, 2, 2 };
            static const int n = sizeof(numbers)/sizeof(numbers[0]);

            array_view<const int> av(concurrency::extent<1>(n), numbers);
            bool r1 = amp_stl_algorithms::any_of(begin(av), end(av), [] (int v) restrict(amp) -> bool { return v>10; });
            Assert::IsFalse(r1);
            bool r2 = amp_stl_algorithms::any_of(begin(av), end(av), [] (int v) restrict(amp) -> bool { return v>5; });
            Assert::IsTrue(r2);
        }

        TEST_METHOD(stl_all_of)
        {
            static const int numbers[] = { 1, 3, 6, 3, 2, 2 };
            static const int n = sizeof(numbers)/sizeof(numbers[0]);

            array_view<const int> av(concurrency::extent<1>(n), numbers);
            bool r1 = amp_stl_algorithms::all_of(begin(av), end(av), [] (int v) restrict(amp) -> bool { return v>10; });
            Assert::IsFalse(r1);
            bool r2 = amp_stl_algorithms::all_of(begin(av), end(av), [] (int v) restrict(amp) -> bool { return v>5; });
            Assert::IsFalse(r2);
        }

        //----------------------------------------------------------------------------
        // copy, copy_if, copy_n, copy_backward
        //----------------------------------------------------------------------------

        TEST_METHOD(stl_copy)
        {
            test_copy(1024);
        }

        void test_copy(const int size)
        {
            std::vector<int> vec(size);
            std::iota(begin(vec), end(vec), 1);
            array_view<int> av(size, vec);

            std::vector<int> result(size, 0);
            array_view<int> result_av(size, result);

            auto result_end = amp_stl_algorithms::copy(begin(av), end(av), begin(result));

            Assert::IsTrue(are_equal(av, result));
            Assert::AreEqual(result_av[size - 1], *--result_end);
        }

        TEST_METHOD(stl_copy_if)
        {
            // These tests copy all the non-zero elements from the numbers array.

            const std::array<int, 5> numbers = { 0, 0, 0, 0, 0 };
            test_copy_if(begin(numbers), end(numbers));

            const std::array<int, 5> numbers0 = { 3, 0, 0, 0, 0 };
            test_copy_if(begin(numbers0), end(numbers0));

            const std::array<float, 5> numbers1 = { 0.0f, 0.0f, 0.0f, 0.0f,3.0f };
            test_copy_if(begin(numbers1), end(numbers1));

            const std::array<int, 12> numbers2 = { -1, 1, 0, 2, 3, 0, 4, 0, 5, 0, 6, 7 };
            //  predicate result:                   1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1
            //  Exclusive scan result:              0, 1, 2, 2, 3, 4, 4, 5, 5, 6, 6, 7, 8
            //  Final result:                      -1, 1, 2, 3, 4, 5, 6, 7
            test_copy_if(begin(numbers2), end(numbers2));

            const std::array<int, 5> numbers3 = { 0, 0, 0, 0, 0 };
            test_copy_if(begin(numbers3), end(numbers3));

#ifdef _DEBUG
            std::vector<int> numbers4(1023);
#else
            std::vector<int> numbers4(1023 * 1029 * 13);
#endif
            generate_data(numbers4);
            test_copy_if(begin(numbers4), end(numbers4));
        }

        template <typename InIt>
        void test_copy_if(InIt first, InIt last)
        {
            typedef typename std::iterator_traits<InIt>::value_type T;
            int size = int(std::distance(first, last));

            // Calculate expected result for copy all non-zeros.

            std::vector<T> expected(size, -42);
            auto expected_end = std::copy_if(first, last, begin(expected), [=] (const T i)
            { 
                return (i != 0) ? 1 : 0; 
            });
            expected.resize(distance(begin(expected), expected_end));

            // Calculate actual result

            array_view<const T> input_av(concurrency::extent<1>(size), &first[0]);
            std::vector<T> result(size, -42);
            array_view<T> result_av(concurrency::extent<1>(size), result);
            auto dest_end = amp_stl_algorithms::copy_if(begin(input_av), 
                end(input_av), begin(result_av), [=] (const T i) restrict(amp) 
            { 
                return (i != 0) ? 1 : 0; 
            });
            result_av.synchronize();

            Assert::AreEqual(expected.size(), size_t(std::distance(begin(result_av), dest_end)));
            for (size_t i = 0; i < expected.size(); ++i)
            {
                Assert::AreEqual(expected[i], result[i]);
            }
        }

        TEST_METHOD(stl_copy_n)
        {
            const int size = 1023;
            std::vector<int> vec(size);
            std::iota(begin(vec), end(vec), 1);
            array_view<int> av(size, vec);

            std::vector<int> result(size, 0);
            array_view<int> result_av(size, result);

            auto result_end = amp_stl_algorithms::copy_n(begin(av), 512, begin(result));

            Assert::IsTrue(are_equal(av.section(0, 512), result_av.section(0, 512)));
            Assert::IsFalse(are_equal(av.section(512, 1), result_av.section(512, 2)));
            Assert::AreEqual(512, int(std::distance(begin(result), result_end)));
        }

        //----------------------------------------------------------------------------
        // count, count_if
        //----------------------------------------------------------------------------

        TEST_METHOD(stl_count)
        {
            static const int numbers[] = {1, 3, 6, 3, 2, 2, 7, 8, 2, 9, 2, 19, 2};
            static const int n = sizeof(numbers)/sizeof(numbers[0]);
            array_view<const int> av(concurrency::extent<1>(n), numbers);
            auto r1 = amp_stl_algorithms::count(begin(av), end(av), 2);
            Assert::AreEqual(5, r1);
            auto r2 = amp_stl_algorithms::count(begin(av), end(av), 17);
            Assert::AreEqual(0, r2);
        }

        TEST_METHOD(stl_count_if)
        {
            static const int numbers[] = {1, 3, 6, 3, 2, 2, 7, 8, 2, 9, 2, 19, 2};
            static const int n = sizeof(numbers)/sizeof(numbers[0]);
            array_view<const int> av(concurrency::extent<1>(n), numbers);
            auto r1 = amp_stl_algorithms::count_if(begin(av), end(av), [=](const int& v) restrict(amp) { return (v == 2); });
            Assert::AreEqual(5, r1);
            auto r2 = amp_stl_algorithms::count_if(begin(av), end(av), [=](const int& v) restrict(amp) { return (v == 17); });
            Assert::AreEqual(0, r2);
        }

        //----------------------------------------------------------------------------
        // equal
        //----------------------------------------------------------------------------

        TEST_METHOD(stl_equal)
        {
            std::vector<int> vec1(1024);
            std::iota(begin(vec1), end(vec1), 1);
            array_view<int> av1(1024, vec1);
            std::vector<int> vec2(1024);
            std::iota(begin(vec2), end(vec2), 1);
            array_view<int> av2(1024, vec2);

            Assert::IsTrue(amp_stl_algorithms::equal(begin(av1), end(av1), begin(av2)));

            av2[512] = 1;

            Assert::IsFalse(amp_stl_algorithms::equal(begin(av1), end(av1), begin(av2)));

            av1 = av1.section(0, 512);

            Assert::IsTrue(amp_stl_algorithms::equal(begin(av1), end(av1), begin(av2)));
        }

        TEST_METHOD(stl_equal_pred)
        {
            std::vector<int> vec1(1024);
            std::iota(begin(vec1), end(vec1), 1);
            array_view<int> av1(1024, vec1);
            std::vector<int> vec2(1024);
            std::iota(begin(vec2), end(vec2), 1);
            array_view<int> av2(1024, vec2);

            auto pred = [=](int& v1, int& v2) restrict(amp) { return ((v1 + 1) == v2); };
            Assert::IsFalse(amp_stl_algorithms::equal(begin(av1), end(av1), begin(av2), pred));

            std::iota(begin(vec2), end(vec2), 2);
            av2.refresh();           

            Assert::IsTrue(amp_stl_algorithms::equal(begin(av1), end(av1), begin(av2), pred));
        }

        // TODO: These iterator tests are in the wrong place, move them.

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

        TEST_METHOD(stl_random_access_iterator_default_ctor)
        {
            array_view_iterator<int> iter1; 
            auto iter2 = array_view_iterator<double>();
        }

        TEST_METHOD(stl_random_access_iterator_copy_assignment_comparison)
        {
            std::vector<int> v1(16);
            array_view<int> a1(16, v1);

            // Copy constructor and assignment
            array_view_iterator<int> iter1 = begin(a1);
            array_view_iterator<int> iter2(iter1);
            array_view_iterator<int> iter3 = iter2;

            // Equality/inequality comparisons
            Assert::IsTrue(begin(a1) == iter1);  
            Assert::IsTrue(begin(a1) == iter2);  
            Assert::IsTrue(begin(a1) == iter3);
            iter1++;
            Assert::IsFalse(begin(a1) == iter1);
        }

        TEST_METHOD(stl_random_access_iterator_dereference)
        {
            std::vector<int> v1(16);
            array_view<int> a1(16, v1);
            array_view_iterator<int> iter = begin(a1);

            // dereference
            iter++;
            *iter = 10;
            Assert::AreEqual(10, a1[1]);

            // offset dereference operator
            iter[2] = 5;
            Assert::AreEqual(5, a1[1 + 2]);
        }

        TEST_METHOD(stl_random_access_iterator_increment_decrement)
        {
            std::vector<int> v1(16);
            array_view<int> a1(16, v1);
            array_view_iterator<int> iter1 = begin(a1);
            array_view_iterator<int> iter2 = begin(a1);

            iter1++;
            iter1 = iter1 + 1;
            iter2 += 2;
            Assert::IsTrue(iter1 == iter2);

            --iter1;
            --iter1;
            iter2 = iter2 - 2;
            Assert::IsTrue(iter1 == iter2);

            iter2 = iter2 - 1;
            iter1 -= 1;
            Assert::IsTrue(iter1 == iter2);
        }

        TEST_METHOD(stl_random_access_iterator_equality)
        {
            std::vector<int> v1(16);
            array_view<int> a1(16, v1);
            array_view_iterator<int> iter1 = begin(a1);
            array_view_iterator<int> iter2 = begin(a1) + 1;

            Assert::IsTrue(iter1 < iter2);
            Assert::IsTrue(iter1 <= iter2);
            Assert::IsTrue(iter2 > iter1);
            Assert::IsTrue(iter2 >= iter1);
        }

        TEST_METHOD(stl_random_access_iterator_increment)
        {
            std::vector<int> v1(16);
            array_view<int> a1(16, v1);
            array_view_iterator<int> iter = begin(a1);

            *iter = 3;
            Assert::AreEqual(3, a1[0]);
            int x1 = *iter++;
            Assert::AreEqual(3, x1);
            *iter++ = 7;
            Assert::AreEqual(7, a1[1]);
        }

        // TODO: Break this up into smaller tests?

        TEST_METHOD(stl_random_access_iterator_in_amp)
        {
            std::vector<int> v1(16);
            array_view<int> a1(16, v1);
            std::vector<int> v2(16);
            array_view<int> result(16, v2);
            result.discard_data();
            parallel_for_each(concurrency::extent<1>(1), [=] (concurrency::index<1> idx) restrict(amp) {
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

        //----------------------------------------------------------------------------
        // generate, generate_n
        //----------------------------------------------------------------------------
        
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

        //----------------------------------------------------------------------------
        // transform
        //----------------------------------------------------------------------------

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

        //----------------------------------------------------------------------------
        // fill, fill_n
        //----------------------------------------------------------------------------

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

        //----------------------------------------------------------------------------
        // iota
        //----------------------------------------------------------------------------

        TEST_METHOD(stl_iota)
        {
            const int size = 1024;
            std::vector<int> vec(size);
            array_view<int> av(size, vec);
            av.discard_data();

            amp_stl_algorithms::iota(begin(av), end(av), 0);

            for (unsigned i = 0; i < av.extent.size(); ++i)
            {
                Assert::AreEqual(int(i), av[i]);
            }
        }

        //----------------------------------------------------------------------------
        // reduce
        //----------------------------------------------------------------------------

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

        //----------------------------------------------------------------------------
        // remove, remove_if, remove_copy, remove_copy_if
        //----------------------------------------------------------------------------

        TEST_METHOD(stl_remove)
        {
            const int size = 10;
            std::array<int, (size - 1)> expected = { 0, 2, 3, 4, 5, 6, 7, 8, 9 };
            std::vector<int> vec(size);
            std::iota(begin(vec), end(vec), 0);
            array_view<int> av(size, vec);
 
            auto expected_end = amp_stl_algorithms::remove(begin(av), end(av), 1);
            av = av.section(0, std::distance(begin(av), expected_end));

            Assert::AreEqual(unsigned(size - 1), av.extent.size());
            Assert::IsTrue(are_equal(expected, av));
            Assert::AreEqual(9, *--expected_end);
        }

        TEST_METHOD(stl_remove_if)
        {
            // These tests remove all the non-zero elements from the numbers array.

            float numbers0[] = { 1, 1, 1, 1, 1 };
            test_remove_if(numbers0, numbers0 + (sizeof(numbers0) / sizeof(numbers0[0])));

            float numbers1[] = { 3, 0, 0, 0, 0 };
            test_remove_if(numbers1, numbers1 + (sizeof(numbers1) / sizeof(numbers1[0])));

            int numbers2[] = { 0, 0, 0, 0, 3 };
            test_remove_if(numbers2, numbers2 + (sizeof(numbers2) / sizeof(numbers2[0])));
            
            int numbers3[] =               { -1, 1, 0, 2, 3, 0, 4, 0, 5, 0, 6, 7 };
            //  Predicate result:             0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0
            //  Scan result:                  0, 0, 0, 1, 1, 1, 2, 2, 3, 3, 4, 4, 4
            //  Final result:                -1, 1, 2, 3, 4, 5, 6, 7
            test_remove_if(numbers3, numbers3 + (sizeof(numbers3) / sizeof(numbers3[0])));
            
#ifdef _DEBUG
            std::vector<int> numbers4(1023);
#else
            std::vector<int> numbers4(1023 * 1029 * 13);
#endif
            generate_data(numbers4);
            test_remove_if(begin(numbers4), end(numbers4));
        }

        template <typename InIt>
        void test_remove_if(InIt first, InIt last)
        {
            typedef typename std::iterator_traits<InIt>::value_type T;
            const int size = int(std::distance(first, last));

            // Calculate expected result, for remove all zeros.

            std::vector<T> expected(size);
            std::copy(first, last, begin(expected));
            auto expected_end = std::remove_if(begin(expected), end(expected), [=] (const T i)
            {
                return (i > 0) ? 1 : 0;
            });
            expected.resize(std::distance(begin(expected), expected_end));

            // Calculate actual result

            array_view<T> input_av(concurrency::extent<1>(size), &first[0]);
            auto result_end = amp_stl_algorithms::remove_if(begin(input_av), 
                end(input_av), [=] (const T i) restrict(amp) 
            { 
                return (i > 0) ? 1 : 0;
            });
            input_av.synchronize();

            Assert::AreEqual(expected.size(), size_t(distance(begin(input_av), result_end)));
            for (size_t i = 0; i < expected.size(); ++i)
            {
                Assert::AreEqual(expected[i], first[i]);
            }
        }

        TEST_METHOD(stl_remove_copy)
        {
            const int size = 10;
            std::array<int, 9> expected = { 0, 2, 3, 4, 5, 6, 7, 8, 9 };
            std::vector<int> vec(size);
            std::iota(begin(vec), end(vec), 0);
            array_view<int> av(size, vec);
            std::vector<int> result(size, 0);
            array_view<int> result_av(size, result);

            auto expected_end = amp_stl_algorithms::remove_copy(begin(av), end(av), begin(result_av), 1);

            Assert::AreEqual(int(expected.size()), std::distance(begin(result_av), expected_end));
            Assert::IsTrue(are_equal(expected, result_av.section(0, int(expected.size()))));
        }

        TEST_METHOD(stl_remove_copy_if)
        {
            const int size = 10;
            std::array<int, (6)> expected = { 0, 1, 2, 3, 4, 5 };
            std::vector<int> vec(size);
            std::iota(begin(vec), end(vec), 0);
            array_view<int> av(size, vec);
            std::vector<int> result(size, 0);
            array_view<int> result_av(size, result);

            auto expected_end = amp_stl_algorithms::remove_copy_if(begin(av), end(av), begin(result_av), [=](int& v) restrict(amp) { return (v > 5); });

            Assert::AreEqual(int(expected.size()), std::distance(begin(result_av), expected_end));
            Assert::IsTrue(are_equal(expected, result_av.section(0, int(expected.size()))));
        }

        //----------------------------------------------------------------------------
        // replace, replace_if, replace_copy, replace_copy_if
        //----------------------------------------------------------------------------
        
        TEST_METHOD(stl_replace)
        {
            const int size = 10;
            std::array<int, size> expected = { 0, -1, 2, 3, 4, 5, 6, 7, 8, 9 };
            std::vector<int> vec(size);
            std::iota(begin(vec), end(vec), 0);
            array_view<int> av(size, vec);

            amp_stl_algorithms::replace(begin(av), end(av), 1, -1);

            Assert::IsTrue(are_equal(expected, av));
        }

        TEST_METHOD(stl_replace_if)
        {
            const int size = 10;
            std::array<int, size> expected = { 0, -1, 2, -1, 4, -1, 6, -1, 8, -1 };
            std::vector<int> vec(size);
            std::iota(begin(vec), end(vec), 0);
            array_view<int> av(size, vec);

            amp_stl_algorithms::replace_if(begin(av), end(av), [=](int v) restrict(amp) { return (v % 2 != 0); }, -1);

            Assert::IsTrue(are_equal(expected, av));
        }

        TEST_METHOD(stl_replace_copy)
        {
            const int size = 10;
            std::array<int, size> expected = { 0, -1, 2, 3, 4, 5, 6, 7, 8, 9 };
            std::vector<int> vec(size);
            std::iota(begin(vec), end(vec), 0);
            array_view<int> av(size, vec);
            std::vector<int> result(size, -2);
            array_view<int> result_av(size, result);

            auto result_end = amp_stl_algorithms::replace_copy(begin(av), end(av), begin(result_av), 1, -1);

            result_av.synchronize();
            Assert::IsTrue(are_equal(expected, result_av));
            Assert::AreEqual(2, std::distance(begin(result_av), result_end));
        }

        TEST_METHOD(stl_replace_copy_if)
        {
            const int size = 10;
            std::array<int, size> expected = { 0, -1, 2, -1, 4, -1, 6, -1, 8, -1 };
            std::vector<int> vec(size);
            std::iota(begin(vec), end(vec), 0);
            array_view<int> av(size, vec);
            std::vector<int> result(size, -2);
            array_view<int> result_av(size, result);

            auto result_end = amp_stl_algorithms::replace_copy_if(begin(av), end(av), begin(result_av), 
				[=](int v) restrict(amp) { return (v % 2 != 0); }, -1);

            Assert::IsTrue(are_equal(expected, result_av));
            Assert::AreEqual(10, std::distance(begin(result_av), result_end));
		}

        TEST_METHOD(stl_replace_copy_if_2)
		{
			const int size = 10;
            std::array<int, size> expected = { -1, -1, -1, -1, -1, 5, 6, 7, 8, 9 };
            std::vector<int> vec(size);
            std::iota(begin(vec), end(vec), 0);
            array_view<int> av(size, vec);
            std::vector<int> result(size, -2);
            array_view<int> result_av(size, result);

            auto result_end = amp_stl_algorithms::replace_copy_if(begin(av), end(av), begin(result_av), 
				[=](int v) restrict(amp) { return (v < 5); }, -1);

			Assert::IsTrue(are_equal(expected, result_av));
            Assert::AreEqual(5, std::distance(begin(result_av), result_end));
        }

        //----------------------------------------------------------------------------
        // is_sorted, is_sorted_until, sort, partial_sort, partial_sort_copy, stable_sort
        //----------------------------------------------------------------------------

        template <typename T>
        class abs_less_equal
        {
        public:
            T operator()(const T &a, const T &b) const restrict(cpu, amp)
            {
                return (abs(a) <= abs(b));
            }

        private:
            static T abs(const T& a)  restrict(cpu, amp) 
            { 
                return (a < 0) ? -a : a; 
            }
        };

        TEST_METHOD(stl_is_sorted_until)
        {
			const int size = 10;
            std::vector<int> vec(size);
            std::iota(begin(vec), end(vec), 0);
            array_view<int> av(size, vec);

            Assert::AreEqual(9, *--amp_stl_algorithms::is_sorted_until(begin(av), end(av), amp_algorithms::less<int>()));
            Assert::AreEqual(9, *--amp_stl_algorithms::is_sorted_until(begin(av), end(av)));
            Assert::IsTrue(amp_stl_algorithms::is_sorted(begin(av), end(av), amp_algorithms::less<int>()));
            Assert::IsTrue(amp_stl_algorithms::is_sorted(begin(av), end(av)));

            av[5] = -4;  // 0, 1, 2, 3, 4, -4, 6, 7, 8, 9

            Assert::AreEqual(9, *--amp_stl_algorithms::is_sorted_until(begin(av), end(av), abs_less_equal<int>()));
            Assert::AreEqual(4, *--amp_stl_algorithms::is_sorted_until(begin(av), end(av)));
            Assert::IsTrue(amp_stl_algorithms::is_sorted(begin(av), end(av), abs_less_equal<int>()));
            Assert::IsFalse(amp_stl_algorithms::is_sorted(begin(av), end(av)));
            Assert::IsTrue(amp_stl_algorithms::is_sorted(begin(av), begin(av) + 5));

            av[5] = 4;   // 0, 1, 2, 3, 4, 4, 6, 7, 8, 9

            Assert::AreEqual(9, *--amp_stl_algorithms::is_sorted_until(begin(av), end(av), amp_algorithms::less_equal<int>()));
            Assert::AreEqual(9, *--amp_stl_algorithms::is_sorted_until(begin(av), end(av)));
            Assert::IsFalse(amp_stl_algorithms::is_sorted(begin(av), end(av), amp_algorithms::less<int>()));
            Assert::IsTrue(amp_stl_algorithms::is_sorted(begin(av), end(av)));

            av[1] = -1;  // 0, -1, 2, 3, 4, 4, 6, 7, 8, 9

            Assert::AreEqual(9, *--amp_stl_algorithms::is_sorted_until(begin(av), end(av), abs_less_equal<int>()));
            Assert::AreEqual(0, *--amp_stl_algorithms::is_sorted_until(begin(av), end(av), amp_algorithms::less_equal<int>()));
            Assert::AreEqual(0, *--amp_stl_algorithms::is_sorted_until(begin(av), end(av)));
            Assert::IsFalse(amp_stl_algorithms::is_sorted(begin(av), end(av), amp_algorithms::less_equal<int>()));
            Assert::IsFalse(amp_stl_algorithms::is_sorted(begin(av), end(av)));
            Assert::IsTrue(amp_stl_algorithms::is_sorted(begin(av), begin(av) + 1));
        }

        //----------------------------------------------------------------------------
        // swap, swap<T, N>, swap_ranges, iter_swap
        //----------------------------------------------------------------------------

        TEST_METHOD(stl_swap_cpu)
        {
            int a = 1;
            int b = 2;

            amp_stl_algorithms::swap(a, b);

            Assert::AreEqual(2, a);
            Assert::AreEqual(1, b);
        }

        TEST_METHOD(stl_swap_amp)
        {
            const int size = 2;
            std::vector<int> vec(size);
            std::iota(begin(vec), end(vec), 1);
            array_view<int> av(2, vec);

            parallel_for_each(concurrency::extent<1>(1), [=](concurrency::index<1> idx) restrict(amp)
            {
               amp_stl_algorithms::swap(av[idx], av[idx + 1]);
            });

            Assert::AreEqual(2, av[0]);
            Assert::AreEqual(1, av[1]);
        }

        TEST_METHOD(stl_swap_n_cpu)
        {
            const int size = 10;
            int arr1[size];
            int arr2[size];
            std::iota(arr1, arr1 + size, 0);
            std::iota(arr2, arr2 + size, -9);

            amp_stl_algorithms::swap<int, 10>(arr1, arr2);

            for (int i = 0; i < size; ++i)
            {
                Assert::AreEqual(i, arr2[i]);
                Assert::AreEqual((-9 + i), arr1[i]);
            }
        }

		
        TEST_METHOD(stl_swap_n_amp)
        {
            std::array<int, 10> expected = { 6, 7, 8, 9, 10, 1, 2, 3, 4, 5 };

            std::vector<int> vec(10);
            std::iota(begin(vec), end(vec), 1);
            array_view<int> av(10, vec);

            parallel_for_each(concurrency::tiled_extent<5>(concurrency::extent<1>(5)), 
				[=](concurrency::tiled_index<5> tidx) restrict(amp)
            {
				tile_static int arr1[5];
				tile_static int arr2[5];

				int idx = tidx.global[0];
				int i = tidx.local[0];
			
				arr1[i] = av[i];
				arr2[i] = av[i + 5];

				tidx.barrier.wait();

				if (i == 0)
				{
					amp_stl_algorithms::swap<int, 5>(arr1, arr2);
				}

				tidx.barrier.wait();

				av[i] = arr1[i];
				av[i + 5] = arr2[i];

				tidx.barrier.wait();
			});

			av.synchronize();
            Assert::IsTrue(are_equal(expected, av));
        }
		
        TEST_METHOD(stl_swap_ranges)
        {
            const int size = 10;
            std::array<int, size> expected = { 0, 6, 7, 8, 4, 5, 1, 2, 3, 9 };
            std::vector<int> vec(size);
            std::iota(begin(vec), end(vec), 0);
            array_view<int> av(size, vec);

            auto expected_end = amp_stl_algorithms::swap_ranges(begin(av) + 1, begin(av) + 4, begin(av) + 6);

            Assert::IsTrue(are_equal(expected, av));
            Assert::AreEqual(0, std::distance(begin(av) + 6 + 3, expected_end));
            Assert::AreEqual(3, *--expected_end);
        }

        TEST_METHOD(stl_swap_iter)
        {
            const int size = 2;
            std::vector<int> vec(size);
            std::iota(begin(vec), end(vec), 1);
            array_view<int> av(2, vec);

            parallel_for_each(concurrency::extent<1>(1), [=](concurrency::index<1> idx) restrict(amp)
            {
                amp_stl_algorithms::iter_swap(begin(av), begin(av) + 1);
            });

            Assert::AreEqual(2, av[0]);
            Assert::AreEqual(1, av[1]);
        }

        //----------------------------------------------------------------------------
        // reverse, reverse_copy
        //----------------------------------------------------------------------------

        TEST_METHOD(stl_reverse)
        {
            test_reverse(1);
            test_reverse(1023);
            test_reverse(1024);
        }

        void test_reverse(const int size)
        {
            std::vector<int> vec(size);
            std::iota(begin(vec), end(vec), 0);
            array_view<int> av(size, vec);

            amp_stl_algorithms::reverse(begin(av), end(av));

            for (int i = 0; i < int(vec.size()); ++i)
            {
                Assert::AreEqual((size - 1 - i), av[i]);
            }
        }

        TEST_METHOD(stl_reverse_copy)
        {
            test_reverse_copy(1);
            test_reverse_copy(1023);
            test_reverse_copy(1024);
        }

        void test_reverse_copy(const int size)
        {
            std::vector<int> vec(size);
            std::iota(begin(vec), end(vec), 0);
            array_view<int> av(size, vec);

            std::vector<int> result(size, 0);
            concurrency::array_view<int> result_av(size, result);

            auto result_end = amp_stl_algorithms::reverse_copy(begin(av), end(av), begin(result_av));

            for (int i = 0; i < int(vec.size()); ++i)
            {
                Assert::AreEqual((size - 1 - i), result_av[i]);
            }

            Assert::AreEqual(result_av[size - 1], *--result_end);
        }
    };
};// namespace tests
