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
* This file contains the implementation for C++ AMP standard algorithms
*---------------------------------------------------------------------------*/

#pragma once
#ifndef _XX_AMP_STL_ALGORITHMS_IMPL_INL_H_BUMPTZI
#define _XX_AMP_STL_ALGORITHMS_IMPL_INL_H_BUMPTZI

#include "amp_algorithms_atomic.h"
#include "amp_algorithms_execution_parameters.h"
#include "amp_iterators.h"
#include "amp_algorithms_pair.h"
#include "amp_algorithms_programming_model.h"
#include "amp_algorithms_type_functions_helpers.h"

#include <amp.h>
#include <amp_short_vectors.h>
#include <functional>
#include <numeric>
#include <ppl.h>
#include <type_traits>
#include <utility>

namespace amp_stl_algorithms
{
#define AMP_ALG ::amp_stl_algorithms::
	inline namespace amp_stl_algorithms_implementation
	{
		// TODO: Lots of the algorithms that typically do a small amount of work per thread should
		// use tiling to save the runtime overhead of having to do this.

		//----------------------------------------------------------------------------
		// adjacent_difference
		//----------------------------------------------------------------------------

		template<typename I1, typename I2, typename Op, int tsz>
		inline I2 _adjacent_difference_single_tile_n(I1 first, Difference_type<I1> n, I2 dest_first,
													 const concurrency::tiled_index<tsz>& tidx, Op op) restrict(amp)
		{
			tile_static AMP_ALG Value_type<I1> tmp[tidx.tile_dim0];

			const AMP_ALG Difference_type<I1> m = _min(tidx.tile_dim0 * 1, n);
			_copy_in_tile_n(first, m, tmp, tidx);

			if (successor(tidx.local[0]) < m) {
				dest_first[successor(tidx.local[0])] = op(first[successor(tidx.local[0])],
														  first[tidx.local[0]]);
			}

			return dest_first + m;
		}

		template<typename I1, typename I2, typename Op, int tsz>
		inline I2 _adjacent_difference_in_tile_n(I1 first, Difference_type<I1> n, I2 dest_first,
												 const concurrency::tiled_index<tsz>& tidx, Op op) restrict(amp)
		{
			Difference_type<I1> m = 0;
			while (positive(n)) {
				if (positive(m) && (tidx.tile_origin == tidx.global)) {
					*dest_first = op(*first, *(first - 1));
				}

				m = _min(tidx.tile_dim0 * 1, n);
				dest_first = _adjacent_difference_single_tile_n(first, m, dest_first, tidx, op);

				n -= m;
				first += m;
			}
			return dest_first;
		}

		template<typename I1, typename I2, typename Op>
		inline I2 _adjacent_difference(I1 first, I1 last, I2 dest_first, Op op)
		{	// TODO: not optimized.
			if (first == last) return dest_first;
			if (std::next(first) == last) return _copy(first, last, dest_first);

			const auto compute_domain = Execution_parameters::tiled_domain(last - first);
			const Difference_type<I1> work_per_tile = Execution_parameters::work_per_tile(last - first);

			concurrency::parallel_for_each(compute_domain, [=, op = std::move(op)](auto&& tidx) restrict(amp) {
				const Difference_type<I1> t = tidx.tile[0] * work_per_tile;
				const Difference_type<I1> n = _min(work_per_tile, last - first - t);

				uniform_invoke(forward<decltype(tidx)>(tidx), [=](auto&&) {
					dest_first[t] = positive(t) ? op(first[t], first[predecessor(t)]) : *first;
				});

				_adjacent_difference_in_tile_n(first + t,
											   n,
											   dest_first + t,
											   forward<decltype(tidx)>(tidx),
											   move(op));
			});
			return dest_first + (last - first);
		}

		template<typename I1, typename I2>
		inline I2 _adjacent_difference(I1 first, I1 last, I2 dest_first)
		{
			return _adjacent_difference(first, last, dest_first, amp_algorithms::minus<>());
		}

		//----------------------------------------------------------------------------
		// all_of, any_of, none_of
		//----------------------------------------------------------------------------

		template<typename I, typename P>
		inline bool _all_of(I first, I last, P p)
		{
			return !_any_of(first, last, [p = std::move(p)](auto&& x) restrict(amp) {
				return !p(AMP_ALG forward<decltype(x)>(x));
			});
		}

		template<typename I, typename P, int tsz>
		inline bool _any_of_in_tile_n(I first,
									  Difference_type<I> n,
									  const concurrency::tiled_index<tsz>& tidx,
									  P p) restrict(amp)
		{
			tile_exclusive_cache<unsigned int, tidx.tile_dim0> found(tidx, [](auto&& out) { out = 0u; });

			for (Difference_type<I> i = tidx.local[0]; (i < n) && zero(found.local()); i += tidx.tile_dim0) {
				if (p(first[i])) found.local() = 1u;
			}

			return one(found.local());
		}

		// Non-standard, OutputIterator must yield an int reference, where the result will be
		// stored. This allows the function to eschew synchronization
		template<typename I1, typename I2, typename P>
		inline void any_of_impl(I1 first, I1 last, I2 dest_first, P p)
		{	// TODO: evaluate removing this.
			concurrency::parallel_for_each(concurrency::extent<1>(1), [=](auto&&) restrict(amp) { *dest_first = 0; });

			const auto compute_domain = Execution_parameters::tiled_domain(last - first);
			const Difference_type<I1> work_per_tile = Execution_parameters::work_per_tile(last - first);

			concurrency::parallel_for_each(compute_domain, [=, p = std::move(p)](auto&& tidx) restrict(amp) {
				const AMP_ALG Difference_type<I1> t = tidx.tile[0] * work_per_tile;

				AMP_ALG tile_exclusive_cache<bool, tidx.tile_dim0> done(AMP_ALG forward<decltype(tidx)>(tidx), [=](auto&& out) {
					AMP_ALG forward<decltype(out)>(out) = AMP_ALG positive(dest_first[0]);
				});

				if (!done.local()) {
					const AMP_ALG Difference_type<I1> n = AMP_ALG _min(work_per_tile, last - first - t);
					const bool r = _any_of_in_tile_n(first + t, n, tidx, move(p));

					uniform_invoke(tidx, [=](auto&&) {
						if (r) {
							concurrency::atomic_fetch_inc(&dest_first[0]);
						}
					});
				}
			});
		}

		template<typename I, typename P>
		inline bool _any_of(I first, I last, P p)
		{
			if (first == last) return false;

			return _find_if(first, last, std::move(p)) != last;
		}

		template<typename I, typename P>
		inline bool _none_of(I first, I last, P p)
		{
			return !_any_of(first, last, std::move(p));
		}

		//----------------------------------------------------------------------------
		// copy, copy_if, copy_n
		//----------------------------------------------------------------------------

		template<typename I1, typename I2, int tsz>
		inline I2 _copy_in_tile_n(I1 first,
								  Difference_type<I1> n,
								  I2 dest_first,
								  const concurrency::tiled_index<tsz>& tidx) restrict(amp)
		{
			if (!positive(n)) return dest_first;

			for (Difference_type<I1> i = tidx.local[0]; i < n; i += tidx.tile_dim0) {
				dest_first[i] = first[i];
			}

			return dest_first + n;
		}

		template<typename I1, typename I2, int tsz>
		inline I2 _copy_in_tile(I1 first, I1 last, I2 dest_first,
								const concurrency::tiled_index<tsz>& tidx) restrict(amp)
		{
			return _copy_in_tile_n(first, last - first, dest_first, tidx);
		}

		template<typename I1, typename I2>
		inline I2 _copy(I1 first, I1 last, I2 dest_first)
		{
			if (first == last) return dest_first;

			concurrency::copy(_details::create_section(first, last - first),
							  _details::create_section(dest_first, last - first));
			return dest_first + (last - first);
		}

		template<typename I1, typename I2, typename P, int tsz>
		inline I2 _copy_if_single_tile_n(I1 first,
										 Difference_type<I1> n,
										 I2 dest_first,
										 const concurrency::tiled_index<tsz>& tidx,
										 P p) restrict(amp)
		{	// This assumes n <= tidx.tile_dim0.
			tile_exclusive_cache<atomic<Difference_type<I2>>, tsz> tile_off(tidx, [=](auto&& out) {
				forward<decltype(out)>(out) = Difference_type<I2>(0);
			});

			if (tidx.local[0] < n)  {
				const Value_type<I1> x = first[tidx.local[0]];
				if (p(x)) {
					dest_first[tile_off.local()++] = x;
				}
			}

			return dest_first + tile_off.local();
		}

		template<typename I1, typename I2, typename P, int tsz>
		inline I2 _copy_if_in_tile_n(I1 first,
									 Difference_type<I1> n,
									 I2 dest_first,
									 Reference<atomic<Difference_type<I2>>> dx,
									 const concurrency::tiled_index<tsz>& tidx,
									 P p) restrict(amp)
		{	// TODO: investigate if this should be recast so as to minimise updating dx at the cost
			// of one extra global read per copied element.
			tile_exclusive_cache<Value_type<I2>[tidx.tile_dim0], tsz> tmp(tidx, [](auto&&){});
			tile_exclusive_cache<Difference_type<I2>, tsz> tile_off(tidx, [](auto&& out) {
				forward<decltype(out)>(out) = Difference_type<I2>(0);
			});
			//tile_static Value_type<I2> tmp[tidx.tile_dim0];
			//tile_static Difference_type<I2> tile_off;

			for (Difference_type<I1> i = 0; i < n; i += tidx.tile_dim0) {
				const Difference_type<I1> m = _min(tidx.tile_dim0 * 1, n - i);
				const auto last_copied = _copy_if_single_tile_n(first + i, m, tmp.local(), tidx, move(p));

				if (last_copied != tmp.local()) {
					const auto cnt = last_copied - tmp.local();
					uniform_invoke(tidx, [=](auto&&) { tile_off.local() = dx.get().fetch_add(cnt); });

					_copy_in_tile(tmp.local(), last_copied, dest_first + tile_off.local(), tidx);

					uniform_invoke(tidx, [=](auto&&) { tile_off.local() += cnt; });
				}
			}

			return dest_first + dx.get();
		}

		template<typename I1, typename I2, typename P>
		inline I2 _copy_if(I1 first, I1 last, I2 dest_first, P p)
		{
			if (first == last) return dest_first;

			concurrency::array_view<atomic<Difference_type<I2>>> o(1);
			concurrency::parallel_for_each(o.extent, [=](auto&&) restrict(amp) { o[0] = AMP_ALG Difference_type<I2>(0); });

			const auto compute_domain = Execution_parameters::tiled_domain(last - first);
			const Difference_type<I1> work_per_tile = Execution_parameters::work_per_tile(last - first);

			concurrency::parallel_for_each(compute_domain, [=, p = std::move(p)](auto&& tidx) restrict(amp) {
				const AMP_ALG Difference_type<I1> t = tidx.tile[0] * work_per_tile;
				const AMP_ALG Difference_type<I1> n = AMP_ALG _min(work_per_tile, last - first - t);
				_copy_if_in_tile_n(first + t, n, dest_first, o[0], tidx, AMP_ALG move(p));
			});

			return dest_first + o[0];
		}

		template<typename I1, typename N, typename I2>
		inline I2 _copy_n(I1 first, N count, I2 dest_first)
		{
			// copy() will handle the case where count == 0.
			return _copy(first, first + count, dest_first);
		}

		//----------------------------------------------------------------------------
		// count, count_if
		//----------------------------------------------------------------------------

		template<typename I, typename T>
		inline Difference_type<I> _count(I first, I last, const T& value)
		{
			return _count_if(first, last, [=](auto&& x) restrict(amp) { return AMP_ALG forward<decltype(x)>(x) == value; });
		}

		template<typename I, typename P, int tsz>
		inline Difference_type<I> _count_if_in_tile_n(I first,
													  Difference_type<I> n,
													  const concurrency::tiled_index<tsz>& tidx,
													  P p) restrict(amp)
		{
			tile_exclusive_cache<Difference_type<I>[tidx.tile_dim0], tidx.tile_dim0> tmp(tidx, [=](auto&& out) {
				AMP_ALG _fill_in_tile_n(AMP_ALG forward<decltype(out)>(out), tidx.tile_dim0, AMP_ALG Difference_type<I>(0), tidx);
			});

			for (Difference_type<I> i = tidx.local[0]; i < n; i += tidx.tile_dim0) {
				tmp[tidx.local[0]] += Difference_type<I>(p(first[i]));
			}

			return tmp.reduce(amp_algorithms::plus<>());
		}

		template<typename I, typename P>
		inline Difference_type<I> _count_if(I first, I last, P p)
		{
			if (first == last) return Difference_type<I>(0);

			const auto tmp = Execution_parameters::temporary_buffer<Difference_type<I>>(last - first);

			const auto d = Execution_parameters::tiled_domain(last - first);
			const Difference_type<I> w = Execution_parameters::work_per_tile(last - first);

			concurrency::parallel_for_each(d, [=, p = std::move(p)](auto&& tidx) restrict(amp) {
				const AMP_ALG Difference_type<I> t = tidx.tile[0] * w;
				const AMP_ALG Difference_type<I> n = AMP_ALG _min(w, last - first - t);

				const AMP_ALG Difference_type<I> c = AMP_ALG _count_if_in_tile_n(first + t,
																				 n,
																				 AMP_ALG forward<decltype(tidx)>(tidx),
																				 AMP_ALG move(p));

				uniform_invoke(tidx, [=](auto&& tile) { tmp[tile] = c; });
			});

			return _reduce(cbegin(tmp), cend(tmp), Difference_type<I>(0), amp_algorithms::plus<>());
		}

		//----------------------------------------------------------------------------
		// equal, equal_range
		//----------------------------------------------------------------------------

		template<typename I1, typename I2, typename P, int tsz>
		inline bool _equal_in_tile_n(I1 first1,
									 Difference_type<I1> n,
									 I2 first2,
									 const concurrency::tiled_index<tsz>& tidx,
									 P p) restrict(amp)
		{
			if (!positive(n)) return true;

			tile_exclusive_cache<bool, tidx.tile_dim0> eq(tidx, [](auto&& out) { out = true; });
			for (Difference_type<I1> i = tidx.local[0]; (i < n) && eq.local(); i += tidx.tile_dim0) {
				if (!p(first1[i], first2[i])) {
					eq.local() = false;
					break;
				}
			}

			return eq.local();
		}

		template<typename I1, typename I2, typename P>
		inline bool _equal(I1 first1, I1 last1, I2 first2, P p)
		{
			if (first1 == last1) return true;

			const concurrency::array_view<unsigned int> neq(1);
			concurrency::parallel_for_each(neq.extent, [=](auto&&) restrict(amp) { neq[0] = 0u; });

			const auto d = Execution_parameters::tiled_domain(last1 - first1);
			const Difference_type<I1> w = Execution_parameters::work_per_tile(last1 - first1);

			concurrency::parallel_for_each(d, [=, p = std::move(p)](auto&& tidx) restrict(amp) {
				if (AMP_ALG positive(neq[0])) return;

				const AMP_ALG Difference_type<I1> t = tidx.tile[0] * w;
				const AMP_ALG Difference_type<I1> n = AMP_ALG _min(w, last1 - first1 - t);

				const bool eq = _equal_in_tile_n(first1 + t,
												 n,
												 first2 + t,
												 AMP_ALG forward<decltype(tidx)>(tidx),
												 AMP_ALG move(p));
				if (!eq) uniform_invoke(AMP_ALG forward<decltype(tidx)>(tidx), [=](auto&&) { neq[0] = 1u; });
			});
			return zero(neq[0]);
		}

		template<typename I1, typename I2>
		inline bool _equal(I1 first1, I1 last1, I2 first2)
		{
			return _equal(first1, last1, first2, amp_algorithms::equal_to<>());
		}

		template<typename I, typename T, typename C>
		inline std::pair<I, I> _equal_range(I first, I last, const T& value, C cmp)
		{	// TODO: optimise
			if (first == last) return std::make_pair(last, last);

			const I l = _lower_bound(first, last, value, cmp);
			const I u = _upper_bound(l, last, value, std::move(cmp));

 			return std::make_pair(l, u);
   		}

		template<typename I, typename T>
		inline std::pair<I, I> _equal_range(I first, I last, const T& value)
		{
			return _equal_range(first, last, value, amp_algorithms::less<>());
		}

		//----------------------------------------------------------------------------
		// fill, fill_n
		//----------------------------------------------------------------------------

		template<typename I, typename T, int tsz>
		inline void _fill_in_tile(I first, I last, const T& value,
									  const concurrency::tiled_index<tsz>& tidx) restrict(amp)
		{
			_fill_in_tile_n(first, last - first, value, tidx);
		}

		template<typename I, typename T>
		inline void _fill(I first, I last, const T& value)
		{
			_generate(first, last, [=]() restrict(amp) { return value; });
		}

		template<typename I, typename T, int tsz>
		inline void _fill_in_tile_n(I first, Difference_type<I> n, const T& value,
									const concurrency::tiled_index<tsz>& tidx) restrict(amp)
		{
			for (Difference_type<I> i = tidx.local[0]; i < n; i += tidx.tile_dim0) {
				first[i] = value;
			}
		}

		template<typename I, typename N, typename T>
		inline I _fill_n(I first, N n, const T& value)
		{
			return _generate_n(first, n, [=]() restrict(amp) { return value; });
		}

		//----------------------------------------------------------------------------
		// find, find_if, find_if_not, find_end, find_first_of, adjacent_find
		//----------------------------------------------------------------------------

		template<typename I, typename P, int tsz>
		inline I _find_if_in_tile_n(I first,
									Difference_type<I> n,
									const concurrency::tiled_index<tsz>& tidx,
									P p) restrict(amp)
		{
			if (!positive(n)) return first;

			tile_exclusive_cache<Difference_type<I>, tidx.tile_dim0> dx(tidx, [=](auto&& out) { out = n; });
			for (Difference_type<I> i = tidx.local[0]; i < dx.local(); i += tidx.tile_dim0) {
				if (p(first[i])) concurrency::atomic_fetch_min(&dx.local(), i);
			}

			return first + dx.local();
		}

		template<typename I, typename P>
		inline I _find_if(I first, I last, P p)
		{
			if (first == last) return last;

			const auto tmp = Execution_parameters::temporary_buffer(last - first, last - first);

			const auto d = Execution_parameters::tiled_domain(last - first);
			const Difference_type<I> w = Execution_parameters::work_per_tile(last - first);

			concurrency::parallel_for_each(d, [=, p = std::move(p)](auto&& tidx) restrict(amp) {
				const AMP_ALG Difference_type<I> t = tidx.tile[0] * w;
				const AMP_ALG Difference_type<I> n = AMP_ALG _min(w, last - first - t);

				const I f = _find_if_in_tile_n(first + t, n, tidx, AMP_ALG move(p));

				if (f != (first + t + n)) {
					uniform_invoke(tidx, [=](auto&& tile) { tmp[tile] = f - first; });
				}
			});

			return first + _reduce(cbegin(tmp), cend(tmp), last - first, [](auto&& x, auto&& y) restrict(amp) {
				return AMP_ALG _min(AMP_ALG forward<decltype(x)>(x), AMP_ALG forward<decltype(y)>(y));
			});
		}

		template<typename I, typename P, int tsz>
		inline I _find_if_not_in_tile_n(I first,
										Difference_type<I> n,
										const concurrency::tiled_index<tsz>& tidx,
										P p) restrict(amp)
		{
			return _find_if_in_tile_n(first, n, tidx, [=](auto&& x) {
				return !p(AMP_ALG forward<decltype(x)>(x));
			});
		}

		template<typename I, typename P>
		inline I _find_if_not(I first, I last, P p)
		{
			return _find_if(first, last, [p = std::move(p)](auto&& x) restrict(amp) {
				return !p(forward<decltype(x)>(x));
			});
		}

		template<typename I, typename T>
		inline I _find(I first, I last, const T& value)
		{
			return _find_if(first, last, [=](auto&& x) restrict(amp) {
				return AMP_ALG forward<decltype(x)>(x) == value;
			});
		}

		template<typename I, typename P, int tsz>
		inline I _adjacent_find_in_tile_n(I first,
										  Difference_type<I> n,
										  const concurrency::tiled_index<tsz>& tidx,
										  P p) restrict(amp)
		{
			tile_static atomic<Difference_type<I>> r;
			if (tidx.tile_origin == tidx.global) {
				r = n;
			}

			Difference_type<I> f = tidx.local[0];
			while (successor(f) < n) {
				if (p(*(first + f), *(first + successor(f)))) break;
				f += tidx.tile_dim0;
			}
			r.fetch_min(f);

			return first + r;
		}

		template<typename I, typename P>
		inline I _adjacent_find(I first, I last, P p)
		{	// TODO: revisit this after implementing zip iterators, might clean it up a bit.
			if (first == last) return last;

			concurrency::array_view<atomic<Difference_type<I>>> r(1);
			concurrency::parallel_for_each(r.extent, [=](auto&&) restrict(amp) { r[0] = last - first; });

			const auto compute_domain = Execution_parameters::tiled_domain(last - first);
			const Difference_type<I> work_per_tile = Execution_parameters::work_per_tile(last - first);

			concurrency::parallel_for_each(compute_domain, [=, p = std::move(p)](auto&& tidx) restrict(amp) {
				const Difference_type<I> t = tidx.tile[0] * work_per_tile;

				if (r[0] < t) return;

				const Difference_type<I> n = _min(work_per_tile, last - first - successor(t));
				const I f = _adjacent_find_in_tile_n(first + t, successor(n), tidx, move(p));

				if (f != (first + t + n)) {
					if (tidx.tile_origin == tidx.global) {
						r[0].fetch_min(f - first);
					}
				}
			});

			return first + r[0];
		}

		template<typename I>
		inline I _adjacent_find(I first, I last)
		{
			return _adjacent_find(first, last, amp_algorithms::equal_to<>());
		}

		//----------------------------------------------------------------------------
		// for_each, for_each_no_return
		//----------------------------------------------------------------------------

		template<typename I, typename F>
		inline void _for_each_no_return(I first, I last, F f)
		{
			if (first == last) return;

			concurrency::parallel_for_each(concurrency::extent<1>(last - first),
										   [=, f = std::move(f)](auto&& idx) restrict(amp) {
				f(first[idx[0]]);
			});
		}

		// F CANNOT contain any array, array_view or textures. Needs to be blittable.
		template<typename I, typename F>
		inline F _for_each(I first, I last, F f)
		{
			if (first == last) return f;

			concurrency::array_view<F> f_av(1, &f);
			_for_each_no_return(first, last, [=](auto&& x) restrict(amp) {
				functor_av[0](forward<decltype(x)>(x));
			});

			return f;
		}

		//----------------------------------------------------------------------------
		// generate, generate_n
		//
		// The "Generator" functor needs to be callable as "g()" and must return a type
		// that is assignable to RandomAccessIterator::value_type.  The functor needs
		// to be blittable and cannot contain any array, array_view, or textures.
		//----------------------------------------------------------------------------

		template<typename I, typename G, int tsz>
		inline I _generate_in_tile_n(I first, Difference_type<I> n, const concurrency::tiled_index<tsz>& tidx, G g) restrict(amp)
		{
			if (!positive(n)) return first;

			for (Difference_type<I> i = tidx.local[0]; i < n; i += tidx.tile_dim0) {
				first[i] = g();
			}

			return first + n;
		}

		template<typename I, typename N, typename G>
		inline I _generate_n(I first, N n, G g)
		{
			if (!positive(n)) return first;

			concurrency::parallel_for_each(concurrency::extent<1>(n),
										   [=, g = std::move(g)](auto&& idx) restrict(amp) {
				first[idx[0]] = g();
			});

			return first + n;
		}

		template<typename I, typename G, int tsz>
		inline void _generate_in_tile(I first,
									  I last,
									  const concurrency::tiled_index<tsz>& tidx,
									  G g) restrict(amp)
		{
			_generate_in_tile_n(first, last - first, tidx, move(g));
		}

		template<typename I, typename G>
		inline void _generate(I first, I last, G g)
		{
			if (first == last) return;

			_generate_n(first, last - first, std::move(g));
		}

		//----------------------------------------------------------------------------
		// includes
		//----------------------------------------------------------------------------

		template<typename I1, typename I2, typename C>
		inline bool _includes(I1 first1, I1 last1, I2 first2, I2 last2, C cmp)
		{	// TODO: not efficient, robust or clean yet. Properify!
			// The compute_domain should account for the size of the ranges (always search the haystack for the needle!)
			if (first1 == last1) return false;
			if (first2 == last2) return true;

			const auto l = _lower_bound(first1, last1, first2, [](auto&& x, auto&& y) restrict(amp) { return x < *y; });
			const auto u = _upper_bound(first1, last1, last2 - 1, [](auto&& x, auto&& y) restrict(amp) { return *x < y; });
			if ((u - l) < (last2 - first2)) return false; // Pigeonhole principle.

			concurrency::array_view<unsigned int> neq(1);
			concurrency::parallel_for_each(neq.extent, [=](auto&&) restrict(amp) { neq[0] = 0u; });

			const auto compute_domain = Execution_parameters::tiled_domain(last2 - first2);
			concurrency::parallel_for_each(compute_domain, [=, cmp = std::move(cmp)](auto&& tidx) restrict(amp) {
				tile_static bool early_out;
				for (Difference_type<I2> i = tidx.tile_origin[0]; i < (last2 - first2); i += compute_domain.size()) {
					const auto n = _min(tidx.tile_dim0 * 1, last2 - first2 - i);
					if (tidx.tile_origin == tidx.global) {
						early_out = ((u - l - i) < n);
						if (early_out) {
							concurrency::atomic_fetch_inc(&neq[0]);
						}
						early_out = early_out || positive(neq[0]);
					}

					// tidx.barrier.wait_with_tile_static_memory_fence();

					tile_static Difference_type<I1> offs[tidx.tile_dim0];
					offs[tidx.local[0]] = i + tidx.local[0];

					tile_static Value_type<I2> vals[tidx.tile_dim0];
					_copy_in_tile_n(first2 + i, n, vals, tidx);

					if (tidx.local[0] < n) {
						while ((offs[tidx.local[0]] < (u - l)) && cmp(*(l + offs[tidx.local[0]]), vals[tidx.local[0]])) {
							++offs[tidx.local[0]];
						}
					}

					// tidx.barrier.wait_with_tile_static_memory_fence();

					tile_static unsigned int not_done;
					do {
						// tidx.barrier.wait_with_tile_static_memory_fence();

						if (tidx.tile_origin == tidx.global) {
							not_done = 0u;
						}

						// tidx.barrier.wait_with_tile_static_memory_fence();

						if (odd(tidx.local[0])) {
							if (offs[tidx.local[0] - 1] == offs[tidx.local[0]]) {
								++offs[tidx.local[0]];
								concurrency::atomic_fetch_inc(&not_done);
							}
						}

						// tidx.barrier.wait_with_tile_static_memory_fence();

						if (positive(tidx.local[0]) && even(tidx.local[0])) {
							if (offs[tidx.local[0] - 1] == offs[tidx.local[0]]) {
								++offs[tidx.local[0]];
								concurrency::atomic_fetch_inc(&not_done);
							}
						}

						// tidx.barrier.wait_with_tile_static_memory_fence();
					} while (positive(not_done));

					tile_static unsigned int ne;
					if (tidx.tile_origin == tidx.global) {
						ne = 0u;
					}

					// tidx.barrier.wait_with_tile_static_memory_fence();

					if (tidx.local[0] < n) {
						for (auto j = offs[tidx.local[0]];; ++j) {
							if (((u - l) < j) || cmp(vals[tidx.local[0]], *(l + j))) {
								concurrency::atomic_fetch_inc(&ne);
								break;
							}
							if (!cmp(*(l + j), vals[tidx.local[0]])) {
								break;
							}
						}
					}
					// tidx.barrier.wait_with_tile_static_memory_fence();

					if (positive(ne)) {
						if ((tidx.tile_origin == tidx.global)) {
							concurrency::atomic_fetch_inc(&neq[0]);
						}
						return;
					}
				}
			});

			return zero(neq[0]);
		}

		template<typename I1, typename I2>
		inline bool _includes(I1 first1, I1 last1, I2 first2, I2 last2)
		{
			return _includes(first1, last1, first2, last2, amp_algorithms::less<>());
		}
		//----------------------------------------------------------------------------
		// inner_product
		//----------------------------------------------------------------------------

		template<typename I1, typename I2, typename Binary_op1, typename Binary_op2, int tsz>
		inline decltype(auto) _inner_product_in_tile_n(I1 first1,
													   Difference_type<I1> n,
													   I2 first2,
													   const concurrency::tiled_index<tsz>& tidx,
													   Binary_op1 op1,
													   Binary_op2 op2) restrict(amp)
		{	// NOTA BENE: this is a non-standard interface, as it does not include an initial value.
			using T1 = Codomain<Binary_op2, Value_type<I1>, Value_type<I2>>;
			using T2 = Codomain<Binary_op2, Codomain<Binary_op2, T1, T1>, T1>;

			const Difference_type<I1> w = rounded_up_quotient(n, tidx.tile_dim0);
			Difference_type<I1> l = tidx.local[0] * w;

			tile_static T2 tmp[tidx.tile_dim0];
			if (l < n) {
				const Difference_type<I1> m = _min(l + w, n);

				tmp[tidx.local[0]] = op2(first1[l], first2[l]);
				while (++l != m) {
					tmp[tidx.local[0]] = op1(tmp[tidx.local[0]], op2(first1[l], first2[l]));
				}
			}

			return _reduce_single_tile_n(tmp, rounded_up_quotient(n, w), tidx, move(op1));
		}

		template<typename I1, typename I2, typename T, typename Binary_op1, typename Binary_op2>
		inline T _inner_product(I1 first1, I1 last1, I2 first2, T value, Binary_op1 op1, Binary_op2 op2)
		{	// TODO: this should be using work-stealing / dynamic(random) pairing.
			//     : should do reduction across tiles, once it's ironed out.
			//     : should, can and will be shortened.
			if (first1 == last1) return value;

			concurrency::array_view<T> r(1);

			using T_D = pair<T, atomic<Difference_type<I1>>>;
			const auto tmp = Execution_parameters::temporary_buffer<T_D>(last1 - first1);
			_fill_n(begin(tmp), tmp.extent.size(), make_pair(value, last1 - first1));

			const auto compute_domain = Execution_parameters::tiled_domain(last1 - first1);
			const Difference_type<I1> work_per_tile = Execution_parameters::work_per_tile(last1 -
																						  first1);

			concurrency::parallel_for_each(compute_domain,
										   [=,
				                            op1 = std::move(op1),
											op2 = std::move(op2)](auto&& tidx) restrict(amp) {
				const Difference_type<I1> t = tidx.tile[0] * work_per_tile;
				const Difference_type<I1> n = _min(work_per_tile, last1 - first1 - t);

				T x = _inner_product_in_tile_n(first1 + t, n, first2 + t, tidx, op1, move(op2));

				if (tidx.tile_origin == tidx.global) {
					if (positive(tidx.tile[0])) {
						while (tmp[predecessor(tidx.tile[0])].second == (last1 - first1));
					}

					const T x_pre = positive(tidx.tile[0]) ? tmp[predecessor(tidx.tile[0])].first
															   : value;
					x = op1(x_pre, x);
					if ((t + n) != (last1 - first1)) {
						tmp[tidx.tile] = make_pair(x, tidx.tile[0]);
					}
					else {
						r[0] = x;
					}
				}
			});

			return r[0];
		}

		template<typename I1, typename I2, typename T>
		inline T _inner_product(I1 first1, I1 last1, I2 first2, T value)
		{
			return _inner_product(first1,
								  last1,
								  first2,
								  std::move(value),
								  amp_algorithms::plus<>(),
								  amp_algorithms::multiplies<>());
		}

		//----------------------------------------------------------------------------
		// iota
		//----------------------------------------------------------------------------

		template<typename T> using has_plus = has<std::plus<>(T, T)>;
		template<typename T> using has_minus = has<std::minus<>(T, T)>;
		template<typename T> using has_multiplies = has<std::multiplies<>(T, T)>;

		template<typename T> using fast_iota = std::enable_if_t<has_plus<T>::value &&
																has_minus<T>::value &&
																has_multiplies<T>::value>;
		template<typename T> using slow_iota = std::enable_if_t<!has_minus<T>::value ||
																!has_multiplies<T>::value>;

		template<typename I, typename T, fast_iota<T>* = nullptr>
		inline void iota_impl(I first, I last, T value)
		{
			T dx = value;
			dx = ++dx - value;

			concurrency::parallel_for_each(concurrency::extent<1>(last - first),
										   [=,
				                            dx = std::move(dx),
										    value = std::move(value)](auto&& idx) restrict(amp) {
				// This is numerically equivalent with incrementing only if T multiplication is exact.
				first[idx[0]] = value + T(idx[0]) * dx;
			});
		}

		template<typename I, typename T, slow_iota<T>* = nullptr>
		inline void iota_impl(I first, I last, T value)
		{
			concurrency::parallel_for_each(concurrency::extent<1>(last - first),
										   [=,
											value = std::move(value)](auto&& idx) restrict(amp) {
				first[idx[0]] = value;

				Difference_type<I> i = idx[0];
				while (i--) ++first[idx[0]];
			});
		}

		template<typename I, typename T>
		inline void _iota(I first, I last, T value)
		{
			// Using SFINAE based dispatch should work here: if T has multiplication and subtraction
			// the fast version can be used, if not we provide a slow but standards conforming one.
			// There is an intermediate solution, using scan, which only requires T to have addition
			if (first == last) return;
			iota_impl(first, last, std::move(value));
		}

		//----------------------------------------------------------------------------
		// lexographical_compare
		//----------------------------------------------------------------------------

		template<typename I1, typename I2, typename C, int tsz>
		inline bool _lexicographical_compare_in_tile_n(I1 first1,
													   Difference_type<I1> n1,
													   I2 first2,
													   Difference_type<I2> n2,
													   const concurrency::tiled_index<tsz>& tidx,
													   C cmp) restrict(amp)
		{
			if (!positive(n2)) return false;
			else if (!positive(n1)) return true;

			tile_static pair<atomic<Difference_type<I1>>, atomic<Difference_type<I1>>> tlg;
			if (tidx.tile_origin == tidx.global) {
				tlg = make_pair(n1, n1);
			}

			for (Difference_type<I1> i = tidx.local[0]; i < n1 && i < n2; i += tidx.tile_dim0) {
				if (cmp(first2[i], first1[i])) {
					tlg.second.fetch_min(i);
					break;
				}

				if (cmp(first1[i], first2[i])) {
					tlg.first.fetch_min(i);
					break;
				}
			}

			if (tlg.first < tlg.second) return true;
			if (tlg.second < tlg.first) return false;
			return n1 < n2;
		}

		template<typename I1, typename I2, typename C>
		inline bool _lexicographical_compare(I1 first1, I1 last1, I2 first2, I2 last2, C cmp)
		{
			if (first2 == last2) return false;
			else if (first1 == last1) return true;

			const concurrency::array_view<pair<atomic<Difference_type<I1>>,
											   atomic<Difference_type<I2>>>> lg(1);
			concurrency::parallel_for_each(lg.extent, [=](auto&&) restrict(amp) {
				lg[0] = make_pair(last1 - first1, last1 - first1);
			});

			const auto compute_domain = Execution_parameters::tiled_domain(last1 - first1);
			const Difference_type<I1> work_per_tile = Execution_parameters::work_per_tile(last1 -
																						  first1);

			concurrency::parallel_for_each(compute_domain,
										   [=, cmp = std::move(cmp)](auto&& tidx) restrict(amp) {
				const Difference_type<I1> t = tidx.tile[0] * work_per_tile;
				if ((t < (last1 - first1)) && (t < (last2 - first2))) {
					tile_static bool early_out; early_out = false; // Temporary workaround.
					if (tidx.tile_origin == tidx.global) {
						early_out = (lg[0].first < t) || (lg[0].second < t);
					}

					if (!early_out) {
						const Difference_type<I1> n1 = _min(work_per_tile, last1 - first1 - t);
						const Difference_type<I2> n2 = _min(work_per_tile, last2 - first2 - t);

						const auto is_less = _lexicographical_compare_in_tile_n(first1 + t,
																				n1,
																				first2 + t,
																				n2,
																				forward<decltype(tidx)>(tidx),
																				move(cmp));

						if (tidx.tile_origin == tidx.global) {
							if (is_less) lg[0].first.fetch_min(t);
							else lg[0].second.fetch_min(t);
						}
					}
				}
			});

			if (lg[0].first < lg[0].second) return true;
			if (lg[0].second < lg[0].first) return false;
			return (last1 - first1) < (last2 - first2);
		}

		template<typename I1, typename I2>
		inline bool _lexicographical_compare(I1 first1, I1 last1, I2 first2, I2 last2)
		{
			return _lexicographical_compare(first1, last1, first2, last2, amp_algorithms::less<>());
		}

		//----------------------------------------------------------------------------
		// lower_bound, upper_bound
		//----------------------------------------------------------------------------

		template<typename I, typename T, typename C>
		inline I _lower_bound(I first, I last, const T& value, C cmp)
		{
			return _partition_point(first, last, [=, cmp = std::move(cmp)](auto&& x) restrict(cpu, amp) {
				return cmp(AMP_ALG forward<decltype(x)>(x), value);
			});
		}

		template<typename I, typename T>
		inline I _lower_bound(I first, I last, const T& value)
		{
			return _lower_bound(first, last, value, amp_algorithms::less<>());
		}

		template<typename I, typename T, typename C>
		inline I _upper_bound(I first, I last, const T& value, C cmp)
		{
			return _partition_point(first, last, [=, cmp = std::move(cmp)](auto&& x) restrict(cpu, amp) {
				return !cmp(value, AMP_ALG forward<decltype(x)>(x));
			});
		}

		template<typename I, typename T>
		inline I _upper_bound(I first, I last, const T& value)
		{
			return _upper_bound(first, last, value, amp_algorithms::less<>());
		}

		//----------------------------------------------------------------------------
		// merge, inplace_merge
		//----------------------------------------------------------------------------

		template<typename I1, typename I2, typename N, typename C>
		inline pair<Difference_type<I1>, Difference_type<I2>> _co_rank_n(I1 first1,
																		 Difference_type<I1> n1,
															   			 I2 first2,
																		 Difference_type<I2> n2,
																		 N idx,
																		 C cmp) restrict(amp)
		{	// TODO: clean-up && flatten a bit.
			if (zero(idx)) return make_pair(0, 0);

			Difference_type<I1> i1 = _min(idx, n1);
			Difference_type<I2> i2 = idx - i1;
			Difference_type<I1> i1_l = _max(0, idx - n2);
			Difference_type<I2> i2_l = 0;

			while (true) {
				if (positive(i1) && (i2 < n2) && cmp(first2[i2], first1[predecessor(i1)])) {
					const Difference_type<I1> h = rounded_up_quotient(i1 - i1_l,
																	  Difference_type<I1>(2));
					i2_l = i2;
					i1 -= h;
					i2 += h;
				}
				else if (positive(i2) && (i1 < n1) && !cmp(first2[predecessor(i2)], first1[i1])) {
					const Difference_type<I2> h = rounded_up_quotient(i2 - i2_l,
																	  Difference_type<I2>(2));
					i1_l = i1;
					i1 += h;
					i2 -= h;
				}
				else {
					return make_pair(i1, i2);
				}
			}
		}

		template<typename I1, typename I2, typename I3, typename C, int tsz>
		inline I3 _compute_co_ranks_in_tile_n(I1 first1,
											  Difference_type<I1> n1,
											  I2 first2,
											  Difference_type<I2> n2,
											  I3 dest_first,
											  const concurrency::tiled_index<tsz>& tidx,
											  C cmp) restrict(amp)
		{
			const auto m = _min(tidx.tile_dim0 * 1,
								successor(rounded_up_quotient(n1 + n2, twice(tidx.tile_dim0))));

			if (tidx.local[0] < m) {
				const auto l = _min(tidx.local[0] * twice(tidx.tile_dim0), n1 + n2);
				dest_first[tidx.local[0]] = _co_rank_n(first1, n1, first2, n2, l, move(cmp));
			}

			return dest_first + m;
		}

		template<typename I1, typename I2, typename I3, typename C, int tsz>
		inline I3 _make_bitonic_range_n(I1 first1,
										Difference_type<I1> n1,
									    I2 first2,
										Difference_type<I2> n2,
									    I3 dest_first,
										Difference_type<I3> n3,
									    const concurrency::tiled_index<tsz>& tidx,
										C cmp) restrict(amp)
		{
			dest_first = _reverse_copy_in_tile_n(first1, n1, dest_first, tidx);
			dest_first = _copy_in_tile_n(first2, n2, dest_first, tidx);

			return dest_first + n3;
		}

		template<typename I, typename C, int tsz>
		inline I _bitonic_merge_single_tile_n(I first,
											  Difference_type<I> m,
											  Difference_type<I> n,
											  const concurrency::tiled_index<tsz>& tidx,
											  C cmp,
											  std::false_type) restrict(amp)
		{	// General case, ensure stability via additional processing.
			tile_static unsigned int id[twice(tidx.tile_dim0)];
			_fill_in_tile_n(id, m, 0u, tidx);
			_fill_in_tile_n(id + m, n - m, 1u, tidx);

			const concurrency::array_view<Value_type<I>> d(n, &*first);
			const concurrency::array_view<unsigned int> i(n, id);

			static_for<tidx.tile_dim0, 0, Inc::div, 2>()([=](auto&& h) {
				// If tidx.tile_dim0 is not even (i.e. h is not even) results are undefined.
				const Difference_type<I> x = even_division_remainder(tidx.local[0], h) +
											 (tidx.local[0] >> binary_logarithm(h)) * twice(h);
				const Difference_type<I> y = x + h;

				if (y < n) {
					if (cmp(d[y], d[x])) {
						_swap(d[y], d[x]);
						_swap(i[y], i[x]);
					}
					else if (!cmp(d[x], d[y])) {
						_compare_and_exchange(i[x], i[y], amp_algorithms::less<>());
					}
				}
			});

			return first + n;
		}

		template<typename I, typename C, int tsz>
		inline I _bitonic_merge_single_tile_n(I first,
											  Difference_type<I>,
											  Difference_type<I> n,
									    	  const concurrency::tiled_index<tsz>& tidx,
											  C cmp,
											  std::true_type) restrict(amp)
		{	// Special case, stability is ensured by default.
			const concurrency::array_view<Value_type<I>> d(n, &*first);

			static_for<tidx.tile_dim0, 0, Inc::div, 2>()([=](auto&& h) {
				// If tidx.tile_dim0 is not even (i.e. h is not even) results are undefined.
				AMP_ALG _bitonic_compare_exchange(AMP_ALG begin(d), n, AMP_ALG forward<decltype(h)>(h), tidx.local[0], cmp);
			});

			return first + n;
		}

		template<typename I1, typename I2, typename I3, typename C, int tsz>
		inline I3 _merge_single_tile_n(I1 first1,
									   Difference_type<I1> n1,
									   I2 first2,
									   Difference_type<I2> n2,
									   I3 dest_first,
									   const concurrency::tiled_index<tsz>& tidx,
									   C cmp) restrict(amp)
		{
			if (!positive(n1)) return _copy_in_tile_n(first2, n2, dest_first, tidx);
			if (!positive(n2)) return _copy_in_tile_n(first1, n1, dest_first, tidx);

			tile_static Value_type<I3> tmp[twice(tidx.tile_dim0)];

			_make_bitonic_range_n(first1, n1, first2, n2, tmp, twice(tidx.tile_dim0), tidx, cmp);

			_bitonic_merge_single_tile_n(tmp, n1, n1 + n2, tidx, move(cmp), std::is_scalar<Value_type<I3>>());

			return _copy_in_tile_n(tmp, n1 + n2, dest_first, tidx);
		}

		template<typename I1, typename I2, typename I3, typename C, int tsz>
		inline I3 _merge_in_tile_n(I1 first1,
								   Difference_type<I1> n1,
								   I2 first2,
								   Difference_type<I2> n2,
							       I3 dest_first,
								   const concurrency::tiled_index<tsz>& tidx,
								   C cmp) restrict(amp)
		{
			while (positive(n1) && positive(n2)) {
				tile_static pair<Difference_type<I1>, Difference_type<I2>> cr[tidx.tile_dim0];
				const auto l_cr = _compute_co_ranks_in_tile_n(first1, n1, first2, n2, cr, tidx, cmp);

				for (auto i = 0; i != predecessor(l_cr - cr); ++i) {
					first1 += cr[i].first;
					first2 += cr[i].second;

					dest_first = _merge_single_tile_n(first1,
													  cr[successor(i)].first - cr[i].first,
													  first2,
													  cr[successor(i)].second - cr[i].second,
													  dest_first,
													  tidx,
													  cmp);
				}

				n1 -= cr[predecessor(l_cr - cr)].first;
				n2 -= cr[predecessor(l_cr - cr)].second;
			}

			dest_first = _copy_in_tile_n(first1, n1, dest_first, tidx);
			dest_first = _copy_in_tile_n(first2, n2, dest_first, tidx);

			return dest_first;
		}

		template<typename I1, typename I2, typename I3, typename C>
		inline I3 _merge(I1 first1, I1 last1, I2 first2, I2 last2, I3 dest_first, C cmp)
		{
			if (first1 == last1) return _copy(first2, last2, dest_first);
			if (first2 == last2) return _copy(first1, last1, dest_first);

			const Difference_type<I3> sz = (last1 - first1) + (last2 - first2);

			const auto compute_domain = Execution_parameters::tiled_domain(sz);
			const Difference_type<I3> work_per_tile = Execution_parameters::work_per_tile(sz);

			concurrency::parallel_for_each(compute_domain,
										   [=, cmp = std::move(cmp)](auto&& tidx) restrict(amp) {
				const AMP_ALG Difference_type<I3> t = tidx.tile[0] * work_per_tile;
				if (t < sz) {
					const AMP_ALG Difference_type<I3> n = AMP_ALG _min(work_per_tile, sz - t);

					using Co_rank = AMP_ALG pair<AMP_ALG Difference_type<I1>, AMP_ALG Difference_type<I2>>;
					AMP_ALG tile_exclusive_cache<Co_rank[2], tidx.tile_dim0> cr(tidx, [](auto&&){});
					AMP_ALG uniform_invoke(tidx, [=, cr = cr](auto&&) {
						cr[0] = _co_rank_n(first1, last1 - first1, first2, last2 - first2, t, cmp);
						cr[1] = _co_rank_n(first1, last1 - first1, first2, last2 - first2, t + n, cmp);
					});
			/*		tile_static pair<Difference_type<I1>, Difference_type<I2>> f1_f2;
					tile_static pair<Difference_type<I1>, Difference_type<I2>> l1_l2;

					if (tidx.tile_origin == tidx.global) {
						f1_f2 = _co_rank_n(first1, last1 - first1, first2, last2 - first2, t, cmp);
						l1_l2 = _co_rank_n(first1, last1 - first1, first2, last2 - first2, t + n, cmp);
					}*/

					_merge_in_tile_n(first1 + cr[0].first,
									 cr[1].first - cr[0].first,
									 first2 + cr[0].second,
									 cr[1].second - cr[0].second,
									 dest_first + t,
									 AMP_ALG forward<decltype(tidx)>(tidx),
									 AMP_ALG move(cmp));
				}
			});

			return dest_first + sz;
		}

		template<typename I1, typename I2, typename I3>
		inline I3 _merge(I1 first1, I1 last1, I2 first2, I2 last2, I3 dest_first)
		{
			return _merge(first1, last1, first2, last2, dest_first, amp_algorithms::less<>());
		}

		template<typename I, typename C>
		inline void _bitonic_compare_exchange(I first,
											  Difference_type<I> n,
											  Difference_type<I> h,
											  Difference_type<I> idx,
											  C cmp) restrict(cpu, amp)
		{
			const Difference_type<I> ix = even_division_remainder(idx, h) +
										  (idx >> binary_logarithm(h)) * twice(h);
			const Difference_type<I> iy = ix + h;

			if (iy < n) _compare_and_exchange(first[ix], first[iy], move(cmp));
		}

		template<typename I, typename C, int tsz>
		inline void _bitonic_merge_in_tile_n(I first,
											 Difference_type<I> n,
											 const concurrency::tiled_index<tsz>& tidx,
											 C cmp) restrict(amp)
		{
			const Difference_type<I> n2 = round_up_to_next_binary_power(n);
			for (Difference_type<I> h = half_nonnegative(n2); positive(h); h = half_nonnegative(h)) {
				for (Difference_type<I> i = tidx.local[0]; i < half_nonnegative(n2); i += tidx.tile_dim0) {
					_bitonic_compare_exchange(first, n, h, i, cmp);
				}
			}
		}

		template<typename I, typename C>
		inline void _bitonic_merge_n(I first, Difference_type<I> m, Difference_type<I> n, C cmp)
		{	// TODO: cleanup and properify.
			if (!positive(m) || !positive(n)) return;

			_reverse(first, first + m);

			const concurrency::extent<1> d(half_nonnegative(round_up_to_next_binary_power(n)));
			const Difference_type<I> w = Execution_parameters::work_per_tile(n);

			for (Difference_type<I> h = d.size(); w <= h; h = half_nonnegative(h)) {
				concurrency::parallel_for_each(d, [=](auto&& idx) restrict(amp) {
					_bitonic_compare_exchange(first, n, h, idx[0], cmp);
				});
			}

			const auto compute_domain = Execution_parameters::tiled_domain(n);
			concurrency::parallel_for_each(compute_domain,
										   [=, cmp = std::move(cmp)](auto&& tidx) restrict(amp) {
				const Difference_type<I> t = tidx.tile[0] * w;
				if (t < n) {
					const Difference_type<I> m = _min(w, n - t);
					_bitonic_merge_in_tile_n(first + t, m, forward<decltype(tidx)>(tidx), move(cmp));
				}
			});
		}

		template<typename I, typename C>
		inline void _inplace_merge(I first, I middle, I last, C cmp)
		{	// TODO: this assumes that we can always get a buffer. Adding the O(NlogN) unbuffered
			// variant is straightforward, but is left for a future version. Furthermore this is
			// not at all optimised yet, and requires tuning.
			if ((first == last) || (middle == last)) return;

			const Difference_type<I> buf_sz = rounded_up_quotient(last - first, 2);
			const concurrency::array_view<Value_type<I>> tmp(buf_sz);

			const concurrency::array_view<pair<Difference_type<I>, Difference_type<I>>> cr(1);
			concurrency::parallel_for_each(cr.extent, [=](auto&&) restrict(amp) {
				cr[0] = _co_rank_n(first, middle - first, middle, last - middle, buf_sz, cmp);
			});

			const Difference_type<I> n = (middle - first) - cr[0].first;

			middle = _rotate(first + cr[0].first, middle, middle + cr[0].second);
			_merge(first,
				   first + cr[0].first,
				   first + cr[0].first,
				   first + cr[0].first + cr[0].second,
				   begin(tmp),
				   cmp);

			_copy(cbegin(tmp), cend(tmp), first);
			_move(middle, last, begin(tmp));

			_merge(cbegin(tmp),
				   cbegin(tmp) + n,
				   cbegin(tmp) + n,
				   cbegin(tmp) + (last - middle),
				   middle,
				   move(cmp));

			//_bitonic_merge_n(first, middle - first, last - first, std::move(cmp));
		}

		template<typename I>
		inline void _inplace_merge(I first, I middle, I last)
		{
			_inplace_merge(first, middle, last, amp_algorithms::less<>());
		}

		//----------------------------------------------------------------------------
		// max, max_element, min, min_element, minmax_element
		//----------------------------------------------------------------------------

		template<typename T, typename U, typename C>
		inline decltype(auto) _compare_select(T&& x, U&& y, C cmp) restrict(amp)
		{
			return cmp(AMP_ALG forward<T>(x), AMP_ALG forward<U>(y)) ? AMP_ALG forward<T>(x) : AMP_ALG forward<U>(y);
		}

		template<typename C1, typename C2>
		struct Pair_comparator {
			Pair_comparator(C1 c1, C2 c2) restrict(cpu, amp) : cmp(move(c1), move(c2)) {};

			template<typename T, typename U>
			bool operator()(T&& x, U&& y) const restrict(cpu, amp)
			{
				if (cmp.first(AMP_ALG forward<decltype(x)>(x).first,
							  AMP_ALG forward<decltype(y)>(y).first)) return true;
				if (cmp.first(AMP_ALG forward<decltype(y)>(y).first,
							  AMP_ALG forward<decltype(x)>(x).first)) return false;
				return cmp.second(AMP_ALG forward<decltype(x)>(x).second,
								  AMP_ALG forward<decltype(y)>(y).second);
			}

			pair<C1, C2> cmp;
		};

		template<typename I, typename C1, typename C2, int tsz>
		inline I _extremum_element_in_tile_n(I first,
											 Difference_type<I> n,
											 const concurrency::tiled_index<tsz>& tidx,
											 C1 cmp1,
											 C2 cmp2) restrict(amp)
		{
			tile_static AMP_ALG pair<AMP_ALG Value_type<I>, AMP_ALG Difference_type<I>> tmp[tidx.tile_dim0];
			tmp[tidx.local[0]] = make_pair(*first, 0);

			for (Difference_type<I> i = tidx.local[0]; i < n; i += tidx.tile_dim0) {
				tmp[tidx.local[0]] = AMP_ALG _compare_select(tmp[tidx.local[0]],
															 AMP_ALG make_pair(first[i], i),
															 AMP_ALG Pair_comparator<C1, C2>(cmp1, cmp2));
			}

			const auto x = _reduce_single_tile_unguarded(tmp, tidx, [=](auto&& x, auto&& y) {
				return AMP_ALG _compare_select(AMP_ALG forward<decltype(x)>(x),
											   AMP_ALG forward<decltype(y)>(y),
											   AMP_ALG Pair_comparator<C1, C2>(cmp1, cmp2));
			});

			return first + x.second;
		}

		//template<typename I, typename C1, typename C2>
		//inline bool _update_extremum_across_tiles(I first,
		//										  atomic<Difference_type<I>>& extremum_idx,
		//										  I candidate,
		//										  C1 cmp1,
		//										  C2 cmp2) restrict(amp)
		//{
		//	Difference_type<I> cur_idx = extremum_idx;

		//	if (cmp1(first[cur_idx], *candidate)) return true;

		//	if (extremum_idx.compare_exchange_strong(cur_idx, candidate - first)) {
		//		if (cmp1(*(first + cur_idx), *candidate)) extremum_idx = cur_idx;
		//		else if (cmp1(*candidate, *(first + cur_idx))) extremum_idx = candidate - first;
		//		else extremum_idx = _compare_select(candidate - first, cur_idx, cmp2);

		//		return true;
		//	}
		//	else {
		//		if (cmp1(first[cur_idx], *candidate)) return true;
		//	}
		//	return false;
		//}

		template<typename I, typename C1, typename C2>
		inline I _extremum_element(I first, I last, C1 cmp1, C2 cmp2)
		{	// TODO: sort out the identity_element for the reduce step.
			if (first == last) return last;

			const auto tmp = Execution_parameters::temporary_buffer<pair<Value_type<I>,
																		 Difference_type<I>>>(last -
																							  first);
			const auto d = Execution_parameters::tiled_domain(last - first);
			const Difference_type<I> w = Execution_parameters::work_per_tile(last - first);

			concurrency::parallel_for_each(d, [=](auto&& tidx) restrict(amp) {
				const AMP_ALG Difference_type<I> t = tidx.tile[0] * w;
				const AMP_ALG Difference_type<I> n = AMP_ALG _min(w, last - first - t);

				const I ix = AMP_ALG _extremum_element_in_tile_n(first + t,
															     n,
															     AMP_ALG forward<decltype(tidx)>(tidx),
															     AMP_ALG move(cmp1),
															     AMP_ALG move(cmp2));
				if (tidx.tile_origin == tidx.global) {
					tmp[tidx.tile] = AMP_ALG make_pair(*ix, ix - first);
				}
			});
			const auto x = _reduce(cbegin(tmp),
								   cend(tmp),
								   *cbegin(tmp),
								   [=,
									cmp1 = std::move(cmp1),
									cmp2 = std::move(cmp2)](auto&& x, auto&& y) restrict(amp) {
				return AMP_ALG _compare_select(AMP_ALG forward<decltype(x)>(x),
											   AMP_ALG forward<decltype(y)>(y),
											   AMP_ALG Pair_comparator<C1, C2>(AMP_ALG move(cmp1),
																			   AMP_ALG move(cmp2)));
			});
			return first + x.second;
		}

		template<typename T, typename C>
		inline constexpr const T& _max(const T& x, const T& y, C cmp) restrict(cpu, amp)
		{
			return cmp(y, x) ? x : y;
		}

		template<typename T>
		inline constexpr const T& _max(const T& x, const T& y) restrict(cpu, amp)
		{
			return _max(x, y, amp_algorithms::less<>());
		}

		template<typename I, typename C>
		inline I _max_element(I first, I last, C cmp)
		{	// This is wrong but aligned with the standard - for max_element
			// we should return the last extremum if there are multiple extrema.
			return _extremum_element(first,
									 last,
									 [cmp = std::move(cmp)](auto&& x, auto&& y) restrict(amp) {
				return cmp(AMP_ALG forward<decltype(y)>(y), AMP_ALG forward<decltype(x)>(x));
			},
									 amp_algorithms::less<>());
		}

		template<typename I>
		inline I _max_element(I first, I last)
		{
			return _max_element(first, last, amp_algorithms::less<>());
		}

		template<typename T, typename C>
		inline constexpr const T& _min(const T& x, const T& y, C cmp) restrict(cpu, amp)
		{
			return cmp(y, x) ? y : x;
		}

		template<typename T>
		inline constexpr const T& _min(const T& x, const T& y) restrict(cpu, amp)
		{
			return _min(x, y, amp_algorithms::less<>());
		}

		template<typename I, typename C>
		inline I _min_element(I first, I last, C cmp)
		{
			return _extremum_element(first, last, std::move(cmp), amp_algorithms::less<>());
		}

		template<typename I>
		inline I _min_element(I first, I last)
		{
			return _min_element(first, last, amp_algorithms::less<>());
		}

		template<typename T, typename C>
		inline constexpr decltype(auto) _minmax(const T& x, const T& y, C cmp) restrict(cpu, amp)
		{
			return cmp(y, x) ? make_pair(y, x) : make_pair(x, y);
		}

		template<typename T>
		inline constexpr decltype(auto) _minmax(const T& x, const T& y) restrict(cpu, amp)
		{
			return _minmax(x, y, amp_algorithms::less<>());
		}

		template<typename T, typename D>
		using Min_max = pair<pair<T, D>, pair<T, D>>;

		template<typename I, typename C1, typename C2>
		inline Min_max<Value_type<I>, Difference_type<I>> _construct_minmax(I first,
																			I ix,
																			I iy,
																			C1 cmp1,
																			C2 cmp2) restrict(amp)
		{
			return _minmax(make_pair(*ix, ix - first),
						   make_pair(*iy, iy - first),
						   Pair_comparator<C1, C2>(move(cmp1), move(cmp2)));
		}

		template<typename T, typename D, typename C1, typename C2>
		inline Min_max<T, D> _combine_minmax(const Min_max<T, D>& x,
											 const Min_max<T, D>& y,
											 C1 cmp1,
											 C2 cmp2) restrict(cpu, amp)
		{
			const Pair_comparator<C1, C2> cmp(AMP_ALG move(cmp1), AMP_ALG move(cmp2));
			return make_pair(_min(x.first, y.first, cmp), _max(x.second, y.second, cmp));
		}

		template<typename I, typename C1, typename C2, int tsz>
		inline pair<I, I> _minmax_element_in_tile_n(I first,
													Difference_type<I> n,
													const concurrency::tiled_index<tsz>& tidx,
													C1 cmp1,
													C2 cmp2) restrict(amp)
		{	// TODO refactor into SoA form.
			if (n < Difference_type<I>(1)) return make_pair(first, first);

			using Min_max_t = Min_max<Value_type<I>, Difference_type<I>>;

			Min_max_t x = _construct_minmax(first, first, first + 1, cmp1, cmp2);
			for (Difference_type<I> i = tidx.local[0]; i < n; i += twice(tidx.tile_dim0)) {
				const Difference_type<I> m = _min(tidx.tile_dim0 * 1, predecessor(n - i));
				const Min_max_t y = _construct_minmax(first, first + i, first + i + m, cmp1, cmp2);
				x = _combine_minmax(x, y, cmp1, cmp2);
			}

			tile_exclusive_cache<Min_max_t[tidx.tile_dim0], tidx.tile_dim0> tmp(tidx, [=](auto&& out) {
				out[tidx.local[0]] = x;
			});

			const auto min_max_x = tmp.reduce([=](auto&& x, auto&& y) {
				return AMP_ALG _combine_minmax(AMP_ALG forward<decltype(x)>(x),
									           AMP_ALG forward<decltype(y)>(y),
									           AMP_ALG move(cmp1),
									           AMP_ALG move(cmp2));
			});

			return make_pair(first + min_max_x.first.second, first + min_max_x.second.second);
		}

		template<typename I, typename C>
		inline std::pair<I, I> _minmax_element(I first, I last, C cmp)
		{
			if ((first == last) || (std::next(first) == last)) return std::make_pair(first, first);

			const auto tmp = Execution_parameters::temporary_buffer<Min_max<Value_type<I>,
																	Difference_type<I>>>(last - first);

			const auto d = Execution_parameters::tiled_domain(last - first);
			const Difference_type<I> w = Execution_parameters::work_per_tile(last - first);

			concurrency::parallel_for_each(d, [=](auto&& tidx) restrict(amp) {
				const AMP_ALG Difference_type<I> t = tidx.tile[0] * w;
				const AMP_ALG Difference_type<I> n = AMP_ALG _min(w, last - first - t);

				const auto min_max = AMP_ALG _minmax_element_in_tile_n(first + t,
																	   n,
																	   AMP_ALG forward<decltype(tidx)>(tidx),
																	   cmp,
																	   amp_algorithms::less<>());
				uniform_invoke(tidx, [=](auto&& tile) {
					tmp[tile] = AMP_ALG make_pair(AMP_ALG make_pair(*min_max.first, min_max.first - first),
												  AMP_ALG make_pair(*min_max.second, min_max.second - first));
				});
			});

			const auto mM = _reduce(cbegin(tmp),
									cend(tmp),
									*cbegin(tmp),
									[=, cmp = std::move(cmp)](auto&& x, auto&& y) restrict(amp) {
				return _combine_minmax(AMP_ALG forward<decltype(x)>(x),
						  			   AMP_ALG forward<decltype(y)>(y),
									   AMP_ALG move(cmp),
									   amp_algorithms::less<>());
			});
			return std::make_pair(first + mM.first.second, first + mM.second.second);
		}

		template<typename I>
		inline std::pair<I, I> _minmax_element(I first, I last)
		{
			return _minmax_element(first, last, amp_algorithms::less<>());
		}

		//----------------------------------------------------------------------------
		// mismatch
		//----------------------------------------------------------------------------

		template<typename I1, typename I2, typename P, int tsz>
		inline amp_stl_algorithms::pair<I1, I2> _mismatch_in_tile_n(I1 first1,
																	Difference_type<I1> n,
																	I2 first2,
																	const concurrency::tiled_index<tsz>& tidx,
																    P p) restrict(amp)
		{
			Difference_type<I1> dx = n;
			for (Difference_type<I1> i = tidx.local[0]; i < n; i += tidx.tile_dim0) {
				if (!p(first1[i], first2[i])) {
					dx = i;
					break;
				}
			}
			tile_exclusive_cache<Difference_type<I1>[tidx.tile_dim0], tidx.tile_dim0> tmp(tidx, [=](auto&& out) {
				out[tidx.local[0]] = dx;
			});
			dx = tmp.reduce([](auto&& x, auto&& y) { return AMP_ALG _min(AMP_ALG forward<decltype(x)>(x),
																         AMP_ALG forward<decltype(y)>(y)); });
			return make_pair(first1 + dx, first2 + dx);
		}

		template<typename I1, typename I2, typename P>
		inline std::pair<I1, I2> _mismatch(I1 first1, I1 last1, I2 first2, P p)
		{
			if (first1 == last1) return std::make_pair(last1, first2);

			const concurrency::array_view<atomic<Difference_type<I1>>> r(1);
			concurrency::parallel_for_each(r.extent, [=](auto&&) restrict(amp) {
				r[0] = last1 - first1;
			});

			const auto compute_domain = Execution_parameters::tiled_domain(last1 - first1);
			const Difference_type<I1> work_per_tile = Execution_parameters::work_per_tile(last1 -
																						  first1);

			concurrency::parallel_for_each(compute_domain, [=](auto&& tidx) restrict(amp) {
				const Difference_type<I1> t = tidx.tile[0] * work_per_tile;
				const Difference_type<I1> n = _min(work_per_tile, last1 - first1 - t);

				const pair<I1, I2> mm = _mismatch_in_tile_n(first1 + t,
												            n,
												            first2 + t,
												            forward<decltype(tidx)>(tidx),
												            move(p));

				if (mm.first != (first1 + t + n)) {
					if (tidx.tile_origin == tidx.global) {
						r[0].fetch_min(mm.first - first1);
					}
				}
			});

			return std::make_pair(first1 + r[0], first2 + r[0]);
		}

		template<typename I1, typename I2>
		inline std::pair<I1, I2> _mismatch(I1 first1, I1 last1, I2 first2)
		{
			return _mismatch(first1, last1, first2, amp_algorithms::equal_to<>());
		}
		//----------------------------------------------------------------------------
		// move, move_backward
		//----------------------------------------------------------------------------

		template<typename I1, typename I2>
		inline I2 _move(I1 first, I1 last, I2 dest_first)
		{	// Given current peculiarities of memory allocation in C++ AMP this is provided
			// primarily for completeness.
			if (first == last) return dest_first;

			concurrency::parallel_for_each(concurrency::extent<1>(last - first),
										   [=](auto&& idx) restrict(amp) {
				dest_first[idx[0]] = AMP_ALG move(first[idx[0]]);
			});

			return dest_first + (last - first);
		}

		//----------------------------------------------------------------------------
		// nth_element
		//----------------------------------------------------------------------------

		template<typename I, typename C>
		inline void _odd_even_sort_one_tile(I first, I last, C cmp)
		{
			concurrency::parallel_for_each(Execution_parameters::tiled_domain(last - first),
										   [=, cmp = std::move(cmp)](auto&& tidx) restrict(amp) {
				const AMP_ALG Difference_type<I> n = AMP_ALG _min(tidx.tile_dim0 * 1, last - first);

				AMP_ALG _pbsn_sort_single_tile_n(first, n, AMP_ALG forward<decltype(tidx)>(tidx), AMP_ALG move(cmp));
			});
		}

		template<typename I, typename C>
		inline void _nth_element(I first, I nth, I last, C cmp)
		{	// TODO: move to three-way partitioning.
			if (nth == last) return;

			while (Execution_parameters::tile_size() < (last - first)) {
				const Value_type<I> pivot = _median_of_3_random_amp(first, last, cmp);

				const I eq0 = _partition(first, last, [=](auto&& x) restrict(cpu, amp) { return cmp(x, pivot); });
				const I eq1 = _partition(eq0, last, [=](auto&& x) restrict(cpu, amp) { return !cmp(pivot, x); });

				if (eq0 <= nth && nth < eq1) return;

				if (nth < eq0) {
					last = eq0;
				}
				else {
					first = eq1;
				}
			}

			if ((last - first) > Difference_type<I>(1)) { // TODO: use dedicated single tile sort here.
				_odd_even_sort_one_tile(first, last, std::move(cmp));
			}
		}

		template<typename I>
		inline void _nth_element(I first, I nth, I last)
		{
			_nth_element(first, nth, last, amp_algorithms::less<>());
		}

		//----------------------------------------------------------------------------
		// partial sum
		//----------------------------------------------------------------------------

		template<typename I1, typename I2, typename Op>
		inline I2 _partial_sum(I1 first, I1 last, I2 dest_first, Op op)
		{	// This is practically an inclusive scan. It is overassuming in what regards Value_type<I1>
			// and the role of its default value as identity element by rapport with Op.
			return _inclusive_scan(first, last, dest_first, Value_type<I1>(), std::move(op)).second;
		}

		template<typename I1, typename I2>
		inline I2 _partial_sum(I1 first, I1 last, I2 dest_first)
		{
			return _partial_sum(first, last, dest_first, amp_algorithms::plus<>());
		}

		//----------------------------------------------------------------------------
		// partition, stable_partition, partition_point, is_partitioned
		//----------------------------------------------------------------------------

		template<typename I, typename P, int tsz>
		inline I _partition_single_tile_n(I first,
										  Difference_type<I> n,
										  const concurrency::tiled_index<tsz>& tidx,
										  P p) restrict(amp)
		{	// TODO: cleanup, properify.
			if (!positive(n)) return first;

			tile_exclusive_cache<atomic<Difference_type<I>>, tidx.tile_dim0> dxl(tidx, [](auto&& out) { AMP_ALG forward<decltype(out)>(out) = 0; });
			tile_exclusive_cache<atomic<Difference_type<I>>, tidx.tile_dim0> dxu(tidx, [=](auto&& out) { AMP_ALG forward<decltype(out)>(out) = n; });
			if (tidx.local[0] < n) {
				const Value_type<I> x = first[tidx.local[0]];
				const Difference_type<I> dx = p(x) ? dxl.local()++ : --dxu.local();
				first[dx] = x;
			}

			return first + dxl.local();
		}

		template<typename I, typename P, int tsz>
		inline I _partition_in_tile_n(I first,
									  Difference_type<I> n,
									  const concurrency::tiled_index<tsz>& tidx,
									  P p) restrict(amp)
		{
			while (positive(n)) {
				const Difference_type<I> m = _min(tidx.tile_dim0 * 1, n);
				const I f = _partition_single_tile_n(first, m, tidx, p);
				const Difference_type<I> o = m - (f - first);
				_swap_ranges_in_tile_n(f, o, make_reverse_iterator(first + n), _min(o, n - m), tidx);
				first = f;
				n -= m;
			}
			return first;
		}

		template<typename I1, typename I2, typename P>
		inline I1 _partition_tiles(I1 first1, I1 last1, I2 dest_first, P p)
		{
			if (first1 == last1) return last1;

			const auto d = Execution_parameters::tiled_domain(last1 - first1);
			const Difference_type<I1> w = Execution_parameters::work_per_tile(last1 - first1);

			concurrency::parallel_for_each(d, [=, p = std::move(p)](auto&& tidx) restrict(amp) {
				const AMP_ALG Difference_type<I1> t = tidx.tile[0] * w;
				const AMP_ALG Difference_type<I1> n = AMP_ALG _min(w, last1 - first1 - t);

				const I1 tile_pp = _partition_in_tile_n(first1 + t,
													    n,
													    AMP_ALG forward<decltype(tidx)>(tidx),
													    AMP_ALG move(p));
				AMP_ALG uniform_invoke(tidx, [=](auto&& tile) { dest_first[tile] = tile_pp - (first1 + t); });
			});
			return first1 + _reduce(dest_first,
									dest_first + Execution_parameters::tile_cnt(last1 - first1),
									Difference_type<I1>(0),
									amp_algorithms::plus<>());
		}

		template<typename I1, typename I2, int tsz>
		inline bool _merge_partitioned_tiles_in_tile(I1 first1,
													 Difference_type<I1> t0,
													 Difference_type<I1> t1,
													 Difference_type<I1> w,
													 I2 first2,
													 const concurrency::tiled_index<tsz>& tidx) restrict(amp)
		{
			const AMP_ALG Difference_type<I1> tx = t0 * w;
			const AMP_ALG Difference_type<I1> ty = t1 * w;

			const AMP_ALG Difference_type<I1> px = first2[t0];
			const AMP_ALG Difference_type<I2> py = first2[t1];

			const AMP_ALG Difference_type<I1> n = AMP_ALG _min(w - px, py);
			_swap_ranges_in_tile_n(make_reverse_iterator(first1 + tx + px + n),
								   n,
								   make_reverse_iterator(first1 + ty + py),
								   n,
								   AMP_ALG forward<decltype(tidx)>(tidx));
			if (AMP_ALG positive(n)) {
				AMP_ALG uniform_invoke(tidx, [=](auto&&) {
					first2[t0] += n;
					first2[t1] -= n;
				});
			}

			return AMP_ALG positive(n);
		}

		template<typename I1, typename I2, typename G, int tsz>
		inline bool _pairwise_merge_partitioned_tiles(I1 first1,
													  Difference_type<I1> w,
													  I2 first2,
													  Difference_type<I2> t_cnt,
													  const concurrency::tiled_index<tsz>& tidx,
													  G g) restrict(amp)
		{
			const AMP_ALG pair<Difference_type<I2>,
							   Difference_type<I2>> t0_t1 = g(tidx.tile[0]);
			if (t0_t1.second < t_cnt) {
				return _merge_partitioned_tiles_in_tile(first1,
														t0_t1.first,
														t0_t1.second,
														w,
														first2,
														tidx);
			}
			return false;
		}

		template<typename I1, typename I2, typename G>
		inline bool _merge_partitioned_tiles_step(I1 first1, I1 last1, I2 first2, G g)
		{
			const auto d = Execution_parameters::tiled_domain(last1 - first1);
			const Difference_type<I1> w = Execution_parameters::work_per_tile(last1 - first1);
			const Difference_type<I1> t_cnt = Execution_parameters::tile_cnt(last1 - first1);

			const auto tmp = Execution_parameters::temporary_buffer(last - first, 0u);

			const concurrency::array_view<atomic<unsigned int>> r(1);
			_fill_n(begin(r), r.extent.size(), 0u);

			concurrency::parallel_for_each(d, [=, g = std::move(g)](auto&& tidx) restrict(amp) {
				const bool s = _pairwise_merge_partitioned_tiles(first1,
																 w,
																 first2,
																 t_cnt,
																 AMP_ALG forward<decltype(tidx)>(tidx),
																 AMP_ALG move(g));

				uniform_invoke(tidx, [=](auto&& tile) { tmp[tile] = s; });
			});

			return _any_of(cbegin(tmp), cend(tmp), [](auto&& x) restrict(cpu, amp) { return positive(x); });
		}

		//template<typename I1, typename I2>
		//inline void _odd_even_merge_partitioned_tiles(I1 first1, I1 last1, I2 first2)
		//{
		//	bool done;
		////	unsigned int cnt = 0u;
		//	do {
		//		// Even.
		//		done = _merge_partitioned_tiles_step(first1, last1, first2, [](auto&& tile) restrict(amp) {
		//			const AMP_ALG Difference_type<I1> t0 = AMP_ALG twice(tile);
		//			const AMP_ALG Difference_type<I1> t1 = AMP_ALG successor(t0);
		//			return AMP_ALG make_pair(t0, t1);
		//		});
		//		// Odd.
		//		done = done && _merge_partitioned_tiles_step(first1, last1, first2, [](auto&& tile) restrict(amp) {
		//			const AMP_ALG Difference_type<I1> t0 = AMP_ALG successor(AMP_ALG twice(tile));
		//			const AMP_ALG Difference_type<I1> t1 = AMP_ALG successor(t0);
		//			return AMP_ALG make_pair(t0, t1);
		//		});
		////		++cnt;
		//	} while (!done);

		////	std::cout << cnt << std::endl;
		////	std::cin.get();
		//}

		//template<typename I1, typename I2>
		//inline void _brick_merge_partitioned_tiles(I1 first1, I1 last1, I2 first2)
		//{
		//	if (first1 == last1) return;

		//	const auto d = Execution_parameters::tiled_domain(last1 - first1);
		//	const Difference_type<I1> w = Execution_parameters::work_per_tile(last1 - first1);
		//	const Difference_type<I1> t_cnt = rounded_up_quotient(last1 - first1, w);

		//	for (Difference_type<I1> h = t_cnt / 1.22; positive(h); h /= 1.22) {
		//		// Even.
		//		_merge_partitioned_tiles_step(first1, last1, first2, [=](auto&& tile) restrict(amp) {
		//			const AMP_ALG Difference_type<I1> t0 = (tile / h) * AMP_ALG twice(h) +
		//												   (tile % h);
		//			const AMP_ALG Difference_type<I1> t1 = t0 + h;
		//			return AMP_ALG make_pair(t0, t1);
		//		});
		//		// Odd.
		//		_merge_partitioned_tiles_step(first1, last1, first2, [=](auto&& tile) restrict(amp) {
		//			const AMP_ALG Difference_type<I1> t0 = (tile / h) * AMP_ALG twice(h) +
		//												   (tile % h) + h;
		//			const AMP_ALG Difference_type<I1> t1 = t0 + h;
		//			return AMP_ALG make_pair(t0, t1);
		//		});
		//	}
		//	//_odd_even_merge_partitioned_tiles(first1, last1, first2);
		//}

		template<typename I1, typename I2>
		inline bool _pbsn_merge_partitioned_tiles_step(I1 first1, I1 last1, I2 first2)
		{	// TODO: cleanup, all the merging network based algos should take a pair coord generator.
			const Difference_type<I2> t_cnt = Execution_parameters::tile_cnt(last1 - first1);
			const Difference_type<I2> next_pot_sz = round_up_to_next_binary_power(t_cnt);

			const auto d = Execution_parameters::tiled_domain(half_nonnegative(next_pot_sz) *
															  Execution_parameters::tile_size());
			const Difference_type<I1> w = Execution_parameters::work_per_tile(last1 - first1);

			const auto tmp = Execution_parameters::temporary_buffer(d.size(), 0);

			Difference_type<I2> h = round_up_to_next_binary_power(t_cnt);
			while (h > 1) {
				const Difference_type<I2> mask = h - 1;
				const Difference_type<I2> half_h = half_nonnegative(h);
				const Difference_type<I2> b_log = binary_logarithm(half_h);

				concurrency::parallel_for_each(d, [=](auto&& tidx) restrict(amp) {
					const AMP_ALG Difference_type<I2> t0 = AMP_ALG even_division_remainder(tidx.tile[0], half_h) +
					 									   (tidx.tile[0] >> b_log) * h;
					const AMP_ALG Difference_type<I2> t1 = t0 ^ mask;
					if (t1 < t_cnt) {
						const bool nm = _merge_partitioned_tiles_in_tile(first1,
																		 t0,
														                 t1,
														                 w,
														                 first2,
														                 AMP_ALG forward<decltype(tidx)>(tidx));
						if (nm) uniform_invoke(tidx, [=](auto&& tile) { tmp[tile] = 1u; });
					}
				});

				h = half_nonnegative(h);
			}
			return _reduce(cbegin(tmp), cend(tmp), 0u, amp_algorithms::plus<>());
		}
		template<typename I1, typename I2>
		inline void _pbsn_merge_partitioned_tiles(I1 first1, I1 last1, I2 first2)
		{
			if ((last1 - first1) < Execution_parameters::tile_size()) return;

			bool not_merged;
			//unsigned int i = 0;
			do {
				//++i;
				not_merged = _pbsn_merge_partitioned_tiles_step(first1, last1, first2);
			} while (not_merged);
			//if (i < binary_logarithm(round_up_to_next_binary_power(Execution_parameters::tile_cnt(last1 - first1)))) std::cout << "Early out: " << i << std::endl;
		}

		template<typename I1, typename I2>
		inline void _merge_partitioned_tiles(I1 first1, I1 last1, I2 first2)
		{	// This just forwards to the merger of choice.
			//_brick_merge_partitioned_tiles(first1, last1, first2);
			_pbsn_merge_partitioned_tiles(first1, last1, first2);
		}

		template<typename I, typename P>
		inline I _partition(I first, I last, P p)
		{	// TODO: cleanup, linearize bottom step.
			if (first == last) return last;

			const auto tmp = Execution_parameters::temporary_buffer(last - first, Difference_type<I>(0));

			const I pp = _partition_tiles(first, last, begin(tmp), std::move(p));
			if (Execution_parameters::tile_size() < (last - first)) {
				_merge_partitioned_tiles(first, last, begin(tmp));
			}

			return pp;
		}

		template<typename I, typename P, int tsz>
		inline I _partition_point_single_tile_n(I first,
												Difference_type<I> n,
												const concurrency::tiled_index<tsz>& tidx,
												P p) restrict(amp)
		{
			tile_exclusive_cache<Difference_type<I>[tidx.tile_dim0], tidx.tile_dim0> pp(tidx, [=](auto&& out){
				out[tidx.local[0]] = (tidx.local[0] < n) ? (p(first[tidx.local[0]]) * AMP_ALG successor(tidx.local[0]))
														 : AMP_ALG Difference_type<I>(0);
			});

			return first + pp.reduce([](auto&& x, auto&& y) { return AMP_ALG _max(AMP_ALG forward<decltype(x)>(x),
																				  AMP_ALG forward<decltype(y)>(y)); });
		}

		template<typename I, typename P, int tsz>
		inline I _partition_point_in_tile_n(I first,
											Difference_type<I> n,
											const concurrency::tiled_index<tsz>& tidx,
											P p) restrict(amp)
		{
			tile_exclusive_cache<Difference_type<I>, tidx.tile_dim0> lb(tidx, [](auto&& out) { out = 0; });
			tile_exclusive_cache<Difference_type<I>, tidx.tile_dim0> ub(tidx, [=](auto&& out) { out = n; });
			tile_exclusive_cache<Value_type<I>[tidx.tile_dim0], tidx.tile_dim0> tmp(tidx, [](auto&&){});

			Difference_type<I> w = rounded_up_quotient(n, tidx.tile_dim0);
			Difference_type<I> m = n;

			while (w > Difference_type<I>(1)) {
				if ((tidx.local[0] * w) < m) {
					const Difference_type<I> l = lb + tidx.local[0] * w;
					const Difference_type<I> o = _min(w, m - tidx.local[0] * w);

					tmp[tidx.local[0]] = first[l];

					const Value_type<I> u = ((tidx.local[0] * w + o) != m) ? tmp[successor(tidx.local[0])]
																		   : first[l + predecessor(o)];
					if (p(u)) concurrency::atomic_fetch_max(&lb.local(), l + o);
					if (!p(tmp[tidx.local[0]])) concurrency::atomic_fetch_min(&ub.local(), l);
				}

				m = ub - lb;
				w = rounded_up_quotient(m, tidx.tile_dim0);
			}

			return _partition_point_single_tile_n(first + lb, ub - lb, tidx, move(p));
		}

		template<typename I, typename P>
		inline I _partition_point(I first, I last, P p)
		{
			if (first == last) return last;

			const concurrency::array_view<Difference_type<I>> r(1);
			concurrency::parallel_for_each(r.extent, [=](auto&&) restrict(amp) { r[0] = last - first; });

			const auto d = Execution_parameters::tiled_domain(last - first);
			const Difference_type<I> w = Execution_parameters::work_per_tile(last - first);

			concurrency::parallel_for_each(d, [=, p = std::move(p)](auto&& tidx) restrict(amp) {
				const AMP_ALG Difference_type<I> t = tidx.tile[0] * w;
				const AMP_ALG Difference_type<I> n = AMP_ALG _min(w, last - first - t);

				if (r[0] < t || p(first[t + AMP_ALG predecessor(n)])) return;
				else if (!p(first[t])) {
					uniform_invoke(AMP_ALG forward<decltype(tidx)>(tidx), [=](auto&&) {
						concurrency::atomic_fetch_min(&r[0], t);
					});
				}
				else {
					const I pp = _partition_point_in_tile_n(first + t,
															n,
															AMP_ALG forward<decltype(tidx)>(tidx),
															AMP_ALG move(p));

					if (pp != (first + t + n)) {
						uniform_invoke(AMP_ALG forward<decltype(tidx)>(tidx), [=](auto&&) {
							concurrency::atomic_fetch_min(&r[0], pp - first);
						});
					}
				}
			});

			return first + r[0];
		}

		template<typename I, typename P>
		inline bool _is_partitioned(I first, I last, P p)
		{
			return last == _find_if(_find_if_not(first, last, p), last, p);
		}

		//----------------------------------------------------------------------------
		// reduce
		//----------------------------------------------------------------------------

		template<typename I, typename Op, int tsz>
		inline decltype(auto) _reduce_single_tile_unguarded(I first,
														    const concurrency::tiled_index<tsz>& tidx,
															Op op) restrict(amp)
		{	// This assumes that it is fed tsz worth of data, otherwise results are undefined.
			const concurrency::array_view<Value_type<I>> d(tsz, &*first); // Hack around lambda capture deficiencies in restrict(amp) contexts.

			static_for<half_nonnegative(tsz), 0u, Inc::div, 2u>()([=](auto&& h) {
				if (tidx.local[0] < AMP_ALG Difference_type<I>(AMP_ALG forward<decltype(h)>(h))) {
					d[tidx.local[0]] = op(d[tidx.local[0]], d[tidx.local[0] + h]);
				}
			});

			return d[0];
		}

		template<typename I, typename Op, int tsz>
		inline decltype(auto) _reduce_single_tile_n(I first,
													Difference_type<I> n,
													const concurrency::tiled_index<tsz>& tidx,
													Op op) restrict(amp)
		{
			if (n == tidx.tile_dim0) {
				return _reduce_single_tile_unguarded(first, tidx, move(op));
			}

			const concurrency::array_view<Value_type<I>> d(n, &*first);
			static_for<half_nonnegative(tsz), 0u, Inc::div, 2u>()([=](Difference_type<I> h) {
				if ((tidx.local[0] + h) < n) {
					if (tidx.local[0] < h) {
						d[tidx.local] = op(d[tidx.local], d[tidx.local + h]);
					}
				}
			});

			return d[0];
		}

		template<typename I, typename T, typename Binary_op, int tsz>
		inline T _reduce_in_tile_n(I first,
								   Difference_type<I> n,
								   const T& identity_element,
								   const concurrency::tiled_index<tsz>& tidx,
								   Binary_op op) restrict(amp)
		{
			T r = identity_element;
			for (Difference_type<I> i = tidx.local[0]; i < n; i += tidx.tile_dim0) {
				r = op(r, first[i]);
			}
			tile_exclusive_cache<T[tidx.tile_dim0], tidx.tile_dim0> tmp(tidx, [=](auto&& out) {
				AMP_ALG forward<decltype(out)>(out)[tidx.local[0]] = r;
			});
			return tmp.reduce(move(op));
		}

		template<typename I1, typename I2, typename T, typename Binary_op>
		inline I2 _reduce_outer(I1 first, I1 last, I2 dest_first, const T& identity_element, Binary_op op)
		{
			const auto d = Execution_parameters::tiled_domain(last - first);
			const Difference_type<I1> w = Execution_parameters::work_per_tile(last - first);

			concurrency::parallel_for_each(d, [=, op = std::move(op)](auto&& tidx) restrict(amp) {
				const AMP_ALG Difference_type<I1> t = tidx.tile[0] * w;
				const AMP_ALG Difference_type<I1> n = AMP_ALG _min(w, last - first - t);

				const T r = AMP_ALG _reduce_in_tile_n(first + t,
													  n,
											          identity_element,
											          AMP_ALG forward<decltype(tidx)>(tidx),
											          op);

				AMP_ALG uniform_invoke(tidx, [=](auto&& tile) { dest_first[tile] = r; });
			});

			return dest_first + Execution_parameters::tile_cnt(last - first);
		}

		template<typename I, typename T, typename Binary_op>
		inline T _reduce_inner(I first, I last, const T& identity_element, Binary_op op)
		{
			T r;
			const auto d = Execution_parameters::tiled_domain(Execution_parameters::tile_size()); // 1 tile.
			concurrency::parallel_for_each(d, [=, r = ref(r)](auto&& tidx) restrict(amp) {
				const T x = _reduce_in_tile_n(first,
											  last - first,
											  identity_element,
											  AMP_ALG forward<decltype(tidx)>(tidx),
											  AMP_ALG move(op));
				uniform_invoke(tidx, [=](auto&&) { r.get() = x; });
			});

			return r;
		}

		template<typename I, typename T, typename Binary_op>
		inline T _reduce(I first, I last, const T& identity_element, Binary_op op)
		{	// TODO: clean up, properify, optimise.
			// Investigate chained propagation vs iterative finalization - the former is shoddy on
			// Intel.
			if (first == last) return identity_element;

			const auto tmp = Execution_parameters::temporary_buffer<Value_type<I>>(last - first);
			_reduce_outer(first, last, begin(tmp), identity_element, op);
			return _reduce_inner(begin(tmp), end(tmp), identity_element, std::move(op));
		}

		//----------------------------------------------------------------------------
		// remove, remove_if, remove_copy, remove_copy_if
		//----------------------------------------------------------------------------

		template<typename I, typename T>
		inline I _remove(I first, I last, const T& value)
		{
			return _remove_if(first, last, [=](auto&& x) restrict(amp) {
				return forward<decltype(x)>(x) == value;
			});
		}

		template<typename I, typename P>
		inline I _remove_if(I first, I last, P p)
		{
			if (first == last) return last;

			return _partition(first,
							  last,
							  [p = std::move(p)](auto&& x) restrict(amp) {
				return !p(forward<decltype(x)>(x));
			});
		}

		template<typename I1, typename I2, typename T>
		inline I2 _remove_copy(I1 first, I1 last, I2 dest_first, const T& value)
		{
			return _copy_if(first,
							last,
							dest_first,
							[=](auto&& x) restrict(amp) {
				return forward<decltype(x)>(x) != value;
			});
		}

		template<typename I1, typename I2, typename P>
		inline I2 _remove_copy_if(I1 first, I1 last, I2 dest_first, P p)
		{
			return _copy_if(first,
							last,
							dest_first,
							[=, p = std::move(p)](auto&& x) restrict(amp) {
				return !p(forward<decltype(x)>(x));
			});
		}

		//----------------------------------------------------------------------------
		// replace, replace_if, replace_copy, replace_copy_if
		//----------------------------------------------------------------------------

		template<typename I, typename T>
		inline void _replace(I first, I last, const T& old_value, const T& new_value)
		{
			_replace_if(first,
						last,
						[=](auto&& x) restrict(amp) { return forward<decltype(x)>(x) == old_value; },
						new_value);
		}

		template<typename I, typename P, typename T>
		inline void _replace_if(I first, I last, P p, const T& new_value)
		{
			if (first == last) return;

			concurrency::parallel_for_each(concurrency::extent<1>(last - first),
										   [=, p = std::move(p)](auto&& idx) restrict(amp) {
				if (p(first[idx[0]])) {
					first[idx[0]] = new_value;
				}
			});
		}

		template<typename I1, typename I2, typename T>
		inline I2 _replace_copy(I1 first, I1 last, I2 dest_first, const T& old_value, const T& new_value)
		{
			return _replace_copy_if(first,
									last,
									dest_first,
									[=](auto&& x) restrict(amp) {
				return forward<decltype(x)>(x) == old_value;
			},
									new_value);
		}

		template<typename I1, typename I2, typename P, typename T>
		inline I2 _replace_copy_if(I1 first, I1 last, I2 dest_first, P p, const T& new_value)
		{
			concurrency::parallel_for_each(concurrency::extent<1>(last - first),
										   [=, p = std::move(p)](auto&& idx) restrict(amp) {
				dest_first[idx[0]] = p(first[idx[0]]) ? new_value : first[idx[0]];
			});

			return dest_first + (last - first);
		}

		//----------------------------------------------------------------------------
		// reverse, reverse_copy
		//----------------------------------------------------------------------------

		template<typename I>
		inline void _reverse(I first, I last)
		{
			if ((first == last) || ((last - first) == Difference_type<I>(1))) return;

			concurrency::parallel_for_each(concurrency::extent<1>(half_nonnegative(last - first)),
										   [=](auto&& idx) restrict(amp) {
				_iter_swap(first + idx[0], last - AMP_ALG successor(idx[0]));
			});
		}

		template<typename I1, typename I2, int tsz>
		inline I2 _reverse_copy_in_tile_n(I1 first,
										  Difference_type<I1> n,
										  I2 dest_first,
									      const concurrency::tiled_index<tsz>& tidx) restrict(amp)
		{
			if (!positive(n)) return dest_first;

			for (Difference_type<I1> i = tidx.local[0]; i < n; i += tidx.tile_dim0) {
				dest_first[i] = first[predecessor(n) - i];
			}

			return dest_first + n;
		}

		template<typename I1, typename I2, int tsz>
		inline I2 _reverse_copy_single_tile(I1 first,
											I1 last,
											I2 dest_first,
											const concurrency::tiled_index<tsz>& tidx) restrict(amp)
		{
			return _reverse_copy_single_tile_n(first, last - first, dest_first, tidx);
		}

		template<typename I1, typename I2>
		inline I2 _reverse_copy(I1 first, I1 last, I2 dest_first)
		{
			if (first == last) return dest_first;

			concurrency::parallel_for_each(concurrency::extent<1>(last - first),
										   [=](auto&& idx) restrict(amp) {
				dest_first[idx[0]] = last[-successor(idx[0])];
			});

			return dest_first + (last - first);
		}

		//----------------------------------------------------------------------------
		// rotate, rotate_copy
		//----------------------------------------------------------------------------

		template<typename I, int tsz>
		inline I _rotate_single_tile_n(I first,
									   I middle,
									   Difference_type<I> n,
									   const concurrency::tiled_index<tsz>& tidx) restrict(amp)
		{
			const auto m = n - (middle - first);
			if (tidx.local[0] < n) {
				_swap(first[tidx.local[0]], first[(tidx.local[0] + m) % n]);
			}
			return first + m;
		}

		template<typename I>
		inline I _rotate(I first, I middle, I last)
		{
			// TODO: this is nice and clean but single-invocation block-swap might be preferable.
			//	   : First and second reverses can and should be collapsed into a single p_f_e.
			if (first == middle) return last;
			if (middle == last) return first;

			_reverse(first, middle);
			_reverse(middle, last);
			_reverse(first, last);

			return first + (last - middle);
		}

		template<typename I1, typename I2, int tsz>
		inline I2 _rotate_copy_in_tile_n(I1 first,
										 Difference_type<I1> mid,
										 Difference_type<I1> n,
										 I2 dest_first,
										 const concurrency::tiled_index<tsz>& tidx) restrict(amp)
		{
			dest_first = _copy_in_tile_n(first, mid, dest_first, tidx);
			return _copy_in_tile_n(first + mid, n - mid, dest_first, tidx);
		}

		template<typename I1, typename I2>
		inline I2 _rotate_copy(I1 first, I1 middle, I1 last, I2 dest_first)
		{
			dest_first = _copy_n(first, middle - first, dest_first);
			return _copy_n(middle, last - middle, dest_first);
		}

		//----------------------------------------------------------------------------
		// inclusive_scan, exclusive_scan
		// inplace_inclusive_scan, inplace_exclusive_scan
		//----------------------------------------------------------------------------

		// These are all non-standard, but scan is fundamental enough to warrant presence

		template<typename I1, typename I2, typename T, typename Op, int tsz>
		inline /*pair<Value_type<I>, Difference_type<I>>*/ auto _exclusive_scan_single_tile_n(I1 first,
																							  Difference_type<I1> n,
																							  I2 dest_first,
																							  const T& identity_element,
																							  const concurrency::tiled_index<tsz>& tidx,
																							  Op op) restrict(amp)
		{
			tile_exclusive_cache<Value_type<I2>[tidx.tile_dim0], tidx.tile_dim0> tmp(tidx, [](auto&&){});
			_copy_in_tile_n(first, n, tmp.local(), tidx);

			static_for<1u, tidx.tile_dim0, Inc::mul, 2u>()([=](auto&& h) {
				tmp[tidx.local[0]] = (tidx.local[0] < h) ? tmp[tidx.local[0]]
													     : op(tmp[tidx.local[0] - h], tmp[tidx.local[0]]);
			});
			*dest_first = identity_element;//uniform_invoke(tidx, [=](auto&&) { *dest_first = identity_element; }); // Temporarily disabled.
			_copy_in_tile_n(tmp.local(), predecessor(n), dest_first + 1, tidx);

			return tmp[predecessor(n)];	//make_pair(tmp[out * n + predecessor(n)], n); // Return scan value and offset to last written.
		}

		template<typename I1, typename I2, typename Op, int tsz>
		inline /*pair<Value_type<I>, Difference_type<I>>*/ auto _inclusive_scan_single_tile_n(I1 first,
																							  Difference_type<I1> n,
																							  I2 dest_first,
																							  const concurrency::tiled_index<tsz>& tidx,
																							  Op op) restrict(amp)
		{
			tile_exclusive_cache<Value_type<I2>[tidx.tile_dim0], tidx.tile_dim0> tmp(tidx, [](auto&&){});
			_copy_in_tile_n(first, n, tmp.local(), tidx);

			static_for<1u, tidx.tile_dim0, Inc::mul, 2u>()([=](auto&& h) {
				tmp[tidx.local[0]] = (tidx.local[0] < h) ? tmp[tidx.local[0]]
													     : op(tmp[tidx.local[0] - h], tmp[tidx.local[0]]);
			});
			_copy_in_tile_n(tmp.local(), n, dest_first, tidx);

			return tmp[predecessor(n)];//make_pair(tmp[out * n + predecessor(n)], n); // Return scan value and offset to last written.
		}


		template<typename I1, typename I2, typename Op, int tsz>
		inline decltype(auto) _scan_single_tile_inclusive(I1 first,
														  I2 dest_first,
														  const concurrency::tiled_index<tsz>& tidx,
														  Op op) restrict(amp)
		{
			return _inclusive_scan_single_tile_n(first, tidx.tile_dim0, dest_first, tidx, move(op));
		}

		template<typename I, typename Op, int tsz>
		inline decltype(auto) _scan_single_tile_exclusive(I first,
														  const Value_type<I>& identity_element,
														  const concurrency::tiled_index<tsz>& tidx,
														  Op op) restrict(amp)
		{
			return _exclusive_scan_single_tile_n(first, tidx.tile_dim0, identity_element, tidx, move(op));
		}

		template<typename I1, typename I2, typename T, typename Op, int tsz>
		inline Value_type<I2> _reduce_rows_n(I1 first,
											 Difference_type<I1> n,
											 I2 dest_first,
											 const T& identity_element,
											 const concurrency::tiled_index<tsz>& tidx,
											 Op op) restrict(amp)
		{
			const Difference_type<I1> w = rounded_up_quotient(n, tidx.tile_dim0);
			const Difference_type<I1> l = tidx.local[0] * w;
			const Difference_type<I1> m = _min(w, n - l);

			T x = identity_element;
			for (Difference_type<I1> i = 0; i < m; ++i) {
				x = op(x, first[l + i]);
			}
			dest_first[tidx.local[0]] = x;

			tile_exclusive_cache<Value_type<I2>[tidx.tile_dim0], tsz> tmp(tidx, [=](auto&& out) {
				out[tidx.local[0]] = x;
			});
			return tmp.reduce(move(op));
		}

		template<typename I1, typename I2, typename I3, typename Op, int tsz>
		inline I3 _scan_rows_n(I1 first1,
							   Difference_type<I1> n,
							   I2 first2,
							   I3 dest_first,
							   const concurrency::tiled_index<tsz>& tidx,
							   Op op) restrict(amp)
		{
			const Difference_type<I1> w = rounded_up_quotient(n, tidx.tile_dim0);
			const Difference_type<I1> l = tidx.local[0] * w;

		//	if (l < n) {
				const Difference_type<I1> m = _min(w, n - l);
				Value_type<I3> x = first2[tidx.local[0]];
				for (Difference_type<I1> i = 0; i < m; ++i) {
					x = op(x, first1[l + i]);
					dest_first[l + i] = x;
				}
		//	}

			return dest_first + n;
		}

		template<typename I1, typename I2, typename T, typename Op>
		inline std::pair<I2, I2> _inclusive_scan_many_to_one_work_assignment_unguarded(I1 first,
																					   I1 last,
																					   I2 dest_first,
																					   const T& identity_element,
																					   Op op)
		{	// WIP: investigation of StreamScan.
			const concurrency::array_view<atomic<Difference_type<I1>>> ta(1);
			_fill_n(begin(ta), 1, Difference_type<I1>(0));

			const auto tmp = Execution_parameters::temporary_buffer<pair<Value_type<I2>, atomic<Difference_type<I1>>>>(last - first);//, make_pair(T(identity_element), last - first));
			_fill_n(begin(tmp), tmp.extent.size(), make_pair(identity_element, last - first));

			const auto d = Execution_parameters::tiled_domain(last - first);
			const Difference_type<I1> w = Execution_parameters::work_per_tile(last - first);

			concurrency::parallel_for_each(d, [=, op = std::move(op)](auto&& tidx) restrict(amp) {
				AMP_ALG tile_exclusive_cache<AMP_ALG Difference_type<I1>, tidx.tile_dim0> linear_tile(tidx, [=](auto&& out) {
					AMP_ALG forward<decltype(out)>(out) = ta[0]++;
				});
				const AMP_ALG Difference_type<I1> t = linear_tile.local() * w;
				const AMP_ALG Difference_type<I1> n = AMP_ALG _min(w, last - first - t);

				const T s = *_inclusive_scan_in_tile_n(first + t, n, dest_first + t, AMP_ALG forward<decltype(tidx)>(tidx), op).first;

				if (AMP_ALG positive(linear_tile.local())) {
					AMP_ALG tile_exclusive_cache<AMP_ALG Value_type<I2>, tidx.tile_dim0> pre(tidx, [=, linear_tile = linear_tile](auto&& out) {
						while (tmp[AMP_ALG predecessor(linear_tile.local())].second == (last - first));
						AMP_ALG forward<decltype(out)>(out) = tmp[AMP_ALG predecessor(linear_tile.local())].first;
					});
					AMP_ALG uniform_invoke(tidx, [=, pre = pre, linear_tile = linear_tile](auto&&) { tmp[linear_tile.local()] = AMP_ALG make_pair(op(pre.local(), s), linear_tile.local()); });
					AMP_ALG _transform_in_tile_n(dest_first + t, n, dest_first + t, AMP_ALG forward<decltype(tidx)>(tidx), [=, pre = pre](auto&& x) { return op(x, pre.local()); });
				}
				else {
					uniform_invoke(tidx, [=](auto&&) { tmp[0] = AMP_ALG make_pair(s, 0); });
				}
			});

			return std::make_pair(dest_first + predecessor(last - first),
								  dest_first + (last - first));
		}

		template<typename I1, typename I2, typename T, typename Op>
		inline I2 _reduce_tiles(I1 first, I1 last, I2 dest_first, const T& identity_element, Op op)
		{
			return _reduce_outer(first, last, dest_first, identity_element, std::move(op));
		}

		template<typename I, typename T, typename Op>
		inline std::pair<I, I> _exclusive_scan_reduced_tiles(I first, I last, const T& identity_element, Op op)
		{
			const auto d = Execution_parameters::tiled_domain(Execution_parameters::tile_size()); // 1 tile.
			concurrency::parallel_for_each(d, [=, op = std::move(op)](auto&& tidx) restrict(amp) {
				_exclusive_scan_in_tile_n(first,
										  last - first,
										  first,
										  identity_element,
										  AMP_ALG forward<decltype(tidx)>(tidx),
										  AMP_ALG move(op));
			});
		/*	std::vector<Value_type<I>> foo(first, last);
			::testtools::scan_cpu_exclusive(std::begin(foo), std::end(foo), first, op);*/
			return std::make_pair(std::prev(last), last);
		}

		template<typename I1, typename I2, typename T, typename U, typename Op, int tsz>
		inline pair<I2, I2> _partial_exclusive_scan_in_tile_n(I1 first,
													          Difference_type<I1> n,
													          I2 dest_first,
													          const T& previous,
													          const U& identity_element,
													          const concurrency::tiled_index<tsz>& tidx,
													          Op op) restrict(amp)
		{
			tile_exclusive_cache<Value_type<I2>[tidx.tile_dim0], tidx.tile_dim0> tmp(tidx, [](auto&&){});

			T pre = previous;
			for (Difference_type<I1> i = 0; i < n; i += tidx.tile_dim0) {
				const Difference_type<I1> m = _min(tidx.tile_dim0 * 1, n - i);

				_copy_in_tile_n(first + i, m, tmp.local(), tidx);

				uniform_invoke(tidx, [=](auto&&) { tmp[0] = op(pre, tmp[0]); });
				T nxt = _exclusive_scan_single_tile_n(tmp.local(), m, identity_element, tidx, op);
				uniform_invoke(tidx, [=](auto&&) { tmp[0] = pre; });
				pre = nxt;

				_copy_in_tile_n(tmp.local(), m, dest_first + i, tidx);
			}

			return make_pair(dest_first + predecessor(n), dest_first + n);
		}

		template<typename I1, typename I2, typename T, typename Op, int tsz>
		inline pair<I2, I2> _exclusive_scan_in_tile_n(I1 first,
													  Difference_type<I1> n,
													  I2 dest_first,
													  const T& identity_element,
													  const concurrency::tiled_index<tsz>& tidx,
													  Op op) restrict(amp)
		{
			tile_exclusive_cache<Value_type<I2>[tidx.tile_dim0], tidx.tile_dim0> tmp(tidx, [](auto&&){});

			T pre = identity_element;
			for (Difference_type<I1> i = 0; i < n; i += tidx.tile_dim0) {
				const Difference_type<I1> m = _min(tidx.tile_dim0 * 1, n - i);

				const T s = _exclusive_scan_single_tile_n(first + i, m, tmp.local(), identity_element, tidx, op);
				dest_first = _transform_in_tile_n(tmp.local(), m, dest_first, tidx, [=](auto&& x) { return op(pre, x); });
				pre = op(pre, s);
			}

			return make_pair(dest_first + predecessor(n), dest_first + n);
		}

		template<typename I1, typename I2, typename T, typename Op, int tsz>
		inline pair<I2, I2> _partial_inclusive_scan_in_tile_n(I1 first,
													          Difference_type<I1> n,
													          I2 dest_first,
													          const T& previous,
													          const concurrency::tiled_index<tsz>& tidx,
													          Op op) restrict(amp)
		{
			tile_exclusive_cache<Value_type<I2>[tidx.tile_dim0], tidx.tile_dim0> tmp(tidx, [](auto&&){});

			T pre = previous;
			for (Difference_type<I1> i = 0; i < n; i += tidx.tile_dim0) {
				const Difference_type<I1> m = _min(tidx.tile_dim0 * 1, n - i);

				const T s = _inclusive_scan_single_tile_n(first + i, m, tmp.local(), tidx, op);
				_transform_in_tile_n(tmp.local(), m, dest_first + i, tidx, [=](auto&& x) { return op(pre, x); });
				pre = op(pre, s);
			}

			return make_pair(dest_first + predecessor(n), dest_first + n);
		}

		template<typename I1, typename I2, typename Op, int tsz>
		inline pair<I2, I2> _inclusive_scan_in_tile_n(I1 first,
													  Difference_type<I1> n,
													  I2 dest_first,
													  const concurrency::tiled_index<tsz>& tidx,
													  Op op) restrict(amp)
		{
			tile_exclusive_cache<Value_type<I2>[tidx.tile_dim0], tidx.tile_dim0> tmp(tidx, [](auto&&) {});

			Difference_type<I1> m = _min(tidx.tile_dim0 * 1, n);
			Value_type<I2> pre = _inclusive_scan_single_tile_n(first, m, dest_first, tidx, op);
			for (Difference_type<I1> i = m; i < n; i += tidx.tile_dim0) {
				m = _min(tidx.tile_dim0 * 1, n - i);
				const Value_type<I2> s = _inclusive_scan_single_tile_n(first + i, m, tmp.local(), tidx, op);
				_transform_in_tile_n(tmp.local(), m, dest_first + i, tidx, [=](auto&& x) { return op(pre, x); });
				pre = op(pre, s);
			}

			return make_pair(dest_first + predecessor(n), dest_first + n);
		}

		template<typename I1, typename I2, typename I3, typename T, typename Op>
		inline std::pair<I2, I2> _scan_tiles(I1 first1,
											 I1 last1,
											 I2 first2,
											 I3 dest_first,
											 const T& identity_element,
											 Op op)
		{
			const auto d = Execution_parameters::tiled_domain(last1 - first1);
			const Difference_type<I1> w = Execution_parameters::work_per_tile(last1 - first1);

			concurrency::parallel_for_each(d, [=, op = std::move(op)](auto&& tidx) restrict(amp) {
				const AMP_ALG Difference_type<I1> t = tidx.tile[0] * w;
				const AMP_ALG Difference_type<I1> n = AMP_ALG _min(w, last1 - first1 - t);

				_partial_inclusive_scan_in_tile_n(first1 + t,
												  n,
												  dest_first + t,
												  first2[tidx.tile[0]],
												  AMP_ALG forward<decltype(tidx)>(tidx),
												  AMP_ALG move(op));
			});
			return std::make_pair(dest_first + predecessor(last1 - first1),
								  dest_first + (last1 - first1));
		}

		template<typename I1, typename I2, typename T, typename Op>
		inline std::pair<I2, I2> _inclusive_scan(I1 first,
												 I1 last,
												 I2 dest_first,
												 const T& identity_element,
												 Op op)
		{	// TODO: refactor, into single function.
			// TODO: this is not the fastest possible scan,
			//       revisit the single-pass scan (StreamScan).
			if (first == last) return std::make_pair(dest_first, dest_first);

			const auto tmp = Execution_parameters::temporary_buffer<T>(last - first);
			_reduce_tiles(first, last, begin(tmp), identity_element, op);
			_exclusive_scan_reduced_tiles(begin(tmp), end(tmp), identity_element, op);
			_scan_tiles(first, last, cbegin(tmp), dest_first, identity_element, std::move(op));

			return std::make_pair(dest_first + predecessor(last - first),
								  dest_first + (last - first));
		}

		template<typename I1, typename I2, typename T>
		inline std::pair<I2, I2> _inclusive_scan(I1 first,
												 I1 last,
												 I2 dest_first,
												 const T& identity_element)
		{
			return _inclusive_scan(first, last, dest_first, identity_element, amp_algorithms::plus<>());
		}

		template<typename I1, typename I2, typename Op>
		inline std::pair<I2, I2> _exclusive_scan(I1 first,
												 I1 last,
												 I2 dest_first,
												 const Value_type<I1>& identity_element,
												 Op op)
		{	// TODO: optimise.
			_inclusive_scan(first, std::prev(last), std::next(dest_first), identity_element, std::move(op));
			_fill_n(dest_first, Difference_type<I2>(1), identity_element);
			return std::make_pair(dest_first + predecessor(predecessor(last - first)),
								  dest_first + predecessor(last - first));
		}

		template<typename I, typename T, typename Op>
		inline std::pair<I, I> _inplace_inclusive_scan(I first,
													   I last,
													   const T& identity_element,
													   Op op)
		{
			return _inclusive_scan(first, last, first, identity_element, std::move(op));
		}

		template<typename I, typename T>
		inline std::pair<I, I> _inplace_inclusive_scan(I first, I last, const T& identity_element)
		{
			return _inplace_inclusive_scan(first, last, identity_element, amp_algorithms::plus<>());
		}

		template<typename I, typename T, typename Op>
		inline std::pair<I, I> _inplace_exclusive_scan(I first,
													   I last,
													   const T& identity_element,
													   Op op)
		{	// TEMPORARY PLACEHOLDER, NOT EFFICIENT
			_inplace_inclusive_scan(first, last, identity_element, std::move(op));
			_rotate(first, last - Difference_type<I>(1), last);
			_fill_n(first, Difference_type<I>(1), identity_element);
			return std::make_pair(first + predecessor(predecessor(last - first)),
								  first + predecessor(last - first));
		}

		template<typename I, typename T>
		inline std::pair<I, I> _inplace_exclusive_scan(I first, I last, const T& identity_element)
		{
			return _inplace_exclusive_scan(first, last, identity_element, amp_algorithms::plus<>());
		}

		//----------------------------------------------------------------------------
		// exclusive_segmented_scan, inclusive_segmented_scan,
		// inplace_exclusive_segmented_scan, inplace_inclusive_segmented_scan
		//----------------------------------------------------------------------------

		//----------------------------------------------------------------------------
		// search
		//----------------------------------------------------------------------------

		//----------------------------------------------------------------------------
		// search_n
		//----------------------------------------------------------------------------

		template<typename I1, typename I2, typename T, typename P>
		inline decltype(auto) _compute_prefixes_search_n(I1 first,
														 I1 last,
														 I2 dest_first,
														 const T& value,
														 P p)
		{	// TODO: use linearized tile indices.
			const auto d = Execution_parameters::tiled_domain(last - first);
			const Difference_type<I1> w = Execution_parameters::work_per_tile(last - first);

			const auto tmp = Execution_parameters::temporary_buffer<atomic<Difference_type<I1>>>(last - first);
			_fill_n(begin(tmp), tmp.extent.size(), last - first);

			concurrency::parallel_for_each(d, [=, p = std::move(p)](auto&& tidx) restrict(amp) {
				const AMP_ALG Difference_type<I1> t = tidx.tile[0] * w;

				const AMP_ALG Difference_type<I1> n = AMP_ALG _min(w, last - first - t);
				const auto rf = _find_if_not_in_tile_n(make_reverse_iterator(first + t + n),
													   n,
													   AMP_ALG forward<decltype(tidx)>(tidx),
													   [=](auto&& x) {
					return p(AMP_ALG forward<decltype(x)>(x), value);
				});

				uniform_invoke(tidx, [=](auto&& tile) {
					const AMP_ALG Difference_type<I1> suffix = rf - AMP_ALG make_reverse_iterator(first + t + n);
					AMP_ALG Difference_type<I1> prefix = 0;
					if (AMP_ALG positive(AMP_ALG forward<decltype(tile)>(tile)) && (suffix == n)) {
						do {
							prefix = tmp[AMP_ALG predecessor(AMP_ALG forward<decltype(tile)>(tile))];
						} while (prefix == (last - first));
					}
					tmp[AMP_ALG forward<decltype(tile)>(tile)] = prefix + suffix;
				});
			});
			return _move(begin(tmp), end(tmp), dest_first);
		}

		template<typename I, typename N, typename T, typename P>
		inline I _search_n(I first, I last, N count, const T& value, P p)
		{	// TODO: cleanup, properify.
			if (!positive(count)) return first;
			if ((last - first) < count) return last;
			if (one(count)) return _find(first, last, value);

			const auto d = Execution_parameters::tiled_domain(last - first);
			const Difference_type<I> w = Execution_parameters::work_per_tile(last - first);

			const auto tmp = Execution_parameters::temporary_buffer<Difference_type<I>>(last - first);
			_compute_prefixes_search_n(first, last, begin(tmp), value, p);

			const auto tmp1 = Execution_parameters::temporary_buffer(last - first, last - first);

			const auto eq = [=](auto&& x) restrict(amp) { return p(AMP_ALG forward<decltype(x)>(x), value); };
			concurrency::parallel_for_each(d, [=, p = std::move(p)](auto&& tidx) restrict(amp) {
				const AMP_ALG Difference_type<I> t = tidx.tile[0] * w;
				const AMP_ALG Difference_type<I> n = AMP_ALG _min(w, last - first - t);

				AMP_ALG tile_exclusive_cache<AMP_ALG Difference_type<I>, tidx.tile_dim0> prefix(tidx, [=](auto&& out) {
					AMP_ALG forward<decltype(out)>(out) = AMP_ALG positive(tidx.tile[0]) ? tmp[AMP_ALG predecessor(tidx.tile[0])] : 0;
				});
				AMP_ALG tile_exclusive_cache<bool, tidx.tile_dim0> done(tidx, [=, prefix = prefix](auto&& out) {
					AMP_ALG forward<decltype(out)>(out) = (prefix + n) < count;
				});

				if (done) return;

				AMP_ALG tile_exclusive_cache<AMP_ALG Difference_type<I>, tidx.tile_dim0> suffix(tidx, [=](auto&& out) {
					AMP_ALG forward<decltype(out)>(out) = tmp[tidx.tile];
				});

				const AMP_ALG Difference_type<I> m = AMP_ALG _max(0, n - suffix);
				I f;
				I l = first + t;
				do {
					f = _find_if_in_tile_n(l, m - (l - (first + t)), AMP_ALG forward<decltype(tidx)>(tidx), eq);
					l = _find_if_not_in_tile_n(f, m - (f - (first + t)), AMP_ALG forward<decltype(tidx)>(tidx), eq);

					const AMP_ALG Difference_type<I> x0 = prefix * (f == first + t);
					const AMP_ALG Difference_type<I> x1 = l - f;
					const AMP_ALG Difference_type<I> x2 = suffix * (l == (first + t + m));

					if (count <= (x0 + x1 + x2)) {
						uniform_invoke(tidx, [=](auto&& tile) { tmp1[tile] = f - first - x0; });
						break;
					}
				} while (f != l);
			});

			return first + *_min_element(cbegin(tmp1), cend(tmp1));
 		}

		template<typename I, typename N, typename T>
		inline I _search_n(I first, I last, N n, const T& value)
		{
			return _search_n(first, last, std::move(n), value, amp_algorithms::equal_to<>());
		}

		//----------------------------------------------------------------------------
		// binary_search
		//----------------------------------------------------------------------------

		template<typename I, typename T, typename C>
		inline bool _binary_search(I first, I last, const T& value, C cmp)
		{
			if (first == last) return false;

			const concurrency::array_view<unsigned int> r(1);
			const I p = _partition_point(first, last, [=](auto&& x) restrict(amp) {
				return cmp(AMP_ALG forward<decltype(x)>(x), value);
			});

			if (p == last) return false;

			concurrency::parallel_for_each(r.extent, [=, cmp = std::move(cmp)](auto&&) restrict(amp) {
				r[0] = !cmp(value, *p);
			});

			return positive(r[0]);
		}

		template<typename I, typename T>
		inline bool _binary_search(I first, I last, const T& value)
		{
			return _binary_search(first, last, value, amp_algorithms::less<>());
		}

		//----------------------------------------------------------------------------
		// set_difference, set_intersection, set_symmetric_difference, set_union
		//----------------------------------------------------------------------------

		template<typename I1, typename I2, typename I3, typename C>
		inline I3 _set_difference(I1 first1, I1 last1, I2 first2, I2 last2, I3 dest_first, C cmp)
		{
			if (first1 == last1) return dest_first;
			if (first2 == last2) return amp_stl_algorithms::copy(first1, last1, dest_first);

			const auto tmp = Execution_parameters::temporary_buffer(last1 - first1);
			const auto compute_domain = Execution_parameters::tiled_domain(last1 - first1);


		}

		template<typename I1, typename I2, typename I3>
		inline I3 _set_difference(I1 first1, I1 last1, I2 first2, I2 last2, I3 dest_first)
		{
			return _set_difference(first1, last1, first2, last2, dest_first, amp_algorithms::less<>());
		}

		//----------------------------------------------------------------------------
		// shuffle, random_shuffle
		//----------------------------------------------------------------------------

		//----------------------------------------------------------------------------
		// is_sorted, is_sorted_until, sort, partial_sort, partial_sort_copy, stable_sort
		//----------------------------------------------------------------------------

		template<typename I, typename C>
		inline I _is_sorted_until(I first, I last, C cmp)
		{
			if (first == last) return last;

			const auto it = _adjacent_find(first,
										   last,
										   [cmp = std::move(cmp)](auto&& x, auto&& y) restrict(amp) {
				return !(cmp(forward<decltype(x)>(x), forward<decltype(y)>(y)));
			});
			return it == last ? it : std::next(it);
		}

		template<typename I>
		inline I _is_sorted_until(I first, I last)
		{
			return _is_sorted_until(first, last, amp_algorithms::less_equal<>());
		}

		template<typename I>
		inline bool _is_sorted(I first, I last)
		{
			return _is_sorted_until(first, last, amp_algorithms::less_equal<>()) == last;
		}

		template<typename I, typename C>
		inline bool _is_sorted(I first, I last, C comp)
		{
			return _is_sorted_until(first, last, std::move(comp)) == last;
		}

		template<typename T, typename C>
		inline constexpr T _median_of_3_xy(const T& x, const T& y, const T& z, C cmp) restrict(cpu, amp)
		{
			return !cmp(z, y) ? y : _max(x, z, move(cmp));
		}

		template<typename T, typename C>
		inline constexpr T _median_of_3(const T& x, const T& y, const T& z, C cmp) restrict(cpu, amp)
		{
			return cmp(y, x) ? _median_of_3_xy(y, x, z, move(cmp))
						     : _median_of_3_xy(x, y, z, move(cmp));
		}

		template<typename I, typename C>
		inline Value_type<I> _median_of_3_random_amp(I first, I last, C cmp)
		{
			static std::mt19937_64 g;
			std::uniform_int_distribution<Difference_type<I>> d(Difference_type<I>(0),
																predecessor(last - first));

			const concurrency::array_view<Value_type<I>> m(1);
			const Difference_type<I> dx = d(g);
			const Difference_type<I> dy = d(g);
			const Difference_type<I> dz = d(g);
			concurrency::parallel_for_each(m.extent,
										   [=,
										   cmp = std::move(cmp)](auto&&) restrict(amp) {
				m[0] = AMP_ALG _median_of_3(first[dx], first[dy], first[dz], AMP_ALG move(cmp));
			});

			return m[0];
		}

		template<typename I, typename T, typename C, int tsz>
		inline pair<Difference_type<I>,
					Difference_type<I>> _partition_3_way_single_tile_n(I first,
				  						                               Difference_type<I> n,
												                       const T& pivot,
												                       const concurrency::tiled_index<tsz>& tidx,
												                       C cmp) restrict(amp)
		{	// TODO: tweak the atomics usage, it serializes atm.
			using Offsets = pair<atomic<Difference_type<I>>, atomic<Difference_type<I>>>;
			tile_exclusive_cache<Offsets, tidx.tile_dim0> dx(tidx, [=](auto&& out) {
				AMP_ALG forward<decltype(out)>(out) = AMP_ALG make_pair(AMP_ALG Difference_type<I>(0),
																		n);
			});

			if (tidx.local[0] < n) {
				const Value_type<I> x = first[tidx.local[0]];
				if (cmp(x, pivot)) {
					first[dx.local().first++] = x;
				}
				else if (cmp(pivot, x)) {
					first[--dx.local().second] = x;
				}
			}
			return make_pair(dx.local().first.load(), dx.local().second.load());
		}

		template<typename I1, typename I2, int tsz>
		inline pair<Difference_type<I1>,
					Difference_type<I2>> _merge_3_way_partitions_in_tile(I1 first1,
																		 Difference_type<I1> n_eq1,
																		 I1 last1,
																		 I2 first2,
																		 Difference_type<I1> n_ls2,
																		 Difference_type<I2> n_eq2,
																		 const concurrency::tiled_index<tsz>& tidx) restrict(amp)
		{
			const auto mn = _swap_ranges_in_tile_n(first1,
												   last1 - first1,
												   first2 + n_ls2 - _min(n_ls2, last1 - first1),
												   n_ls2,
												   tidx);

			const auto dx = _max(0, mn.first - n_eq1); // Out of place elems, greater before equal in second partition.
			const auto pq = _swap_ranges_in_tile_n(first2 + n_ls2 - dx,
												   n_ls2,
												   first2 + n_ls2 + n_eq2 - _min(dx, n_eq2),
												   n_ls2 + n_eq2,
												   tidx);

			const auto dy = _min(n_ls2 + n_eq2 - dx, last1 - (first1 + n_eq1 + dx)); // Out of place elems, greater before equal in final partition.
			const auto rs = _swap_ranges_in_tile_n(first1 + n_eq1 + dx,
												   last1 - first1,
												   first2 + n_ls2 + n_eq2 - dx - dy,
												   n_ls2 + n_eq2 - dx,
												   tidx);

			return make_pair(n_ls2, n_ls2 + n_eq2 - dx - dy);
		}

	/*	template<typename I, typename T, typename C, int tsz>
		inline pair<I, I> _partition_3_way_in_tile_n(I first,
													 Difference_type<I> n,
													 const T& pivot,
													 const concurrency::tiled_index<tsz>& tidx,
													 C cmp) restrict(amp)
		{
			using Offsets = AMP_ALG pair<AMP_ALG Difference_type<I>, AMP_ALG Difference_type<I>>;
			AMP_ALG tile_exclusive_cache<Offsets, tidx.tile_dim0> off(tidx, [=](auto&& out) {
				AMP_ALG forward<decltype(out)>(out) = AMP_ALG make_pair(AMP_ALG Difference_type<I>(0),
														                AMP_ALG Difference_type<I>(0));
			});

			AMP_ALG tile_exclusive_cache<AMP_ALG Value_type<I>[tidx.tile_dim0], tidx.tile_dim0> tmp(tidx, [](auto&&){});
			for (AMP_ALG Difference_type<I> i = 0; i < n; i += tidx.tile_dim0) {
				const AMP_ALG Difference_type<I> m = _min(tidx.tile_dim0 * 1, n - i);
				_copy_in_tile_n(first + i, m, tmp.local(), tidx);

				const auto pp = _partition_3_way_single_tile_n(tmp.local(), m, pivot, tidx, move(cmp));

				if (positive(pp.second)) {
					if (off.local().first != i) {
						_merge_3_way_partitions_in_tile(first + off.local().first,
														off.local().second - off.local().first,
														first + i,
														tmp.local(),
														pp.first,
														pp.second - pp.first,
														AMP_ALG forward<decltype(tidx)>(tidx));
					}
					_copy_in_tile_n(tmp.local(), m, first + i, tidx);
				}
				uniform_invoke(tidx, [=](auto&&) {
					off.local().first += pp.first;
					off.local().second += pp.second;
				});
			}
			_fill_in_tile_n(first + off.local().first,
							off.local().second - off.local().first,
							pivot,
							AMP_ALG forward<decltype(tidx)>(tidx));

			return AMP_ALG make_pair(first + off.local().first, first + off.local().second);
		}*/

		//template<typename I1, typename I2, typename T, typename C>
		//inline std::pair<I1, I1> _partition_tiles_3_way(I1 first,
		//											    I1 last,
		//											    I2 dest_first,
		//												const T& pivot,
		//												C cmp)
		//{
		//	const auto d = Execution_parameters::tiled_domain(last - first);
		//	const Difference_type<I1> w = Execution_parameters::work_per_tile(last - first);

		//	concurrency::parallel_for_each(d, [=, cmp = std::move(cmp)](auto&& tidx) restrict(amp) {
		//		const AMP_ALG Difference_type<I1> t = tidx.tile[0] * w;
		//		const AMP_ALG Difference_type<I1> n = AMP_ALG _min(w, last - first - t);

		//		const AMP_ALG pair<I1, I1> p = _partition_3_way_in_tile_n(first + t,
		//									 				              n,
		//																  pivot,
		//																  AMP_ALG forward<decltype(tidx)>(tidx),
		//																  AMP_ALG move(cmp));
		//		uniform_invoke(tidx, [=](auto&& tile) {
		//			dest_first[tile] = AMP_ALG make_pair(p.first - (first + t), p.second - (first + t));
		//		});
		//	});

		//	const auto pp_dx = _reduce(dest_first,
		//							   dest_first + rounded_up_quotient(last - first, w),
		//							   make_pair(Difference_type<I1>(0), Difference_type<I1>(0)),
		//							   [](auto&& x, auto&& y) restrict(amp) {
		//		return AMP_ALG make_pair(x.first + y.first, x.second + y.second);
		//	});

		//	return std::make_pair(first + pp_dx.first, first + pp_dx.second);
		//}

		//template<typename I1, typename I2>
		//inline void _merge_3_way_partitioned_tiles(I1 first1, I1 last1, I2 first2)
		//{

		//}

		//template<typename I, typename T, typename C>
		//inline std::pair<I, I> _partition_3_way(I first, I last, const T& pivot, C cmp)
		//{
		//	if (first == last) return std::make_pair(last, last);

		//	using Offsets = typename pair<Difference_type<I>, Difference_type<I>>;
		//	const auto tmp = Execution_parameters::temporary_buffer<Offsets>(last - first);

		//	const std::pair<I, I> pp = _partition_tiles_3_way(first, last, begin(tmp), pivot, move(cmp));
		//	if ((pp.first != first) && (pp.second != last)) {
		//		_merge_3_way_partitioned_tiles(first, last, cbegin(tmp));
		//	}

		//	return pp;
		//}

		template<typename I, typename C>
		inline bool _odd_even_sort_tiles_pass(I first, I last, C cmp)
		{
			if ((last - first) == Difference_type<I>(1)) return true;

			concurrency::array_view<atomic<unsigned int>> swaps(1);
			concurrency::parallel_for_each(swaps.extent, [=](auto&&) restrict(amp) { swaps[0] = 0u; });

			const auto compute_domain = Execution_parameters::tiled_domain(half_nonnegative(last - first));
			concurrency::parallel_for_each(compute_domain,
										   [=, cmp = std::move(cmp)](auto&& tidx) restrict(amp) {
				AMP_ALG Difference_type<I> t = AMP_ALG twice(tidx.tile_origin[0]);
				while (t < (last - first)) {
					const auto n = AMP_ALG _min(AMP_ALG twice(tidx.tile_dim0), last - first - t);

					tile_static AMP_ALG Value_type<I> vals[AMP_ALG twice(tidx.tile_dim0)];
					AMP_ALG _copy_in_tile_n(first + t, n, vals, AMP_ALG forward<decltype(tidx)>(tidx));

					const bool not_done = AMP_ALG _rank_sort_single_tile_n(vals,
																		   n,
																		   AMP_ALG forward<decltype(tidx)>(tidx),
																		   cmp);

					AMP_ALG _copy_in_tile_n(vals, n, first + t, AMP_ALG forward<decltype(tidx)>(tidx));

					if ((tidx.tile_origin == tidx.global) && not_done && AMP_ALG zero(swaps[0].load())) {
						++swaps[0];
					}

					t += AMP_ALG twice(compute_domain.size());
				}
			});
			return zero(swaps[0].load());
		}

		template<typename I, typename C>
		inline void _odd_even_sort_tiles_unguarded(I first, I last, C cmp)
		{
			bool sorted;
			do {
				sorted = _odd_even_sort_tiles_pass(first + 1, last, cmp); // Odd.
				sorted = _odd_even_sort_tiles_pass(first, last, cmp) && sorted; // Even.
			} while (!sorted);
		}

		template<typename I, typename C>
		inline void _partition_into_tiles(I first, I last, C cmp)
		{	// This is just a proof-of-concept. It is neither fast enough nor an optimal
			// implementation of Quicksort - will be overhauled for release.
			while ((1 << 16) < (last - first)) {
				const Value_type<I> pivot = _median_of_3_random_amp(first, last, cmp);
				const I l = _partition(first, last, [=](auto&& x) restrict(amp) { return cmp(x, pivot); });
				const I u = _partition(l, last, [=](auto&& x) restrict(amp) { return !cmp(pivot, x); });

				if ((l == first) && (u == last)) return;

				if ((l - first) < (last - u)) {
					_partition_into_tiles(first, l, cmp);
					first = u;
				}
				else {
					_partition_into_tiles(u, last, cmp);
					last = l;
				}
				std::cout << last - first << ' ';
			}
		//	std::cout << last - first << std::endl;
		}
		template<typename I, typename C, int tsz>
		inline bool _rank_sort_single_tile_n(I first,
											 Difference_type<I> n,
											 const concurrency::tiled_index<tsz>& tidx,
											 C cmp) restrict(amp)
		{
			tile_exclusive_cache<Value_type<I>[tidx.tile_dim0], tidx.tile_dim0> tmp(tidx, [=](auto&& out) {
				_copy_in_tile_n(first, n, AMP_ALG forward<decltype(out)>(out), tidx);
			});

			tile_exclusive_cache<unsigned int[tidx.tile_dim0], tidx.tile_dim0> uns(tidx, [=](auto&& out){
				AMP_ALG _fill_in_tile_n(AMP_ALG forward<decltype(out)>(out), tidx.tile_dim0, 0u, tidx);
			});

			if (tidx.local[0] < n) {
				Difference_type<I> dx = 0;
				for (auto i = 0; i != n; ++i) {
					if (!cmp(tmp[tidx.local[0]], tmp[i])) {
						if (cmp(tmp[i], tmp[tidx.local[0]])) ++dx;
						else dx += i < tidx.local[0];
					}
				}

				uns[tidx.local[0]] += dx != tidx.local[0];
				if (tidx.local[0] != dx) {
					first[dx] = tmp[tidx.local[0]];
				}
			}
			return uns.reduce(amp_algorithms::plus<>()) != 0u;
		}

		template<typename I, typename C, int tsz>
		inline bool _pbsn_sort_single_tile_n(I first,
											 Difference_type<I> n,
											 const concurrency::tiled_index<tsz>& tidx,
											 C cmp) restrict(amp)
		{
			tile_exclusive_cache<Value_type<I>[AMP_ALG twice(tidx.tile_dim0)], tidx.tile_dim0> tmp(tidx, [=](auto&& out) {
				AMP_ALG _copy_in_tile_n(first, n, AMP_ALG forward<decltype(out)>(out), tidx);
			});
			tile_exclusive_cache<unsigned int[tidx.tile_dim0], tidx.tile_dim0> uns(tidx, [=](auto&& out) {
				AMP_ALG _fill_in_tile_n(AMP_ALG forward<decltype(out)>(out), tidx.tile_dim0, 0u, tidx);
			});

			static_for<AMP_ALG twice(tidx.tile_dim0), 0, Inc::div, 2>()([=](auto&&) {
				AMP_ALG static_for<AMP_ALG twice(tidx.tile_dim0), 1, AMP_ALG Inc::div, 2>()([=](auto&& h) {
					const unsigned int mask = AMP_ALG predecessor(h);
					const AMP_ALG Difference_type<I> half_h = AMP_ALG half_nonnegative(h);
					const AMP_ALG Difference_type<I> b_log = AMP_ALG binary_logarithm(half_h);

					const AMP_ALG Difference_type<I> x0 = AMP_ALG even_division_remainder(tidx.local[0], half_h) +
														  (tidx.local[0] >> b_log) * h;
					const AMP_ALG Difference_type<I> x1 = x0 ^ mask;
					if (AMP_ALG zero(x0 & half_h) && x1 < n) {
						uns[tidx.local[0]] += AMP_ALG _compare_and_exchange(first[x0], first[x1], cmp);
					}
				});
			});

			return uns.reduce(amp_algorithms::plus<>()) != 0u;
		}

		template<typename I, typename C>
		inline void _sort_tiles(I first, I last, Reference<unsigned int> swapped, C cmp)
		{
			if ((first == last) || (first == std::prev(last))) return;

			const auto d = Execution_parameters::tiled_domain(last - first);
			const Difference_type<I> w = Execution_parameters::work_per_tile(last - first);

			concurrency::parallel_for_each(d, [=, cmp = std::move(cmp)](auto&& tidx) restrict(amp) {
				const AMP_ALG Difference_type<I> t = tidx.tile[0] * w;
				const AMP_ALG Difference_type<I> n = AMP_ALG _min(w, last - first - t);
				for (AMP_ALG Difference_type<I> i = 0; i < n; i += 2 * tidx.tile_dim0) {
					const AMP_ALG Difference_type<I> m = AMP_ALG _min(2 * tidx.tile_dim0 * 1, n - i);
					const bool uns = _pbsn_sort_single_tile_n(first + t + i,
															  m,
															  AMP_ALG forward<decltype(tidx)>(tidx),
															  cmp);
					if (uns) uniform_invoke(tidx, [=](auto&&) { swapped.get() = 1u; });
				}
			});
		}

		template<typename I, typename C, int tsz>
		inline bool _pbsn_sort_step_in_tile_n(I first,
											  Difference_type<I> b,
											  Difference_type<I> m,
											  Difference_type<I> n,
											  Difference_type<I> h,
											  const concurrency::tiled_index<tsz>& tidx,
											  C cmp) restrict(amp)
		{
			static constexpr Difference_type<I> unroll = 4;

			tile_exclusive_cache<unsigned int[tidx.tile_dim0], tidx.tile_dim0> uns(tidx, [=](auto&& out) {
				AMP_ALG _fill_in_tile_n(AMP_ALG forward<decltype(out)>(out), tidx.tile_dim0, 0u, tidx);
			});

			const Difference_type<I> mask = h - 1;
			const Difference_type<I> half_h = half_nonnegative(h);
			const Difference_type<I> b_log = binary_logarithm(half_h);

			for (Difference_type<I> i = unroll * tidx.local[0]; i < m; i += unroll * tidx.tile_dim0) {
				static_for<0u, unroll>()([=](auto&& j) {
						const AMP_ALG Difference_type<I> x0 = AMP_ALG even_division_remainder(b + i + j, half_h) +
															  ((b + i + j) >> b_log) * h;
						const AMP_ALG Difference_type<I> x1 = x0 ^ mask;
						uns[tidx.local[0]] += x1 < n ? AMP_ALG _compare_and_exchange(first[x0], first[x1], cmp)
													 : 0u;
				});
			}

			return uns.reduce(amp_algorithms::plus<>()) != 0u;
		}

		template<typename I1, typename I2, typename C>
		inline void _pbsn_sort_outer(I1 first, I1 last, I2 dest_first, C cmp)
		{
			if (first == last || first == std::prev(last)) return;

			const Difference_type<I1> next_pot_sz = round_up_to_next_binary_power(last - first);
			const auto d = Execution_parameters::tiled_domain(half_nonnegative(next_pot_sz));
			const Difference_type<I1> w = Execution_parameters::work_per_tile(half_nonnegative(next_pot_sz));

			Difference_type<I1> h = next_pot_sz;
			while (h > round_up_to_next_binary_power(w)) {
				concurrency::parallel_for_each(d, [=](auto&& tidx) restrict(amp) {
					const AMP_ALG Difference_type<I1> t = tidx.tile[0] * w;
					const AMP_ALG Difference_type<I1> n = AMP_ALG _min(w, last - first - t);

					if (_pbsn_sort_step_in_tile_n(first, t, n, last - first, h, tidx, cmp)) {
						AMP_ALG uniform_invoke(tidx, [=](auto&& tile) { dest_first[tile] = 1u; });
					}
				});
				h = half_nonnegative(h);
			}
		}

		template<typename I1, typename I2, typename C>
		inline void _pbsn_sort_inner(I1 first, I1 last, I2 dest_first, C cmp)
		{
			const auto d = Execution_parameters::tiled_domain(half_nonnegative(round_up_to_next_binary_power(last - first)));
			const Difference_type<I1> w = Execution_parameters::work_per_tile(half_nonnegative(round_up_to_next_binary_power(last - first)));

			concurrency::parallel_for_each(d, [=](auto&& tidx) restrict(amp) {
				AMP_ALG Difference_type<I1> h = AMP_ALG round_up_to_next_binary_power(w);
				const AMP_ALG Difference_type<I1> t = tidx.tile[0] * h;
				if (t < (last - first)) {
					const AMP_ALG Difference_type<I1> n = AMP_ALG _min(h, last - first - t);

					bool unsorted = false;
					while (h > 1) {
						unsorted = _pbsn_sort_step_in_tile_n(first, t, n, last - first, h, tidx, cmp) || unsorted;
						h = AMP_ALG half_nonnegative(h);
					}

					if (unsorted) AMP_ALG uniform_invoke(tidx, [=](auto&& tile) { dest_first[tile] = 1u; });
				}
			});
		}

		template<typename I, typename C>
		inline bool _pbsn_sort(I first, I last, C cmp)
		{
			if (first == last || first == std::prev(last)) return true;

			const auto tmp = Execution_parameters::temporary_buffer(half_nonnegative(round_up_to_next_binary_power(last - first)), 0u);

			_pbsn_sort_outer(first, last, begin(tmp), cmp);
			_pbsn_sort_inner(first, last, begin(tmp), cmp);

			return _reduce(cbegin(tmp), cend(tmp), 0u, amp_algorithms::plus<>()) == 0u;
		}

		template<typename T, typename C>
		inline bool _compare_and_exchange(T& x, T& y, C cmp) restrict(cpu, amp)
		{
			if (cmp(y, x)) { _swap(x, y); return true; }
			return false;
		}

		template<typename I, typename C>
		inline void _odd_even_sort_sorted_tiles(I first, I last, C cmp)
		{
			if (first == last || first == std::prev(last)) return;

		/*	const auto tmp = Execution_parameters::temporary_buffer<unsigned int>(last - first);
			std::fill(begin(tmp), end(tmp), 0u);
			const auto d = Execution_parameters::tiled_domain(last - first);
			const Difference_type<I> w = Execution_parameters::work_per_tile(last - first);*/
			const concurrency::array_view<atomic<unsigned int>> foo(1);
			do {
				foo[0] = 0u;

				const concurrency::extent<1> d(last - first);
				// Even.
				concurrency::parallel_for_each(d, [=](auto&& idx) restrict(amp) {
					const AMP_ALG Difference_type<I> x0 = idx[0] * 2;
					const AMP_ALG Difference_type<I> x1 = x0 + 1;
					if (x1 < (last - first)) {
						const bool bs = AMP_ALG _compare_and_exchange(first[x0], first[x1], cmp);
						if (bs) ++foo[0];
					}
					//	}
				});

				// Odd.
				concurrency::parallel_for_each(d, [=](auto&& idx) restrict(amp) {
					const AMP_ALG Difference_type<I> x0 = idx[0] * 2 + 1;
					const AMP_ALG Difference_type<I> x1 = x0 + 1;
					if (x1 < (last - first)) {
						const bool bs = AMP_ALG _compare_and_exchange(first[x0], first[x1], cmp);
						if (bs) ++foo[0];
					}
				});

				std::cout << foo[0] << ' ';
			} while (foo[0].load());
			std::cout << std::endl;
		}

		template<typename I, typename C>
		inline void _sort(I first, I last, C cmp)
		{
			if ((first == last) || first == std::prev(last)) return;
			bool sorted;
			//unsigned int i = 0u;
			do {
				sorted = _pbsn_sort(first, last, cmp);
				//++i;
			} while (!sorted);
			//if (i != binary_logarithm(round_up_to_next_binary_power(last - first))) std::cout << "Early out " << i << std::endl;
		}

		template<typename I>
		inline void _sort(I first, I last)
		{
			_sort(first, last, amp_algorithms::less<>());
		}

		template<typename I, typename C>
		inline void _partial_sort(I first, I middle, I last, C cmp)
		{
			_nth_element(first, middle, last, cmp);
			_sort(first, middle, move(cmp));
		}

		template<typename I>
		inline void _partial_sort(I first, I middle, I last)
		{
			_partial_sort(first, middle, last, amp_algorithms::less<>());
		}

		//----------------------------------------------------------------------------
		// swap, swap<T, N>, swap_ranges, iter_swap
		//----------------------------------------------------------------------------

		template<typename T>
		void _swap(T& x, T& y) restrict(cpu, amp)
		{
			T t = move(x);
			x = move(y);
			y = move(t);
		}

		// TODO: Are there other overloads of swap() that should be implemented.
		template<typename T, int N>
		inline void _swap(T (&x)[N], T (&y)[N]) restrict(cpu, amp)
		{
			const concurrency::array_view<T> xa(N, &x[0]);
			const concurrency::array_view<T> ya(N, &y[0]);
			static_for<0u, N>()([=](auto&& i) {
				_swap(xa[forward<decltype(i)>(i)], ya[forward<decltype(i)>(i)]);
			});
		}

		template<typename I1, typename I2, int tsz>
		inline pair<Difference_type<I1>,
					Difference_type<I2>> _swap_ranges_in_tile_n(I1 first1,
													            Difference_type<I1> n1,
																I2 first2,
																Difference_type<I2> n2,
																const concurrency::tiled_index<tsz>& tidx) restrict(amp)
		{
			if (!positive(n1) || !positive(n2)) return make_pair(Difference_type<I1>(0), Difference_type<I2>(0));

			const Difference_type<I1> n = _min(n1, n2);
			for (Difference_type<I1> i = tidx.local[0]; i < n; i += tidx.tile_dim0) {
				_iter_swap(first1 + i, first2 + i);
			}
			return make_pair(n, n);
		}

		template<typename I1, typename I2>
		inline I2 _swap_ranges(I1 first1, I1 last1, I2 first2)
		{
			concurrency::parallel_for_each(concurrency::extent<1>(last1 - first1), [=](auto&& idx) restrict(amp) {
				_iter_swap(first1 + idx[0], first2 + idx[0]);
			});

			return first2 + (last1 - first1);
		}

		template<typename I1, typename I2>
		inline void _iter_swap(I1 i0, I2 i1) restrict(cpu, amp)
		{
			_swap(*i0, *i1);
		}

		//----------------------------------------------------------------------------
		// transform (Unary)
		//----------------------------------------------------------------------------

		// The "UnaryFunction" functor needs to be callable as "func(ConstRandomAccessIterator::value_type)".
		// The functor needs to be blittable and cannot contain any array, array_view, or textures.

		template<typename I1, typename I2, typename Unary_op, int tsz>
		inline I2 _transform_single_tile_n(I1 first,
										   Difference_type<I1> n,
										   I2 dest_first,
										   const concurrency::tiled_index<tsz>& tidx,
										   Unary_op op) restrict(amp)
		{
			if (!positive(n)) return dest_first;

			if (tidx.local[0] < n) {
				dest_first[tidx.local[0]] = op(first[tidx.local[0]]);
			}

			return dest_first + n;
		}

		template<typename I1, typename I2, typename I3, typename Binary_op, int tsz>
		inline I3 _transform_single_tile_n(I1 first1,
										   Difference_type<I1> n,
										   I2 first2,
										   I3 dest_first,
										   const concurrency::tiled_index<tsz>& tidx,
										   Binary_op op) restrict(amp)
		{
			if (!positive(n)) return dest_first;

			if (tidx.local[0] < n) {
				dest_first[tidx.local[0]] = op(first1[tidx.local[0]], first2[tidx.local[0]]);
			}

			return dest_first + n;
		}

		template<typename I1, typename I2, typename Unary_op, int tsz>
		inline I2 _transform_in_tile_n(I1 first,
									   Difference_type<I1> n,
									   I2 dest_first,
									   const concurrency::tiled_index<tsz>& tidx,
									   Unary_op op) restrict(amp)
		{
			if (!positive(n)) return dest_first;

			for (Difference_type<I1> i = 0; i < n; i += tidx.tile_dim0) {
				const Difference_type<I1> m = _min(tidx.tile_dim0 * 1, n - i);
				dest_first = _transform_single_tile_n(first + i, m, dest_first, tidx, op);
			}

			return dest_first;
		}

		template<typename I1, typename I2, typename I3, typename Binary_op, int tsz>
		inline I3 _transform_in_tile_n(I1 first1,
									   Difference_type<I1> n,
									   I2 first2,
									   I3 dest_first,
									   const concurrency::tiled_index<tsz>& tidx,
									   Binary_op op) restrict(amp)
		{
			if (!positive(n)) return dest_first;

			for (Difference_type<I1> i = 0; i < n; i += tidx.tile_dim0) {
				const Difference_type<I1> m = _min(tidx.tile_dim0 * 1, n - i);
				dest_first = _transform_single_tile_n(first1 + i, m, first2 + i, dest_first, tidx, op);
			}

			return dest_first;
		}

		template<typename I1, typename I2, typename Unary_op>
		inline I2 _transform(I1 first, I1 last, I2 dest_first, Unary_op op)
		{
			if (first == last) return dest_first;

			concurrency::parallel_for_each(concurrency::extent<1>(last - first),
										   [=, op = std::move(op)](auto&& idx) restrict(amp) {
				dest_first[idx[0]] = op(first[idx[0]]);
			});

			return dest_first + (last - first);
		}

		// The "BinaryFunction" functor needs to be callable as "func(ConstRandomAccessIterator1::value_type,
		// ConstRandomAccessIterator2::value_type)". The functor needs to be blittable and cannot
		// contain any array, array_view, or textures.

		template<typename I1, typename I2, typename I3, typename Binary_op>
		inline I3 _transform(I1 first1, I1 last1, I2 first2, I3 dest_first, Binary_op op)
		{
			if (first1 == last1) return dest_first;

			concurrency::parallel_for_each(concurrency::extent<1>(last1 - first1),
										   [=, op = std::move(op)](auto&& idx) restrict(amp) {
				dest_first[idx[0]] = op(first1[idx[0]], first2[idx[0]]);
			});

			return dest_first + (last1 - first1);
		}

		//----------------------------------------------------------------------------
		// unique, unique_copy
		//----------------------------------------------------------------------------

		template<typename I, typename P, int tsz>
		inline I _unique_single_tile_n(I first,
									   Difference_type<I> n,
									   const concurrency::tiled_index<tsz>& tidx,
									   P p) restrict(amp)
		{	// This assumes that n <= successor(tidx.tile_dim0)
			tile_static Difference_type<I> tile_dx;
			if (tidx.tile_origin == tidx.global) {
				tile_dx = Difference_type<I>(0);
			}

			if (successor(tidx.local[0]) < n) {
				const Value_type<I> x = *(first + tidx.local[0]);
				const Value_type<I>	y = *(first + successor(tidx.local[0]));

				if (!p(x, y)) {
					first[concurrency::atomic_fetch_inc(&tile_dx)] = y;
				}
			}

			return first + tile_dx;
		}

		template<typename I, typename P, int tsz>
		inline I _unique_in_tile_n(I first,
								   Difference_type<I> n,
								   const Value_type<I>& x0,
								   const concurrency::tiled_index<tsz>& tidx,
								   P p) restrict(amp)
		{
			tile_static Difference_type<I> tile_dx;
			if (tidx.tile_origin == tidx.global) {
				tile_dx = Difference_type<I>(0);
			}

			tile_static Value_type<I> tmp[successor(tidx.tile_dim0)];

			for (Difference_type<I> i = 0; i < n; i += tidx.tile_dim0) {
				if (tidx.tile_origin == tidx.global) {
					tmp[0] = positive(i) ? tmp[tidx.tile_dim0] : x0;
				}
				const auto m = _min(tidx.tile_dim0 * 1, n - i);
				_copy_in_tile_n(first + i, m, tmp + 1, tidx);

				const auto last_unique = _unique_single_tile_n(tmp, successor(m), tidx, p);
				if (last_unique != tmp) {
					_copy_in_tile(tmp, last_unique, first + tile_dx, tidx);
					if (tidx.tile_origin == tidx.global) {
						tile_dx += last_unique - tmp;
					}
				}
			}

			return first + tile_dx;
		}

		template<typename I, typename P>
		inline I _unique(I first, I last, P p)
		{	// TODO: cleanup and optimisation.
			if (first == last) return first;
			if ((last - first) == Difference_type<I>(1)) return last;

			const Difference_type<I> work_per_tile = Execution_parameters::work_per_tile(predecessor(last - first));

			const auto dx = Execution_parameters::temporary_buffer(predecessor(last - first), last - first);
			const auto tmp = Execution_parameters::temporary_buffer<Value_type<I>>(predecessor(last - first));
			concurrency::parallel_for_each(tmp.extent, [=](auto&& idx) restrict(amp) {
				const Difference_type<I> t = successor(idx[0] * work_per_tile);
				tmp[idx] = first[predecessor(t)];
			});

			const auto compute_domain = Execution_parameters::tiled_domain(predecessor(last - first));
			concurrency::parallel_for_each(compute_domain, [=, p = std::move(p)](auto&& tidx) restrict(amp) {
				const Difference_type<I> t = successor(tidx.tile[0] * work_per_tile);
				const auto n = _min(work_per_tile, last - first - t);

				I last_unique = _unique_in_tile_n(first + t, n, tmp[tidx.tile], tidx, move(p));

				if (positive(tidx.tile[0])) {
					tile_static Difference_type<I> tile_dx;
					if (tidx.tile_origin == tidx.global) {
						do {
							tile_dx = _uncached_load(&dx[predecessor(tidx.tile[0])]);
						} while (tile_dx == (last - first));
					}

					if ((last_unique != (first + t)) && (tile_dx != t)) {
						_copy_in_tile(last_unique - _min(last_unique - (first + t),t - tile_dx),
									  last_unique,
									  first + tile_dx,
									  tidx);
					}
					last_unique = first + tile_dx + (last_unique - (first + t));
				}

				if (tidx.tile_origin == tidx.global) {
					dx[tidx.tile] = last_unique - first;
				}
			});
			return first + dx[predecessor(dx.extent.size())];
		}

		template<typename I>
		inline I _unique(I first, I last)
		{
			return _unique(first, last, amp_algorithms::equal_to<>());
		}

		template<typename I1, typename I2, typename P, int tsz>
		inline I2 _unique_copy_in_tile_n(I1 first,
										 Difference_type<I1> n,
										 I2 dest_first,
										 Difference_type<I2>& dx,
										 const concurrency::tiled_index<tsz>& tidx,
										 P p) restrict(amp)
		{
			tile_static Difference_type<I2> tile_dx;
			if (tidx.tile_origin == tidx.global) {
				tile_dx = Difference_type<I2>(0);
			}

			tile_static Value_type<I2> tmp[successor(tidx.tile_dim0)];

			for (Difference_type<I1> i = 1; i < n; i += tidx.tile_dim0) {
				const auto m = _min(successor(tidx.tile_dim0), n - predecessor(i));
				_copy_in_tile_n(first + predecessor(i), m, tmp, tidx);

				const auto last_unique = _unique_single_tile_n(tmp, m, tidx, p);
				if (last_unique != tmp) {
					if (tidx.tile_origin == tidx.global) {
						tile_dx = concurrency::atomic_fetch_add(&dx, last_unique - tmp);
					}

					_copy_in_tile(tmp, last_unique, dest_first + tile_dx, tidx);
				}
			}

			return dest_first + tile_dx;
		}

		template<typename I1, typename I2, typename P>
		inline I2 _unique_copy(I1 first, I1 last, I2 dest_first, P p)
		{
			if (first == last) return dest_first;
			if (std::next(first) == last) return amp_stl_algorithms::copy(first, last, dest_first);

			concurrency::array_view<Difference_type<I2>> off(1);
			concurrency::parallel_for_each(off.extent, [=](auto&&) restrict(amp) {
				*dest_first = *first;
				off[0] = Difference_type<I2>(1);
			});

			const auto compute_domain = Execution_parameters::tiled_domain(predecessor(last - first));
			const Difference_type<I1> work_per_tile = Execution_parameters::work_per_tile(predecessor(last - first));

			concurrency::parallel_for_each(compute_domain, [=, p = std::move(p)](auto&& tidx) restrict(amp) {
				const Difference_type<I1> t = successor(tidx.tile[0] * work_per_tile); // Offset by 1 since we already populated *dest_first.
				const Difference_type<I1> n = _min(successor(work_per_tile), last - first - predecessor(t));
				_unique_copy_in_tile_n(first + predecessor(t), n, dest_first, off[0], tidx, move(p)); // Grab final element of prior block, correct length for the offsetting.
			});

			return dest_first + off[0];
		}

		template<typename I1, typename I2>
		inline I2 _unique_copy(I1 first, I1 last, I2 dest_first)
		{
			return _unique_copy(first, last, dest_first, amp_algorithms::equal_to<>());
		}
	}	   // namespace amp_stl_algorithms
}
#endif // _XX_AMP_STL_ALGORITHMS_IMPL_INL_H_BUMPTZI