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
* This file serves as a passthrough from the pre-compile step that establishes
* the native SIMD width into the implementation of the library. The structure
* and contents of this file should not be modified directly.
*---------------------------------------------------------------------------*/
#pragma once
#ifndef _NATIVE_SIMD_WIDTH_H_BUMPTZI
#define _NATIVE_SIMD_WIDTH_H_BUMPTZI

namespace amp_stl_algorithms
{
	inline namespace amp_stl_algorithms_implementation
	{
		static constexpr decltype(concurrency::tiled_index<1>::tile_dim0) native_simd_width = 64;
	}  // namespace amp_stl_algorithms_implementation
}      // namespace amp_stl_algorithms
#endif // _NATIVE_SIMD_WIDTH_H_BUMPTZI