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
* C++ AMP algorithms library.
*
* This file contains the C++ AMP algorithms
*---------------------------------------------------------------------------*/

// TODO: Does it really make a lot of sense to declare two namespaces or should everything be flattened into amp_algorithms?
// TODO: Here the functions are defined here. In the STL implementation they are defined in the main header file 
// and just declared in the public one. Is this by design?

#pragma once

#include <amp.h>

#include <xx_amp_algorithms_impl.h>
#include <xx_amp_stl_algorithms_impl_inl.h>
#include <amp_indexable_view.h>

namespace amp_algorithms
{
    // TODO: Need to remove this dependency on direct3d.
    namespace direct3d
    {
        class scan;
    }

#pragma region Arithmetic, comparison, logical and bitwise operators

    //----------------------------------------------------------------------------
    // Arithmetic operations
    //----------------------------------------------------------------------------

    template <typename T>
    class plus
    {
    public:
        T operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return (a + b);
        }
    };

    template <typename T>
    class minus
    {
    public:
        T operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return (a - b);
        }
    };

    template <typename T>
    class multiplies
    {
    public:
        T operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return (a * b);
        }
    };

    template <typename T>
    class divides
    {
    public:
        T operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return (a / b);
        }
    };

    template <typename T>
    class modulus
    {
    public:
        T operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return (a % b);
        }
    };

    template <typename T>
    class negates
    {
    public:
        T operator()(const T &a) const restrict(cpu, amp)
        {
            return (-a);
        }
    };

    template<int N, unsigned int P = 0>
    struct log2
    {
        enum { value = log2<N / 2, P + 1>::value };
    };

    template <unsigned int P>
    struct log2<0, P>
    {
        enum { value = P };
    };

    template <unsigned int P>
    struct log2<1, P>
    {
        enum { value = P };
    };

    template<unsigned int N>
    struct is_power_of_two
    {
        enum { value = ((count_bits<N, _details::bit32>::value == 1) ? TRUE : FALSE) };
    };

    // While 1 is technically 2^0, for the purposes of calculating 
    // tile size it isn't useful.

    template <>
    struct is_power_of_two<1>
    {
        enum { value = FALSE };
    };

    //----------------------------------------------------------------------------
    // Bitwise operations
    //----------------------------------------------------------------------------

    template <typename T>
    class bit_and
    {
    public:
        T operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return (a & b);
        }
    };

    template <typename T>
    class bit_or
    {
    public:
        T operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return (a | b);
        }
    };

    template <typename T>
    class bit_xor
    {
    public:
        T operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return (a ^ b);
        }
    };

    namespace _details
    {
        static const unsigned int bit08 = 0x80;
        static const unsigned int bit16 = 0x8000;
        static const unsigned int bit32 = 0x80000000;

        template<unsigned int N, int MaxBit>
        struct is_bit_set
        {
            enum { value = (N & MaxBit) ? 1 : 0 };
        };
    };

    template<unsigned int N, unsigned int MaxBit>
    struct count_bits
    {
        enum { value = (_details::is_bit_set<N, MaxBit>::value + count_bits<N, (MaxBit >> 1)>::value) };
    };

    template<unsigned int N>
    struct count_bits<N, 0>
    {
        enum { value = FALSE };
    };

    //----------------------------------------------------------------------------
    // Comparison operations
    //----------------------------------------------------------------------------

    template <typename T>
    class equal_to
    {
    public:
        bool operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return (a == b);
        }
    };

    template <typename T>
    class not_equal_to
    {
    public:
        bool operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return (a != b);
        }
    };

    template <typename T>
    class greater
    {
    public:
        bool operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return (a > b);
        }
    };

    template <typename T>
    class less
    {
    public:
        bool operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return (a < b);
        }
    };

    template <typename T>
    class greater_equal
    {
    public:
        bool operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return (a >= b);
        }
    };

    template <typename T>
    class less_equal
    {
    public:
        bool operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return (a <= b);
        }
    };

#ifdef max
#error amp_algorithms encountered a definition of the macro max.
#endif

    template <typename T>
    class max
    {
    public:
        T operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return ((a < b) ? b : a);
        }
    };

#ifdef min
#error amp_algorithms encountered a definition of the macro min.
#endif

    template <typename T>
    class min
    {
    public:
        T operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return ((a < b) ? a : b);
        }
    };

    //----------------------------------------------------------------------------
    // Logical operations
    //----------------------------------------------------------------------------

    template<class T>
    class logical_not
    {
    public:
        bool operator()(const T& a) const restrict(cpu, amp)
        {
            return (!a);
        }
    };

    template<class T>
    class logical_and
    {
    public:
        bool operator()(const T& a, const T& b) const restrict(cpu, amp)
        {
            return (a && b);
        }
    };

    template<class T>
    class logical_or
    {
    public:
        bool operator()(const T& a, const T& b) const restrict(cpu, amp)
        {
            return (a || b);
        }
    };

    // TODO_NOT_IMPLEMENTED: Implement not1() and not2() if appropriate.

#pragma endregion

#pragma region Byte pack and unpack, padded read and write

    //----------------------------------------------------------------------------
    // Byte pack and unpack
    //----------------------------------------------------------------------------

    template<int index, typename T>
    inline unsigned pack_byte(const T& value) restrict(cpu, amp)
    {
        static_assert(index < sizeof(T), "Index out of range.");
        return (value && 0xFF) << (index * 8);
    }

    template<typename T>
    inline unsigned pack_byte(const T& value, unsigned index) restrict(cpu, amp)
    {
        //assert(index < sizeof(T));
        return (value && 0xFF) << (index * 8);
    }

    template<int index, typename T>
    inline unsigned unpack_byte(const T& value) restrict(cpu, amp)
    {
        static_assert(index < sizeof(T), "Index out of range.");
        return (value >> (index * 8)) & 0xFF;
    }

    template<typename T>
    inline unsigned unpack_byte(const T& value, unsigned index) restrict(cpu, amp)
    {
        //assert(index < sizeof(T));
        return (value >> (index * 8)) & 0xFF;
    }

    template<typename T>
    unsigned bit_count() restrict(cpu, amp)
    {
        return sizeof(T) * 8;
    }

    //----------------------------------------------------------------------------
    // container padded_read & padded_write
    //----------------------------------------------------------------------------

    template <typename InputIndexableView, int N>
    inline typename InputIndexableView::value_type padded_read(const InputIndexableView& arr, const concurrency::index<N> idx) restrict(cpu, amp)
    {
        return arr.extent.contains(idx) ? arr[idx] : typename InputIndexableView::value_type();
    }

    template <typename InputIndexableView>
    inline typename InputIndexableView::value_type padded_read(const InputIndexableView& arr, const int idx) restrict(cpu, amp)
    {
        return padded_read<InputIndexableView, 1>(arr, concurrency::index<1>(idx));
    }

    template <typename InputIndexableView, int N>
    inline void padded_write(InputIndexableView& arr, const concurrency::index<N> idx, const typename InputIndexableView::value_type &value) restrict(cpu, amp)
    {
        if (arr.extent.contains(idx))
        {
            arr[idx] = value;
        }
    }

    template <typename InputIndexableView>
    inline void padded_write(InputIndexableView& arr, const int idx, const typename InputIndexableView::value_type &value) restrict(cpu, amp)
    {
        padded_write<InputIndexableView, 1>(arr, concurrency::index<1>(idx), value);
    }

#pragma endregion

    //----------------------------------------------------------------------------
    // fill
    //----------------------------------------------------------------------------

    template<typename OutputIndexableView, typename T>
    void fill(const concurrency::accelerator_view &accl_view, OutputIndexableView& output_view, const T& value)
    {
        ::amp_algorithms::generate(accl_view, output_view, [value]() restrict(amp) { return value; });
    }

    template<typename OutputIndexableView, typename T>
    void fill(OutputIndexableView& output_view, const T& value)
    {
        ::amp_algorithms::generate(output_view, [value]() restrict(amp) { return value; });
    }

    //----------------------------------------------------------------------------
    // generate
    //----------------------------------------------------------------------------

    template <typename OutputIndexableView, typename Generator>
    void generate(const concurrency::accelerator_view &accl_view, OutputIndexableView& output_view, const Generator& generator)
    {
        _details::parallel_for_each(accl_view, output_view.extent, [output_view, generator](concurrency::index<indexable_view_traits<OutputIndexableView>::rank> idx) restrict(amp) {
            output_view[idx] = generator();
        });
    }

    template <typename OutputIndexableView, typename Generator>
    void generate(OutputIndexableView& output_view, const Generator& generator)
    {
        ::amp_algorithms::generate(_details::auto_select_target(), output_view, generator);
    }

    //----------------------------------------------------------------------------
    // merge_sort
    //----------------------------------------------------------------------------

    // TODO_NOT_IMPLEMENTED: merge_sort
    template <typename T, typename BinaryOperator>
    void merge_sort(const concurrency::accelerator_view& accl_view, concurrency::array_view<unsigned int>& input_view, BinaryOperator op)
    {
    }

    template <typename T>
    void merge_sort(const concurrency::accelerator_view& accl_view, concurrency::array_view<unsigned int>& input_view)
    {
        ::amp_algorithms::merge_sort(accl_view, input_view, amp_algorithms::less<T>());
    }

    //----------------------------------------------------------------------------
    // radix_sort
    //----------------------------------------------------------------------------
    //
    // References:
    //
    // "Introduction to GPU Radix Sort" http://www.heterogeneouscompute.org/wordpress/wp-content/uploads/2011/06/RadixSort.pdf
    //
    // "Designing Efficient Sorting Algorithms for Manycore GPUs" http://www.nvidia.com/docs/io/67073/nvr-2008-001.pdf
    // "Histogram Calculation in CUDA" http://docs.nvidia.com/cuda/samples/3_Imaging/histogram/doc/histogram.pdf
    //
    // TODO: Move this to the _impl file?
    namespace _details
    {
        template<typename T, int key_bit_width>
        inline int radix_key_value(const T value, const unsigned key_idx) restrict(amp, cpu)
        {
            const T mask = (1 << key_bit_width) - 1;
            return (value >> (key_idx * key_bit_width)) & mask;
        }

        template <typename T>
        inline void initialize_bins(T* const bin_data, const int bin_count) restrict(amp)
        {
            for (int b = 0; b < bin_count; ++b)
            {
                bin_data[b] = T(0);
            }
        }

        template<typename T>
        inline void merge_bins(T* const left, const T* const right, const int bin_count) restrict(amp)
        {
            for (int b = 0; b < bin_count; ++b)
            {
                left[b] += right[b];
            }
        }

        template <typename T, int key_bit_width, int tile_size>
        void radix_sort_tile_by_key(T* const tile_data, concurrency::tiled_index<tile_size> tidx, const int key_idx) restrict(amp)
        {
            static_assert((tile_size <= 256), "The tile size must be less than or equal to 256.");
            static_assert((key_bit_width >=1), "The radix bit width must be greater than or equal to one.");
            static_assert((key_bit_width <= 2), "The radix bit width must be less than or equal to two.");

            const unsigned bin_count = 1 << key_bit_width;
            const int idx = tidx.local[0];

            // Increment histogram bins for each element.

            tile_static unsigned tile_radix_values[tile_size];
            int radix_value = _details::radix_key_value<T, key_bit_width>(tile_data[idx], key_idx);  // TODO: Get rid of this and recalculate?
            tile_radix_values[idx] = pack_byte(1, radix_value);

            tidx.barrier.wait_with_tile_static_memory_fence();

            tile_static unsigned histogram_bins_scan[bin_count];
            if (idx == 0)
            {
                // Calculate histogram of radix values .

                unsigned global_histogram = 0;
                for (int i = 0; i < tile_size; ++i)
                {
                    global_histogram += tile_radix_values[i];
                }

                // Scan to get offsets for each histogram bin.

                histogram_bins_scan[0] = 0;
                histogram_bins_scan[1] = unpack_byte<0>(global_histogram);
                if (key_bit_width > 1)
                {
                    histogram_bins_scan[2] = unpack_byte<1>(global_histogram) + histogram_bins_scan[1];
                    histogram_bins_scan[3] = unpack_byte<2>(global_histogram) + histogram_bins_scan[2];
                }
            }
            _details::scan_tile<tile_size, amp_algorithms::scan_mode::exclusive>(tile_radix_values, tidx, amp_algorithms::plus<T>());

            // Shuffle data into sorted order.

            T tmp = tile_data[idx];
            tidx.barrier.wait_with_tile_static_memory_fence();
            int i = histogram_bins_scan[radix_value] + unpack_byte(tile_radix_values[idx], radix_value);
            tile_data[i] = tmp;
        }

        template <typename T, int key_bit_width, int tile_size>
        void radix_sort_by_key(const concurrency::accelerator_view& accl_view, concurrency::array_view<T>& input_view, concurrency::array_view<T>& output_view, const int key_idx)
        {
            static const int tile_key_bit_width = 2;
            static const unsigned type_width = sizeof(T) * 8;
            static const int bin_count = 1 << key_bit_width;

            static_assert((tile_size >= bin_count), "The tile size must be greater than or equal to the radix key bin count.");
            static_assert((type_width % key_bit_width == 0), "The sort key width must be divisible by the type width.");
            static_assert((key_bit_width % tile_key_bit_width == 0), "The key bit width must be divisible by the tile key bit width.");
            
            const concurrency::tiled_extent<tile_size> compute_domain = output_view.get_extent().tile<tile_size>().pad();
            const int tile_count = compute_domain.size() / tile_size;

            concurrency::array<int, 2> per_tile_rdx_offsets(concurrency::extent<2>(tile_count, bin_count), accl_view);
            concurrency::array<int> global_rdx_offsets(bin_count, accl_view);
            concurrency::array<int, 1> tile_histograms(concurrency::extent<1>(bin_count * tile_count), accl_view);

            concurrency::parallel_for_each(accl_view, compute_domain, [=, &global_rdx_offsets](concurrency::tiled_index<tile_size> tidx) restrict(amp)
            {
                if (tidx.local[0] < bin_count)
                    global_rdx_offsets[tidx.local[0]] = 0;
            });

            concurrency::parallel_for_each(accl_view, compute_domain, [=, &per_tile_rdx_offsets, &global_rdx_offsets, &tile_histograms](concurrency::tiled_index<tile_size> tidx) restrict(amp)
            {
                const int gidx = tidx.global[0];
                const int tlx = tidx.tile[0];
                const int idx = tidx.local[0];
                tile_static T tile_data[tile_size];
                tile_static int per_thread_rdx_histograms[tile_size][bin_count];

                // Initialize histogram bins and copy data into tiles.

                initialize_bins(per_thread_rdx_histograms[idx], bin_count);
                tile_data[idx] = input_view[gidx];

                // Increment radix bins for each element on each tile.

                if (gidx < int(input_view.extent.size()))
                {
                    per_thread_rdx_histograms[idx][_details::radix_key_value<T, key_bit_width>(tile_data[idx], key_idx)]++;
                }
                tidx.barrier.wait_with_tile_static_memory_fence();

                // One thread per tile collapses tile values and copies the results into the global histogram.

                if (idx == 0)                                                                   // TODO: This could be more efficient. Don't do it all on one thread.
                {
                    for (int i = 1; i < tile_size; ++i)
                    {
                        merge_bins(per_thread_rdx_histograms[0], per_thread_rdx_histograms[i], bin_count);
                    }
                }
                tidx.barrier.wait_with_tile_static_memory_fence();

                // First bin_count threads per tile increment counts for global histogram and copies tile histograms to global memory.

                if (idx < bin_count)
                {
                    concurrency::atomic_fetch_add(&global_rdx_offsets[idx], per_thread_rdx_histograms[0][idx]);
                }
                tidx.barrier.wait();

                //output_view[gidx] = (idx < bin_count) ? per_thread_rdx_histograms[0][idx] : 0;            // Dump per-tile histograms, per_tile_rdx_histograms   

                // Exclusive scan the tile histogram to calculate the per-tile offsets.

                tile_static unsigned scan_data[bin_count];                                      // TODO: Is this copy really needed or can I do this in place?
                if (idx < bin_count)
                {
                    scan_data[idx] = per_thread_rdx_histograms[0][idx];
                }
                tidx.barrier.wait();
                _details::scan_tile<tile_size, scan_mode::exclusive>(scan_data, tidx, amp_algorithms::plus<T>());

                if (idx < bin_count)
                {
                    per_tile_rdx_offsets[tlx][idx] = scan_data[idx];
                    tile_histograms[(idx * tile_count) + tlx] = per_thread_rdx_histograms[0][idx];
                }
            });
        
            concurrency::parallel_for_each(accl_view, compute_domain, [=, &global_rdx_offsets, &tile_histograms](concurrency::tiled_index<tile_size> tidx) restrict(amp)
            {
                const int gidx = tidx.global[0];
                const int tlx = tidx.tile[0];
                const int idx = tidx.local[0];

                //output_view[gidx] = (gidx < bin_count * tile_count) ? tile_histograms[gidx] : 0;          // Dump per-tile histograms, per_tile_rdx_histograms_tp,

                // Calculate global radix offsets from the global radix histogram. All tiles do this but only the first one records the result.

                tile_static int scan_data[bin_count];
                if (gidx < bin_count)
                {
                    scan_data[gidx] = global_rdx_offsets[gidx];
                }
                tidx.barrier.wait_with_tile_static_memory_fence();

                _details::scan_tile<tile_size, scan_mode::exclusive>(scan_data, tidx, amp_algorithms::plus<T>());

                if (gidx < bin_count)
                {
                    global_rdx_offsets[gidx] = scan_data[gidx];
                }
            });

            // TODO: Need to remove this dependency on direct3d. Only using it because we don't have a segmented scan AMP-only implementation yet.
            amp_algorithms::direct3d::bitvector flags(bin_count * tile_count); 
            flags.initialize(tile_count);
            concurrency::array<unsigned int> input_flags(static_cast<unsigned>(flags.data.size()), flags.data.begin());
            amp_algorithms::direct3d::scan s(bin_count * tile_count, accl_view);
            s.segmented_scan_exclusive<int>(tile_histograms, tile_histograms, input_flags, amp_algorithms::direct3d::scan_direction::forward, amp_algorithms::plus<T>());

            concurrency::parallel_for_each(accl_view, compute_domain, [=, &per_tile_rdx_offsets, &tile_histograms, &global_rdx_offsets](concurrency::tiled_index<tile_size> tidx) restrict(amp)
            {
                const int gidx = tidx.global[0];
                const int tlx = tidx.tile[0];
                const int idx = tidx.local[0];

                tile_static T tile_data[tile_size];
                tile_data[idx] = input_view[gidx];
                tidx.barrier.wait_with_tile_static_memory_fence();

                //if (idx < bin_count) { output_view[gidx] = per_tile_rdx_offsets[tlx][idx]; }              // Dump per tile offsets, per_tile_rdx_offsets
                //if (idx < bin_count * tile_count) { output_view[gidx] = tile_histograms[gidx]; }          // Dump tile offsets, xxx
                //if (idx < bin_count) { output_view[gidx] = tile_histograms[(idx * tile_count) + tlx]; }   // Dump tile offsets, tile_rdx_offsets
                //output_view[gidx] = (gidx < bin_count) ? global_rdx_offsets[gidx] : 0;                    // Dump global offsets, global_rdx_offsets

                // Sort elements within each tile.

                const int keys_per_tile = (key_bit_width / tile_key_bit_width);
                for (int k = (keys_per_tile * key_idx); k < (keys_per_tile * (key_idx + 1)); ++k)
                {
                    _details::radix_sort_tile_by_key<T, tile_key_bit_width, tile_size>(tile_data, tidx, k); 
                }
                tidx.barrier.wait_with_tile_static_memory_fence();
                
                //output_view[gidx] = tile_data[idx];                                                       // Dump sorted per-tile data, sorted_per_tile

                // Move tile sorted elements to global destination.

                const int rdx = _details::radix_key_value<T, key_bit_width>(tile_data[idx], key_idx);
                const int dest_gidx = idx - per_tile_rdx_offsets[tlx][rdx] + tile_histograms[(rdx * tile_count) + tlx] + global_rdx_offsets[rdx];

                //output_view[gidx] = dest_gidx;                                                            // Dump destination indices, dest_gidx

                output_view[dest_gidx] = tile_data[idx];
            });
        }

        template <typename T, int key_bit_width, int tile_size>
        void radix_sort(const concurrency::accelerator_view& accl_view, concurrency::array_view<T>& input_view, concurrency::array_view<T>& output_view)
        {
            static const int tile_key_bit_width = 2;
            static const unsigned type_width = bit_count<T>();
            static const int bin_count = 1 << key_bit_width;
            static const int key_count = type_width / key_bit_width;

            for (int key_idx = 0; key_idx < key_count; ++key_idx)
            {
                _details::radix_sort_by_key<T, key_bit_width, tile_size>(accl_view, input_view, output_view, key_idx);
                std::swap(output_view, input_view);
            }
            std::swap(input_view, output_view);
        }
    }

    template <typename T>
    inline void radix_sort(const concurrency::accelerator_view& accl_view, concurrency::array_view<T>& input_view)
    {
        static const int bin_width = 4;
        static const int tile_size = 256;
        _details::radix_sort<T, bin_width, tile_size>(accl_view, input_view);
    }

    template <typename T>
    inline void radix_sort(concurrency::array_view<T>& input_view)
    {
        radix_sort(_details::auto_select_target(), input_view);
    }

    //----------------------------------------------------------------------------
    // reduce
    //----------------------------------------------------------------------------

    // Generic reduction template for binary operators that are commutative and associative
    template <typename InputIndexableView, typename BinaryFunction>
    typename std::result_of<BinaryFunction(const typename indexable_view_traits<InputIndexableView>::value_type&, const typename indexable_view_traits<InputIndexableView>::value_type&)>::type
        reduce(const concurrency::accelerator_view &accl_view, const InputIndexableView &input_view, const BinaryFunction &binary_op)
    {
        const int tile_size = 512;
        return _details::reduce<tile_size, 10000, InputIndexableView, BinaryFunction>(accl_view, input_view, binary_op);
    }

    template <typename InputIndexableView, typename BinaryFunction>
    typename std::result_of<BinaryFunction(const typename indexable_view_traits<InputIndexableView>::value_type&, const typename indexable_view_traits<InputIndexableView>::value_type&)>::type
        reduce(const InputIndexableView &input_view, const BinaryFunction &binary_op)
    {
        return reduce(_details::auto_select_target(), input_view, binary_op);
    }

    //----------------------------------------------------------------------------
    // scan - C++ AMP implementation
    //----------------------------------------------------------------------------
    //
    // Scan implementation using the same algorithm described here and used by the CUDPP library.
    //
    // https://research.nvidia.com/sites/default/files/publications/nvr-2008-003.pdf
    //
    // For a full overview of various scan implementations see:
    //
    // https://sites.google.com/site/duanemerrill/ScanTR2.pdf
    //
    // TODO: There may be some better scan implementations that are described in the second reference. Investigate.
    // TODO: Scan only supports Rank of 1.
    // TODO: Scan does not support segmented scan or forwards/backwards.
    // TODO: IMPORTANT! Scan uses information about the warp size. Consider using an algorithm that does not need to use this.

    // This header needs these enums but they also have to be defined in the _impl header for use by the 
    // main STL header, which includes the _impl.
#if !defined(AMP_ALGORITHMS_ENUMS)
#define AMP_ALGORITHMS_ENUMS
    enum class scan_mode : int
    {
        exclusive = 0,
        inclusive = 1
    };

    enum class scan_direction : int
    {
        forward = 0,
        backward = 1
    };
#endif

    template <int TileSize, scan_mode _Mode, typename _BinaryFunc, typename InputIndexableView>
    inline void scan(const InputIndexableView& input_view, InputIndexableView& output_view, const _BinaryFunc& op)
    {
        _details::scan<TileSize, _Mode, _BinaryFunc>(_details::auto_select_target(), input_view, output_view, op);
    }

    template <typename IndexableView>
    void scan_exclusive(const concurrency::accelerator_view& accl_view, const IndexableView& input_view, IndexableView& output_view)
    {
        _details::scan<_details::scan_default_tile_size, amp_algorithms::scan_mode::exclusive>(accl_view, input_view, output_view, amp_algorithms::plus<IndexableView::value_type>());
    }

    template <typename IndexableView>
    void scan_exclusive(const IndexableView& input_view, IndexableView& output_view)
    {
        _details::scan<_details::scan_default_tile_size, amp_algorithms::scan_mode::exclusive>(_details::auto_select_target(), input_view, output_view, amp_algorithms::plus<typename IndexableView::value_type>());
    }

    template <typename IndexableView>
    void scan_inclusive(const concurrency::accelerator_view& accl_view, const IndexableView& input_view, IndexableView& output_view)
    {
        _details::scan<_details::scan_default_tile_size, amp_algorithms::scan_mode::inclusive>(accl_view, input_view, output_view, amp_algorithms::plus<IndexableView::value_type>());
    }

    template <typename IndexableView>
    void scan_inclusive(const IndexableView& input_view, IndexableView& output_view)
    {
        _details::scan<_details::scan_default_tile_size, amp_algorithms::scan_mode::inclusive>(_details::auto_select_target(), input_view, output_view, amp_algorithms::plus<typename IndexableView::value_type>());
    }

    //----------------------------------------------------------------------------
    // transform (unary)
    //----------------------------------------------------------------------------

    template <typename ConstInputIndexableView, typename OutputIndexableView, typename UnaryFunc>
    void transform(const concurrency::accelerator_view &accl_view, const ConstInputIndexableView& input_view, OutputIndexableView& output_view, const UnaryFunc& func)
    {
        _details::parallel_for_each(accl_view, output_view.extent, [input_view,output_view,func] (concurrency::index<indexable_view_traits<OutputIndexableView>::rank> idx) restrict(amp) {
            output_view[idx] = func(input_view[idx]);
        });
    }

    template <typename ConstInputIndexableView, typename OutputIndexableView, typename UnaryFunc>
    void transform(const ConstInputIndexableView& input_view, OutputIndexableView& output_view, const UnaryFunc& func)
    {
        ::amp_algorithms::transform(_details::auto_select_target(), input_view, output_view, func);
    }

    //----------------------------------------------------------------------------
    // transform (binary)
    //----------------------------------------------------------------------------

    template <typename ConstInputIndexableView1, typename ConstInputIndexableView2, typename OutputIndexableView, typename BinaryFunc>
    void transform(const concurrency::accelerator_view &accl_view, const ConstInputIndexableView1& input_view1, const ConstInputIndexableView2& input_view2, OutputIndexableView& output_view, const BinaryFunc& func)
    {
        _details::parallel_for_each(accl_view, output_view.extent, [input_view1,input_view2,output_view,func] (concurrency::index<indexable_view_traits<OutputIndexableView>::rank> idx) restrict(amp) {
            output_view[idx] = func(input_view1[idx], input_view2[idx]);
        });
    }

    template <typename ConstInputIndexableView1, typename ConstInputIndexableView2, typename OutputIndexableView, typename BinaryFunc>
    void transform(const ConstInputIndexableView1& input_view1, const ConstInputIndexableView2& input_view2, OutputIndexableView& output_view, const BinaryFunc& func)
    {
        ::amp_algorithms::transform(_details::auto_select_target(), input_view1, input_view2, output_view, func);
    }
} // namespace amp_algorithms
