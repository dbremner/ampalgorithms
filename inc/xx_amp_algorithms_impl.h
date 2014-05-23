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

#pragma once

#include <amp.h>
#include <assert.h>
#include <sstream>

#include <xx_amp_algorithms_impl_inl.h>
#include <amp_indexable_view.h>

namespace amp_algorithms
{
#if !defined(AMP_ALGORITHMS_ENUMS)
#define AMP_ALGORITHMS_ENUMS
    enum class scan_mode : int
    {
        exclusive = 0,
        inclusive = 1
    };

    // TODO: Duplicated in both amp and direct3d namespaces. Fix this?
    enum class scan_direction : int
    {
        forward = 0,
        backward = 1
    };
#endif

    namespace _details
    {
        inline concurrency::accelerator_view auto_select_target()
        {
#if _MSC_VER < 1800
            static concurrency::accelerator_view auto_select_accelerator_view = concurrency::accelerator(concurrency::accelerator::cpu_accelerator).create_view();
            return auto_select_accelerator_view;
#else
            return concurrency::accelerator::get_auto_selection_view();
#endif
        }

        //----------------------------------------------------------------------------
        // parallel_for_each implementation
        //----------------------------------------------------------------------------

        template <int _Rank, typename _Kernel_type>
        void parallel_for_each(const concurrency::accelerator_view &_Accl_view, const concurrency::extent<_Rank>& _Compute_domain, const _Kernel_type &_Kernel)
        {
#if _MSC_VER < 1800
            _Host_Scheduling_info _SchedulingInfo = { NULL };
            if (_Accl_view != _details::auto_select_target()) 
            {
                _SchedulingInfo._M_accelerator_view = concurrency::details::_Get_accelerator_view_impl_ptr(_Accl_view);
            }

            concurrency::details::_Parallel_for_each(&_SchedulingInfo, _Compute_domain, _Kernel);
#else
            concurrency::parallel_for_each(_Accl_view, _Compute_domain, _Kernel);
#endif
        }

        template <int _Dim0, int _Dim1, int _Dim2, typename _Kernel_type>
        void parallel_for_each(const concurrency::accelerator_view &_Accl_view, const concurrency::tiled_extent<_Dim0, _Dim1, _Dim2>& _Compute_domain, const _Kernel_type& _Kernel)
        {
#if _MSC_VER < 1800
            _Host_Scheduling_info _SchedulingInfo = { NULL };
            if (_Accl_view != _details::auto_select_target()) 
            {
                _SchedulingInfo._M_accelerator_view = concurrency::details::_Get_accelerator_view_impl_ptr(_Accl_view);
            }

            concurrency::details::_Parallel_for_each(&_SchedulingInfo, _Compute_domain, _Kernel);
#else
            concurrency::parallel_for_each(_Accl_view, _Compute_domain, _Kernel);
#endif
        }

        template <int _Dim0, int _Dim1, typename _Kernel_type>
        void parallel_for_each(const concurrency::accelerator_view &_Accl_view, const concurrency::tiled_extent<_Dim0, _Dim1>& _Compute_domain, const _Kernel_type& _Kernel)
        {
#if _MSC_VER < 1800
            _Host_Scheduling_info _SchedulingInfo = { NULL };
            if (_Accl_view != _details::auto_select_target()) 
            {
                _SchedulingInfo._M_accelerator_view = concurrency::details::_Get_accelerator_view_impl_ptr(_Accl_view);
            }

            concurrency::details::_Parallel_for_each(&_SchedulingInfo, _Compute_domain, _Kernel);
#else
            concurrency::parallel_for_each(_Accl_view, _Compute_domain, _Kernel);
#endif
        }

        template <int _Dim0, typename _Kernel_type>
        void parallel_for_each(const concurrency::accelerator_view &_Accl_view, const concurrency::tiled_extent<_Dim0>& _Compute_domain, const _Kernel_type& _Kernel)
        {
#if _MSC_VER < 1800
            _Host_Scheduling_info _SchedulingInfo = { NULL };
            if (_Accl_view != _details::auto_select_target()) 
            {
                _SchedulingInfo._M_accelerator_view = concurrency::details::_Get_accelerator_view_impl_ptr(_Accl_view);
            }

            concurrency::details::_Parallel_for_each(&_SchedulingInfo, _Compute_domain, _Kernel);
#else
            concurrency::parallel_for_each(_Accl_view, _Compute_domain, _Kernel);
#endif
        }

        //----------------------------------------------------------------------------
        // reduce implementation
        //---------------------------------------------------------------------------- 

        // This function performs an in-place reduction through co-operating threads within a tile.
        // The input data is in the parameter "mem" and is reduced in-place modifying its existing contents
        // The output (reduced result) is contained in "mem[0]" at the end of this function
        // The parameter "partial_data_length" is used to indicate if the size of data in "mem" to be
        // reduced is same as the tile size and if not what is the length of valid data in "mem".
        template <typename T, unsigned int tile_size, typename functor>
        void tile_local_reduction(T* const mem, concurrency::tiled_index<tile_size> tid, const functor& op, int partial_data_length) restrict(amp)
        {
            // local index
            int local = tid.local[0];

            if (partial_data_length < tile_size)
            {
                // unrolled for performance
                if (partial_data_length >  512) { if (local < (partial_data_length - 512)) { mem[0] = op(mem[0], mem[512]); } tid.barrier.wait_with_tile_static_memory_fence(); }
                if (partial_data_length >  256) { if (local < (partial_data_length - 256)) { mem[0] = op(mem[0], mem[256]); } tid.barrier.wait_with_tile_static_memory_fence(); }
                if (partial_data_length >  128) { if (local < (partial_data_length - 128)) { mem[0] = op(mem[0], mem[128]); } tid.barrier.wait_with_tile_static_memory_fence(); }
                if (partial_data_length >   64) { if (local < (partial_data_length - 64)) { mem[0] = op(mem[0], mem[64]); } tid.barrier.wait_with_tile_static_memory_fence(); }
                if (partial_data_length >   32) { if (local < (partial_data_length - 32)) { mem[0] = op(mem[0], mem[32]); } tid.barrier.wait_with_tile_static_memory_fence(); }
                if (partial_data_length >   16) { if (local < (partial_data_length - 16)) { mem[0] = op(mem[0], mem[16]); } tid.barrier.wait_with_tile_static_memory_fence(); }
                if (partial_data_length >    8) { if (local < (partial_data_length - 8)) { mem[0] = op(mem[0], mem[8]); } tid.barrier.wait_with_tile_static_memory_fence(); }
                if (partial_data_length >    4) { if (local < (partial_data_length - 4)) { mem[0] = op(mem[0], mem[4]); } tid.barrier.wait_with_tile_static_memory_fence(); }
                if (partial_data_length >    2) { if (local < (partial_data_length - 2)) { mem[0] = op(mem[0], mem[2]); } tid.barrier.wait_with_tile_static_memory_fence(); }
                if (partial_data_length >    1) { if (local < (partial_data_length - 1)) { mem[0] = op(mem[0], mem[1]); } tid.barrier.wait_with_tile_static_memory_fence(); }
            }
            else
            {
                // unrolled for performance
                if (tile_size >= 1024) { if (local < 512) { mem[0] = op(mem[0], mem[512]); } tid.barrier.wait_with_tile_static_memory_fence(); }
                if (tile_size >= 512) { if (local < 256) { mem[0] = op(mem[0], mem[256]); } tid.barrier.wait_with_tile_static_memory_fence(); }
                if (tile_size >= 256) { if (local < 128) { mem[0] = op(mem[0], mem[128]); } tid.barrier.wait_with_tile_static_memory_fence(); }
                if (tile_size >= 128) { if (local < 64) { mem[0] = op(mem[0], mem[64]); } tid.barrier.wait_with_tile_static_memory_fence(); }
                if (tile_size >= 64) { if (local < 32) { mem[0] = op(mem[0], mem[32]); } tid.barrier.wait_with_tile_static_memory_fence(); }
                if (tile_size >= 32) { if (local < 16) { mem[0] = op(mem[0], mem[16]); } tid.barrier.wait_with_tile_static_memory_fence(); }
                if (tile_size >= 16) { if (local < 8) { mem[0] = op(mem[0], mem[8]); } tid.barrier.wait_with_tile_static_memory_fence(); }
                if (tile_size >= 8) { if (local < 4) { mem[0] = op(mem[0], mem[4]); } tid.barrier.wait_with_tile_static_memory_fence(); }
                if (tile_size >= 4) { if (local < 2) { mem[0] = op(mem[0], mem[2]); } tid.barrier.wait_with_tile_static_memory_fence(); }
                if (tile_size >= 2) { if (local < 1) { mem[0] = op(mem[0], mem[1]); } tid.barrier.wait_with_tile_static_memory_fence(); }
            }
        }

        // Generic reduction of a 1D indexable view with a reduction binary functor
        template<unsigned int tile_size,
            unsigned int max_tiles,
            typename InputIndexableView,
            typename BinaryFunction>
            typename std::result_of<BinaryFunction(const typename indexable_view_traits<InputIndexableView>::value_type&, const typename indexable_view_traits<InputIndexableView>::value_type&)>::type
            reduce(const concurrency::accelerator_view &accl_view, const InputIndexableView &input_view, const BinaryFunction &binary_op)
        {
                // The input view must be of rank 1
                static_assert(indexable_view_traits<InputIndexableView>::rank == 1, "The input indexable view must be of rank 1");
                typedef typename std::result_of<BinaryFunction(const typename indexable_view_traits<InputIndexableView>::value_type&, const typename indexable_view_traits<InputIndexableView>::value_type&)>::type result_type;

                // runtime sizes
                int n = input_view.extent.size();
                unsigned int tile_count = (n + tile_size - 1) / tile_size;
                tile_count = std::min(tile_count, max_tiles);

                // simultaneous live threads
                const unsigned int thread_count = tile_count * tile_size;

                // global buffer (return type)
                concurrency::array_view<result_type> global_buffer_view(concurrency::array<result_type>(tile_count, concurrency::accelerator(concurrency::accelerator::cpu_accelerator).default_view, accl_view));

                // configuration
                concurrency::extent<1> extent(thread_count);

                _details::parallel_for_each(
                    accl_view,
                    extent.tile<tile_size>(),
                    [=](concurrency::tiled_index<tile_size> tid) restrict(amp)
                {
                    // shared tile buffer
                    tile_static result_type local_buffer[tile_size];

                    int idx = tid.global[0];

                    // this threads's shared memory pointer
                    result_type& smem = local_buffer[tid.local[0]];

                    // this variable is used to test if we are on the edge of data within tile
                    int partial_data_length = n - tid.tile[0] * tile_size;

                    // initialize local buffer
                    smem = input_view[concurrency::index<1>(idx)];
                    // next chunk
                    idx += thread_count;

                    // fold data into local buffer
                    while (idx < n)
                    {
                        // reduction of smem and X[idx] with results stored in smem
                        smem = binary_op(smem, input_view[concurrency::index<1>(idx)]);

                        // next chunk
                        idx += thread_count;
                    }

                    // synchronize
                    tid.barrier.wait_with_tile_static_memory_fence();

                    // reduce all values in this tile
                    _details::tile_local_reduction(&smem, tid, binary_op, partial_data_length);

                    // only 1 thread per tile does the inter tile communication
                    if (tid.local[0] == 0)
                    {
                        // write to global buffer in this tiles
                        global_buffer_view[tid.tile[0]] = smem;
                    }
                });

                // 2nd pass reduction
                result_type *pGlobalBufferViewData = global_buffer_view.data();
                result_type retVal = pGlobalBufferViewData[0];
                for (unsigned int i = 1; i < tile_count; ++i) {
                    retVal = binary_op(retVal, pGlobalBufferViewData[i]);
                }

                return retVal;
        }

        //----------------------------------------------------------------------------
        // scan implementation
        //----------------------------------------------------------------------------

        // The current scan implementation uses the warp size.
#if (defined(USE_REF) || defined(_DEBUG))
        static const int scan_warp_size = 4;
        static const int scan_default_tile_size = 8;
#else
        static const int scan_warp_size = 32;
        static const int scan_default_tile_size = 512;
#endif

        template <amp_algorithms::scan_mode _Mode, typename _BinaryOp, typename T>
        T scan_warp(T* const tile_data, const int idx, const _BinaryOp& op) restrict(amp)
        {
            const int warp_max = _details::scan_warp_size - 1;
            const int widx = idx & warp_max;

            if (widx >= 1)
                tile_data[idx] = op(tile_data[idx - 1], tile_data[idx]);
            if ((scan_warp_size > 2) && (widx >= 2))
                tile_data[idx] = op(tile_data[idx - 2], tile_data[idx]);
            if ((scan_warp_size > 4) && (widx >= 4))
                tile_data[idx] = op(tile_data[idx - 4], tile_data[idx]);
            if ((scan_warp_size > 8) && (widx >= 8))
                tile_data[idx] = op(tile_data[idx - 8], tile_data[idx]);
            if ((scan_warp_size > 16) && (widx >= 16))
                tile_data[idx] = op(tile_data[idx - 16], tile_data[idx]);
            if ((scan_warp_size > 32) && (widx >= 32))
                tile_data[idx] = op(tile_data[idx - 32], tile_data[idx]);

            if (_Mode == scan_mode::inclusive)
                return tile_data[idx];
            return (widx > 0) ? tile_data[idx - 1] : T();
        }

        template <int TileSize, scan_mode _Mode, typename _BinaryOp, typename T>
        T scan_tile(T* const tile_data, concurrency::tiled_index<TileSize> tidx, const _BinaryOp& op) restrict(amp)
        {
            static_assert(is_power_of_two<scan_warp_size>::value, "Warp size must be an exact power of 2.");

            const int warp_max = _details::scan_warp_size - 1;
            const int lidx = tidx.local[0];
            const int warp_id = lidx >> log2<scan_warp_size>::value;

            // Step 1: Intra-warp scan in each warp
            auto val = scan_warp<_Mode, _BinaryOp>(tile_data, lidx, op);
            tidx.barrier.wait_with_tile_static_memory_fence();

            // Step 2: Collect per-warp partial results
            if ((lidx & warp_max) == warp_max)
                tile_data[warp_id] = tile_data[lidx];
            tidx.barrier.wait_with_tile_static_memory_fence();

            // Step 3: Use 1st warp to scan per-warp results
            if (warp_id == 0)
                scan_warp<scan_mode::inclusive>(tile_data, lidx, op);
            tidx.barrier.wait_with_tile_static_memory_fence();

            // Step 4: Accumulate results from Steps 1 and 3
            if (warp_id > 0)
                val = op(tile_data[warp_id - 1], val);
            tidx.barrier.wait_with_tile_static_memory_fence();

            // Step 5: Write and return the final result
            tile_data[lidx] = val;
            tidx.barrier.wait_with_tile_static_memory_fence();
            return val;
        }

        template <int TileSize, scan_mode _Mode, typename _BinaryFunc, typename InputIndexableView>
        inline void scan(const concurrency::accelerator_view& accl_view, const InputIndexableView& input_view, InputIndexableView& output_view, const _BinaryFunc& op)
        {
            static_assert(TileSize >= _details::scan_warp_size, "Tile size must be at least the size of a single warp.");
            static_assert(TileSize % _details::scan_warp_size == 0, "Tile size must be an exact multiple of warp size.");
            static_assert(TileSize <= (_details::scan_warp_size * _details::scan_warp_size), "Tile size must less than or equal to the square of the warp size.");
            assert(output_view.extent[0] >= _details::scan_warp_size);

            typedef InputIndexableView::value_type T;

            auto compute_domain = output_view.extent.tile<TileSize>().pad();
            concurrency::array<T, 1> tile_results(compute_domain / TileSize, accl_view);
            concurrency::array_view<T, 1> tile_results_vw(tile_results);
            // 1 & 2. Scan all tiles and store results in tile_results.
            concurrency::parallel_for_each(accl_view, compute_domain, [=](concurrency::tiled_index<TileSize> tidx) restrict(amp)
            {
                const int gidx = tidx.global[0];
                const int lidx = tidx.local[0];
                tile_static T tile_data[TileSize];
                tile_data[lidx] = padded_read(input_view, gidx);
                tidx.barrier.wait_with_tile_static_memory_fence();

                auto val = _details::scan_tile<TileSize, _Mode>(tile_data, tidx, amp_algorithms::plus<T>());
                if (lidx == (TileSize - 1))
                {
                    tile_results_vw[tidx.tile[0]] = val;
                    if (_Mode == scan_mode::exclusive)
                        tile_results_vw[tidx.tile[0]] += input_view[gidx];
                }
                padded_write(output_view, gidx, tile_data[lidx]);
            });

            // 3. Scan tile results.
            if (tile_results_vw.extent[0] > TileSize)
            {
                scan<TileSize, amp_algorithms::scan_mode::exclusive>(accl_view, tile_results_vw, tile_results_vw, op);
            }
            else
            {
                concurrency::parallel_for_each(accl_view, compute_domain, [=](concurrency::tiled_index<TileSize> tidx) restrict(amp)
                {
                    const int gidx = tidx.global[0];
                    const int lidx = tidx.local[0];
                    tile_static T tile_data[TileSize];
                    tile_data[lidx] = tile_results_vw[gidx];
                    tidx.barrier.wait_with_tile_static_memory_fence();

                    _details::scan_tile<TileSize, amp_algorithms::scan_mode::exclusive>(tile_data, tidx, amp_algorithms::plus<T>());

                    tile_results_vw[gidx] = tile_data[lidx];
                    tidx.barrier.wait_with_tile_static_memory_fence();
                });
            }
            // 4. Add the tile results to the individual results for each tile.
            concurrency::parallel_for_each(accl_view, compute_domain, [=](concurrency::tiled_index<TileSize> tidx) restrict(amp)
            {
                const int gidx = tidx.global[0];
                if (gidx < output_view.extent[0])
                    output_view[gidx] += tile_results_vw[tidx.tile[0]];
            });
        }

        //----------------------------------------------------------------------------
        // radix sort implementation
        //----------------------------------------------------------------------------

    } // namespace amp_algorithms::_details

} // namespace amp_algorithms
