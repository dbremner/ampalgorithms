/*----------------------------------------------------------------------------
* Copyright (c) Microsoft Corp.
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
* C++ AMP standard algorithms library.
*
* This file contains the helpers classes in amp_algorithms::_details namespace
*---------------------------------------------------------------------------*/

#include "stdafx.h"

#include "testtools.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace amp_algorithms;
using namespace testtools;

namespace amp_algorithms_tests
{
    std::wstring Msg(std::vector<int>& expected, std::vector<int>& actual, size_t width = 32)
    {
        std::wostringstream msg;
        msg << container_width(50) << L"[" << expected << L"] != [" << actual << L"]" << std::endl;
        return msg.str();
    }

    TEST_CLASS_CATEGORY(amp_scan_tests, "amp")
    // {

    private:

        // This is to allow the tests to pass when run on the REF accelerator. In all other cases the warp 
        // size should be assumed to be 32.
#if (defined(USE_REF) || defined(_DEBUG))
        static const int warp_size = 4;
        static const int max_tile_size = warp_size * warp_size;
#else
        static const int warp_size = 4;
        static const int max_tile_size = warp_size * warp_size;
#endif

    public:
        TEST_CLASS_INITIALIZE(initialize_tests)
        {
            set_default_accelerator(L"amp_scan_tests");
        }

        TEST_METHOD(amp_details_scan_tile)
        {
            static const int tile_size = 4;
            std::array<unsigned, 16> input = { 3, 2, 1, 6, 10, 11, 13, 0, 15, 10, 5, 14, 4, 12, 9, 8 };
            std::array<unsigned, 16> expected = { 0, 3, 5, 6, 0, 10, 21, 34, 0, 15, 25, 30, 0, 4, 16, 25 };

            array_view<unsigned> input_av(int(input.size()), input);
            std::array<unsigned, 16> output;
            array_view<unsigned> output_av(int(output.size()), output);
            amp_algorithms::fill(output_av, 0);
            concurrency::tiled_extent<tile_size> compute_domain = input_av.get_extent().tile<4>().pad();

            concurrency::parallel_for_each(compute_domain, [=](concurrency::tiled_index<tile_size> tidx) restrict(amp)
            {
                const int gidx = tidx.global[0];
                const int idx = tidx.local[0];
                tile_static int tile_data[tile_size];
                tile_data[idx] = input_av[gidx];

                amp_algorithms::_details::scan_tile<tile_size, scan_mode::exclusive>(tile_data, tidx, amp_algorithms::plus<int>(), tile_size);

                output_av[gidx] = tile_data[idx];
            });

            Assert::IsTrue(are_equal(expected, output_av));
        }

        TEST_METHOD(amp_details_scan_tile_partial)
        {
            std::array<unsigned, 7> input = { 3, 2, 1, 6, 10, 11, 5 };
            std::array<unsigned, 7> expected = { 0, 3, 5, 6, 12, 22, 33 };

            array_view<unsigned> input_av(int(input.size()), input);
            std::array<unsigned, 7> output;
            array_view<unsigned> output_av(int(output.size()), output);
            amp_algorithms::fill(output_av, 0);
            concurrency::tiled_extent<16> compute_domain = input_av.get_extent().tile<16>().pad();

            concurrency::parallel_for_each(compute_domain, [=](concurrency::tiled_index<16> tidx) restrict(amp)
            {
                const int gidx = tidx.global[0];
                const int idx = tidx.local[0];
                tile_static int tile_data[16];
                tile_data[idx] = input_av[gidx];

                amp_algorithms::_details::scan_tile<16, scan_mode::exclusive>(tile_data, tidx, amp_algorithms::plus<int>(), 7);

                output_av[gidx] = tile_data[idx];
            });

            Assert::IsTrue(are_equal(expected, output_av));
        }

        TEST_METHOD(amp_scan_exclusive_single_warp)
        {
            std::vector<int> input(warp_size, 1);
            concurrency::array_view<int, 1> input_vw(int(input.size()), input);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 0);

            scan<warp_size, scan_mode::exclusive>(input_vw, input_vw, amp_algorithms::plus<int>());
            
            input_vw.synchronize();
            Assert::IsTrue(expected == input, Msg(expected, input).c_str());
        }

        TEST_METHOD(amp_scan_inclusive_single_warp)
        {
            std::vector<int> input(warp_size, 1);
            concurrency::array_view<int, 1> input_vw(int(input.size()), input);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 1);

            scan<warp_size, scan_mode::inclusive>(input_vw, input_vw, amp_algorithms::plus<int>());

            input_vw.synchronize();
            Assert::IsTrue(expected == input, Msg(expected, input).c_str());
        }

        TEST_METHOD(amp_scan_exclusive_multi_warp)
        {
            std::vector<int> input(max_tile_size, 1);
            concurrency::array_view<int, 1> input_vw(int(input.size()), input);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 0);

            scan<max_tile_size, scan_mode::exclusive>(input_vw, input_vw, amp_algorithms::plus<int>());

            input_vw.synchronize();
            Assert::IsTrue(expected == input, Msg(expected, input).c_str());
        }

        TEST_METHOD(amp_scan_inclusive_multi_warp)
        {
            std::vector<int> input(max_tile_size, 1);
            concurrency::array_view<int, 1> input_vw(int(input.size()), input);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 1);

            scan<max_tile_size, scan_mode::inclusive>(input_vw, input_vw, amp_algorithms::plus<int>());

            input_vw.synchronize();
            Assert::IsTrue(expected == input, Msg(expected, input).c_str());
        }

        TEST_METHOD(amp_scan_exclusive_multi_tile)
        {
            std::vector<int> input(warp_size * 4, 1);
            concurrency::array_view<int, 1> input_vw(int(input.size()), input);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 0);

            scan<warp_size, scan_mode::exclusive>(input_vw, input_vw, amp_algorithms::plus<int>());

            input_vw.synchronize();
            Assert::IsTrue(expected == input, Msg(expected, input).c_str());
        }

        TEST_METHOD(amp_scan_inclusive_multi_tile)
        {
            std::vector<int> input(warp_size * 4, 1);
            concurrency::array_view<int, 1> input_vw(int(input.size()), input);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 1);

            scan<warp_size, scan_mode::inclusive>(input_vw, input_vw, amp_algorithms::plus<int>());

            input_vw.synchronize();
            Assert::IsTrue(expected == input, Msg(expected, input).c_str());
        }

        TEST_METHOD(amp_scan_exclusive_multi_warp_multi_tile)
        {
            std::vector<int> input(warp_size * 4 * 4, 1);
            concurrency::array_view<int, 1> input_vw(int(input.size()), input);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 0);

            scan<warp_size * 4, scan_mode::exclusive>(input_vw, input_vw, amp_algorithms::plus<int>());

            input_vw.synchronize();
            Assert::IsTrue(expected == input, Msg(expected, input).c_str());
        }
        
        TEST_METHOD(amp_scan_inclusive_multi_warp_multi_tile)
        {
            std::vector<int> input(warp_size * 4 * 4, 1);
            concurrency::array_view<int, 1> input_vw(int(input.size()), input);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 1);

            scan<warp_size * 4, scan_mode::inclusive>(input_vw, input_vw, amp_algorithms::plus<int>());

            input_vw.synchronize();
            Assert::IsTrue(expected == input, Msg(expected, input).c_str());
        }

        TEST_METHOD(amp_scan_exclusive_incomplete_warp)
        {
            std::vector<int> input(warp_size + 2, 1);
            concurrency::array_view<int, 1> input_vw(int(input.size()), input);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 0);

            scan<warp_size, scan_mode::exclusive>(input_vw, input_vw, amp_algorithms::plus<int>());

            input_vw.synchronize();
            Assert::IsTrue(expected == input, Msg(expected, input).c_str());
        }
        
        TEST_METHOD(amp_scan_inclusive_incomplete_warp)
        {
            std::vector<int> input(warp_size + 2, 1);
            concurrency::array_view<int, 1> input_vw(int(input.size()), input);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 1);

            scan<warp_size, scan_mode::inclusive>(input_vw, input_vw, amp_algorithms::plus<int>());

            input_vw.synchronize();
            Assert::IsTrue(expected == input, Msg(expected, input).c_str());
        }

        TEST_METHOD(amp_scan_exclusive_recursive_scan)
        {
            std::vector<int> input(warp_size * (warp_size + 2), 1);
            concurrency::array_view<int, 1> input_vw(int(input.size()), input);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 0);

            scan<warp_size, scan_mode::exclusive>(input_vw, input_vw, amp_algorithms::plus<int>());

            input_vw.synchronize();
            Assert::IsTrue(expected == input, Msg(expected, input).c_str());
        }

        TEST_METHOD(amp_scan_inclusive_recursive_scan)
        {
            std::vector<int> input(warp_size * (warp_size + 2), 1);
            concurrency::array_view<int, 1> input_vw(int(input.size()), input);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 1);

            scan<warp_size, scan_mode::inclusive>(input_vw, input_vw, amp_algorithms::plus<int>());

            input_vw.synchronize();
            Assert::IsTrue(expected == input, Msg(expected, input).c_str());
        }

        TEST_METHOD(amp_scan_exclusive_2)
        {
            std::array<int, 12> input_data = { 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1 };
            std::vector<int> input(32);
            std::copy(begin(input_data), end(input_data), begin(input));
            concurrency::array_view<int, 1> input_vw(int(input.size()), input);
            std::vector<int> expected(input.size());
            scan_sequential_exclusive(begin(input), end(input), begin(expected));

            scan<warp_size, scan_mode::exclusive>(input_vw, input_vw, amp_algorithms::plus<int>());

            input_vw.synchronize();
            Assert::IsTrue(expected == input, Msg(expected, input).c_str());
        }

        TEST_METHOD(amp_scan_exclusive)
        {
            const int tile_size = warp_size * 4;
            std::vector<int> input(tile_size * (tile_size + 10));
            generate_data(input);
            concurrency::array_view<int, 1> input_vw(int(input.size()), input);
            std::vector<int> expected(input.size());
            scan_sequential_exclusive(begin(input), end(input), begin(expected));

            scan<warp_size, scan_mode::exclusive>(input_vw, input_vw, amp_algorithms::plus<int>());

            input_vw.synchronize();
            Assert::IsTrue(expected == input, Msg(expected, input).c_str());
        }

        TEST_METHOD(amp_scan_inclusive)
        {
            const int tile_size = warp_size * 4;
            std::vector<int> input(tile_size * (tile_size + 10));
            generate_data(input);
            concurrency::array_view<int, 1> input_vw(int(input.size()), input);
            std::vector<int> result(input.size(), -1);
            std::vector<int> expected(input.size());
            scan_sequential_inclusive(begin(input), end(input), begin(expected));

            scan<warp_size, scan_mode::inclusive>(input_vw, input_vw, amp_algorithms::plus<int>());

            input_vw.synchronize();
            Assert::IsTrue(expected == input, Msg(expected, input).c_str());
        }
    };
}
