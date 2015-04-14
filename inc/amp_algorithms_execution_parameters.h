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
* C++ AMP standard algorithm library.
*
* This file contains the declaration and definition for class Execution_Parameters
* which subsumes all relevant quantities that we rely on throughout the implementation
* of the library. It (more specifically the tsz member) is also part of the conduit
* through which we ensure that we are always executing against the native
* (as far as we are interested) SIMD width and thus can avoid using barriers.
*---------------------------------------------------------------------------*/

#pragma once
#ifndef _AMP_ALGORITHMS_EXECUTION_PARAMETERS_H_BUMPTZI
#define _AMP_ALGORITHMS_EXECUTION_PARAMETERS_H_BUMPTZI

#include "native_simd_width.h"

#include <amp_algorithms_type_functions_helpers.h>
#include <amp.h>

namespace amp_stl_algorithms
{
	inline namespace amp_stl_algorithms_implementation
	{
		//------------------------------------------------------------------------------
		// Centralized execution parameters, will allow moving to the barier-free model.
		// NOTA BENE: the mechanism we use for identifying the native SIMD width (tsz)
		// is quite primitive and not intended for final consumption!
		//------------------------------------------------------------------------------
		class Execution_parameters {
#if defined(_DEBUG)
			static constexpr decltype(concurrency::tiled_index<1>::tile_dim0) tsz = 4;
#else
			static constexpr decltype(concurrency::tiled_index<1>::tile_dim0) tsz = /*4;*/native_simd_width;
#endif
			static constexpr decltype(tsz) max_tiles = 65535; /*D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;*/
			static constexpr decltype(auto) max_lanes = max_tiles * tsz;
		public:
			static constexpr decltype(tsz) tile_size() { return tsz; };
			static constexpr decltype(max_tiles) maximum_tile_cnt() { return max_tiles; };

			template<typename N>
			static N work_per_tile(N sz) restrict(cpu, amp)
			{
				const N available_tiles = _min(N(rounded_up_quotient(sz, tsz)), N(max_tiles));
				const N necessary_tiles = rounded_up_quotient(sz, tsz);
				//return (sz + (available_tiles - N(1))) / available_tiles; // This minimizes work_per_tile.
				return rounded_up_quotient(necessary_tiles, available_tiles) * tsz; // This minimizes tile count.
			};

			template<typename N>
			static decltype(auto) tiled_domain(N sz) restrict(cpu, amp)
			{
				return concurrency::tiled_extent<tsz>(concurrency::extent<1>(rounded_up_quotient(sz, work_per_tile(sz)) * tsz));
			};

			template<typename N>
			static constexpr N tile_cnt(N sz) restrict(cpu, amp)
			{
				return rounded_up_quotient(sz, work_per_tile(sz));
			};

			template<typename T, typename N>
			static decltype(auto) temporary_buffer(N sz)
			{
				return concurrency::array_view<T>(rounded_up_quotient(sz, work_per_tile(sz)));
			};

			template<typename T, typename N>
			static decltype(auto) temporary_buffer(N sz, T&& init)
			{
				const auto b = temporary_buffer<T>(sz);
				_fill_n(begin(b), b.extent.size(), std::forward<T>(init));
				return b;
			};
		};
	}  // namespace amp_stl_algorithms_implementation
}	   // namespace amp_stl_algorithms
#endif // _AMP_ALGORITHMS_EXECUTION_PARAMETERS_H_BUMPTZI