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
* This file contains the ADTs and functions necessary for the HePA C++
* programming model that we employ for implementing the algorithms library.
* It represents an augmentation and further development of the C++ AMP
* programming model, building upon it. It is fully detailed in the supporting
* documents and articles: TO BE COMPLETED.
* TODO: this is just a slice of the programming model, which will be fully
* included incrementally.
*---------------------------------------------------------------------------*/

#pragma once
#ifndef _AMP_ALGORITHMS_PROGRAMMING_MODEL_H_BUMPTZI
#define _AMP_ALGORITHMS_PROGRAMMING_MODEL_H_BUMPTZI

#include "amp_algorithms_type_functions_helpers.h"

#include <amp.h>

namespace amp_stl_algorithms
{
	inline namespace amp_stl_algorithms_implementation
	{
		//----------------------------------------------------------------------------
		// uniform_invoke
		// TODO: extract documentation from HEPA doc, add here.
		//----------------------------------------------------------------------------

		template<typename F, int tsz>
		inline void uniform_invoke(const concurrency::tiled_index<tsz>& tidx, F&& fn) restrict(amp)
		{
			// Arbitrarily choose a single lane - by convention we choose the first.
			if (tidx.tile_origin == tidx.global) {
				forward<F>(fn)(tidx.tile[0]);
			}
		}

		//----------------------------------------------------------------------------
		// tile_exclusive_cache - temporary name
		// TODO: extract documentation from HEPA doc, add here.
		// TODO: refactor into common base + dedicated derivations.
		//----------------------------------------------------------------------------

		template<typename T, int tsz, typename Enable = void> class tile_exclusive_cache;

		template<typename T, int tsz>
		class tile_exclusive_cache<T, tsz, std::enable_if_t<!std::is_array<T>::value>> {
			// Note: const-ness is a property of T, hence it is valid use to modify a non-const T
			// through a const tile_exclusive_cache<T>.
			template<typename U, typename E = void> struct T_wrapper;
			template<typename U>
			struct T_wrapper<U, std::enable_if_t<sizeof(U) < sizeof(int)>> {
				alignas(sizeof(int)) mutable U x;
				operator U&() const restrict(cpu, amp) { return x; };
			};
			using T_AMP = static_if_t<T_wrapper<T>, T, sizeof(T) < sizeof(int)>;
		public:
			tile_exclusive_cache() = delete;

			template<typename F>
			explicit tile_exclusive_cache(const concurrency::tiled_index<tsz>& tidx, F&& init_fn) restrict(cpu);

			template<typename F>
			explicit tile_exclusive_cache(const concurrency::tiled_index<tsz>& tidx, F&& init_fn) restrict(amp);

			tile_exclusive_cache(const tile_exclusive_cache&) = default;
			tile_exclusive_cache(tile_exclusive_cache&&) = default;

			tile_exclusive_cache& operator=(const tile_exclusive_cache&) = default;
			tile_exclusive_cache& operator=(tile_exclusive_cache&&) = default;

			~tile_exclusive_cache() restrict(cpu, amp) {};

			operator T&() const restrict(cpu, amp) { return local(); }
			T& local() const restrict(cpu, amp) { return data[0]; };
		private:
			concurrency::array_view<T_AMP> data;
			concurrency::array_view<const concurrency::tiled_index<tsz>> tidx;
		};

		template<typename T, int tsz>
		template<typename F>
		tile_exclusive_cache<T, tsz, std::enable_if_t<!std::is_array<T>::value>>::tile_exclusive_cache(const concurrency::tiled_index<tsz>& tile, F&& init_fn) restrict(cpu)
			: data(1), tidx(1, &tile)
		{
			thread_local static T_AMP t; // In practice we should optimally align this for SIMD processing.
			data = concurrency::array_view<T_AMP>(1, &t);
			std::forward<F>(init_fn)(local());
		}

		template<typename T, int tsz>
		template<typename F>
		tile_exclusive_cache<T, tsz, std::enable_if_t<!std::is_array<T>::value>>::tile_exclusive_cache(const concurrency::tiled_index<tsz>& tile, F&& init_fn) restrict(amp)
			: data(1, nullptr), tidx(1, &tile)
		{
			tile_static T_AMP t;
			data = concurrency::array_view<T_AMP>(1, &t);
//			if (std::is_scalar<T>::value) {
				uniform_invoke(tidx[0], [=, out = data](auto&&) {
					init_fn(static_cast<T&>(out[0]));
				}); // Somewhat rotten, workaround for restrict(amp) capture limitations.
//			}
//			else {
//				forward<F>(init_fn)(local());
//			}
		}

		template<typename T, int tsz>
		class tile_exclusive_cache<T, tsz, std::enable_if_t<std::is_array<T>::value &&
													        std::rank<T>::value == 1>> {
			// Note that only flat arrays are supported initially.
			// Note: const-ness is a property of T, hence it is valid use to modify a non-const T
			// through a const tile_exclusive_cache<T>.
			template<typename U> struct Array_wrapper { U arr; };

			template<typename U, typename E = void> struct T_wrapper;
			template<typename U>
			struct T_wrapper<U, std::enable_if_t<sizeof(U) < sizeof(unsigned int)>> {
				alignas(sizeof(unsigned int)) mutable U x;
				operator U&() const restrict(cpu, amp) { return x; };
			};

			static constexpr decltype(auto) sz = std::extent<T>::value;
			using T_AMP = static_if_t<Array_wrapper<T_wrapper<std::remove_all_extents_t<T>>[sz]>,
									  Array_wrapper<T>,
									  sizeof(std::remove_all_extents_t<T>) < sizeof(int)>;
		public:
			using value_type = std::remove_all_extents_t<T>;

			template<typename F>
			tile_exclusive_cache(const concurrency::tiled_index<tsz>& tidx, F&& init_fn) restrict(cpu);
			template<typename F>
			tile_exclusive_cache(const concurrency::tiled_index<tsz>& tidx, F&& init_fn) restrict(amp);

			tile_exclusive_cache(const tile_exclusive_cache&) = default;
			tile_exclusive_cache(tile_exclusive_cache&&) = default;

			tile_exclusive_cache& operator=(const tile_exclusive_cache&) = default;
			tile_exclusive_cache& operator=(tile_exclusive_cache&&) = default;

			value_type& operator[](int idx) const restrict(cpu, amp) { return data[0].arr[idx]; };

			// Terrible name, probably. Unclean too - the local function should be revisited.
			operator T&() const restrict(cpu, amp) { return local(); };
			T& local() const restrict(cpu, amp) { return reinterpret_cast<T&>(data[0].arr); };

			static constexpr auto size() restrict(cpu, amp) -> decltype(sz) { return sz; };

			template<typename I>
			auto load(I first, I last) const restrict(cpu, amp) -> Pointer<value_type>;

			template<typename Op>
			auto reduce(Op op) const restrict(cpu, amp) -> value_type;
		private:
			concurrency::array_view<T_AMP> data;
			concurrency::array_view<const concurrency::tiled_index<tsz>> tidx;
		};

		template<typename T, int tsz>
		template<typename F>
		tile_exclusive_cache<T, tsz, std::enable_if_t<std::is_array<T>::value &&
			  									      std::rank<T>::value == 1>>::tile_exclusive_cache(const concurrency::tiled_index<tsz>& tile, F&& init_fn) restrict(cpu)
			: data(1), tidx(1, &tile)
		{
			thread_local static T_AMP t;
			data = concurrency::array_view<T_AMP>(1, &t);
			std::forward<F>(init_fn)(local());
		}

		template<typename T, int tsz>
		template<typename F>
		tile_exclusive_cache<T, tsz, std::enable_if_t<std::is_array<T>::value &&
													  std::rank<T>::value == 1>>::tile_exclusive_cache(const concurrency::tiled_index<tsz>& tile, F&& init_fn) restrict(amp)
			: data(1, nullptr), tidx(1, &tile)
		{
			tile_static T_AMP t;
			data = concurrency::array_view<T_AMP>(1, &t);
			forward<F>(init_fn)(local());
		}

		template<typename T, int tsz>
		template<typename I>
		inline auto tile_exclusive_cache<T, tsz, std::enable_if_t<std::is_array<T>::value &&
																  std::rank<T>::value == 1>>::load(I first, I last) const restrict(cpu, amp) -> Pointer<value_type>
		{	// TODO: should probably use existing algorithms;
			if (first == last) return data[0].arr;

			for (Difference_type<I> i = tidx[0].local[0]; i < (last - first); i += tsz) {
				data[0].arr[i] = first[i];
			}

			return data[0].arr + (last - first);
		}

		template<typename T, int tsz>
		template<typename Op>
		inline auto tile_exclusive_cache<T, tsz, std::enable_if_t<std::is_array<T>::value &&
																  std::rank<T>::value == 1>>::reduce(Op op) const restrict(cpu, amp) -> value_type
		{	// TODO: should use the existing algorithms
			for (decltype(size()) i = tidx[0].local[0] + tsz; i < sz; i += tsz) {
				data[0].arr[tidx[0].local[0]] = op(data[0].arr[tidx[0].local[0]], data[0].arr[i]);
			}

			static_for<half_nonnegative(tsz), 0u, Inc::div, 2u>()([=,
																   d = this->data,
																   i = this->tidx[0].local[0]](auto&& h) {
				if (i < h) {
					d[0].arr[i] = op(d[0].arr[i], d[0].arr[i + h]);
				}
			});

			return data[0].arr[0];
		}
	}  // namespace amp_stl_algorithms_implementation
}	   // namespace amp_stl_algorithms
#endif // _AMP_ALGORITHMS_PROGRAMMING_MODEL_H_BUMPTZI
