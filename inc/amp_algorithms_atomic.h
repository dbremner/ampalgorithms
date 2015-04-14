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
* This file contains an atomic ADT that is a subset of the std::atomic one,
* usable in restrict(amp) contexts.
*---------------------------------------------------------------------------*/

#pragma once
#ifndef _AMP_ALGORITHMS_ATOMIC_H_BUMPTZI
#define _AMP_ALGORITHMS_ATOMIC_H_BUMPTZI

#include <amp.h>
#include <cstdlib>
#include <type_traits>

namespace amp_stl_algorithms
{
	template<typename T, typename Enable> class atomic; // Undefined.

	// Integral types support all operations.
	template<typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
	class atomic {
	public:
		atomic() = default;
		constexpr atomic(T init) restrict(cpu, amp)	: x(move(init)) { /* This is not atomic! */};
		atomic(const atomic&) = delete;
		atomic(atomic&&) = delete;

		atomic& operator=(const atomic&) = delete;
		atomic& operator=(atomic&&) = delete;

		T operator=(T in) restrict(cpu, amp) { store(in); return x; }
		static constexpr bool is_lock_free() restrict(cpu, amp) { return true; };

		void store(T in) restrict(cpu) { x = in; /* NOT IMPLEMENTED FOR CPU*/ };
		void store(T in) restrict(amp) { concurrency::atomic_exchange(&x, in); };

		T load() const restrict(cpu) { return x; };
		T load() const restrict(amp) { return concurrency::atomic_fetch_and(&x, UINT_MAX); };

		operator T() const restrict(cpu, amp) { return load(); };

		T exchange(T in) restrict(amp) { return concurrency::atomic_exchange(&x, in); };
		bool compare_exchange_strong(T& expected, T in) restrict(amp) { return concurrency::atomic_compare_exchange(&x, &expected, in); };

		T fetch_add(T y) restrict(amp) { return concurrency::atomic_fetch_add(&x, y); };
		T fetch_sub(T y) restrict(amp) { return concurrency::atomic_fetch_sub(&x, y); };
		T fetch_and(T y) restrict(amp) { return concurrency::atomic_fetch_and(&x, y); };
		T fetch_or(T y) restrict(amp) { return concurrency::atomic_fetch_or(&x, y); };
		T fetch_xor(T y) restrict(amp) { return concurrency::atomic_fetch_xor(&x, y); };

		// Non-standard but useful
		T fetch_max(T y) restrict(amp) { return concurrency::atomic_fetch_max(&x, y); };
		T fetch_min(T y) restrict(amp) { return concurrency::atomic_fetch_min(&x, y); };
		// End of non-standard extensions.

		T operator++() restrict(amp) { return concurrency::atomic_fetch_inc(&x) + T(1); };
		T operator++(int) restrict(amp) { return concurrency::atomic_fetch_inc(&x); };

		T operator--() restrict(amp) { return concurrency::atomic_fetch_dec(&x) - T(1); };
		T operator--(int) restrict(amp) { return concurrency::atomic_fetch_dec(&x); };

		T operator+=(T y) restrict(amp) { return fetch_add(y); };
		T operator-=(T y) restrict(amp) { return fetch_sub(y); };

		T operator&=(T y) restrict(amp) { return fetch_and(y); };
		T operator|=(T y) restrict(amp) { return fetch_or(y); };
		T operator^=(T y) restrict(amp) { return fetch_xor(y); };
	private:
		mutable T x; // Mutable is needed due to emulation of atomic_load via AND with UINT_MAX
	};

	// Other atomic cases unimplemented yet (i.e. floating-point or pointer types).
}	   // namespace amp_stl_algorithms
#endif // _AMP_ALGORITHMS_ATOMIC_H_BUMPTZI
