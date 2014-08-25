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
* This file contains the unit tests for sort.
*---------------------------------------------------------------------------*/
#include "stdafx.h"

#include <amp_algorithms.h>
#include <amp_algorithms_direct3d.h>
#include "testtools.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace amp_algorithms;
using namespace testtools;

namespace amp_algorithms_tests
{
    std::wstring Msg2(std::vector<int>& expected, std::vector<int>& actual, size_t width = 32)
    {
        std::wostringstream msg;
        msg << container_width(50) << L"[" << expected << L"] != [" << actual << L"]" << std::endl;
        return msg.str();
    }

    TEST_CLASS_CATEGORY(amp_sort_tests, "amp")
    // {
private:
#if (defined(USE_REF) || defined(_DEBUG))
        static const int warp_size = 4;
        static const int max_tile_size = warp_size * warp_size;
#else
        static const int warp_size = 32;
        static const int max_tile_size = warp_size * warp_size;
#endif
    public:
        TEST_CLASS_INITIALIZE(initialize_tests)
        {
            set_default_accelerator(L"amp_sort_tests");
        }

        TEST_METHOD(amp_details_radix_key_value_tests)
        {
            //  0 0000  0  0        8 1000  2  0
            //  1 0001  0  1        9 1001  2  1
            //  2 0010  0  2       10 1010  2  2
            //  3 0011  0  3       11 1011  2  3
            //  4 0100  1  0       12 1100  3  0
            //  5 0101  1  1       13 1101  3  1
            //  6 0110  1  2       14 1110  3  2
            //  7 0111  1  3       15 1111  3  3

            enum parameter
            {
                index = 0,
                value = 1,
                expected = 2
            };

            std::array<std::tuple<unsigned, int, int>, 5> theories =
            {
                std::make_tuple(0, 3, 3),   // 000010 => ----10
                std::make_tuple(0, 1, 1),   // 000001 => ----01
                std::make_tuple(1, 3, 0),   // 000011 => --00--
                std::make_tuple(1, 13, 3),  // 001101 => --11--
                std::make_tuple(2, 45, 2),  // 101101 => 10----
            };
            
            for (auto t : theories)
            {
                int result = amp_algorithms::_details::radix_key_value<int, 2>(std::get<parameter::value>(t), std::get<parameter::index>(t));
                Assert::AreEqual(std::get<parameter::expected>(t), result);
            }
        }

        TEST_METHOD(amp_details_radix_sort_tile_by_key_index_0_tile_4)
        {
            std::array<unsigned, 16> input =     { 3,  2,  1,  6,   10, 11, 13,  0,   15, 10,  5, 14,   4, 12,  9,  8 };
            // Key 0 values, 2 bit key:            3   2   1   2     2   3   1   0     3   2   1   2    0   0   1   0
            std::array<unsigned, 16> expected  = { 1,  2,  6,  3,    0, 13, 10, 11,    5, 10, 14, 15,   4, 12,  8,  9 };
            array_view<unsigned> input_av(int(input.size()), input);

            concurrency::tiled_extent<4> compute_domain = input_av.get_extent().tile<4>().pad();

            concurrency::parallel_for_each(compute_domain, [=](concurrency::tiled_index<4> tidx) restrict(amp)
            {
                const int gidx = tidx.global[0];
                const int idx = tidx.local[0];
                tile_static int tile_data[4];

                tile_data[idx] = input_av[gidx];
                tidx.barrier.wait();

                amp_algorithms::_details::radix_sort_tile_by_key<int, 2, 4>(tile_data, input_av.extent.size(), tidx, 0);

                tidx.barrier.wait();
                input_av[gidx] = tile_data[idx];
            });

            input_av.synchronize();
            Assert::IsTrue(are_equal(expected, input_av));
        }

        TEST_METHOD(amp_details_radix_sort_tile_by_key_index_0_tile_32)
        {
            std::array<unsigned, 16> input =     { 3,  2,  1,  6,   10, 11, 13,  0,   15, 10,  5, 14,   4, 12,  9,  8 };
            // rdx                                 3   2   1   2     2   3   1   0     3   2   1   2    0   0   1   0
            std::array<unsigned, 16> expected  = { 0,  4, 12,  8,    1, 13,  5,  9,    2,  6, 10, 10,  14,  3, 11, 15 };
            array_view<unsigned> input_av(int(input.size()), input);

            concurrency::tiled_extent<32> compute_domain = input_av.get_extent().tile<32>().pad();

            concurrency::parallel_for_each(compute_domain, [=](concurrency::tiled_index<32> tidx) restrict(amp)
            {
                const int gidx = tidx.global[0];
                const int idx = tidx.local[0];
                tile_static int tile_data[32];

                tile_data[idx] = input_av[gidx];
                tidx.barrier.wait();

                amp_algorithms::_details::radix_sort_tile_by_key<int, 2, 32>(tile_data, input_av.extent.size(), tidx, 0);

                tidx.barrier.wait();
                input_av[gidx] = tile_data[idx];
            });

            input_av.synchronize();
            Assert::IsTrue(are_equal(expected, input_av));
        }

        TEST_METHOD(amp_details_radix_sort_tile_by_key_index_0_tile_32_data_32)
        {
            std::array<unsigned long, 32> input = { 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0 };
            // rdx                                 3   2   1   2     2   3   1   0     3   2   1   2    0   0   1   0
            std::array<unsigned long, 32> expected = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3 };
            array_view<unsigned long> input_av(int(input.size()), input);

            concurrency::tiled_extent<32> compute_domain = input_av.extent.tile<32>().pad();

            concurrency::parallel_for_each(compute_domain, [=](concurrency::tiled_index<32> tidx) restrict(amp)
            {
                const int gidx = tidx.global[0];
                const int idx = tidx.local[0];
                tile_static unsigned long tile_data[32];

                if (gidx < input_av.extent[0])
                {
                    tile_data[idx] = input_av[gidx];
                }
                tidx.barrier.wait();

                amp_algorithms::_details::radix_sort_tile_by_key<unsigned long, 2, 32>(tile_data, input_av.extent.size(), tidx, 0);

                tidx.barrier.wait();
                if (gidx < input_av.extent[0])
                {
                    input_av[gidx] = tile_data[idx];
                }
            });

            input_av.synchronize();
            Assert::IsTrue(are_equal(expected, input_av));
        }

        TEST_METHOD(amp_details_radix_sort_tile_by_key_index_0_tile_256)
        {
            std::array<unsigned, 16> input = { 3, 2, 1, 6, 10, 11, 13, 0, 15, 10, 5, 14, 4, 12, 9, 8 };
            // rdx                                 3   2   1   2     2   3   1   0     3   2   1   2    0   0   1   0
            std::array<unsigned, 16> expected = { 0, 4, 12, 8, 1, 13, 5, 9, 2, 6, 10, 10, 14, 3, 11, 15 };
            array_view<unsigned> input_av(int(input.size()), input);

            concurrency::tiled_extent<256> compute_domain = input_av.extent.tile<256>().pad();

            concurrency::parallel_for_each(compute_domain, [=](concurrency::tiled_index<256> tidx) restrict(amp)
            {
                const int gidx = tidx.global[0];
                const int idx = tidx.local[0];
                tile_static int tile_data[256];

                tile_data[idx] = input_av[gidx];
                tidx.barrier.wait();

                amp_algorithms::_details::radix_sort_tile_by_key<int, 2, 256>(tile_data, input_av.extent.size(), tidx, 0);

                tidx.barrier.wait();
                input_av[gidx] = tile_data[idx];
            });

            input_av.synchronize();
            Assert::IsTrue(are_equal(expected, input_av));
        }

        TEST_METHOD(amp_details_radix_sort_tile_by_key__index_1_tile_4)
        {
            std::array<unsigned, 16> input = { 1, 2, 6, 3, 0, 13, 10, 11, 5, 10, 14, 15, 4, 12, 8, 9 };
            // Key 1 values, 2 bit key:            0   0   1   0    0   3   2   2    1   2   3   3    1   3   2   2
            std::array<unsigned, 16> expected = { 1, 2, 3, 6, 0, 10, 11, 13, 5, 10, 14, 15, 4, 8, 9, 12 };

            array_view<unsigned> input_av(int(input.size()), input);

            concurrency::tiled_extent<4> compute_domain = input_av.get_extent().tile<4>().pad();
            concurrency::parallel_for_each(compute_domain, [=](concurrency::tiled_index<4> tidx) restrict(amp)
            {
                const int gidx = tidx.global[0];
                const int idx = tidx.local[0];
                tile_static int tile_data[4];
                tile_data[idx] = input_av[gidx];
                tidx.barrier.wait();

                amp_algorithms::_details::radix_sort_tile_by_key<int, 2, 4>(tile_data, 16, tidx, 1);

                tidx.barrier.wait();
                input_av[gidx] = tile_data[idx];
            });

            input_av.synchronize();
            Assert::IsTrue(are_equal(expected, input_av));
        }

        TEST_METHOD(amp_details_radix_sort_by_key_index_0_tile_4)
        {
            // gidx                                                    0   1   2   3     4   5   6   7     8   9  10  11    12  13  14  15
            std::array<unsigned, 16> input =                         { 3,  2,  1,  6,   10, 11, 13,  0,   15, 10,  5, 14,    4, 12,  9,  8 };
            // rdx =                                                   3,  2,  1,  2,    2,  3,  1,  0,    3,  2,  1,  2,    0,  0,  1,  0

            std::array<unsigned, 16> per_tile_rdx_histograms =       { 0,  1,  2,  1,    1,  1,  1,  1,    0,  1,  2,  1,    3,  1,  0,  0 };
            std::array<unsigned, 16> per_tile_rdx_offsets =          { 0,  0,  1,  3,    0,  1,  2,  3,    0,  0,  1,  3,    0,  3,  4,  4 };

            std::array<unsigned, 16> per_tile_rdx_histograms_tp =    { 0,  1,  0,  3,    1,  1,  1,  1,    2,  1,  2,  0,    1,  1,  1,  0 };
            std::array<unsigned, 16> xxx =                           { 0,  0,  1,  1,    0,  1,  2,  3,    0,  2,  3,  5,    0,  1,  2,  3 };
            std::array<unsigned, 16> tile_rdx_offsets =              { 0,  0,  0,  0,    0,  1,  2,  1,    1,  2,  3,  2,    1,  3,  5,  3 };
                                                                     
            std::array<unsigned, 16> global_histogram =              { 4,  4,  5,  3,                              0,0,0,0,0,0,0,0,0,0,0,0 };
            std::array<unsigned, 16> global_rdx_offsets =            { 0,  4,  8, 13,                              0,0,0,0,0,0,0,0,0,0,0,0 };
                                                                     
            std::array<unsigned, 16> sorted_per_tile =               { 1,  2,  6,  3,    0, 13, 10, 11,    5, 10, 14, 15,    4, 12,  8,  9 };
            // rdx =                                                   1,  2,  2,  3,    0,  1,  2,  3,    1,  2,  2,  3,    0,  0,  0,  1
            std::array<unsigned, 16> dest_gidx =                     { 4,  8,  9, 13,    0,  5, 10, 14,    6, 11, 12, 15,    1,  2,  3,  7 }; 
                                                                     
            std::array<unsigned, 16> sorted_by_key_0 =               { 0,  4, 12,  8,    1, 13,  5,  9,    2,  6, 10, 10,   14,  3, 11, 15 };

            // dest_gidx = idx - per_tile_rdx_offsets[tlx][rd x_0] + per_tile_rdx_offsets[tlx][rdx_0] + tile_rdx_offsets[(rdx * tile_count) + tlx] + global_rdx_offsets[rdx_0]
            //                                             
            //                                                         1 => 0 - 0  + 0 +  4 = 4
            //                                                         2 => 1 - 1  + 0 +  8 = 8 
            //                                                         2 => 2 - 1  + 0 +  8 = 9
            //                                                         3 => 3 - 3  + 0 + 13 = 13
            //                                                                         
            //                                                                           0 => 0 - 0  + 0 +  0 = 0
            //                                                                           1 => 1 - 1  + 1 +  4 = 5
            //                                                                           2 => 2 - 2  + 2 +  8 = 10
            //                                                                           3 => 3 - 3  + 1 + 13 = 14
            //                                                                         
            //                                                                                             1 => 0 - 0  + 2 +  4 = 6
            //                                                                                             2 => 1 - 1  + 3 +  8 = 11
            //                                                                                             2 => 2 - 1  + 3 +  8 = 12
            //                                                                                             3 => 3 - 3  + 2 + 13 = 15

            array_view<unsigned> input_av(int(input.size()), input);
            std::array<unsigned, 16> output;
            array_view<unsigned> output_av(int(output.size()), output);
            amp_algorithms::fill(output_av, 0);

            amp_algorithms::_details::radix_sort_by_key<unsigned, 2, 4>(amp_algorithms::_details::auto_select_target(), input_av, output_av, 0);

            output_av.synchronize();
            Assert::IsTrue(are_equal(sorted_by_key_0, output_av));
        }

        TEST_METHOD(amp_details_radix_sort_by_key_index_0_tile_8)
        {
            // gidx                                                    0   1   2   3   4   5   6   7     8   9  10  11  12  13  14  15
            std::array<unsigned, 16> input =                         { 3,  2,  1,  6, 10, 11, 13,  0,   15, 10,  5, 14,  4, 12,  9,  8 };
            // rdx =                                                   3,  2,  1,  2,  2,  3,  1,  0,    3,  2,  1,  2,  0,  0,  1,  0

            std::array<unsigned, 16> per_tile_rdx_histograms =       { 1,  2,  3,  2,        0,0,0,0,    3,  2,  2,  1,        0,0,0,0 };
            std::array<unsigned, 16> per_tile_rdx_offsets =          { 0,  1,  3,  6,        0,0,0,0,    0,  3,  5,  7,        0,0,0,0 };

            std::array<unsigned, 16> per_tile_rdx_histograms_tp =    { 1,  3,    2,  2,    3,  2,    2,  1,            0,0,0,0,0,0,0,0 };
            std::array<unsigned, 16> xxx =                           { 0,  1,    0,  2,    0,  3,    0,  2,            0,0,0,0,0,0,0,0 };
            std::array<unsigned, 16> tile_rdx_offsets =              { 0,  0,  0,  0,        0,0,0,0,    1,  2,  3,  2,        0,0,0,0 };

            std::array<unsigned, 16> global_histogram =              { 4,  4,  5,  3,                          0,0,0,0,0,0,0,0,0,0,0,0 };
            std::array<unsigned, 16> global_rdx_offsets =            { 0,  4,  8, 13,                          0,0,0,0,0,0,0,0,0,0,0,0 };
                                                                    
            std::array<unsigned, 16> sorted_by_key_0 =               { 0,  4, 12,  8,  1, 13,  5,  9,    2,  6, 10, 10, 14,  3, 11, 15 };
                                                           
            array_view<unsigned> input_av(int(input.size()), input);
            std::array<unsigned, 16> output;
            array_view<unsigned> output_av(int(output.size()), output);
            amp_algorithms::fill(output_av, 0);

            amp_algorithms::_details::radix_sort_by_key<unsigned, 2, 8>(amp_algorithms::_details::auto_select_target(), input_av, output_av, 0);

            output_av.synchronize();
            Assert::IsTrue(are_equal(sorted_by_key_0, output_av));
        }

        TEST_METHOD(amp_details_radix_sort_by_key_index_0_tile_32)
        {
            // gidx                                                    0   1   2   3   4   5   6   7     8   9  10  11  12  13  14  15
            std::array<unsigned, 16> input =                         { 3,  2,  1,  6, 10, 11, 13,  0,   15, 10,  5, 14,  4, 12,  9,  8 };
            // rdx =                                                   3,  2,  1,  2,  2,  3,  1,  0,    3,  2,  1,  2,  0,  0,  1,  0

            std::array<unsigned, 16> per_tile_rdx_histograms =       { 4,  4,  5,  3,                          0,0,0,0,0,0,0,0,0,0,0,0 };
            std::array<unsigned, 16> per_tile_rdx_offsets =          { 0,  4,  8, 13,                          0,0,0,0,0,0,0,0,0,0,0,0 };

            std::array<unsigned, 16> per_tile_rdx_histograms_tp =    { 4,  4,  5,  3,                          0,0,0,0,0,0,0,0,0,0,0,0 };
            std::array<unsigned, 16> xxx =                           { 0,  0,  0,  0,                          0,0,0,0,0,0,0,0,0,0,0,0 };
            std::array<unsigned, 16> tile_rdx_offsets =              { 0,  0,  0,  0,                          0,0,0,0,0,0,0,0,0,0,0,0 };

            std::array<unsigned, 16> global_histogram =              { 4,  4,  5,  3,                          0,0,0,0,0,0,0,0,0,0,0,0 };
            std::array<unsigned, 16> global_rdx_offsets =            { 0,  4,  8, 13,                          0,0,0,0,0,0,0,0,0,0,0,0 };

            std::array<unsigned, 16> sorted_by_key_0 =               { 0,  4, 12,  8,    1, 13,  5,  9,    2,  6, 10, 10,   14,  3, 11, 15 };
            array_view<unsigned> input_av(int(input.size()), input);
            std::array<unsigned, 16> output;
            array_view<unsigned> output_av(int(output.size()), output); 
            amp_algorithms::fill(output_av, 0);

            amp_algorithms::_details::radix_sort_by_key<unsigned, 2, 32>(amp_algorithms::_details::auto_select_target(), input_av, output_av, 0);

            output_av.synchronize();
            Assert::IsTrue(are_equal(sorted_by_key_0, output_av));
        }

        TEST_METHOD(amp_details_radix_sort_by_key_index_1_tile_4)
        {
            std::array<unsigned, 16> input =                         { 0,  4, 12,  8,   1, 13,  5,  9,    2,  6, 10, 10,   14,  3, 11, 15 };
            std::array<unsigned, 16> sorted_by_key_1 =               { 0,  1,  2,  3,   4,  5,  6,  8,    9, 10, 10, 11,   12, 13, 14, 15 };

            array_view<unsigned> input_av(int(input.size()), input);
            std::array<unsigned, 16> output;
            array_view<unsigned> output_av(int(output.size()), output);
            amp_algorithms::fill(output_av, 0);

            amp_algorithms::_details::radix_sort_by_key<unsigned, 2, 8>(amp_algorithms::_details::auto_select_target(), input_av, output_av, 1);

            output_av.synchronize();
            Assert::IsTrue(are_equal(sorted_by_key_1, output_av));
        }

        TEST_METHOD(amp_details_radix_sort_by_key_index_1_tile_8)
        {
            std::array<unsigned, 16> input =                         { 0,  4, 12,  8,   1, 13,  5,  9,    2,  6, 10, 10,   14,  3, 11, 15 };
            std::array<unsigned, 16> sorted_by_key_1 =               { 0,  1,  2,  3,    4,  5,  6,  8,    9, 10, 10, 11,   12, 13, 14, 15 };
            array_view<unsigned> input_av(int(input.size()), input);
            std::array<unsigned, 16> output;
            array_view<unsigned> output_av(int(output.size()), output);
            amp_algorithms::fill(output_av, 0);

            amp_algorithms::_details::radix_sort_by_key<unsigned, 2, 8>(amp_algorithms::_details::auto_select_target(), input_av, output_av, 1);

            output_av.synchronize();
            Assert::IsTrue(are_equal(sorted_by_key_1, output_av));
        }

        TEST_METHOD(amp_details_radix_sort_tile_4)
        {
            std::array<unsigned, 16> input =                         { 3,  2,  1,  6,   10, 11, 13,  0,   15, 10,  5, 14,    4, 12,  9,  8 };
            std::array<unsigned, 16> sorted =                        { 0,  1,  2,  3,    4,  5,  6,  8,    9, 10, 10, 11,   12, 13, 14, 15 };
            array_view<unsigned> input_av(int(input.size()), input);
            std::array<unsigned, 16> output;
            array_view<unsigned> output_av(int(output.size()), output);
            amp_algorithms::fill(output_av, 0);
            
            amp_algorithms::_details::radix_sort<unsigned, 2, 4>(amp_algorithms::_details::auto_select_target(), input_av, output_av);

            output_av.synchronize();
            Assert::IsTrue(are_equal(sorted, output_av));
        }

        TEST_METHOD(amp_details_radix_sort_tile_8)
        {
            std::array<unsigned, 16> input =                         { 3,  2,  1,  6,   10, 11, 13,  0,   15, 10,  5, 14,    4, 12,  9,  8 };
            std::array<unsigned, 16> sorted =                        { 0,  1,  2,  3,    4,  5,  6,  8,    9, 10, 10, 11,   12, 13, 14, 15 };
            array_view<unsigned> input_av(int(input.size()), input);
            std::array<unsigned, 16> output;
            array_view<unsigned> output_av(int(output.size()), output);
            amp_algorithms::fill(output_av, 0);
            
            amp_algorithms::_details::radix_sort<unsigned, 2, 8>(amp_algorithms::_details::auto_select_target(), input_av, output_av);

            output_av.synchronize();
            Assert::IsTrue(are_equal(sorted, output_av));
        }

        TEST_METHOD(amp_details_radix_sort_tile_32)
        {
            std::array<unsigned, 16> input =                         { 3,  2,  1,  6,   10, 11, 13,  0,   15, 10,  5, 14,    4, 12,  9,  8 };
            std::array<unsigned, 16> sorted =                        { 0,  1,  2,  3,    4,  5,  6,  8,    9, 10, 10, 11,   12, 13, 14, 15 };
            array_view<unsigned> input_av(int(input.size()), input);
            std::array<unsigned, 16> output;
            array_view<unsigned> output_av(int(output.size()), output);
            amp_algorithms::fill(output_av, 0);
            
            amp_algorithms::_details::radix_sort<unsigned, 2, 32>(amp_algorithms::_details::auto_select_target(), input_av, output_av);

            output_av.synchronize();
            Assert::IsTrue(are_equal(sorted, output_av));
        }

        TEST_METHOD(amp_details_radix_sort_tile_4_data_1024)
        {
            std::vector<int> input(1024, 1);
            std::iota(rbegin(input), rend(input), 0);
            concurrency::array_view<int, 1> input_av(int(input.size()), input);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 0);
            std::vector<int> output(input.size(), 0);
            array_view<int> output_av(int(output.size()), output);

            amp_algorithms::_details::radix_sort<int, 2, 4>(amp_algorithms::_details::auto_select_target(), input_av, output_av);

            output_av.synchronize();
            Assert::IsTrue(are_equal(expected, output_av));
        }

        TEST_METHOD(amp_details_radix_sort_tile_8_data_1024)
        {
            std::vector<int> input(1024, 1);
            std::iota(rbegin(input), rend(input), 0);
            concurrency::array_view<int, 1> input_av(int(input.size()), input);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 0);
            std::vector<int> output(input.size(), 0);
            array_view<int> output_av(int(output.size()), output);

            amp_algorithms::_details::radix_sort<int, 2, 8>(amp_algorithms::_details::auto_select_target(), input_av, output_av);

            output_av.synchronize();
            Assert::IsTrue(are_equal(expected, output_av));
        }

        TEST_METHOD(amp_details_radix_sort_tile_16_data_1024)
        {
            std::vector<int> input(1024, 1);
            std::iota(rbegin(input), rend(input), 0);
            concurrency::array_view<int, 1> input_av(int(input.size()), input);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 0);
            std::vector<int> output(input.size(), 0);
            array_view<int> output_av(int(output.size()), output);

            amp_algorithms::_details::radix_sort<int, 2, 16>(amp_algorithms::_details::auto_select_target(), input_av, output_av);

            output_av.synchronize();
            Assert::IsTrue(are_equal(expected, output_av));
        }

        TEST_METHOD(amp_details_radix_sort_tile_32_data_1024)
        {
            std::vector<int> input(1024, 1);
            std::iota(rbegin(input), rend(input), 0);
            concurrency::array_view<int, 1> input_av(int(input.size()), input);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 0);
            std::vector<int> output(input.size(), 0);
            array_view<int> output_av(int(output.size()), output);

            amp_algorithms::_details::radix_sort<int, 2, 32>(amp_algorithms::_details::auto_select_target(), input_av, output_av);

            output_av.synchronize();
            Assert::IsTrue(are_equal(expected, output_av));
        }


        TEST_METHOD(amp_radix_sort_16)
        {
            std::vector<int> input(16, 1);
            std::iota(rbegin(input), rend(input), 0);
            concurrency::array_view<int, 1> input_vw(int(input.size()), input);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 0);
            std::vector<int> output(input.size(), 0);
            array_view<int> output_av(int(output.size()), output);

            radix_sort(input_vw, output_av);

            output_av.synchronize();
            Assert::IsTrue(are_equal(expected, output_av));
        }

        TEST_METHOD(amp_radix_sort_64)
        {
            std::vector<int> input(64, 1);
            std::iota(rbegin(input), rend(input), 0);
            concurrency::array_view<int, 1> input_vw(int(input.size()), input);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 0);
            std::vector<int> output(input.size(), 0);
            array_view<int> output_av(int(output.size()), output);

            radix_sort(input_vw, output_av);

            output_av.synchronize();
            Assert::IsTrue(are_equal(expected, output_av));
        }

        TEST_METHOD(amp_radix_sort_128)
        {
            std::vector<int> input(128, 1);
            std::iota(rbegin(input), rend(input), 0);
            concurrency::array_view<int, 1> input_vw(int(input.size()), input);
            std::vector<int> expected(input.size());
            std::iota(begin(expected), end(expected), 0);
            std::vector<int> output(input.size(), 0);
            array_view<int> output_av(int(output.size()), output);

            radix_sort(input_vw, output_av);

            output_av.synchronize();
            Assert::IsTrue(are_equal(expected, output_av));
        }

        // TODO: More tests for radix sort.
    };
}; // namespace amp_algorithms_tests

// TODO: Finish make_array_view, assuming we really need it.

template< typename ConstRandomAccessIterator >
void make_array_view( ConstRandomAccessIterator first, ConstRandomAccessIterator last )
{
    return array_view(std::distance(first, last), first);
}
