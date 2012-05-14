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

using namespace concurrency;

namespace _details
{
    inline accelerator_view auto_select_target()
    {
        static accelerator_view auto_select_accelerator_view = concurrency::accelerator(accelerator::cpu_accelerator).create_view();
        return auto_select_accelerator_view;
    }

    template <int _Rank, typename _Kernel_type>
    void parallel_for_each(const accelerator_view &_Accl_view, const extent<_Rank>& _Compute_domain, const _Kernel_type &_Kernel)
    {
        _Host_Scheduling_info _SchedulingInfo = { NULL };
        if (_Accl_view != _details::auto_select_target()) 
        {
            _SchedulingInfo._M_accelerator_view = details::_Get_accelerator_view_impl_ptr(_Accl_view);
        }

        details::_Parallel_for_each(&_SchedulingInfo, _Compute_domain, _Kernel);
    }

    template <int _Dim0, int _Dim1, int _Dim2, typename _Kernel_type>
    void parallel_for_each(const accelerator_view &_Accl_view, const tiled_extent<_Dim0, _Dim1, _Dim2>& _Compute_domain, const _Kernel_type& _Kernel)
    {
        _Host_Scheduling_info _SchedulingInfo = { NULL };
        if (_Accl_view != _details::auto_select_target()) 
        {
            _SchedulingInfo._M_accelerator_view = details::_Get_accelerator_view_impl_ptr(_Accl_view);
        }

        details::_Parallel_for_each(&_SchedulingInfo, _Compute_domain, _Kernel);
    }

    template <int _Dim0, int _Dim1, typename _Kernel_type>
    void parallel_for_each(const accelerator_view &_Accl_view, const tiled_extent<_Dim0, _Dim1>& _Compute_domain, const _Kernel_type& _Kernel)
    {
        _Host_Scheduling_info _SchedulingInfo = { NULL };
        if (_Accl_view != _details::auto_select_target()) 
        {
            _SchedulingInfo._M_accelerator_view = details::_Get_accelerator_view_impl_ptr(_Accl_view);
        }

        details::_Parallel_for_each(&_SchedulingInfo, _Compute_domain, _Kernel);
    }

    template <int _Dim0, typename _Kernel_type>
    void parallel_for_each(const accelerator_view &_Accl_view, const tiled_extent<_Dim0>& _Compute_domain, const _Kernel_type& _Kernel)
    {
        _Host_Scheduling_info _SchedulingInfo = { NULL };
        if (_Accl_view != _details::auto_select_target()) 
        {
            _SchedulingInfo._M_accelerator_view = details::_Get_accelerator_view_impl_ptr(_Accl_view);
        }

        details::_Parallel_for_each(&_SchedulingInfo, _Compute_domain, _Kernel);
    }

} // namespace amp_algorithms::_details

} // namespace amp_algorithms
