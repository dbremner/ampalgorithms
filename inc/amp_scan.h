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

namespace amp_algorithms
{
#pragma region New Scan implementation

    enum class scan_mode
    {
        exclusive = 0,
        inclusive = 1
    };

    namespace _details
    {
        enum class warp : int
        {
            max_idx = 31,
            size = max_idx + 1
        };
        // TODO: Don't use #define! 
        #define WARP_SIZE 32

        // 
        template <scan_mode _Mode, typename _BinaryOp, typename T>
        T scan_warp(T* const p, const int idx, const _BinaryOp& op) restrict(amp)
        {
            const int widx = idx & 31;
            // TODO: Unroll? Unroll for 32 wide warp or 64? Templated unroll?
            if (widx >= 1) p[idx] = op(p[idx - 1], p[idx]);
            if (widx >= 2) p[idx] = op(p[idx - 2], p[idx]);
            if (widx >= 4) p[idx] = op(p[idx - 4], p[idx]);
            if (widx >= 8) p[idx] = op(p[idx - 8], p[idx]);
            if (widx >= 16) p[idx] = op(p[idx - 16], p[idx]);

            if (_Mode == scan_mode::inclusive)
                return p[idx];
            return (widx > 0) ? p[idx - 1] : T();
        }

        template <int TileSize, scan_mode _Mode, typename _BinaryOp, typename T>
        T scan_tile(T* const p, tiled_index<TileSize> tidx, const _BinaryOp& op) restrict(amp)
        {
            const int idx = tidx.local[0];
            const int warp_id = idx >> 5;
            const int widx = idx & 31;

            // Step 1: Intra-warp scan in each warp
            auto val = scan_warp<_Mode, _BinaryOp>(p, idx, op);
            tidx.barrier.wait_with_tile_static_memory_fence();

            // Step 2: Collect per-warp partial results
            if (widx == 31)
                p[warp_id] = p[idx];
            tidx.barrier.wait_with_tile_static_memory_fence();

            // Step 3: Use 1st warp to scan per-warp results
            if (warp_id == 0)
                scan_warp<scan_mode::inclusive>(p, idx, op);
            tidx.barrier.wait_with_tile_static_memory_fence();

            // Step 4: Accumulate results from Steps 1 and 3
            if (warp_id > 0)
                val = op(p[warp_id - 1], val);
            tidx.barrier.wait_with_tile_static_memory_fence();

            // Step 5: Write and return the final result
            p[idx] = val;
            tidx.barrier.wait_with_tile_static_memory_fence();

            return val;
        }

        template <int TileSize, scan_mode _Mode, typename _BinaryOp, typename T>
        inline void scan_new(const concurrency::array<T, 1>& in, concurrency::array<T, 1>& out, const _BinaryOp& op)
        {
            static_assert(TileSize >= WARP_SIZE, "Tile size must be at least the size of a single warp.");
            static_assert(TileSize % WARP_SIZE == 0, "Tile size must be an exact multiple of warp size.");
            static_assert(TileSize <= (WARP_SIZE * WARP_SIZE), "Tile size must less than or equal to the square of the warp size.");

            const int size = out.extent[0];
            assert(size >= WARP_SIZE);
            concurrency::array<T, 1> tile_results(size / TileSize);

            // 1 & 2. Scan all tiles and store results in tile_results.
            concurrency::parallel_for_each(concurrency::tiled_extent<TileSize>(concurrency::extent<1>(size)),
                [=, &in, &out, &tile_results](concurrency::tiled_index<TileSize> tidx) restrict(amp)
            {
                tile_static T tile_data[TileSize];
                tile_data[tidx.local[0]] = in[tidx.global[0]];
                tidx.barrier.wait_with_tile_static_memory_fence();

                auto val = _details::scan_tile<TileSize, _Mode>(tile_data, tidx, amp_algorithms::plus<T>());

                if (tidx.local[0] == (TileSize - 1))
                {
                    tile_results[tidx.tile[0]] = val;

                    if (_Mode == scan_mode::exclusive)
                        tile_results[tidx.tile[0]] += in[tidx.global[0]];
                }
                out[tidx.global[0]] = tile_data[tidx.local[0]];
            });

            // 3. Scan tile results.
            // TODO: Fix this. Right now it will only work if there are no more than TileSize tiles.
            concurrency::parallel_for_each(concurrency::tiled_extent<TileSize>(concurrency::extent<1>(TileSize)),
                [=, &tile_results](concurrency::tiled_index<TileSize> tidx) restrict(amp)
            {
                tile_static T tile_data[TileSize];
                tile_data[tidx.local[0]] = tile_results[tidx.global[0]];
                tidx.barrier.wait_with_tile_static_memory_fence();

                _details::scan_tile<TileSize, amp_algorithms::scan_mode::exclusive>(tile_data, tidx, amp_algorithms::plus<T>());

                tile_results[tidx.global[0]] = tile_data[tidx.local[0]];
                tidx.barrier.wait_with_tile_static_memory_fence();
            });
            std::cerr << tile_results << std::endl;

            // 4. Add the tile results to the individual results for each tile.
            concurrency::parallel_for_each(concurrency::tiled_extent<TileSize>(concurrency::extent<1>(size)),
                [=, &out, &tile_results](concurrency::tiled_index<TileSize> tidx) restrict(amp)
            {
                out[tidx.global[0]] += tile_results[tidx.tile[0]];
            });
        }
    }

    template <int TileSize, typename InIt, typename OutIt>
    inline void scan_exclusive_new(InIt first, InIt last, OutIt dest_first)
    {
        typedef InIt::value_type T;

        const int size = int(distance(first, last));
        concurrency::array<T, 1> in(size);
        concurrency::array<T, 1> out(size);
        copy(first, last, in);

        _details::scan_new<TileSize, amp_algorithms::scan_mode::exclusive>(in, out, amp_algorithms::plus<T>());

        copy(out, dest_first);
    }

    template <int TileSize, typename InIt, typename OutIt>
    inline void scan_inclusive_new(InIt first, InIt last, OutIt dest_first)
    {
        typedef InIt::value_type T;

        const int size = int(distance(first, last));
        concurrency::array<T, 1> in(size);
        concurrency::array<T, 1> out(size);
        copy(first, last, in);

        _details::scan_new<TileSize, amp_algorithms::scan_mode::inclusive>(in, out, amp_algorithms::plus<T>());

        copy(out, dest_first);
    }

#pragma endregion

#pragma region Original Scan implementaton
    //===============================================================================
    // Exclusive scan, output element at i contains the sum of elements [0]...[i-1].
    //===============================================================================
    //
    // The following implementation is based on this GPU Gems article. 
    //
    // http.developer.nvidia.com/GPUGems3/gpugems3_ch39.html
    //
    // It has been superseded by further research, found here:
    //
    // https://research.nvidia.com/sites/default/files/publications/nvr-2008-003.pdf

    template <int TileSize, typename InIt, typename OutIt>
    inline void ExclusiveScanOptimized(InIt first, InIt last, OutIt dest_first)
    {
        typedef InIt::value_type T;

        const int size = int(distance(first, last));
        concurrency::array<T, 1> in(size);
        concurrency::array<T, 1> out(size);
        copy(first, last, in);   
        details::ScanOptimized<TileSize, amp_algorithms::scan_mode::exclusive>(concurrency::array_view<T, 1>(in), concurrency::array_view<T, 1>(out));
        copy(out, dest_first);
    }

    template <int TileSize, typename T>
    inline void ExclusiveScanOptimized(concurrency::array_view<T, 1> input_view, concurrency::array_view<T, 1> output_view)
    {
        details::ScanOptimized<TileSize, amp_algorithms::scan_mode::exclusive, T>(input_view, output_view);
    }

    //===============================================================================
    // Inclusive scan, output element at i contains the sum of elements [0]...[i].
    //===============================================================================

    template <int TileSize, typename InIt, typename OutIt>
    inline void InclusiveScanOptimized(InIt first, InIt last, OutIt dest_first)
    {
        typedef InIt::value_type T;

        const int size = int(std::distance(first, last));
        concurrency::array<T, 1> in(size);
        concurrency::array<T, 1> out(size);
        copy(first, last, in);
        details::ScanOptimized<TileSize, int(amp_algorithms::scan_mode::inclusive)>(concurrency::array_view<T, 1>(in), concurrency::array_view<T, 1>(out));
        copy(out, dest_first);
    }

    template <int TileSize, typename T>
    inline void InclusiveScanOptimized(concurrency::array_view<T, 1> input_view, concurrency::array_view<T, 1> output_view)
    {
        details::ScanOptimized<TileSize, amp_algorithms::scan_mode::inclusive, T>(input_view, output_view);
    }

    //===============================================================================
    //  Implementation. Not supposed to be called directly.
    //===============================================================================

    namespace details
    {
        template <int TileSize, int Mode, typename T>
        void ScanTiled(concurrency::array_view<T, 1> input_view, concurrency::array_view<T, 1> output_view)
        {
            static_assert((Mode == int(scan_mode::exclusive) || Mode == int(scan_mode::inclusive)), "Mode must be either inclusive or exclusive.");
            static_assert(is_power_of_two<TileSize>::result, "TileSize must be a power of 2.");
            assert(input_view.extent[0] == output_view.extent[0]);
            assert(input_view.extent[0] > 0);

            const int elementCount = input_view.extent[0];
            const int tileCount = (elementCount + TileSize - 1) / TileSize;

            // Compute tile-wise scans and reductions.
            concurrency::array<T> tileSums(tileCount);
            details::ComputeTilewiseExclusiveScanTiled<TileSize, Mode>(concurrency::array_view<const T>(input_view), concurrency::array_view<T>(output_view), concurrency::array_view<T>(tileSums));

            if (tileCount > 1)
            {
                // Calculate the initial value of each tile based on the tileSums.
                concurrency::array<T> tmp(tileSums.extent);
                ScanTiled<TileSize, int(scan_mode::exclusive)>(concurrency::array_view<T>(tileSums), concurrency::array_view<T>(tmp));
                output_view.discard_data();
                parallel_for_each(concurrency::extent<1>(elementCount), [=, &tileSums, &tmp](concurrency::index<1> idx) restrict(amp)
                {
                    int tileIdx = idx[0] / TileSize;
                    output_view[idx] += tmp[tileIdx];
                });
            }
        }

        // For each tile calculate the inclusive scan.

        template <int TileSize, int Mode, typename T>
        void ComputeTilewiseExclusiveScanTiled(concurrency::array_view<const T> input_view, concurrency::array_view<T> tilewiseOutput, concurrency::array_view<T> tileSums)
        {
            const int elementCount = input_view.extent[0];
            const int tileCount = (elementCount + TileSize - 1) / TileSize;
            const int threadCount = tileCount * TileSize;

            tilewiseOutput.discard_data();
            parallel_for_each(concurrency::extent<1>(threadCount).tile<TileSize>(), [=](concurrency::tiled_index<TileSize> tidx) restrict(amp)
            {
                const int tid = tidx.local[0];
                const int gid = tidx.global[0];

                tile_static T tile_data[2][TileSize];
                int inIdx = 0;
                int outIdx = 1;

                // Do the first pass (offset = 1) while loading elements into tile_static memory.

                if (gid < elementCount)
                {
                    if (tid >= 1)
                        tile_data[outIdx][tid] = input_view[gid - 1] + input_view[gid];
                    else
                        tile_data[outIdx][tid] = input_view[gid];
                }
                tidx.barrier.wait_with_tile_static_memory_fence();

                for (int offset = 2; offset < TileSize; offset *= 2)
                {
                    flip_indeces(inIdx, outIdx);

                    if (gid < elementCount)
                    {
                        if (tid >= offset)
                            tile_data[outIdx][tid] = tile_data[inIdx][tid - offset] + tile_data[inIdx][tid];
                        else
                            tile_data[outIdx][tid] = tile_data[inIdx][tid];
                    }
                    tidx.barrier.wait_with_tile_static_memory_fence();
                }

                // Copy tile results out. For exclusive scan shift all elements right.

                if (gid < elementCount)
                {
                    // For exclusive scan calculate the last value
                    if (Mode == int(scan_mode::inclusive))
                        tilewiseOutput[gid] = tile_data[outIdx][tid];
                    else
                    if (tid == 0)
                        tilewiseOutput[gid] = T(0);
                    else
                        tilewiseOutput[gid] = tile_data[outIdx][tid - 1];
                }

                // Last thread in tile updates the tileSums.
                if (tid == TileSize - 1)
                    tileSums[tidx.tile[0]] = tile_data[outIdx][tid - 1] + +input_view[gid];
            });
        }

        void flip_indeces(int& index1, int& index2) restrict(amp)
        {
            index1 = 1 - index1;
            index2 = 1 - index2;
        }

        // TODO: Not using conflict free offsets yet!
        template <int BlockSize, int LogBlockSize>
        inline int conflict_free_offset(int n) restrict(amp)
        {
            return n >> BlockSize + n >> (2 * LogBlockSize);
        }

        template <int TileSize, int Mode, typename T>  
        void ScanOptimized(concurrency::array_view<T, 1> input_view, concurrency::array_view<T, 1> output_view)
        {
            const int domainSize = TileSize * 2;
            const int elementCount = input_view.extent[0];
            const int tileCount = (elementCount + domainSize - 1) / domainSize;

            static_assert((Mode == int(scan_mode::exclusive) || Mode == int(scan_mode::inclusive)), "Mode must be either inclusive or exclusive.");
            static_assert(is_power_of_two<TileSize>::result, "TileSize must be a power of 2.");
            assert(elementCount > 0);
            assert(elementCount == output_view.extent[0]);
            assert((elementCount / domainSize) >= 1);

            // Compute scan for each tile and store their total values in tileSums
            concurrency::array<T> tileSums(tileCount);
            details::ComputeTilewiseScanExclusive<TileSize, Mode>(concurrency::array_view<const T>(input_view), output_view, concurrency::array_view<T>(tileSums));
        
            if (tileCount > 1)
            {
                // Calculate the initial value of each tile based on the tileSums.
                concurrency::array<T> tileSumScan(tileSums.extent);
                ScanTiled<TileSize, int(scan_mode::exclusive)>(concurrency::array_view<T>(tileSums), concurrency::array_view<T>(tileSumScan));

                // Add the tileSums all the elements in each tile except the first tile.
                output_view.discard_data();
                parallel_for_each(concurrency::extent<1>(elementCount - domainSize), [=, &tileSumScan] (concurrency::index<1> idx) restrict (amp) 
                {
                    const int tileIdx = (idx[0] + domainSize) / domainSize;
                    output_view[idx + domainSize] += tileSumScan[tileIdx];
                });
            }
        }

        template <int TileSize, int Mode, typename T>
        void ComputeTilewiseScanExclusive(concurrency::array_view<const T, 1> input_view, concurrency::array_view<T> tilewiseOutput, concurrency::array_view<T, 1> tileSums)
        {
            static const int domainSize = TileSize * 2;
            const int elementCount = input_view.extent[0];
            const int tileCount = (elementCount + TileSize - 1) / TileSize;
            const int threadCount = tileCount * TileSize;

            tilewiseOutput.discard_data();
            tileSums.discard_data();
            parallel_for_each(concurrency::extent<1>(threadCount).tile<TileSize>(), [=](concurrency::tiled_index<TileSize> tidx) restrict(amp)
            {
                const int tid = tidx.local[0];
                const int tidx2 = tidx.local[0] * 2;
                const int gidx2 = tidx.global[0] * 2;
                tile_static T tile_data[domainSize];

                // Load data into tile_data, load 2x elements per tile.

                if (gidx2 + 1 < elementCount)
                {
                    tile_data[tidx2] = input_view[gidx2];
                    tile_data[tidx2 + 1] = input_view[gidx2 + 1];
                }

                // Up sweep (reduce) phase.

                int offset = 1;
                for (int stride = TileSize; stride > 0; stride >>= 1)
                {
                    tidx.barrier.wait_with_tile_static_memory_fence();
                    if (tid < stride)
                    {
                        const int ai = offset * (tidx2 + 1) - 1;
                        const int bi = offset * (tidx2 + 2) - 1; 
                        tile_data[bi] += tile_data[ai];
                    }
                    offset *= 2;
                }
                
                //  Zero highest element in tile
                if (tid == 0) 
                    tile_data[domainSize - 1] = 0;
                
                // Down sweep phase.
                // Now: offset = domainSize

                for (int stride = 1; stride <= TileSize; stride *= 2)
                {
                    offset >>= 1;
                    tidx.barrier.wait_with_tile_static_memory_fence();
                
                    if (tid < stride)
                    {
                        const int ai = offset * (tidx2 + 1) - 1; 
                        const int bi = offset * (tidx2 + 2) - 1; 
                        T t = tile_data[ai]; 
                        tile_data[ai] = tile_data[bi]; 
                        tile_data[bi] += t;
                    }
                }
                tidx.barrier.wait_with_tile_static_memory_fence();

                // Copy tile results out. For inclusive scan shift all elements left.

                if (gidx2 + 1 - (2 * Mode) < elementCount)
                {
                    tilewiseOutput[gidx2] = tile_data[tidx2 + Mode]; 
                    tilewiseOutput[gidx2 + 1] = tile_data[tidx2 + 1 + Mode];
                }

                // Copy tile total out, this is the inclusive total.

                if (tid == (TileSize - 1))
                {
                    // For inclusive scan calculate the last value
                    if (Mode == int(scan_mode::inclusive))
                        tilewiseOutput[gidx2 + 1] = tile_data[domainSize - 1] + input_view[gidx2 + 1];
                    
                    tileSums[tidx.tile[0]] = tile_data[domainSize - 1] + input_view[gidx2 + 1];
                }
            });
        }
    }
#pragma endregion
}
