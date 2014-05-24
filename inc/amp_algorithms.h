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

// TODO: Here the functions are defined here. In the STL implementation they are defined in the main header file 
// and just declared in the public one. Is this by design?

#pragma once

#include <amp.h>

#include <xx_amp_algorithms_impl.h>
#include <xx_amp_stl_algorithms_impl_inl.h>
#include <amp_indexable_view.h>

namespace amp_algorithms
{

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

    // TODO: Where should these be declared? Technically they are STL algorithms but they are already used by the direct3d namespace
    // TODO: Does it really make a lot of sense to declare two namespaces or should everything be flattened into amp_algorithms?
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

    // TODO: Implement not1() and not2() if appropriate.

#pragma endregion

    //----------------------------------------------------------------------------
    // Byte pack and unpack
    //----------------------------------------------------------------------------

    template<int index>
    inline unsigned pack_byte(const unsigned value) restrict(cpu, amp)
    {
        return (value && 0xFF) << (index * 8);
    }

    inline unsigned pack_byte(const unsigned value, unsigned index) restrict(cpu, amp)
    {
        return (value && 0xFF) << (index * 8);
    }

    template<int index>
    inline unsigned unpack_byte(const unsigned value) restrict(cpu, amp)
    {
        return (value >> (index * 8)) & 0xFF;
    }

    inline unsigned unpack_byte(const unsigned value, unsigned index) restrict(cpu, amp)
    {
        return (value >> (index * 8)) & 0xFF;
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

    // TODO: NOT IMPLEMENTED radix_sort
    template <typename T, typename BinaryOperator>
    void merge_sort(const concurrency::accelerator_view& accl_view, concurrency::array_view<unsigned int>& input_view, BinaryOperator op)
    {
    }

    // TODO: NOT IMPLEMENTED radix_sort
    template <typename T>
    void merge_sort(const concurrency::accelerator_view& accl_view, concurrency::array_view<unsigned int>& input_view)
    {
        ::amp_algorithms::merge_sort(accl_view, input_view, amp_algorithms::less<T>());
    }

    //----------------------------------------------------------------------------
    // radix_sort
    //----------------------------------------------------------------------------
    // "Introduction to GPU Radix Sort" http://www.heterogeneouscompute.org/wordpress/wp-content/uploads/2011/06/RadixSort.pdf
    // "Designing Efficient Sorting Algorithms for Manycore GPUs" http://www.nvidia.com/docs/io/67073/nvr-2008-001.pdf
    // https://www.cs.auckland.ac.nz/software/AlgAnim/radixsort.html
    //
    // http://www.intel.com/content/www/us/en/research/intel-labs-radix-sort-mic-report.html
    // http://www.cse.uconn.edu/~huang/fall12_5304/Presentation_Final/GPU_Sorting.pdf
    // http://www.cs.virginia.edu/~dgm4d/papers/RadixSortTR.pdf
    // http://xxx.lanl.gov/pdf/1008.2849
    // http://www.rebe.rau.ro/RePEc/rau/jisomg/WI12/JISOM-WI12-A11.pdf
    //
    //
    // "Histogram Calculation in CUDA" http://docs.nvidia.com/cuda/samples/3_Imaging/histogram/doc/histogram.pdf
    //
    // TODO: Move this to the impl file?
    namespace _details
    {
        template<typename T, int key_bit_width>
        inline int radix_key_value(const T value, const unsigned key_idx) restrict(amp, cpu)
        {
            const T mask = (1 << key_bit_width) - 1;
            const unsigned key_offset = key_idx * key_bit_width;
            return (value >> key_offset) & mask;
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
                // Calculate histogram of radix values.

                unsigned histogram_bins = 0;
                for (int i = 0; i < tile_size; ++i)
                {
                    histogram_bins += tile_radix_values[i];
                }

                // Scan to get offsets for each histogram bin.

                histogram_bins_scan[0] = 0;
                histogram_bins_scan[1] = unpack_byte<0>(histogram_bins);
                if (key_bit_width > 1)
                {
                    histogram_bins_scan[2] = unpack_byte<1>(histogram_bins) + histogram_bins_scan[1];
                    histogram_bins_scan[3] = unpack_byte<2>(histogram_bins) + histogram_bins_scan[2];
                }
            }
            _details::scan_tile<tile_size, amp_algorithms::scan_mode::exclusive>(tile_radix_values, tidx, amp_algorithms::plus<T>());

            // Shuffle data into sorted order.

            T tmp = tile_data[idx];
            tidx.barrier.wait_with_tile_static_memory_fence();
            int offset = histogram_bins_scan[radix_value] + unpack_byte(tile_radix_values[idx], radix_value);
            tile_data[offset] = tmp;
        }

        template <typename T, int key_bit_width, int tile_size>
        void radix_sort_by_key(const concurrency::accelerator_view& accl_view, const concurrency::array_view<T>& input_view, concurrency::array_view<T>& output_view, const int key_idx)
        {
            static const unsigned type_width = sizeof(T) * 8;
            static_assert((type_width % key_bit_width == 0), "The sort key width must be divisible by the type width.");
            static_assert((key_bit_width % 2 == 0), "The key bit width must be divisible by two.");

            static const unsigned bin_count = 1 << key_bit_width;
            static const T bin_mask = bin_count - 1;
            static const int elements_per_thread = 1;          // TODO: Doesn't have to be a constant?

            // histogram all elements in a block
            concurrency::array<unsigned> histogram_bins(bin_count, accl_view);
            concurrency::array_view<unsigned> histogram_bins_vw(histogram_bins);

            concurrency::tiled_extent<tile_size> compute_domain = input_view.get_extent().tile<tile_size>().pad();

            concurrency::parallel_for_each(accl_view, compute_domain, [=, &histogram_bins](concurrency::tiled_index<tile_size> tidx) restrict(amp)
            {
                // Each thread has its own histogram
                tile_static unsigned tile_bins[tile_size][bin_count];
                tile_static T tile_data[tile_size];
                const int gidx = tidx.global[0];
                const int idx = tidx.local[0];
                const int start_elem = idx * elements_per_thread;

                // One thread initializes the global histogram bins.
                if (gidx == 0)
                {
                    //initialize_bins(histogram_bins_vw[b], bin_count);
                }
                initialize_bins(tile_bins[idx], bin_count);

                // Increment bins for each element.
                if (gidx < input_view.extent[0])
                {
                    tile_bins[idx][_details::radix_key_value<T, key_bit_width>(input_view[gidx], key_idx)]++;
                }
                tidx.barrier.wait_with_tile_static_memory_fence();

                // TODO: This could be more efficient. Don't do it all on one thread.
                if (idx == 0)
                {
                    // Thread zero merges local histograms.
                    for (int i = 1; i < tile_size; ++i)
                    {
                        merge_bins(tile_bins[0], tile_bins[i], bin_count);
                    }

                    // TODO: This isn't smart either but it'll get things working.

                    // Thread zero copies and merges data with global histogram.
                    for (int b = 0; b < bin_count; ++b)
                    {
                        concurrency::atomic_fetch_add(&histogram_bins_vw(b), tile_bins[0][b]);
                    }

                    //_details::scan_tile<tile_size, amp_algorithms::scan_mode::exclusive>(tile_bins[0], tidx, amp_algorithms::plus<T>());
                }

                // Sort tiles by 
                tidx.barrier.wait();
                for (int i = 0; i < (key_bit_width / 2); ++i)
                {
                    radix_sort_tile_by_key<T, 2, tile_size>(tile_data, tidx, i);
                }

                // Shuffle data into sorted order.

                const T tmp = tile_data[idx];
                tidx.barrier.wait_with_tile_static_memory_fence();
                //int offset = idx - tile_bins[tmp];
                //input_view[offset] = tmp;
            });

            _details::scan<tile_size, scan_mode::exclusive>(accl_view, histogram_bins_vw, histogram_bins_vw, amp_algorithms::plus<unsigned int>());

            // Reorder the elements based on the offsets.
            concurrency::parallel_for_each(accl_view, compute_domain, [=](concurrency::tiled_index<tile_size> tidx) restrict(amp)
            {
                const int gidx = tidx.global[0];
                const int idx = tidx.local[0];

                // TODO: Will this work if the input and output views overlap or are the same?
                //const int d = tile_bins[] + /* local offset +  */ histogram_bins_vw[tidx.tile];
                //output_view[gidx] = input_view[d];
            });
        }

        // TODO: T is limited to only integer types. Need to modify the template to restrict this.
        template <typename T, int key_bit_width, int tile_size>
        void radix_sort(const concurrency::accelerator_view& accl_view, concurrency::array_view<T>& input_view)
        {
        }
    }

    // TODO: NOT IMPLEMENTED radix_sort
    inline void radix_sort(const concurrency::accelerator_view& accl_view, concurrency::array_view<int>& input_view)
    {
        static const int bin_width = 4;
        static const int tile_size = 256;
        _details::radix_sort<int, bin_width, tile_size>(accl_view, input_view);
    }

    // TODO: NOT IMPLEMENTED radix_sort
    inline void radix_sort(concurrency::array_view<int>& input_view)
    {
        radix_sort(_details::auto_select_target(), input_view);
    }

    /*
    inline void radix_sort(concurrency::array_view<int>& input_view, const unsigned int digit_width)
    {
    ::amp_algorithms::_details::radix_sort<int, 4>(_details::auto_select_target(), input_view);
    }

    inline void radix_sort(const concurrency::accelerator_view& accl_view, concurrency::array_view<unsigned int>& input_view, const unsigned int digit_width)
    {
    ::amp_algorithms::_details::radix_sort<unsigned int, 4>(accl_view, input_view);
    }

    inline void radix_sort(concurrency::array_view<unsigned int>& input_view)
    {
    ::amp_algorithms::_details::radix_sort<unsigned int, 4>(_details::auto_select_target(), input_view);
    }
    */

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
