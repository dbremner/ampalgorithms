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

namespace amp_algorithms_tests
{
    enum class scan_type
    {
        scan,
        multiscan,
        segmented
    };

    struct bitvector;

    TEST_CLASS(scan_tests)
    {
        TEST_METHOD(amp_scan_backwards)
        {
            const bool backwards = true;

            test_scan<float>(backwards);
            test_scan<unsigned int>(backwards);
            test_scan<int>(backwards);
            test_scan_bitwise_op<int>(backwards);
        }

        TEST_METHOD(amp_scan_forwards)
        {
            const bool backwards = false;

            test_scan<float>(backwards);
            test_scan<unsigned int>(backwards);
            test_scan<int>(backwards);
            test_scan_bitwise_op<int>(backwards);
        }

        TEST_METHOD(amp_multiscan_backwards)
        {
            const bool backwards = true;

            test_multiscan<int>(backwards);
            test_multiscan<unsigned int>(backwards);
            test_multiscan<float>(backwards);
            test_multiscan_bitwise_op<int>(backwards);
        }

        TEST_METHOD(amp_multiscan_forwards)
        {
            const bool backwards = false;

            test_multiscan<int>(backwards);
            test_multiscan<unsigned int>(backwards);
            test_multiscan<float>(backwards);
            test_multiscan_bitwise_op<int>(backwards);
        }

        TEST_METHOD(amp_segmented_scan_backwards)
        {
            const bool backwards = true;

            test_segmented<int>(backwards);
            test_segmented<unsigned int>(backwards);
            test_segmented<float>(backwards);
            test_segmented_bitwise_op<int>(backwards);
        }

        TEST_METHOD(amp_segmented_scan_forwards)
        {
            const bool backwards = false;

            test_segmented<int>(backwards);
            test_segmented<unsigned int>(backwards);
            test_segmented<float>(backwards);
            test_segmented_bitwise_op<int>(backwards);
        }

        TEST_METHOD(amp_scan_other)
        {
            accelerator ref(accelerator::direct3d_ref);
            accelerator_view ref_view = ref.create_view();

            const int elem_count = 10;
            std::vector<unsigned int> in(elem_count, 1);

            concurrency::array<unsigned int> input(concurrency::extent<1>(elem_count), in.begin(), ref_view);
            // use max_scan_size and max_scan_count greater than actual usage
            scan s(2 * elem_count, elem_count, ref_view);

            // 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 -> 
            s.scan_exclusive(input, input);
            // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 -> 
            s.scan_exclusive(input, input, scan_direction::forward, amp_algorithms::sum<unsigned int>());
            // 0, 0, 1, 3, 6, 10, 15, 21, 28, 36 -> 

            unsigned int flg = 8; // 001000 in binary, so our segment is in here: 0, 0, 1, | 3, 6, 10, 15, 21, 28, 36
            concurrency::array<unsigned int> flags(1, &flg, ref_view);
            s.segmented_scan_exclusive(input, input, flags, scan_direction::backward, amp_algorithms::sum<unsigned int>());
            // 1, 1, 0, 116, 110, 100, 85, 64, 36, 0

            // Copy out
            in = input;

            unsigned int expected_results[elem_count] = {1, 1, 0, 116, 110, 100, 85, 64, 36, 0};
            for (unsigned int i = 0; i < in.size(); ++i)
            {
                Assert::AreEqual(expected_results[i], in[i]);
            }
        }

        TEST_METHOD(amp_scan_error_handling)
        {
            accelerator ref(accelerator::direct3d_ref);
            accelerator_view ref_view = ref.create_view();

            const int elem_count = 10;
            std::vector<unsigned int> in(elem_count, 1);

            concurrency::array<unsigned int> input(concurrency::extent<1>(elem_count), in.begin(), ref_view);

            Assert::ExpectException<runtime_exception>([&]() {
                scan s2(2 * elem_count, elem_count, accelerator().default_view);
                s2.scan_exclusive(input, input);
            }, 
                L"Expected exception for non-matching accelerator_view in scan object");

            Assert::ExpectException<runtime_exception>([&]() {
                scan s2(2 * elem_count, elem_count, ref_view);
                concurrency::array<unsigned int> output(elem_count, ref.create_view());
                s2.scan_exclusive(input, output);
            },
                L"Expected exception for non-matching accelerator_view in output");

            Assert::ExpectException<runtime_exception>([&]() {
                scan s2(elem_count-1, ref_view);
                s2.scan_exclusive(input, input);
            },
                L"Expected exception for scan object with max_scan_size < scan_size");

            Assert::ExpectException<runtime_exception>([&]() {
                scan s2(elem_count, 0, ref_view);
            },
                L"Expected exception for scan object with max_scan_count == 0");

            Assert::ExpectException<runtime_exception>([&]() {
                scan s2(elem_count, 1, ref_view);
                concurrency::array<unsigned int, 2> in2(10, 10);
                s2.multi_scan_exclusive(in2, in2, scan_direction::forward, amp_algorithms::sum<unsigned int>());
            },
                L"Expected exception for scan object with max_scan_count < scan_count");

            // Check scan binding cleanup
            array_view<const unsigned int> view(input);
            concurrency::array<unsigned int> output(input.extent, input.accelerator_view);
            parallel_for_each(output.extent, [view, &output] (concurrency::index<1> idx) restrict(amp) {
                output[idx] = view[idx];
            });
            output.accelerator_view.wait();
        }

    private:
        template<typename T, typename BinaryFunction>
        void test_scan_internal(int column_count, BinaryFunction op, std::string test_name, bool backwards, bool inplace, scan_type test_type = scan_type::scan, unsigned int row_count = 1)
        {
            Logger::WriteMessage(get_extended_test_name<T, BinaryFunction>(test_name, backwards, inplace).c_str());

            std::vector<T> in(row_count * column_count);
            generate_data(in);
            std::vector<T> out(row_count * column_count);
            bitvector flags(column_count);

            // Construct scan object
            scan s(column_count, row_count);

            // Convert bool to direction type
            scan_direction direction = backwards ? scan_direction::backward : scan_direction::forward;

            // Run scan
            if (test_type == scan_type::multiscan)
            {
                concurrency::extent<2> e2(row_count, column_count);
                concurrency::array<T, 2> input(e2, in.begin());
                concurrency::array<T, 2> output(e2);

                s.multi_scan_exclusive(input, inplace ? input : output, direction, op);
                copy(inplace ? input : output, out.begin());
            }
            else
            {
                concurrency::extent<1> e(column_count);
                concurrency::array<T> input(e, in.begin());
                concurrency::array<T> output(e);
                bitvector *flags_ptr = nullptr;

                if (test_type == scan_type::scan)
                {
                    s.scan_exclusive(input, inplace ? input : output, direction, op);
                }
                else
                {
                    flags.generate_data();
                    concurrency::array<unsigned int> input_flags(static_cast<unsigned int>(flags.data.size()), flags.data.begin());
                    s.segmented_scan_exclusive(input, inplace ? input : output, input_flags, direction, op);
                }

                copy(inplace ? input : output, out.begin());
            }

            // Now time to verify the results with host-side computation
            verify_scan_results(backwards, /*exclusive=*/true, op, in, out, column_count, column_count, row_count, flags);
        }

        template<typename T>
        void test_scan(bool backwards)
        {
            test_scan_internal<T>(10, amp_algorithms::sum<T>(), "Test scan", backwards, /*in_place=*/ false);
            test_scan_internal<T>(11, amp_algorithms::max<T>(), "Test scan", backwards, /*in_place=*/ true);
            test_scan_internal<T>(2 * 1024, amp_algorithms::min<T>(), "Test scan", backwards, /*in_place=*/ false);
            test_scan_internal<T>(2 * 1024 + 1, amp_algorithms::mul<T>(), "Test scan", backwards, /*in_place=*/ false);
        }

        template<>
        void test_scan<unsigned int>(bool backwards)
        {
            test_scan_internal<unsigned int>(10, amp_algorithms::sum<unsigned int>(), "Test scan", backwards, /*in_place=*/ true);
            test_scan_internal<unsigned int>(777, amp_algorithms::mul<unsigned int>(), "Test scan", backwards, /*in_place=*/ false);
        }

        template<typename T>
        void test_scan_bitwise_op(bool backwards)
        {
            test_scan_internal<T>(77, amp_algorithms::bit_xor<T>(), "Test scan", backwards, /*in_place=*/ true);
            test_scan_internal<T>(66, amp_algorithms::bit_and<T>(), "Test scan", backwards, /*in_place=*/ false);
            test_scan_internal<T>(12345, amp_algorithms::bit_or<T>(), "Test scan", backwards, /*in_place=*/ true);
        }

        template<typename T>
        void test_multiscan(bool backwards)
        {
            test_scan_internal<T>(144, amp_algorithms::sum<T>(), "Test multiscan", backwards, /*in_place=*/ false, scan_type::multiscan, 12);
            test_scan_internal<T>(2048, amp_algorithms::sum<T>(), "Test multiscan", backwards, /*in_place=*/ true, scan_type::multiscan, 1024);
            test_scan_internal<T>(3333, amp_algorithms::max<T>(), "Test multiscan", backwards, /*in_place=*/ false, scan_type::multiscan, 3);
            test_scan_internal<T>(128, amp_algorithms::mul<T>(), "Test multiscan", backwards, /*in_place=*/ true, scan_type::multiscan, 8);
            test_scan_internal<T>(127, amp_algorithms::min<T>(), "Test multiscan", backwards, /*in_place=*/ false, scan_type::multiscan, 3);
        }

        template<>
        void test_multiscan<unsigned int>(bool backwards)
        {
            test_scan_internal<unsigned int>(144, amp_algorithms::sum<unsigned int>(), "Test multiscan", backwards, /*in_place=*/ false, scan_type::multiscan, 12);
            test_scan_internal<unsigned int>(2048, amp_algorithms::sum<unsigned int>(), "Test multiscan", backwards, /*in_place=*/ true, scan_type::multiscan, 64);
            test_scan_internal<unsigned int>(128, amp_algorithms::mul<unsigned int>(), "Test multiscan", backwards, /*in_place=*/ false, scan_type::multiscan, 8);
        }

        template<typename T>
        void test_multiscan_bitwise_op(bool backwards)
        {
            test_scan_internal<T>(8, amp_algorithms::bit_and<T>(), "Test multiscan", backwards, /*in_place=*/ true, scan_type::multiscan, 2);
            test_scan_internal<T>(2048, amp_algorithms::bit_or<T>(), "Test multiscan", backwards, /*in_place=*/ false, scan_type::multiscan, 2);
            test_scan_internal<T>(3072, amp_algorithms::bit_xor<T>(), "Test multiscan",  backwards, /*in_place=*/ true, scan_type::multiscan, 3);
        }

        template<typename T>
        void test_segmented(bool backwards)
        {
            test_scan_internal<T>(7123127, amp_algorithms::sum<T>(), "Test segmented scan", backwards, /*inplace=*/false, scan_type::segmented);
            test_scan_internal<T>(31, amp_algorithms::mul<T>(), "Test segmented scan", backwards, /*inplace=*/true, scan_type::segmented);
            test_scan_internal<T>(222, amp_algorithms::min<T>(), "Test segmented scan", backwards, /*inplace=*/false, scan_type::segmented);
            test_scan_internal<T>(333, amp_algorithms::max<T>(), "Test segmented scan", backwards, /*inplace=*/true, scan_type::segmented);
        }

        template<>
        void test_segmented<unsigned int>(bool backwards)
        {
            test_scan_internal<unsigned int>(7123127, amp_algorithms::sum<unsigned int>(), "Test segmented scan", backwards, /*inplace=*/false, scan_type::segmented);
            test_scan_internal<unsigned int>(111, amp_algorithms::mul<unsigned int>(), "Test segmented scan", backwards, /*inplace=*/true, scan_type::segmented);
        }

        template<typename T>
        void test_segmented_bitwise_op(bool backwards)
        {
            test_scan_internal<T>(234, amp_algorithms::bit_and<T>(), "Test segmented scan", backwards, /*inplace=*/false, scan_type::segmented);
            test_scan_internal<T>(432, amp_algorithms::bit_or<T>(), "Test segmented scan", backwards, /*inplace=*/true, scan_type::segmented);
            test_scan_internal<T>(444, amp_algorithms::bit_xor<T>(), "Test segmented scan", backwards, /*inplace=*/true, scan_type::segmented);
        }

        // A host side verification for scan, multiscan and segmented scan
        template <typename T, typename BinaryFunction>
        void verify_scan_results(bool backwards, bool exclusive, BinaryFunction op, std::vector<T> &in, std::vector<T> &out, unsigned int scan_size, unsigned int scan_pitch, unsigned int scan_count, bitvector &flags) 
        {
            // For each sub-scan
            for (unsigned int current_scan_num=0; current_scan_num<scan_count; ++current_scan_num)
            {
                T expected_scan_result;
                for (unsigned int i=current_scan_num * scan_pitch; i<scan_size; ++i)
                {
                    int pos = i; // pos is used to reference into output and input arrays depending on the direction we go from the front or the back
                    if (backwards)
                    {
                        pos = current_scan_num * scan_pitch + scan_size - 1 - i;
                    }

                    if (i == current_scan_num * scan_pitch || flags.is_new_segment(pos, backwards))
                    {
                        // Establish first result, either it is identity (for exclusive) or first/last element depending on scan direction for inclusive scan
                        if (exclusive)
                        {
                            expected_scan_result = get_identity<T>(op);
                        }
                        else
                        {
                            expected_scan_result = in[pos];
                        }
                    }

                    // Inclusive is computed as pre-fix op
                    if (!exclusive && i != current_scan_num * scan_pitch)
                    {
                        expected_scan_result = op(expected_scan_result, in[pos]);
                    }

                    // Compare results
                    Assert::IsTrue(compare(out[pos], expected_scan_result));

                    // Exclusive is computed as post-fix op
                    if (exclusive)
                    {
                        expected_scan_result = op(expected_scan_result, in[pos]);
                    }
                }
            }
        }

        template<typename T, typename BinaryFunction>
        std::string get_extended_test_name(std::string test_name, bool backwards, bool inplace)
        {
            std::stringstream postfix;
            postfix << test_name << " ";
            postfix << typeid(T).name() << " ";
            postfix << get_binary_function_info<BinaryFunction>().name();

            postfix << (backwards ? " backward" : " forward");
            postfix << (inplace ? " in-place" : " not-in-place");

            return postfix.str();
        }

        template<typename BinaryFunction>
        struct get_binary_function_info
        {
            std::string name() { return "Unrecognized function"; }
        };

        template<typename T>
        struct get_binary_function_info<amp_algorithms::sum<T>>
        {
            std::string name() { return "sum"; }
        };

        template<typename T>
        struct get_binary_function_info<amp_algorithms::min<T>>
        {
            std::string name() { return "min"; }
        };

        template<typename T>
        struct get_binary_function_info<amp_algorithms::max<T>>
        {
            std::string name() { return "max"; }
        };

        template<typename T>
        struct get_binary_function_info<amp_algorithms::mul<T>>
        {
            std::string name() { return "mul"; }
        };

        template<typename T>
        struct get_binary_function_info<amp_algorithms::bit_and<T>>
        {
            std::string name() { return "bitwise and"; }
        };

        template<typename T>
        struct get_binary_function_info<amp_algorithms::bit_or<T>>
        {
            std::string name() { return "bitwise or"; }
        };

        template<typename T>
        struct get_binary_function_info<amp_algorithms::bit_xor<T>>
        {
            std::string name() { return "bitwise xor"; }
        };
    };

    template<typename value_type, typename functor>
    value_type get_identity(functor op)
    {
        if (typeid(op) == typeid(amp_algorithms::max<value_type>))
        {
            return std::numeric_limits<value_type>::lowest();
        }
        if (typeid(op) == typeid(amp_algorithms::min<value_type>))
        {
            return std::numeric_limits<value_type>::max();
        }
        if (typeid(op) == typeid(amp_algorithms::mul<value_type>))
        {
            return 1;
        }
        if (typeid(op) == typeid(amp_algorithms::bit_and<value_type>))
        {
            return (value_type)(0xFFFFFFFF);
        }
        else
        {
            return 0;
        }
    }

    // Represents compressed flag array for segmented scan
    struct bitvector 
    {
        bitvector(unsigned int scan_size) : m_scan_size(scan_size)
        {
            data = std::vector<unsigned int>(bits_pad_to_uint(scan_size), 0);
        }

        void generate_data()
        {
            srand(2012);    // Set random number seed so tests are reproducable.

            unsigned int flag_counter = 0;
            for (unsigned int idx = 0; idx<data.size() && flag_counter < m_scan_size; ++idx)
            {
                unsigned int bag_of_bits = data[idx];
                for(unsigned int offset = 0; offset < bit_count<unsigned int>() && flag_counter < m_scan_size; ++offset)
                {
                    if (rand() % 13  == 0) // flip the bit to 1
                    {
                        bag_of_bits |= 1 << offset;	
                    }

                    flag_counter++;
                }
                data[idx] = bag_of_bits;
            }
        }

        bool is_new_segment(unsigned int pos, bool backwards)
        {
            // When we encounter flag going backwards it means, 
            // that it is the first element of this segment (last element to be scanned going backwards)
            // for simplification we increment 'pos' and always look for flags behind our current position.
            if (backwards)
            {
                pos = pos + 1;
            }

            unsigned int idx = pos / bit_count<unsigned int>();
            unsigned int offset = pos % bit_count<unsigned int>();

            unsigned int bag_of_bits = data[idx];
            return (1 << offset & bag_of_bits) > 0 ? true : false;
        }

        std::vector<unsigned int> data;

    private:
        template<typename T>
        unsigned int bit_count()
        {
            return sizeof(T) * 8;
        }

        unsigned int bits_pad_to_uint(unsigned int bits)
        {
            return (bits + bit_count<unsigned int>() - 1) / bit_count<unsigned int>();
        }

        unsigned int m_scan_size;
    };
};
