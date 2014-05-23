
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

#include <xx_amp_algorithms_direct3d_impl.h>
#include <amp_indexable_view.h>

namespace amp_algorithms
{
    //----------------------------------------------------------------------------
    // scan - D3D implementation wrapper
    //----------------------------------------------------------------------------

    namespace direct3d
    {
        // direct3d namespace has its own declarations of scan_mode and scan_direction.

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
            void scan_exclusive(const concurrency::array<T> &input_array, concurrency::array<T> &output_array, scan_direction direction, const BinaryFunction &binary_op)
            {
                // Scan is special case of multiscan where scan_size == scan_pitch and scan_count = 1
                scan_internal(input_array, output_array, direction, binary_op, input_array.extent.size(), input_array.extent.size(), 1);
            }

            // Performs forward exclusive scan (overload with direction already specified)
            template <typename T, typename BinaryFunction>
            void scan_exclusive(const concurrency::array<T> &input_array, concurrency::array<T> &output_array, const BinaryFunction &binary_op)
            {
                scan_exclusive(input_array, output_array, scan_direction::forward, binary_op);
            }

            // Performs forward exclusive prefix sum (overload with direction and binary function already specified)
            template <typename T>
            void scan_exclusive(const concurrency::array<T> &input_array, concurrency::array<T> &output_array)
            {
                scan_exclusive(input_array, output_array, scan_direction::forward, amp_algorithms::plus<T>());
            }

            // Performs exclusive multi scan is specified direction
            template <typename T, typename BinaryFunction>
            void multi_scan_exclusive(const concurrency::array<T, 2> &input_array, concurrency::array<T, 2> &output_array, scan_direction direction, const BinaryFunction &binary_op)
            {
                scan_internal(input_array, output_array, direction, binary_op, input_array.extent[1], input_array.extent[1], input_array.extent[0]);
            }

            // Performs exclusive segmented scan in specified direction
            template <typename T, typename BinaryFunction>
            void segmented_scan_exclusive(const concurrency::array<T> &input_array, concurrency::array<T> &output_array, const concurrency::array<unsigned int> &flags_array, scan_direction direction, const BinaryFunction &binary_op)
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

                // Create typed UAVs
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
                set_direction(scan_direction::forward);
            }

            // Common subset of scan setup for multiscan and scan
            template <typename T, unsigned int Rank, typename BinaryFunction>
            void scan_internal(const concurrency::array<T, Rank> &input_array, concurrency::array<T, Rank> &output_array, scan_direction direction, const BinaryFunction &binary_op, unsigned int scan_size, unsigned int scan_pitch, unsigned int scan_count)
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
            void set_direction(scan_direction direction)
            {
                if (m_selected_scan_direction != direction)
                {
                    std::string msg = "Failed to set scan direction";
                    _details::_check_hresult(m_scan->SetScanDirection(direction == scan_direction::forward ? D3DX11_SCAN_DIRECTION_FORWARD : D3DX11_SCAN_DIRECTION_BACKWARD), msg);
                    _details::_check_hresult(m_segmented_scan->SetScanDirection(direction == scan_direction::forward ? D3DX11_SCAN_DIRECTION_FORWARD : D3DX11_SCAN_DIRECTION_BACKWARD), msg);
                    m_selected_scan_direction = direction;
                }
            }

            // Scan data members 
            Microsoft::WRL::ComPtr<ID3DX11Scan> m_scan; // capable of scan and multiscan
            Microsoft::WRL::ComPtr<ID3DX11SegmentedScan> m_segmented_scan;

            Microsoft::WRL::ComPtr<ID3D11Device> m_device;
            Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_immediate_context;
            const concurrency::accelerator_view m_scan_accelerator_view;

            scan_direction m_selected_scan_direction;
        };
    }
}
