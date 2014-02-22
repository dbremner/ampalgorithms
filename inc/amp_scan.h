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

namespace Extras
{
    enum BitWidth
    {
        Bit08 = 0x80,
        Bit16 = 0x8000,
        Bit32 = 0x80000000
    } ;

    template<unsigned int N>
    struct IsPowerOfTwoStatic
    {
        enum 
        { 
            result = ((CountBitsStatic<N, Bit32>::result == 1) ? TRUE : FALSE)
        };
    };

    // While 1 is technically 2^0, for the purposes of calculating 
    // tile size it isn't useful.
    template <>
    struct IsPowerOfTwoStatic<1>
    {
        enum { result = FALSE };
    };

    template<unsigned int N, unsigned int MaxBit>
    struct CountBitsStatic
    {
        enum
        { 
            result = (IsBitSetStatic<N, MaxBit>::result + CountBitsStatic<N, (MaxBit >> 1)>::result) 
        };
    };

    // Ensure that template program terminates.
    template<unsigned int N>
    struct CountBitsStatic<N, 0>
    {
        enum { result = FALSE };
    };

    template<unsigned int N, int Bit>
    struct IsBitSetStatic
    {
        enum { result = (N & Bit) ? 1 : 0 };
    };

    //===============================================================================
    // Exclusive scan, output element at i contains the sum of elements [0]...[i-1].
    //===============================================================================

    template <int TileSize, typename InIt, typename OutIt>
    inline void ExclusiveScanOptimized(InIt first, InIt last, OutIt outFirst)
    {
        typedef InIt::value_type T;

        const int size = int(distance(first, last));
        concurrency::array<T, 1> in(size);
        concurrency::array<T, 1> out(size);
        copy(first, last, in);   
        details::ScanOptimized<TileSize, details::kExclusive>(concurrency::array_view<T, 1>(in), concurrency::array_view<T, 1>(out));
        copy(out, outFirst);
    }

    template <int TileSize, typename T>  
    inline void ExclusiveScanOptimized(concurrency::array_view<T, 1> input, concurrency::array_view<T, 1> output)
    {
        details::ScanOptimized<TileSize, details::kExclusive, T>(input, output);
    }

    //===============================================================================
    // Inclusive scan, output element at i contains the sum of elements [0]...[i].
    //===============================================================================

    template <int TileSize, typename InIt, typename OutIt>
    inline void InclusiveScanOptimized(InIt first, InIt last, OutIt outFirst)
    {
        typedef InIt::value_type T;

        const int size = int(distance(first, last));
        concurrency::array<T, 1> in(size);
        concurrency::array<T, 1> out(size);
        copy(first, last, in);      
        details::ScanOptimized<TileSize, details::kInclusive>(concurrency::array_view<T, 1>(in), concurrency::array_view<T, 1>(out));
        copy(out, outFirst);
    }

    template <int TileSize, typename T>  
    inline void InclusiveScanOptimized(concurrency::array_view<T, 1> input, concurrency::array_view<T, 1> output)
    {
        details::ScanOptimized<TileSize, details::kInclusive, T>(input, output);
    }

    //===============================================================================
    //  Implementation. Not supposed to be called directly.
    //===============================================================================

    namespace details
    {
        enum ScanMode
        {
            kExclusive = 0,
            kInclusive = 1
        };

        template <int TileSize, int Mode, typename T>
        void ScanTiled(concurrency::array_view<T, 1> input, concurrency::array_view<T, 1> output)
        {
            static_assert((Mode == details::kExclusive || Mode == details::kInclusive), "Mode must be either inclusive or exclusive.");
            static_assert(IsPowerOfTwoStatic<TileSize>::result, "TileSize must be a power of 2.");
            assert(input.extent[0] == output.extent[0]);
            assert(input.extent[0] > 0);

            const int elementCount = input.extent[0];
            const int tileCount = (elementCount + TileSize - 1) / TileSize;

            // Compute tile-wise scans and reductions.
            concurrency::array<T> tileSums(tileCount);
            details::ComputeTilewiseExclusiveScanTiled<TileSize, Mode>(concurrency::array_view<const T>(input), concurrency::array_view<T>(output), concurrency::array_view<T>(tileSums));

            if (tileCount > 1)
            {
                // Calculate the initial value of each tile based on the tileSums.
                concurrency::array<T> tmp(tileSums.extent);
                ScanTiled<TileSize, details::kExclusive>(concurrency::array_view<T>(tileSums), concurrency::array_view<T>(tmp));
                output.discard_data();
                parallel_for_each(concurrency::extent<1>(elementCount), [=, &tileSums, &tmp](concurrency::index<1> idx) restrict(amp)
                {
                    int tileIdx = idx[0] / TileSize;
                    output[idx] += tmp[tileIdx];
                });
            }
        }

        // For each tile calculate the inclusive scan.

        template <int TileSize, int Mode, typename T>
        void ComputeTilewiseExclusiveScanTiled(concurrency::array_view<const T> input, concurrency::array_view<T> tilewiseOutput, concurrency::array_view<T> tileSums)
        {
            const int elementCount = input.extent[0];
            const int tileCount = (elementCount + TileSize - 1) / TileSize;
            const int threadCount = tileCount * TileSize;

            tilewiseOutput.discard_data();
            parallel_for_each(concurrency::extent<1>(threadCount).tile<TileSize>(), [=](concurrency::tiled_index<TileSize> tidx) restrict(amp)
            {
                const int tid = tidx.local[0];
                const int gid = tidx.global[0];

                tile_static T tileData[2][TileSize];
                int inIdx = 0;
                int outIdx = 1;

                // Do the first pass (offset = 1) while loading elements into tile_static memory.

                if (gid < elementCount)
                {
                    if (tid >= 1)
                        tileData[outIdx][tid] = input[gid - 1] + input[gid];
                    else
                        tileData[outIdx][tid] = input[gid];
                }
                tidx.barrier.wait_with_tile_static_memory_fence();

                for (int offset = 2; offset < TileSize; offset *= 2)
                {
                    SwitchIndeces(inIdx, outIdx);

                    if (gid < elementCount)
                    {
                        if (tid >= offset)
                            tileData[outIdx][tid] = tileData[inIdx][tid - offset] + tileData[inIdx][tid];
                        else
                            tileData[outIdx][tid] = tileData[inIdx][tid];
                    }
                    tidx.barrier.wait_with_tile_static_memory_fence();
                }

                // Copy tile results out. For exclusive scan shift all elements right.

                if (gid < elementCount)
                {
                    // For exclusive scan calculate the last value
                    if (Mode == details::kInclusive)
                        tilewiseOutput[gid] = tileData[outIdx][tid];
                    else
                    if (tid == 0)
                        tilewiseOutput[gid] = T(0);
                    else
                        tilewiseOutput[gid] = tileData[outIdx][tid - 1];
                }

                // Last thread in tile updates the tileSums.
                if (tid == TileSize - 1)
                    tileSums[tidx.tile[0]] = tileData[outIdx][tid - 1] + +input[gid];
            });
        }

        void SwitchIndeces(int& index1, int& index2) restrict(amp, cpu)
        {
            index1 = 1 - index1;
            index2 = 1 - index2;
        }

        template <int BlockSize, int LogBlockSize>
        inline int ConflictFreeOffset(int n) restrict(amp)
        {
            return n >> BlockSize + n >> (2 * LogBlockSize);
        }

        // For each tile calculate the exclusive scan.
        //
        // http.developer.nvidia.com/GPUGems3/gpugems3_ch39.html

        template <int TileSize, int Mode, typename T>  
        void ScanOptimized(concurrency::array_view<T, 1> input, concurrency::array_view<T, 1> output)
        {
            const int domainSize = TileSize * 2;
            const int elementCount = input.extent[0];
            const int tileCount = (elementCount + domainSize - 1) / domainSize;

            static_assert((Mode == details::kExclusive || Mode == details::kInclusive), "Mode must be either inclusive or exclusive.");
            static_assert(IsPowerOfTwoStatic<TileSize>::result, "TileSize must be a power of 2.");
            assert(elementCount > 0);
            assert(elementCount == output.extent[0]);
            assert((elementCount / domainSize) >= 1);

            // Compute scan for each tile and store their total values in tileSums
            concurrency::array<T> tileSums(tileCount);
            details::ComputeTilewiseExclusiveScanOptimized<TileSize, Mode>(concurrency::array_view<const T>(input), output, concurrency::array_view<T>(tileSums));
        
            if (tileCount > 1)
            {
                // Calculate the initial value of each tile based on the tileSums.
                concurrency::array<T> tileSumScan(tileSums.extent);
                ScanTiled<TileSize, details::kExclusive>(concurrency::array_view<T>(tileSums), concurrency::array_view<T>(tileSumScan));

                // Add the tileSums all the elements in each tile except the first tile.
                output.discard_data();
                parallel_for_each(concurrency::extent<1>(elementCount - domainSize), [=, &tileSumScan] (concurrency::index<1> idx) restrict (amp) 
                {
                    const int tileIdx = (idx[0] + domainSize) / domainSize;
                    output[idx + domainSize] += tileSumScan[tileIdx];
                });
            }
        }

        template <int TileSize, int Mode, typename T>
        void ComputeTilewiseExclusiveScanOptimized(concurrency::array_view<const T, 1> input, concurrency::array_view<T> tilewiseOutput, concurrency::array_view<T, 1> tileSums)
        {
            static const int domainSize = TileSize * 2;
            const int elementCount = input.extent[0];
            const int tileCount = (elementCount + TileSize - 1) / TileSize;
            const int threadCount = tileCount * TileSize;

            tilewiseOutput.discard_data();
            tileSums.discard_data();
            parallel_for_each(concurrency::extent<1>(threadCount).tile<TileSize>(), [=](concurrency::tiled_index<TileSize> tidx) restrict(amp)
            {
                const int tid = tidx.local[0];
                const int tidx2 = tidx.local[0] * 2;
                const int gidx2 = tidx.global[0] * 2;
                tile_static T tileData[domainSize];

                // Load data into tileData, load 2x elements per tile.

                if (gidx2 + 1 < elementCount)
                {
                    tileData[tidx2] = input[gidx2];
                    tileData[tidx2 + 1] = input[gidx2 + 1];
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
                        tileData[bi] += tileData[ai];
                    }
                    offset *= 2;
                }
                
                //  Zero highest element in tile
                if (tid == 0) 
                    tileData[domainSize - 1] = 0;
                
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
                        T t = tileData[ai]; 
                        tileData[ai] = tileData[bi]; 
                        tileData[bi] += t;
                    }
                }
                tidx.barrier.wait_with_tile_static_memory_fence();

                // Copy tile results out. For inclusive scan shift all elements left.

                if (gidx2 + 1 - (2 * Mode) < elementCount)
                {
                    tilewiseOutput[gidx2] = tileData[tidx2 + Mode]; 
                    tilewiseOutput[gidx2 + 1] = tileData[tidx2 + 1 + Mode];
                }

                // Copy tile total out, this is the inclusive total.

                if (tid == (TileSize - 1))
                {
                    // For inclusive scan calculate the last value
                    if (Mode == details::kInclusive)
                        tilewiseOutput[gidx2 + 1] = tileData[domainSize - 1] + input[gidx2 + 1];
                    
                    tileSums[tidx.tile[0]] = tileData[domainSize - 1] + input[gidx2 + 1];
                }
            });
        }
    }
}
