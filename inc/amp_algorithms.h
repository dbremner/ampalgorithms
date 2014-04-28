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
#include <wrl\client.h>

#include <xx_amp_algorithms_impl.h>
#include <amp_indexable_view.h>

namespace amp_algorithms
{
#pragma region Arithmetic, comparison, logical and bitwise,
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
    // fill
    //----------------------------------------------------------------------------

    template<typename OutputIndexableView, typename T>
    void fill(const concurrency::accelerator_view &accl_view, OutputIndexableView& output_view, const T& value)
    {
        :::amp_algorithms::generate(accl_view, output_view, [value]() restrict(amp) { return value; });
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
    // padded_read & padded_write
    //----------------------------------------------------------------------------

    template <typename TContainer, int N>
    inline typename TContainer::value_type padded_read(const TContainer& arr, const concurrency::index<N> idx) restrict(cpu, amp)
    {
        return arr.extent.contains(idx) ? arr[idx] : typename TContainer::value_type();
    }

    template <typename TContainer>
    inline typename TContainer::value_type padded_read(const TContainer& arr, const int idx) restrict(cpu, amp)
    {
        return padded_read<TContainer, 1>(arr, concurrency::index<1>(idx));
    }

    template <typename TContainer, int N>
    inline void padded_write(TContainer& arr, const concurrency::index<N> idx, const typename TContainer::value_type &value) restrict(cpu, amp)
    {
        if (arr.extent.contains(idx))
        {
            arr[idx] = value;
        }
    }

    template <typename TContainer>
    inline void padded_write(TContainer& arr, const int idx, const typename TContainer::value_type &value) restrict(cpu, amp)
    {
        padded_write<TContainer, 1>(arr, concurrency::index<1>(idx), value);
    }

    //----------------------------------------------------------------------------
    // radix_sort
    //----------------------------------------------------------------------------
    // http://www.heterogeneouscompute.org/wordpress/wp-content/uploads/2011/06/RadixSort.pdf
    //
    // http://www.intel.com/content/www/us/en/research/intel-labs-radix-sort-mic-report.html
    // http://www.cse.uconn.edu/~huang/fall12_5304/Presentation_Final/GPU_Sorting.pdf
    // http://www.cs.virginia.edu/~dgm4d/papers/RadixSortTR.pdf
    // http://xxx.lanl.gov/pdf/1008.2849
    // http://www.rebe.rau.ro/RePEc/rau/jisomg/WI12/JISOM-WI12-A11.pdf
    //
    // "Designing Efficient Sorting Algorithms for Manycore GPUs" http://www.nvidia.com/docs/io/67073/nvr-2008-001.pdf
    //
    // "Histogram Calculation in CUDA" http://docs.nvidia.com/cuda/samples/3_Imaging/histogram/doc/histogram.pdf
    //
    // TODO: Move this to the impl file?
    namespace _details
    {
        template<typename T, int key_size>
        int radix_key_value(const T value, const unsigned key_idx) restrict(amp, cpu)
        {
            const T mask = (1 << key_size) - 1;
            return (value & (mask << key_idx)) >> key_idx;
        }

        // TODO: T is limited to only integer types. Need to modify the template to restrict this.
        template <typename T, int key_size, int tile_size>
        void radix_sort(const concurrency::accelerator_view& accl_view, concurrency::array_view<T>& input_view)
        {
        }

        template <typename T, int key_size, int tile_size>
        void histogram_tile(const concurrency::array_view<T>& input_view, concurrency::array_view<T>& output_view,
            const int key_idx)
        {
            static const unsigned type_width = sizeof(T)* 8;
            static_assert((type_width % key_size == 0), "The sort key width must be an exact multiple of the type width.");

            static const unsigned bin_count = 1 << key_size;
            static const T bin_mask = bin_count - 1;
            static const int elements_per_thread = 1;          // TODO: Doesn't have to be a constant?

            // histogram all elements in a block
            concurrency::array<unsigned> histogram_bins(bin_count);

            concurrency::tiled_extent<tile_size> compute_domain = input_view.get_extent().tile<tile_size>().pad();

            concurrency::parallel_for_each(compute_domain,
                [=, &histogram_bins](concurrency::tiled_index<tile_size> tidx) restrict(amp)
            {
                // Each thread has its own histogram
                tile_static unsigned tile_bins[tile_size][bin_count];
                const int gidx = tidx.global[0];
                const int idx = tidx.local[0];
                const int start_elem = idx * elements_per_thread;

                // One thread initializes the global histogram bins.
                if (gidx == 0)
                {
                    for (int b = 0; b < bin_count; ++b)
                    {
                        histogram_bins(b) = 0;
                    }
                }

                // Initialize bins for this thread
                for (int b = 0; b < bin_count; ++b)
                {
                    tile_bins[idx][b] = 0u;
                }

                // Increment bins for each element.
                for (int i = start_elem; i < (start_elem + elements_per_thread); ++i)
                {
                    if (gidx < input_view.extent[0])
                        tile_bins[idx][_details::radix_key_value<T, key_size>(input_view[gidx], key_idx)]++;
                }

                // Wait for all threads to finish incrementing.
                tidx.barrier.wait();

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
                        concurrency::atomic_fetch_add(&histogram_bins(b), tile_bins[0][b]);
                    }
                }
            });

#if _DEBUG
            {
                std::vector<unsigned> bins(4);
                concurrency::copy(histogram_bins, begin(bins));
            }
#endif
            // prefix scan the histogram results to get offsets.
            // TODO: This scan supports multi-tile. Probably need a simpler version that uses only one tile.
            concurrency::array<unsigned> histogram_scan(bin_count);
            amp_algorithms::direct3d::::scan s(2 * bin_count);
            s.scan_exclusive(histogram_bins, histogram_bins);
#if _DEBUG
            {
                std::vector<unsigned> scans(4);
                concurrency::copy(histogram_bins, begin(scans));
            }
#endif
            // Sort elements for each tile to maximise memory affinity when writing to global memory.

            // reorder the elements based on the offsets.

            concurrency::parallel_for_each(compute_domain,
                [=, &histogram_bins](concurrency::tiled_index<tile_size> tidx) restrict(amp)
            {
                const int gidx = tidx.global[0];
                const int idx = tidx.local[0];

                const int d = idx - 0;
                output_view[d] = input_view[];
            });
        }
    }

    template<typename T>
    inline void merge_bins(const T* left, const T*  right, const int bin_count) restrict(amp)
    {
        for (int b = 0; b < bin_count; ++b)
        {
            left[b] += right[b];
        }
    }

    // TODO: NOT IMPLEMENTED radix_sort
    inline void radix_sort(const concurrency::accelerator_view& accl_view, concurrency::array_view<int>& input_view)
    {
        static const int bin_width = 4;
        static const int tile_size = 256;
        ::amp_algorithms::_details::radix_sort<int, bin_width, tile_size>(accl_view, input_view);
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
            return _details::reduce<512, 10000, InputIndexableView, BinaryFunction>(accl_view, input_view, binary_op);
    }

    template <typename InputIndexableView, typename BinaryFunction>
    typename std::result_of<BinaryFunction(const typename indexable_view_traits<InputIndexableView>::value_type&, const typename indexable_view_traits<InputIndexableView>::value_type&)>::type
        reduce(const InputIndexableView &input_view, const BinaryFunction &binary_op)
    {
            return reduce(_details::auto_select_target(), input_view, binary_op);
    }

    //----------------------------------------------------------------------------
    // scan - D3D implementation wrapper
    //----------------------------------------------------------------------------

    enum class scan_mode : int
    {
        exclusive = 0,
        inclusive = 1
    };

    // TODO: Need to support forward and reverse scan directions and segmented scan for non-DX scan implementation.
    enum class scan_direction : int
    {
        forward = 0,
        backward = 1
    };

    namespace direct3d
    {
        class scan
        {
        public:
            // Constructs scan object, this constructor provides ability to define max_scan_count for multiscan
            scan(unsigned int max_scan_size, unsigned int max_scan_count, const concurrency::accelerator_view &target_accel_view = concurrency::accelerator().default_view) : m_scan_accelerator_view(target_accel_view)
            {
                initialize_scan(max_scan_size, max_scan_count);
            }

            // Constructs scan object 
            scan(unsigned int max_scan_size, const concurrency::accelerator_view &target_accel_view = concurrency::accelerator().default_view) : m_scan_accelerator_view(target_accel_view)
            {
                initialize_scan(max_scan_size, 1);
            }

            // Performs exclusive scan in specified direction
            template <typename T, typename BinaryFunction>
            void scan_exclusive(const concurrency::array<T> &input_array, concurrency::array<T> &output_array, amp_algorithms::scan_direction direction, const BinaryFunction &binary_op)
            {
                // Scan is special case of multiscan where scan_size == scan_pitch and scan_count = 1
                scan_internal(input_array, output_array, direction, binary_op, input_array.extent.size(), input_array.extent.size(), 1);
            }

            // Performs forward exclusive scan (overload with direction already specified)
            template <typename T, typename BinaryFunction>
            void scan_exclusive(const concurrency::array<T> &input_array, concurrency::array<T> &output_array, const BinaryFunction &binary_op)
            {
                scan_exclusive(input_array, output_array, amp_algorithms::scan_direction::forward, binary_op);
            }

            // Performs forward exclusive prefix sum (overload with direction and binary function already specified)
            template <typename T>
            void scan_exclusive(const concurrency::array<T> &input_array, concurrency::array<T> &output_array)
            {
                scan_exclusive(input_array, output_array, amp_algorithms::scan_direction::forward, amp_algorithms::plus<T>());
            }

            // Performs exclusive multi scan is specified direction
            template <typename T, typename BinaryFunction>
            void multi_scan_exclusive(const concurrency::array<T, 2> &input_array, concurrency::array<T, 2> &output_array, amp_algorithms::scan_direction direction, const BinaryFunction &binary_op)
            {
                scan_internal(input_array, output_array, direction, binary_op, input_array.extent[1], input_array.extent[1], input_array.extent[0]);
            }

            // Performs exclusive segmented scan in specified direction
            template <typename T, typename BinaryFunction>
            void segmented_scan_exclusive(const concurrency::array<T> &input_array, concurrency::array<T> &output_array, const concurrency::array<unsigned int> &flags_array, amp_algorithms::scan_direction direction, const BinaryFunction &binary_op)
            {
                static_assert(_details::_dx_scan_type_helper<T>::is_type_supported, "Unsupported type for scan");
                static_assert(_details::_dx_scan_op_helper<BinaryFunction>::is_op_supported, "Unsupported binary function for scan");

                // Verify that we have the same accelerator view for both input, output and scan object
                if (input_array.accelerator_view != output_array.accelerator_view || input_array.accelerator_view != flags_array.accelerator_view || input_array.accelerator_view != m_scan_accelerator_view)
                {
                    throw runtime_exception("The accelerator_view for input_array, output_array, flags_array and scan object has to be the same.", E_INVALIDARG);
                }

                // Get d3d11 buffer pointers
                Microsoft::WRL::ComPtr<ID3D11Buffer> src_buffer(_details::_get_d3d11_buffer_ptr(input_array));
                Microsoft::WRL::ComPtr<ID3D11Buffer> flags_buffer(_details::_get_d3d11_buffer_ptr(flags_array));
                Microsoft::WRL::ComPtr<ID3D11Buffer> dst_buffer(_details::_get_d3d11_buffer_ptr(output_array));

                // Create typed uavs
                Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> src_view(_details::_create_d3d11_uav(m_device, src_buffer, _details::_dx_scan_type_helper<T>::dx_view_type));
                Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> flags_view(_details::_create_d3d11_uav(m_device, flags_buffer, DXGI_FORMAT_R32_UINT));
                // 2nd view is only needed if destination buffer is different from source buffer (not-in-place scan)
                Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> dst_view;
                if (src_buffer.Get() == dst_buffer.Get())
                {
                    dst_view = src_view;
                }
                else
                {
                    dst_view = _details::_create_d3d11_uav(m_device, dst_buffer, _details::_dx_scan_type_helper<T>::dx_view_type);
                }

                set_direction(direction);
                _details::_dx_state_cleaner cleaner(m_immediate_context);
                auto hr_result = m_segmented_scan->SegScan(_details::_dx_scan_type_helper<T>::dx_scan_type, _details::_dx_scan_op_helper<BinaryFunction>::dx_op_type, input_array.extent.size(), src_view.Get(), flags_view.Get(), dst_view.Get());
                _details::_check_hresult(hr_result, "Failed to perform scan");
            }

        private:
            // Common subset of initialization for both scan constructors
            void initialize_scan(unsigned int max_scan_size, unsigned int max_scan_count)
            {
                // Get device and context handles
                _ASSERTE(m_device.Get() == nullptr);
                m_device = _details::_get_d3d11_device_ptr(m_scan_accelerator_view);
                _ASSERTE(m_immediate_context.Get() == nullptr);
                m_device->GetImmediateContext(m_immediate_context.GetAddressOf());

                // Create DirectX scan objects
                std::string msg = "Failed to create scan object";
                _details::_check_hresult(D3DX11CreateScan(m_immediate_context.Get(), max_scan_size, max_scan_count, m_scan.GetAddressOf()), msg);
                _details::_check_hresult(D3DX11CreateSegmentedScan(m_immediate_context.Get(), max_scan_size, m_segmented_scan.GetAddressOf()), msg);

                // Set default direction
                set_direction(amp_algorithms::scan_direction::forward);
            }

            // Common subset of scan setup for multiscan and scan
            template <typename T, unsigned int Rank, typename BinaryFunction>
            void scan_internal(const concurrency::array<T, Rank> &input_array, concurrency::array<T, Rank> &output_array, amp_algorithms::scan_direction direction, const BinaryFunction &binary_op, unsigned int scan_size, unsigned int scan_pitch, unsigned int scan_count)
            {
                static_assert(_details::_dx_scan_type_helper<T>::is_type_supported, "Unsupported type for scan");
                static_assert(_details::_dx_scan_op_helper<BinaryFunction>::is_op_supported, "Currently only fixed set of binary functions is allowed, we are working to remove this limitation");

                // Verify that we have the same accelerator view for both input, output and scan object
                if (input_array.accelerator_view != output_array.accelerator_view || input_array.accelerator_view != m_scan_accelerator_view)
                {
                    throw runtime_exception("The accelerator_view for input_array, output_array and scan object has to be the same.", E_INVALIDARG);
                }

                // Note: DirectX library performs validation for scan_size, pitch etc, so it would be a dup and unnecessary perf impact to do it here

                // Get d3d11 buffer pointers
                Microsoft::WRL::ComPtr<ID3D11Buffer> src_buffer(_details::_get_d3d11_buffer_ptr(input_array));
                Microsoft::WRL::ComPtr<ID3D11Buffer> dst_buffer(_details::_get_d3d11_buffer_ptr(output_array));

                // Create typed UAVs
                Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> src_view(_details::_create_d3d11_uav(m_device, src_buffer, _details::_dx_scan_type_helper<T>::dx_view_type));
                // 2nd view is only needed if destination buffer is different from source buffer (not-in-place scan)
                Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> dst_view;
                if (src_buffer.Get() == dst_buffer.Get())
                {
                    dst_view = src_view;
                }
                else
                {
                    dst_view = _details::_create_d3d11_uav(m_device, dst_buffer, _details::_dx_scan_type_helper<T>::dx_view_type);
                }

                set_direction(direction);
                _details::_dx_state_cleaner cleaner(m_immediate_context);
                auto hr_result = m_scan->Multiscan(_details::_dx_scan_type_helper<T>::dx_scan_type, _details::_dx_scan_op_helper<BinaryFunction>::dx_op_type, scan_size, scan_pitch, scan_count, src_view.Get(), dst_view.Get());
                _details::_check_hresult(hr_result, "Failed to perform scan");
            }

            // Changes scan direction
            void set_direction(amp_algorithms::scan_direction direction)
            {
                if (m_selected_scan_direction != direction)
                {
                    std::string msg = "Failed to set scan direction";
                    _details::_check_hresult(m_scan->SetScanDirection(direction == amp_algorithms::scan_direction::forward ? D3DX11_SCAN_DIRECTION_FORWARD : D3DX11_SCAN_DIRECTION_BACKWARD), msg);
                    _details::_check_hresult(m_segmented_scan->SetScanDirection(direction == amp_algorithms::scan_direction::forward ? D3DX11_SCAN_DIRECTION_FORWARD : D3DX11_SCAN_DIRECTION_BACKWARD), msg);
                    m_selected_scan_direction = direction;
                }
            }

            // Scan data members 
            Microsoft::WRL::ComPtr<ID3DX11Scan> m_scan; // capable of scan and multiscan
            Microsoft::WRL::ComPtr<ID3DX11SegmentedScan> m_segmented_scan;

            Microsoft::WRL::ComPtr<ID3D11Device> m_device;
            Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_immediate_context;
            const concurrency::accelerator_view m_scan_accelerator_view;

            amp_algorithms::scan_direction m_selected_scan_direction;
        };
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

    namespace _details
    {
#ifdef USE_REF
        static const int warp_size = 4;
#else
        static const int warp_size = 32;
#endif
        static const int warp_max = _details::warp_size - 1;

        // TODO: Scan still needs optimizing.

        template <scan_mode _Mode, typename _BinaryOp, typename T>
        T scan_warp(T* const tile_data, const int idx, const _BinaryOp& op) restrict(amp)
        {
            const int widx = idx & _details::warp_max;

            if (widx >= 1)
                tile_data[idx] = op(tile_data[idx - 1], tile_data[idx]);
            if ((warp_size > 2) && (widx >= 2))
                tile_data[idx] = op(tile_data[idx - 2], tile_data[idx]);
            if ((warp_size > 4) && (widx >= 4))
                tile_data[idx] = op(tile_data[idx - 4], tile_data[idx]);
            if ((warp_size > 8) && (widx >= 8))
                tile_data[idx] = op(tile_data[idx - 8], tile_data[idx]);
            if ((warp_size > 16) && (widx >= 16))
                tile_data[idx] = op(tile_data[idx - 16], tile_data[idx]);
            if ((warp_size > 32) && (widx >= 32))
                tile_data[idx] = op(tile_data[idx - 32], tile_data[idx]);

            if (_Mode == scan_mode::inclusive)
                return tile_data[idx];
            return (widx > 0) ? tile_data[idx - 1] : T();
        }

        template <int TileSize, scan_mode _Mode, typename _BinaryOp, typename T>
        T scan_tile(T* const tile_data, concurrency::tiled_index<TileSize> tidx, const _BinaryOp& op) restrict(amp)
        {
            static_assert(is_power_of_two<warp_size>::value, "Warp size must be an exact power of 2.");
            const int lidx = tidx.local[0];
            const int warp_id = lidx >> log2<warp_size>::value;

            // Step 1: Intra-warp scan in each warp
            auto val = scan_warp<_Mode, _BinaryOp>(tile_data, lidx, op);
            tidx.barrier.wait_with_tile_static_memory_fence();

            // Step 2: Collect per-warp partial results
            if ((lidx & warp_max) == _details::warp_max)
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
    } // namespace _details

    template <int TileSize, scan_mode _Mode, typename _BinaryOp, typename T>
    inline void scan_new(const concurrency::array<T, 1>& input_array, concurrency::array<T, 1>& output_array, const _BinaryOp& op)
    {
        static_assert(TileSize >= _details::warp_size, "Tile size must be at least the size of a single warp.");
        static_assert(TileSize % _details::warp_size == 0, "Tile size must be an exact multiple of warp size.");
        static_assert(TileSize <= (_details::warp_size * _details::warp_size), "Tile size must less than or equal to the square of the warp size.");

        assert(output_array.extent[0] >= _details::warp_size);
        auto compute_domain = output_array.extent.tile<TileSize>().pad();
        concurrency::array<T, 1> tile_results(compute_domain / TileSize);

        // 1 & 2. Scan all tiles and store results in tile_results.
        concurrency::parallel_for_each(compute_domain,
            [=, &input_array, &output_array, &tile_results](concurrency::tiled_index<TileSize> tidx) restrict(amp)
        {
            const int gidx = tidx.global[0];
            const int lidx = tidx.local[0];
            tile_static T tile_data[TileSize];
            tile_data[lidx] = padded_read(input_array, gidx);
            tidx.barrier.wait_with_tile_static_memory_fence();

            auto val = _details::scan_tile<TileSize, _Mode>(tile_data, tidx, amp_algorithms::plus<T>());
            if (lidx == (TileSize - 1))
            {
                tile_results[tidx.tile[0]] = val;
                if (_Mode == scan_mode::exclusive)
                    tile_results[tidx.tile[0]] += input_array[gidx];
            }
            padded_write(output_array, gidx, tile_data[lidx]);
        });

        // 3. Scan tile results.
        if (tile_results.extent[0] > TileSize)
        {
            scan_new<TileSize, amp_algorithms::scan_mode::exclusive>(tile_results, tile_results, op);
        }
        else
        {
            concurrency::parallel_for_each(compute_domain,
                [=, &tile_results](concurrency::tiled_index<TileSize> tidx) restrict(amp)
            {
                const int gidx = tidx.global[0];
                const int lidx = tidx.local[0];
                tile_static T tile_data[TileSize];
                tile_data[lidx] = tile_results[gidx];
                tidx.barrier.wait_with_tile_static_memory_fence();

                _details::scan_tile<TileSize, amp_algorithms::scan_mode::exclusive>(tile_data, tidx, amp_algorithms::plus<T>());

                tile_results[gidx] = tile_data[lidx];
                tidx.barrier.wait_with_tile_static_memory_fence();
            });
        }
        // 4. Add the tile results to the individual results for each tile.
        concurrency::parallel_for_each(compute_domain,
            [=, &output_array, &tile_results](concurrency::tiled_index<TileSize> tidx) restrict(amp)
        {
            const int gidx = tidx.global[0];
            if (gidx < output_array.extent[0])
                output_array[gidx] += tile_results[tidx.tile[0]];
        });
    }

    // TODO: Refactor this to remove duplicate code. Also need to decide on final API.

    template <int TileSize, typename InIt, typename OutIt>
    inline void scan_exclusive_new(InIt first, InIt last, OutIt dest_first)
    {
        typedef InIt::value_type T;

        const int size = int(std::distance(first, last));
        concurrency::array<T, 1> in(size);
        concurrency::array<T, 1> out(size);
        concurrency::copy(first, last, in);

        scan_new<TileSize, amp_algorithms::scan_mode::exclusive>(in, out, amp_algorithms::plus<T>());

        concurrency::copy(out, dest_first);
    }

    template <int TileSize, typename InIt, typename OutIt>
    inline void scan_inclusive_new(InIt first, InIt last, OutIt dest_first)
    {
        typedef InIt::value_type T;

        const int size = int(std::distance(first, last));
        concurrency::array<T, 1> in(size);
        concurrency::array<T, 1> out(size);
        concurrency::copy(first, last, in);

        scan_new<TileSize, amp_algorithms::scan_mode::inclusive>(in, out, amp_algorithms::plus<T>());

        concurrency::copy(out, dest_first);
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

#include <xx_amp_algorithms_impl_inl.h>
