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

#include <functional>
#include <numeric>

#include <amp_stl_algorithms.h>
#include <amp_algorithms.h>
#include <xx_amp_algorithms_impl_inl.h>

namespace amp_stl_algorithms
{
    namespace _details
    {
        template<class ConstRandomAccessIterator>
        static inline decltype(auto) create_section(ConstRandomAccessIterator iter, Difference_type<ConstRandomAccessIterator> dist)
        {
            auto base_view = _details::array_view_iterator_helper<ConstRandomAccessIterator>::get_base_array_view(iter);
            return base_view.section(concurrency::index<1>(iter - begin(base_view)), concurrency::extent<1>(dist));
        }

        class Execution_parameters {
            static constexpr int tsz = 256;
            static constexpr int max_tiles = 65535; // DX limitation.
            static constexpr int max_lanes = max_tiles * tsz;
        public:
            static constexpr int tile_size() { return tsz; };
            static concurrency::tiled_extent<tsz> tiled_domain(unsigned int sz) restrict(cpu, amp) {
                return concurrency::extent<1>(sz < max_lanes ? sz : max_lanes).tile<tsz>().pad();
            };
            static constexpr inline unsigned int tile_cnt(unsigned int sz) restrict(cpu, amp) {
                return sz / tsz + (sz % tsz ? 1u : 0u);
            };
        };
    }

	//----------------------------------------------------------------------------
	// Computational bases for types
	//----------------------------------------------------------------------------

	template<typename T, std::enable_if_t<std::is_integral<T>::value>* = nullptr>
	static inline constexpr T successor(T x) restrict(cpu, amp)
	{
		return x + T(1);
	}
	template<typename T, std::enable_if_t<std::is_integral<T>::value>* = nullptr>
	static inline constexpr T predecessor(T x) restrict(cpu, amp)
	{
		return x - T(1);
	}
	template<typename T, std::enable_if_t<std::is_integral<T>::value>* = nullptr>
	static inline constexpr T twice(T x) restrict(cpu, amp)
	{
		return x + x;
	}
	template<typename T, std::enable_if_t<std::is_integral<T>::value>* = nullptr>
	static inline constexpr T half_nonnegative(T x) restrict(cpu, amp)
	{
		return x / T(2);
	}
	template<typename T, std::enable_if_t<std::is_integral<T>::value>* = nullptr>
	static inline constexpr T binary_scale_down_nonnegative(T x, T k) restrict(cpu, amp)
	{
		while (k) {
			x = half_nonnegative(x);
			k = predecessor(k);
		}
		return x;
	}
	template<typename T, std::enable_if_t<std::is_integral<T>::value>* = nullptr>
	static inline constexpr T binary_scale_up_nonnegative(T x, T k) restrict(cpu, amp)
	{
		while (k) {
			x = twice(x);
			k = half_nonnegative(x);
		}
		return x;
	}
	template<typename T, std::enable_if_t<std::is_integral<T>::value>* = nullptr>
	static inline constexpr bool positive(T x) restrict(cpu, amp)
	{
		return x > T(0);
	}
	template<typename T, std::enable_if_t<std::is_integral<T>::value>* = nullptr>
	static inline constexpr bool negative(T x) restrict(cpu, amp)
	{
		return x < T(0);
	}
	template<typename T, std::enable_if_t<std::is_integral<T>::value>* = nullptr>
	static inline constexpr bool zero(T x) restrict(cpu, amp)
	{
		return x == T(0);
	}
	template<typename T, std::enable_if_t<std::is_integral<T>::value>* = nullptr>
	static inline constexpr bool one(T x) restrict(cpu, amp)
	{
		return x == T(1);
	}
	template<typename T, std::enable_if_t<std::is_integral<T>::value>* = nullptr>
	static inline constexpr bool even(T x) restrict(cpu, amp)
	{
		return (x % T(2)) == T(0);
	}
	template<typename T, std::enable_if_t<std::is_integral<T>::value>* = nullptr>
	static inline constexpr bool odd(T x) restrict(cpu, amp)
	{
		return !even(x);
	}

    // TODO: Get the tests, header and internal implementations into the same logical order.
    // TODO: Lots of the algorithms that typically do a small amount of work per thread should use tiling to save the runtime overhead of having to do this.

    //----------------------------------------------------------------------------
    // adjacent_difference
    //----------------------------------------------------------------------------

    template<typename ConstRandomAccessIterator,typename RandomAccessIterator, typename BinaryOperation>
    inline RandomAccessIterator adjacent_difference(ConstRandomAccessIterator first, ConstRandomAccessIterator last, RandomAccessIterator dest_first, BinaryOperation&& op)
    {
        if (first == last) return dest_first;

		const auto compute_domain = _details::Execution_parameters::tiled_domain(last - first);
        concurrency::parallel_for_each(compute_domain, [=, op = std::forward<BinaryOperation>(op)](auto&& tidx) restrict(amp) {
			tile_static Value_type<RandomAccessIterator> vals[tidx.tile_dim0 + 1];
			for (decltype(last - first) i = tidx.tile_origin[0]; i < (last - first); i += compute_domain.size()) {
				const auto n = amp_algorithms::min<>()(tidx.tile_dim0 * 1, last - first - i - 1);

				if (tidx.local[0] < n) {
					vals[tidx.local[0] + 1] = *(first + i + tidx.local[0] + 1);
				}
				if (tidx.tile_origin == tidx.global) {
					vals[0] = *(first + i);
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

				if (tidx.local[0] < n) {
					*(dest_first + i + tidx.local[0] + 1) = op(vals[tidx.local[0] + 1], vals[tidx.local[0]]);
				}
				if (positive(tidx.global[0])) {
					*dest_first = *first;
				}
			}
        });

        return dest_first + (last - first);
    }

    template<typename ConstRandomAccessIterator,typename RandomAccessIterator>
    inline RandomAccessIterator adjacent_difference(ConstRandomAccessIterator first, ConstRandomAccessIterator last, RandomAccessIterator dest_first)
    {
        return amp_stl_algorithms::adjacent_difference(first, last, dest_first, amp_algorithms::minus<>());
    }

    //----------------------------------------------------------------------------
    // all_of, any_of, none_of
    //----------------------------------------------------------------------------

    template<typename ConstRandomAccessIterator,  typename UnaryPredicate>
    inline bool all_of(ConstRandomAccessIterator first, ConstRandomAccessIterator last, UnaryPredicate&& p)
    {
		return !amp_stl_algorithms::any_of(first, last, [p = std::forward<UnaryPredicate>(p)](auto&& v) restrict(amp) { return !p(v); });
    }

    // Non-standard, OutputIterator must yield an int reference, where the result will be
    // stored. This allows the function to eschew synchronization
    template<typename ConstRandomAccessIterator, typename UnaryPredicate, typename RandomIterator>
    inline void any_of_impl(ConstRandomAccessIterator first, ConstRandomAccessIterator last, RandomIterator dest_first, UnaryPredicate&& p)
    {
        concurrency::parallel_for_each(concurrency::extent<1>(1), [=](auto&&) restrict(amp) { *dest_first = 0; });

		const auto compute_domain = _details::Execution_parameters::tiled_domain(last - first);
		concurrency::parallel_for_each(compute_domain, [=, p = std::forward<UnaryPredicate>(p)](auto&& tidx) restrict(amp) {
			for (auto i = tidx.tile_origin[0]; i < (last - first); i += compute_domain.size()) {
				tile_static bool early_out;
				if (tidx.tile_origin == tidx.global) {
					early_out = positive(*dest_first);
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

				if (early_out) return;

				tile_static Value_type<RandomIterator> cnt;
				if (tidx.tile_origin == tidx.global) cnt = 0u;

				tidx.barrier.wait_with_tile_static_memory_fence();

				const auto p_val = ((first + i + tidx.local[0]) < last) ? p(*(first + i + tidx.local[0])) : false;
				if (p_val) {
					concurrency::atomic_fetch_inc(&cnt);
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

				if (positive(cnt)) {
					if (tidx.tile_origin == tidx.global) {
						concurrency::atomic_fetch_inc(&*dest_first);
					}
					return;
				}
			}
		});
    }

    // Standard, builds of top of the non-standard async version above, and adds a sync to
    // materialize the result.
    template<typename ConstRandomAccessIterator, typename UnaryPredicate>
    inline bool any_of(ConstRandomAccessIterator first, ConstRandomAccessIterator last, UnaryPredicate&& p)
    {
		if (first == last) return false;

        concurrency::array_view<unsigned int> found_any_av(1);
        amp_stl_algorithms::any_of_impl(first, last, amp_stl_algorithms::begin(found_any_av), std::forward<UnaryPredicate>(p));
        return positive(found_any_av[0]);
    }

    template<typename ConstRandomAccessIterator, typename UnaryPredicate>
    inline bool none_of(ConstRandomAccessIterator first, ConstRandomAccessIterator last, UnaryPredicate&& p)
    {
        return !amp_stl_algorithms::any_of(first, last, std::forward<UnaryPredicate>(p));
    }

    //----------------------------------------------------------------------------
    // copy, copy_if, copy_n
    //----------------------------------------------------------------------------

    template<typename ConstRandomAccessIterator, typename RandomAccessIterator>
    inline RandomAccessIterator copy(ConstRandomAccessIterator first,  ConstRandomAccessIterator last, RandomAccessIterator dest_first)
    {
        if (first == last) return dest_first;

        concurrency::copy(_details::create_section(first, last - first), dest_first);
        return dest_first + (last - first);
    }

    template<typename ConstRandomAccessIterator, typename RandomAccessIterator, typename UnaryPredicate>
    inline RandomAccessIterator copy_if(ConstRandomAccessIterator first, ConstRandomAccessIterator last, RandomAccessIterator dest_first, UnaryPredicate&& pred)
    {
        if (first == last) return dest_first;

        concurrency::array_view<Difference_type<ConstRandomAccessIterator>> off(1);
        concurrency::parallel_for_each(off.extent, [=](auto&&) restrict(amp) { off[0] = 0u; });

        const auto compute_domain = _details::Execution_parameters::tiled_domain(last - first);
        concurrency::parallel_for_each(compute_domain, [=, pred = std::forward<UnaryPredicate>(pred)](auto&& tidx) restrict(amp) {
			for (Difference_type<ConstRandomAccessIterator> i = tidx.tile_origin[0]; i < (last - first); i += compute_domain.size()) {
				const auto n = amp_stl_algorithms::min(tidx.tile_dim0 * 1, last - first - i);

				tile_static Value_type<ConstRandomAccessIterator> vals[tidx.tile_dim0];
				_copy_single_tile(first + i, first + i + n, vals, tidx);

				tile_static Difference_type<ConstRandomAccessIterator> tile_off;
				if (tidx.tile_origin == tidx.global) {
					tile_off = Difference_type<ConstRandomAccessIterator>(0);
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

				Difference_type<ConstRandomAccessIterator> o(0);
				if ((tidx.local[0] < n) && pred(vals[tidx.local[0]])) {
					o = concurrency::atomic_fetch_inc(&tile_off);
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

				if (zero(tile_off)) continue;

				tidx.barrier.wait_with_tile_static_memory_fence();

				if (tidx.tile_origin == tidx.global) {
					tile_off = concurrency::atomic_fetch_add(&off[0], tile_off);
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

				if ((tidx.local[0] < n) && pred(vals[tidx.local[0]])) {
					*(dest_first + tile_off + o) = vals[tidx.local[0]];
				}
			}
        });

        return dest_first + off[0];
    }

    template<typename ConstRandomAccessIterator, typename Size, typename RandomAccessIterator>
    inline RandomAccessIterator copy_n(ConstRandomAccessIterator first, Size count, RandomAccessIterator dest_first)
    {
        // copy() will handle the case where count == 0.
        return amp_stl_algorithms::copy(first, first + count, dest_first);
    }

    //----------------------------------------------------------------------------
    // count, count_if
    //----------------------------------------------------------------------------

    template<typename ConstRandomAccessIterator, typename T>
    inline Difference_type<ConstRandomAccessIterator> count(ConstRandomAccessIterator first, ConstRandomAccessIterator last, T&& value)
    {
        return amp_stl_algorithms::count_if(first, last, [value = std::forward<T>(value)](auto&& v) restrict(amp) { return v == value; });
    }

    template<typename ConstRandomAccessIterator, typename UnaryPredicate>
    inline Difference_type<ConstRandomAccessIterator> count_if(ConstRandomAccessIterator first, ConstRandomAccessIterator last, UnaryPredicate&& p)
    {
        if (first == last) return last - first;

        concurrency::array_view<Difference_type<ConstRandomAccessIterator>> cnt(1);
        concurrency::parallel_for_each(cnt.extent, [=](auto&& idx) restrict(amp) { cnt[idx] = 0; });

        const auto compute_domain = _details::Execution_parameters::tiled_domain(last - first);
        concurrency::parallel_for_each(compute_domain, [=, p = std::forward<UnaryPredicate>(p)] (auto&& tidx) restrict (amp) {
            Difference_type<ConstRandomAccessIterator> c(0);
            for (auto i = tidx.global[0]; i < (last - first); i += compute_domain.size()) {
                if (p(*(first + i))) ++c;
            }

            tile_static Difference_type<ConstRandomAccessIterator> cs;
			if (tidx.tile_origin == tidx.global) {
				cs = Difference_type<ConstRandomAccessIterator>(0);
			}

			tidx.barrier.wait_with_tile_static_memory_fence();

			if (positive(c)) {
				concurrency::atomic_fetch_add(&cs, c);
			}

            tidx.barrier.wait_with_tile_static_memory_fence();

            if ((tidx.tile_origin == tidx.global) && positive(cs)) {
				concurrency::atomic_fetch_add(&cnt[0], cs);
			}
        });

        return cnt[0];
    }

    //----------------------------------------------------------------------------
    // equal, equal_range
    //----------------------------------------------------------------------------

    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2, typename BinaryPredicate>
    inline bool equal(ConstRandomAccessIterator1 first1, ConstRandomAccessIterator1 last1, ConstRandomAccessIterator2 first2, BinaryPredicate&& p)
    {
        if (first1 == last1) return true;

        const concurrency::array_view<unsigned int> neq(1);
        concurrency::parallel_for_each(neq.extent, [=](auto&&) restrict(amp) { neq[0] = 0u; });

        const auto compute_domain = _details::Execution_parameters::tiled_domain(last1 - first1);
        concurrency::parallel_for_each(compute_domain, [=, p = std::forward<BinaryPredicate>(p)](auto&& tidx) restrict(amp) {
			for (auto i = tidx.tile_origin[0]; i < (last1 - first1); i += compute_domain.size()) {
				tile_static bool early_out;
				if (tidx.tile_origin == tidx.global) {
					early_out = positive(neq[0]);
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

				if (early_out) return;

				tile_static unsigned int ne;
				if (tidx.tile_origin == tidx.global) {
					ne = 0u;
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

				const auto n = amp_algorithms::min<>()(tidx.tile_dim0 * 1, last1 - first1 - i);
				if (tidx.local[0] < n) {
					if (!p(*(first1 + i + tidx.local[0]), *(first2 + i + tidx.local[0]))) {
						concurrency::atomic_fetch_inc(&ne);
					}
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

				if (positive(ne)) {
					if (tidx.tile_origin == tidx.global) {
						concurrency::atomic_fetch_inc(&neq[0]);
					}
					return;
				}
			}
        });
        return zero(neq[0]);
    }

    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2>
    inline bool equal(ConstRandomAccessIterator1 first1, ConstRandomAccessIterator1 last1, ConstRandomAccessIterator2 first2)
    {
        return amp_stl_algorithms::equal(first1, last1, first2, amp_algorithms::equal_to<>());
    }

	template<typename I, typename T, typename C>
	inline std::pair<I, I> equal_range(I first, I last, T&& value, C&& cmp)
	{
		if (first == last) return std::make_pair(last, last);

		const concurrency::array_view<amp_stl_algorithms::pair<Difference_type<I>, Difference_type<I>>> res(1);
		concurrency::parallel_for_each(res.extent, [=](auto&&) restrict(amp) { res[0] = Value_type<decltype(res)>(last - first, last - first); });

		const auto compute_domain = _details::Execution_parameters::tiled_domain(last - first);
		concurrency::parallel_for_each(compute_domain, [=, value = std::forward<T>(value), cmp = std::forward<C>(cmp)](auto&& tidx) restrict(amp) {
			tile_static bool early_out;
			tile_static bool skip;
			for (auto i = tidx.tile_origin[0]; i < (last - first); i += compute_domain.size()) {
				tidx.barrier.wait_with_tile_static_memory_fence();

				const auto n = amp_algorithms::min<>()(tidx.tile_dim0 * 1, last - first - i);

				if (tidx.tile_origin == tidx.global) {
					early_out = (res[0].first < i) && (res[0].second < i);
					skip = cmp(*(first + i + n - 1), value);
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

				if (early_out) return;
				if (skip) continue;

				tile_static amp_stl_algorithms::pair<Difference_type<I>, Difference_type<I>> r;
				if (tidx.tile_origin == tidx.global) {
					r = decltype(r)(last - first, last - first);
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

				if (tidx.local[0] < n) {
					if (!cmp(*(first + i + tidx.local[0]), value)) {
						concurrency::atomic_fetch_min(&r.first, i + tidx.local[0]);
					}
					if (cmp(value, *(first + i + tidx.local[0]))) {
						concurrency::atomic_fetch_min(&r.second, i + tidx.local[0]);
					}
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

				if (tidx.tile_origin == tidx.global) {
					concurrency::atomic_fetch_min(&res[0].first, r.first);
					concurrency::atomic_fetch_min(&res[0].second, r.second);
				}
			}
		});
		return std::make_pair(first + res[0].first, first + res[0].second);
	}

	template<typename I, typename T>
	inline std::pair<I, I> equal_range(I first, I last, T&& value)
	{
		return amp_stl_algorithms::equal_range(first, last, std::forward<T>(value), amp_algorithms::less<>());
	}

    //----------------------------------------------------------------------------
    // fill, fill_n
    //----------------------------------------------------------------------------

    template<typename RandomAccessIterator, typename T>
    inline void fill(RandomAccessIterator first, RandomAccessIterator last, T&& value)
    {
        amp_stl_algorithms::generate(first, last, [value = std::forward<T>(value)]() restrict(amp) { return value; });
    }

    template<typename RandomAccessIterator, typename Size, typename T>
    inline RandomAccessIterator fill_n(RandomAccessIterator first, Size count, T&& value)
    {
        return amp_stl_algorithms::generate_n(first, count, [value = std::forward<T>(value)]() restrict(amp) { return value; });
    }

    //----------------------------------------------------------------------------
    // find, find_if, find_if_not, find_end, find_first_of, adjacent_find
    //----------------------------------------------------------------------------

    template<typename ConstRandomAccessIterator, typename UnaryPredicate>
    inline ConstRandomAccessIterator find_if(ConstRandomAccessIterator first, ConstRandomAccessIterator last, UnaryPredicate&& p)
    {
        if (first == last) return last;

        concurrency::array_view<Difference_type<ConstRandomAccessIterator>> ridx(1);
        concurrency::parallel_for_each(ridx.extent, [=](auto&&) restrict(amp) { ridx[0] = last - first; });

        const auto compute_domain = _details::Execution_parameters::tiled_domain(last - first);
        concurrency::parallel_for_each(compute_domain, [=, p = std::forward<UnaryPredicate>(p)] (auto&& tidx) restrict(amp) {
			for (auto i = tidx.tile_origin[0]; i < (last - first); i += compute_domain.size()) {
				tile_static bool early_out;
				if (tidx.tile_origin == tidx.global) {
					early_out = ridx[0] < i;
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

				if (early_out) return;

				tile_static Difference_type<ConstRandomAccessIterator> r;
				if (tidx.tile_origin == tidx.global) {
					r = last - first;
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

				const auto n = amp_stl_algorithms::min(tidx.tile_dim0*1, last - first - i);
				if (tidx.local[0] < n) {
					if (p(*(first + i + tidx.local[0]))) {
						concurrency::atomic_fetch_min(&r, i + tidx.local[0]);
					}
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

				if (r != (last - first)) {
					if (tidx.tile_origin == tidx.global) {
						concurrency::atomic_fetch_min(&ridx[0], r);
					}
					return;
				}
			}
        });

        return first + ridx[0];
    }

    template<typename ConstRandomAccessIterator, typename UnaryPredicate>
    inline ConstRandomAccessIterator find_if_not( ConstRandomAccessIterator first, ConstRandomAccessIterator last, UnaryPredicate&& p)
    {
        return amp_stl_algorithms::find_if(first, last, [p = std::forward<UnaryPredicate>(p)](auto&& v) restrict(amp) { return !p(v); });
    }

    template<typename ConstRandomAccessIterator, typename T>
    inline ConstRandomAccessIterator find( ConstRandomAccessIterator first, ConstRandomAccessIterator last, T&& value)
    {
        return amp_stl_algorithms::find_if(first, last, [=, value = std::forward<T>(value)] (auto&& curr_val) restrict(amp) {
            return curr_val == value;
        });
    }

    template<typename ConstRandomAccessIterator, typename Predicate>
    inline ConstRandomAccessIterator adjacent_find(ConstRandomAccessIterator first, ConstRandomAccessIterator last, Predicate&& p)
    {
        if (first == last) return last;

		concurrency::array_view<Difference_type<ConstRandomAccessIterator>> ridx(1);
		concurrency::parallel_for_each(ridx.extent, [=](auto&&) restrict(amp) { ridx[0] = last - first; });

		const auto compute_domain = _details::Execution_parameters::tiled_domain(last - first);
		concurrency::parallel_for_each(compute_domain, [=, p = std::forward<Predicate>(p)](auto&& tidx) restrict(amp) {
			for (auto i = tidx.tile_origin[0]; i < (last - first); i += compute_domain.size()) {
				tile_static bool early_out;
				if (tidx.tile_origin == tidx.global) {
					early_out = ridx[0] < i;
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

				if (early_out) return;

				tile_static Difference_type<ConstRandomAccessIterator> r;
				if (tidx.tile_origin == tidx.global) {
					r = last - first;
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

				const auto n = amp_stl_algorithms::min(tidx.tile_dim0 + 1, last - first - i);
				if (tidx.local[0] < n) {
					if (p(*(first + i + tidx.local[0]), *(first + i + tidx.local[0] + 1))) {
						concurrency::atomic_fetch_min(&r, i + tidx.local[0]);
					}
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

				if (r != (last - first)) {
					if (tidx.tile_origin == tidx.global) {
						concurrency::atomic_fetch_min(&ridx[0], r);
					}
					return;
				}
			}
		});

        return first + ridx[0];
    }

    template<typename ConstRandomAccessIterator>
    inline ConstRandomAccessIterator adjacent_find(ConstRandomAccessIterator first, ConstRandomAccessIterator last)
    {
        return amp_stl_algorithms::adjacent_find(first, last, amp_algorithms::equal_to<>());
    }

    //----------------------------------------------------------------------------
    // for_each, for_each_no_return
    //----------------------------------------------------------------------------

    template<typename ConstRandomAccessIterator, typename UnaryFunction>
    inline void for_each_no_return(ConstRandomAccessIterator first, ConstRandomAccessIterator last, UnaryFunction&& f)
    {
        if (first == last) return;

        concurrency::parallel_for_each(concurrency::extent<1>(last - first), [=, f = std::forward<UnaryFunction>(f)] (auto&& idx) restrict(amp) {
            f(*(first + idx[0]));
        });
    }

    // UnaryFunction CANNOT contain any array, array_view or textures. Needs to be blittable.
    template<typename ConstRandomAccessIterator, typename UnaryFunction>
    inline UnaryFunction for_each(ConstRandomAccessIterator first, ConstRandomAccessIterator last, UnaryFunction&& f)
    {
        if (first == last) return f;

        concurrency::array_view<UnaryFunction> functor_av(1, &f);
        for_each_no_return(first, last,[=] (auto&& val) restrict (amp) {
            functor_av[{0}](val);
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

    template<typename RandomAccessIterator, typename Size, typename Generator>
    inline RandomAccessIterator generate_n(RandomAccessIterator first, Size count, Generator&& g)
    {
        if (zero(count)) return first;

        concurrency::parallel_for_each(concurrency::extent<1>(count), [=, g = std::forward<Generator>(g)] (auto&& idx) restrict(amp) {
            *(first + idx[0]) = g();
        });

        return first + count;
    }

    template <typename RandomAccessIterator, typename Generator>
    inline void generate(RandomAccessIterator first, RandomAccessIterator last, Generator&& g)
    {
        if (first == last) return;

        amp_stl_algorithms::generate_n(first, last - first, std::forward<Generator>(g));
    }

    //----------------------------------------------------------------------------
    // includes
    //----------------------------------------------------------------------------

	template<typename I1, typename I2, typename C>
	inline bool includes(I1 first1, I1 last1, I2 first2, I2 last2, C&& cmp)
	{	// TODO: not efficient, robust or clean yet. Properify!
		if (first1 == last1) return false;
		if (first2 == last2) return true;

		const auto l = amp_stl_algorithms::lower_bound(first1, last1, first2, [](auto&& x, auto&& y) restrict(amp) { return x < *y; });
		const auto u = amp_stl_algorithms::upper_bound(first1, last1, last2 - 1, [](auto&& x, auto&& y) restrict(amp) { return *x < y; });
		if ((u - l) < (last2 - first2)) return false; // Pigeonhole principle.

		concurrency::array_view<unsigned int> neq(1);
		concurrency::parallel_for_each(neq.extent, [=](auto&&) restrict(amp) { neq[0] = 0u; });

		const auto compute_domain = _details::Execution_parameters::tiled_domain(last2 - first2);
		concurrency::parallel_for_each(compute_domain, [=, cmp = std::forward<C>(cmp)](auto&& tidx) restrict(amp) {
			tile_static bool early_out;
			for (decltype(last2 - first2) i = tidx.tile_origin[0]; i < (last2 - first2); i += compute_domain.size()) {
				const auto n = amp_algorithms::min<>()(tidx.tile_dim0 * 1, last2 - first2 - i);
				if (tidx.tile_origin == tidx.global) {
					early_out = ((u - l - i) < n);
					if (early_out) {
						concurrency::atomic_fetch_inc(&neq[0]);
					}
					early_out = early_out || positive(neq[0]);
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

				tile_static Difference_type<I1> offs[tidx.tile_dim0];
				offs[tidx.local[0]] = i + tidx.local[0];

				tile_static Value_type<I2> vals[tidx.tile_dim0];
				_copy_single_tile(first2 + i, first2 + i + n, vals, tidx);

				if (tidx.local[0] < n) {
					while ((offs[tidx.local[0]] < (u - l)) && cmp(*(l + offs[tidx.local[0]]), vals[tidx.local[0]])) {
						++offs[tidx.local[0]];
					}
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

				tile_static unsigned int not_done;
				do {
					tidx.barrier.wait_with_tile_static_memory_fence();

					if (tidx.tile_origin == tidx.global) {
						not_done = 0u;
					}

					tidx.barrier.wait_with_tile_static_memory_fence();

					if (odd(tidx.local[0])) {
						if (offs[tidx.local[0] - 1] == offs[tidx.local[0]]) {
							++offs[tidx.local[0]];
							concurrency::atomic_fetch_inc(&not_done);
						}
					}

					tidx.barrier.wait_with_tile_static_memory_fence();

					if (positive(tidx.local[0]) && even(tidx.local[0])) {
						if (offs[tidx.local[0] - 1] == offs[tidx.local[0]]) {
							++offs[tidx.local[0]];
							concurrency::atomic_fetch_inc(&not_done);
						}
					}

					tidx.barrier.wait_with_tile_static_memory_fence();
				}
				while (positive(not_done));

				tile_static unsigned int ne;
				if (tidx.tile_origin == tidx.global) {
					ne = 0u;
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

				if (tidx.local[0] < n) {
					for (auto j = offs[tidx.local[0]]; ; ++j) {
						if (((u - l) < j) || cmp(vals[tidx.local[0]], *(l + j))) {
							concurrency::atomic_fetch_inc(&ne);
							break;
						}
						if (!cmp(*(l + j), vals[tidx.local[0]])) {
							break;
						}
					}
				}
				tidx.barrier.wait_with_tile_static_memory_fence();

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
	inline bool includes(I1 first1, I1 last1, I2 first2, I2 last2)
	{
		return amp_stl_algorithms::includes(first1, last1, first2, last2, amp_algorithms::less<>());
	}
    //----------------------------------------------------------------------------
    // inner_product
    //----------------------------------------------------------------------------

    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2, typename T, typename BinaryOperation1, typename BinaryOperation2>
    inline T inner_product(ConstRandomAccessIterator1 first1, ConstRandomAccessIterator1 last1, ConstRandomAccessIterator2 first2, T&& value, BinaryOperation1&& binary_op1, BinaryOperation2&& binary_op2)
    {
        if (first1 == last1) return value;

		const auto compute_domain = _details::Execution_parameters::tiled_domain(last1 - first1);
        concurrency::array_view<T> tmp(compute_domain.size() / _details::Execution_parameters::tile_size());
        concurrency::array_view<unsigned int> locks(tmp.extent);

		static constexpr unsigned int locked = 1u;
        static constexpr unsigned int unlocked = 0u;
        concurrency::parallel_for_each(locks.extent, [=](auto&& idx) restrict(amp) { locks[idx] = locked; });

        concurrency::parallel_for_each(compute_domain, [=, value = std::forward<T>(value), binary_op1 = std::forward<BinaryOperation1>(binary_op1), binary_op2 = std::forward<BinaryOperation2>(binary_op2)](auto&& tidx) restrict(amp) {
			tile_static Value_type<ConstRandomAccessIterator1> vals[tidx.tile_dim0];
			T partial_prod;

			for (Difference_type<ConstRandomAccessIterator1> i = tidx.tile_origin[0]; i < (last1 - first1); i += compute_domain.size()) {
				const auto n = amp_stl_algorithms::min(tidx.tile_dim0 * 1, last1 - first1 - i);

				if (tidx.local[0] < n) {
					vals[tidx.local[0]] = binary_op2(*(first1 + i + tidx.local[0]), *(first2 + i + tidx.local[0]));
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

				const auto pp = _reduce_single_tile_unguarded(vals, tidx, binary_op1);

				partial_prod = positive(i / compute_domain.size()) ? binary_op1(partial_prod, pp) : pp;
			}

			if (tidx.tile_origin == tidx.global) {
				if (zero(tidx.tile[0])) {
                    tmp[tidx.tile] = binary_op1(partial_prod, value);
                }
                else {
                    auto s = unlocked;
                    while (!concurrency::atomic_compare_exchange(&locks[tidx.tile - 1], &s, locked)) {
                        s = unlocked;
                    }
                    tmp[tidx.tile] = binary_op1(partial_prod, tmp[tidx.tile - 1]);
                }
                locks[tidx.tile] = unlocked;
            }
        });

        return tmp[tmp.extent.size() - 1u];
    }

	template<typename I1, typename I2, typename T>
	inline T inner_product(I1 first1, I1 last1, I2 first2, T&& value)
	{
		return amp_stl_algorithms::inner_product(first1, last1, first2, std::forward<T>(value), amp_algorithms::plus<>(), amp_algorithms::multiplies<>());
	}
    //----------------------------------------------------------------------------
    // iota
    //----------------------------------------------------------------------------

    template<typename T> using has_plus = has<std::plus<>(T, T)>;
    template<typename T> using has_minus = has<std::minus<>(T, T)>;
    template<typename T> using has_multiplies = has<std::multiplies<>(T, T)>;

    template<typename T> using fast_iota = std::enable_if_t<has_plus<T>::value && has_minus<T>::value && has_multiplies<T>::value>;
    template<typename T> using slow_iota = std::enable_if_t<!has_minus<T>::value || !has_multiplies<T>::value>;

    template<typename I, typename T, fast_iota<T>* = nullptr>
    inline void iota_impl(I first, I last, T&& value)
    {
        auto increment = value;
        increment = ++increment - value;

        concurrency::parallel_for_each(concurrency::extent<1>(last - first), [=, value = std::forward<T>(value)](auto&& idx) restrict(amp) {
            *(first + idx[0]) = value + T(idx[0]) * increment;  // This is numerically equivalent with incrementing only if T multiplication is exact.
        });
    }

    template<typename I, typename T, slow_iota<T>* = nullptr>
    inline void iota_impl(I first, I last, T&& value)
    {
        concurrency::parallel_for_each(concurrency::extent<1>(last - first), [=, value = std::forward<T>(value)](auto&& idx) restrict(amp) {
            *(first + idx[0]) = value;
            for (auto i = idx[0]; i; --i) {
				++*(first + idx[0])
			};
        });
    }

    template<typename RandomAccessIterator, typename T>
    inline void iota(RandomAccessIterator first, RandomAccessIterator last, T&& value)
    {
        // Using SFINAE based dispatch should work here: if T has multiplication and subtraction
        // the fast version can be used, if not we provide a slow but standards conforming one.
        // There is an intermediate solution, using scan, which only requires T to have addition
        if (first == last) return;
        iota_impl(first, last, std::forward<T>(value));
    }

    //----------------------------------------------------------------------------
    // lexographical_compare
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------
    // lower_bound, upper_bound
    //----------------------------------------------------------------------------

    template<typename I, typename T, typename C>
    inline I lower_bound(I first, I last, T&& value, C&& cmp)
    {
		return amp_stl_algorithms::partition_point(first, last, [value = std::forward<T>(value), cmp = std::forward<C>(cmp)](auto&& x) restrict(cpu, amp) {
			return cmp(x, value);
		});
    }

    template<typename I, typename T>
    inline I lower_bound(I first, I last, T&& value)
    {
        return lower_bound(first, last, std::forward<T>(value), amp_algorithms::less<>());
    }

	template<typename I, typename T, typename C>
	inline I upper_bound(I first, I last, T&& value, C&& cmp)
	{
		return amp_stl_algorithms::partition_point(first, last, [value = std::forward<T>(value), cmp = std::forward<C>(cmp)](auto&& x) restrict(cpu, amp) {
			return !cmp(value, x);
		});
	}

	template<typename I, typename T>
	inline I upper_bound(I first, I last, T&& value)
	{
		return upper_bound(first, last, std::forward<T>(value), amp_algorithms::less<>());
	}

    //----------------------------------------------------------------------------
    // merge, inplace_merge
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------
    // max, max_element, min, min_element, minmax_element
    //----------------------------------------------------------------------------
	template<typename I, typename C>
	I _extremum_element(I first, I last, C&& cmp)
	{	// TODO: Should be properified around the in-tile reduce step, somewhat unclean (after reduce overhaul).
		//	   : Should really be using work-stealing, as opposed to the fixed pipeline propagation (after v1).
		if (first == last) return last;

		const auto compute_domain = _details::Execution_parameters::tiled_domain(last - first);
		const concurrency::array_view<Difference_type<I>> ridx(compute_domain.size() / _details::Execution_parameters::tile_size());
		concurrency::parallel_for_each(ridx.extent, [=](auto&& idx) restrict(amp) { ridx[idx] = last - first; });

		concurrency::parallel_for_each(compute_domain, [=, cmp = std::forward<C>(cmp)](auto&& tidx) restrict(amp) {
			using Val_idx = amp_stl_algorithms::pair<Value_type<I>, Difference_type<I>>;

			tile_static Val_idx extrema[tidx.tile_dim0];
			extrema[tidx.local[0]] = tidx.global[0] < (last - first) ? Val_idx(*(first + tidx.global[0]), tidx.global[0]) :
									 (std::is_default_constructible<Value_type<I>>::value ? Val_idx(Value_type<I>(), 0) : Val_idx(*first, 0));

			for (Difference_type<I> i = tidx.tile_origin[0] + compute_domain.size(); i < (last - first); i += compute_domain.size()) {
				const auto n = amp_stl_algorithms::min(tidx.tile_dim0*1, last - first - i);
				if (tidx.local[0] < n) {
					if (cmp(*(first + i + tidx.local[0]), extrema[tidx.local[0]].first)) {
						extrema[tidx.local[0]].first = *(first + i + tidx.local[0]);
						extrema[tidx.local[0]].second = i + tidx.local[0];
					}
				}
			}

			tidx.barrier.wait_with_tile_static_memory_fence();

			auto extremum = _reduce_single_tile_unguarded(extrema, tidx, [c = cmp](auto&& x, auto&& y) {
				if (c(x.first, y.first)) return x;
				if (c(y.first, x.first)) return y;
				return Value_type<decltype(x)>(x.first, amp_stl_algorithms::min(x.second, y.second));
			});

			tidx.barrier.wait_with_tile_static_memory_fence();

			if (tidx.tile_origin == tidx.global) {
				if (positive(tidx.tile[0])) {
					auto s = last - first;
					while (concurrency::atomic_compare_exchange(&ridx[tidx.tile - 1], &s, ridx[tidx.tile - 1])) {
						s = last - first;
					}

					if (cmp(*(first + ridx[tidx.tile - 1]), extremum.first)) {
						extremum = Val_idx(*(first + ridx[tidx.tile - 1]), ridx[tidx.tile - 1]);
					}
					else if (!cmp(extremum.first, *(first + ridx[tidx.tile - 1]))) {
						extremum.second = amp_stl_algorithms::min(extremum.second, ridx[tidx.tile - 1]);
					}
				}

				ridx[tidx.tile] = extremum.second;
			}
		});

		return first + ridx[ridx.extent.size() - 1];
	}

	template<typename T, typename C>
	inline /*constexpr*/ const T& max(const T& x, const T& y, C&& cmp) restrict(cpu, amp)
	{
		return cmp(y, x) ? x : y;
	}

	template<typename T>
	inline /*constexpr*/ const T& max(const T& x, const T& y) restrict(cpu, amp)
	{
		return amp_stl_algorithms::max(x, y, amp_algorithms::less<>());
	}

	template<typename I, typename C>
	inline I max_element(I first, I last, C&& cmp)
	{
		return amp_stl_algorithms::_extremum_element(first, last, [cmp = std::forward<C>(cmp)](auto&& x, auto&& y) restrict(amp) {
			return cmp(y, x);
		});
	}

	template<typename I>
	inline I max_element(I first, I last)
	{
		return amp_stl_algorithms::max_element(first, last, amp_algorithms::less<>());
	}

	template<typename T, typename C>
	inline /*constexpr*/ const T& min(const T& x, const T& y, C&& cmp) restrict(cpu, amp)
	{
		return cmp(y, x) ? y : x;
	}

	template<typename T>
	inline /*constexpr*/ const T& min(const T& x, const T& y) restrict(cpu, amp)
	{
		return amp_stl_algorithms::min(x, y, amp_algorithms::less<>());
	}

	template<typename I, typename C>
	inline I min_element(I first, I last, C&& cmp)
	{
		return amp_stl_algorithms::_extremum_element(first, last, std::forward<C>(cmp));
	}

	template<typename I>
	inline I min_element(I first, I last)
	{
		return amp_stl_algorithms::min_element(first, last, amp_algorithms::less<>());
	}

	template<typename T, typename C>
	inline /*constexpr*/ amp_stl_algorithms::pair<const T, const T> minmax(const T& x, const T& y, C&& cmp) restrict(cpu, amp)
	{
		return cmp(y, x) ? amp_stl_algorithms::pair<const T, const T>(y, x) :
						   amp_stl_algorithms::pair<const T, const T>(x, y);
	}

	template<typename T>
	inline /*constexpr*/ amp_stl_algorithms::pair<const T, const T> minmax(const T& x, const T& y) restrict(cpu, amp)
	{
		return minmax(x, y, amp_algorithms::less<>());
	}

	template<typename T, typename D>
	using Val_idx = amp_stl_algorithms::pair<T, D>;
	template<typename T, typename D>
	using Min_max = amp_stl_algorithms::pair<Val_idx<T, D>, Val_idx<T, D>>;

	template<typename I, typename C>
	Min_max<Value_type<I>, Difference_type<I>> _construct_minmax(I first, I x, I y, C&& cmp) restrict(cpu, amp)
	{
		using Mm = Min_max<Value_type<I>, Difference_type<I>>;
		using VI = Val_idx<Value_type<I>, Difference_type<I>>;

		if (cmp(*y, *x)) return Mm(VI(*y, y - first), VI(*x, x - first));
		else return Mm(VI(*x, x - first), VI(*y, y - first));
	}

	template<typename T, typename D, typename C>
	Min_max<T, D> _combine_minmax(const Min_max<T, D>& x, const Min_max<T, D>& y, C&& cmp) restrict(cpu, amp)
	{
		const auto m = cmp(y.first.first, x.first.first) ? y.first :
					   cmp(x.first.first, y.first.first) ? x.first :
					   (y.first.second < x.first.second) ? y.first : x.first;
		const auto M = cmp(y.second.first, x.second.first) ? x.second :
					   cmp(x.second.first, y.second.first) ? y.second :
					   (y.second.second < x.second.second) ? x.second : y.second;
		return Min_max<T, D>(m, M);
	}

	template<typename I, typename C>
	inline std::pair<I, I> minmax_element(I first, I last, C&& cmp)
	{	// TODO: Should be properified around the in-tile reduce step, somewhat unclean (after reduce overhaul).
		//	   : Should really be using work-stealing, as opposed to the fixed pipeline propagation (after v1).
		//	   : Pessimistic about picking the init-value (over-reads first and first + 1). Re-think.
		if ((first == last) || (first == (last - 1))) return std::make_pair(first, first);

		const auto compute_domain = _details::Execution_parameters::tiled_domain(last - first);

		const concurrency::array_view<Difference_type<I>> midx(compute_domain.size() / _details::Execution_parameters::tile_size());
		const concurrency::array_view<Difference_type<I>> Midx(compute_domain.size() / _details::Execution_parameters::tile_size());
		concurrency::parallel_for_each(midx.extent, [=](auto&& idx) restrict(amp) {
			midx[idx] = last - first;
			Midx[idx] = last - first;
		});

		concurrency::parallel_for_each(compute_domain, [=, cmp = std::forward<C>(cmp)](auto&& tidx) restrict(amp) {
			tile_static Min_max<Value_type<I>, Difference_type<I>> extrema[tidx.tile_dim0];
			extrema[tidx.local[0]] = _construct_minmax(first, first, first + 1, cmp);

			for (Difference_type<I> i = tidx.tile_origin[0]; i < (last - first); i += 2 * compute_domain.size()) {
				const auto n = amp_stl_algorithms::min(tidx.tile_dim0 * 1, last - first - i);

				if (tidx.local[0] < n) {
					const auto o = (Difference_type<I>(i + compute_domain.size() + tidx.local[0]) < (last - first)) ? compute_domain.size() : 0;
					const auto c = _construct_minmax(first, first + i + tidx.local[0], first + i + o + tidx.local[0], cmp);
					extrema[tidx.local[0]] = _combine_minmax(extrema[tidx.local[0]], c, cmp);
				}
			}

			tidx.barrier.wait_with_tile_static_memory_fence();

			auto extremum = _reduce_single_tile_unguarded(extrema, tidx, [c = cmp](auto&& x, auto&& y) { return _combine_minmax(x, y, c); });

			if (tidx.tile_origin == tidx.global) {
				if (positive(tidx.tile[0])) {
					auto s = last - first;
					while (concurrency::atomic_compare_exchange(&Midx[tidx.tile - 1], &s, Midx[tidx.tile - 1])) {
						s = last - first;
					}


					using VI = std::remove_reference_t<decltype(extremum.first)>;
					extremum = _combine_minmax(extremum, decltype(extremum)(VI(*(first + midx[tidx.tile - 1]), midx[tidx.tile - 1]),
																			VI(*(first + Midx[tidx.tile - 1]), Midx[tidx.tile - 1])), cmp);
				}
				midx[tidx.tile] = extremum.first.second;
				Midx[tidx.tile] = extremum.second.second;
			}
		});

		return std::make_pair(first + midx[midx.extent.size() - 1],
							  first + Midx[Midx.extent.size() - 1]);
	}

	template<typename I>
	inline std::pair<I, I> minmax_element(I first, I last)
	{
		return amp_stl_algorithms::minmax_element(first, last, amp_algorithms::less<>());
	}

    //----------------------------------------------------------------------------
    // mismatch
    //----------------------------------------------------------------------------
	template<typename I1, typename I2, typename P>
	inline std::pair<I1, I2> mismatch(I1 first1, I1 last1, I2 first2, P&& p)
	{
		if (first1 == last1) return std::make_pair(last1, first2);

		const concurrency::array_view<Difference_type<I1>> ridx(1);
		concurrency::parallel_for_each(ridx.extent, [=](auto&&) restrict(amp) { ridx[0] = last1 - first1; });

		const auto compute_domain = _details::Execution_parameters::tiled_domain(last1 - first1);
		concurrency::parallel_for_each(compute_domain, [=](auto&& tidx) restrict(amp) {
			for (auto i = tidx.tile_origin[0]; i < (last1 - first1); i += compute_domain.size()) {
				tile_static bool early_out;
				if (tidx.tile_origin == tidx.global) {
					early_out = (ridx[0] < i);
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

				if (early_out) return;

				tile_static Difference_type<I1> r;
				if (tidx.tile_origin == tidx.global) {
					r = last1 - first1;
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

				const auto n = amp_algorithms::min<>()(tidx.tile_dim0 * 1, last1 - first1 - i);
				if (tidx.local[0] < n) {
					if (!p(*(first1 + i + tidx.local[0]), *(first2 + i + tidx.local[0]))) {
						concurrency::atomic_fetch_min(&ridx[0], i + tidx.local[0]);
					}
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

				if (r != (last1 - first1)) {
					if (tidx.tile_origin == tidx.global) {
						concurrency::atomic_fetch_min(&ridx[0], r);
					}
					return;
				}
			}
		});

		return std::make_pair(first1 + ridx[0], first2 + ridx[0]);
	}

	template<typename I1, typename I2>
	inline std::pair<I1, I2> mismatch(I1 first1, I1 last1, I2 first2)
	{
		return amp_stl_algorithms::mismatch(first1, last1, first2, amp_algorithms::equal_to<>());
	}
    //----------------------------------------------------------------------------
    // move, move_backward
    //----------------------------------------------------------------------------

	template<typename I1, typename I2>
	inline I2 move(I1 first, I1 last, I2 dest_first)
	{	// Given current peculiarities of memory allocation in C++ AMP this is provided
		// primarily for completion.
		if (first == last) return dest_first;

		concurrency::parallel_for_each(concurrency::extent<1>(last - first), [=](auto&& idx) restrict(amp) {
			*(dest_first + idx[0]) = move(*(first + idx));
		});

		return dest_first + (last - first);
	}

    //----------------------------------------------------------------------------
    // nth_element
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------
    // partial sum
    //----------------------------------------------------------------------------

	template<typename I1, typename I2, typename Op>
	inline I2 partial_sum(I1 first, I1 last, I2 dest_first, Op&& op)
	{	// This is practically an inclusive scan.
		return amp_stl_algorithms::inclusive_scan(first, last, dest_first, Value_type<I1>(), std::forward<Op>(op));
	}

	template<typename I1, typename I2>
	inline I2 partial_sum(I1 first, I1 last, I2 dest_first)
	{
		return amp_stl_algorithms::partial_sum(first, last, dest_first, amp_algorithms::plus<>());
	}

    //----------------------------------------------------------------------------
    // partition, stable_partition, partition_point, is_partitioned
    //----------------------------------------------------------------------------
	template<typename I, int tsz>
	inline void _reverse_single_tile(I first, I last, const concurrency::tiled_index<tsz>& tidx) restrict(amp)
	{
		if (first == last || first == (last - 1)) return;

		for (auto i = tidx.local[0]; i < half_nonnegative(last - first); i += tidx.tile_dim0) {
			amp_stl_algorithms::iter_swap(first + i, last - i - 1);
		}

		tidx.barrier.wait_with_tile_static_memory_fence();
	}

//	template<typename I, int tsz>
//	inline I _rotate_single_tile(I first, I middle, I last, const concurrency::tiled_index<tsz>& tidx) restrict(amp)
//	{
//		if (first == middle) return last;
//		if (middle == last) return first;
//
//		_reverse_single_tile(first, middle, tidx);
//		_reverse_single_tile(middle, last, tidx);
//		_reverse_single_tile(first, last, tidx);
//
//		return first + (last - middle);
//	}
	template<typename I, typename P, int tsz>
	inline I _partition_single_tile_unguarded(I first, I last, const concurrency::tiled_index<tsz>& tidx, P&& pred) restrict(amp)
	{
		// This assumes that (last - first) <= tidx.tile_dim0. Will not work otherwise.
		tile_static amp_stl_algorithms::pair<Difference_type<I>, Difference_type<I>> off;
		if (tidx.tile_origin == tidx.global) {
			off = decltype(off)(0u, last - first);
		}

		tidx.barrier.wait_with_tile_static_memory_fence();

		const auto n = amp_stl_algorithms::min(tidx.tile_dim0 * 1, last - first);

		Value_type<I> v;
		decltype(off) o(0, 0);

		if (tidx.local[0] < n) {
			v = *(first + tidx.local[0]);
			o.first = concurrency::atomic_fetch_add(&off.first, pred(v));
			o.second = concurrency::atomic_fetch_sub(&off.second, !pred(v));
		}

		tidx.barrier.wait_with_tile_static_memory_fence();

		if (tidx.local[0] < n) {
			if (pred(v)) {
				*(first + o.first) = v;
			}
			else {
				*(first + o.second - 1) = v;
			}
		}

		tidx.barrier.wait_with_tile_static_memory_fence();

		return first + off.first;
 	}

	template<typename I1, typename I2, int tsz>
	inline I2 _copy_single_tile(I1 first1, I1 last1, I2 first2, const concurrency::tiled_index<tsz>& tidx) restrict(amp)
	{
		for (auto i = tidx.local[0]; i < (last1 - first1); i += tidx.tile_dim0) {
			*(first2 + i) = *(first1 + i);
		}

		tidx.barrier.wait_with_tile_static_memory_fence();

		return first2 + (last1 - first1);
	}

	template<typename I1, typename I2, int tsz>
	inline decltype(auto) _swap_ranges_single_tile(I1 first1, I1 last1, I2 first2, I2 last2, const concurrency::tiled_index<tsz>& tidx) restrict(amp)
	{
		const auto n = amp_algorithms::min<>()(last1 - first1, last2 - first2);
		for (auto i = tidx.local[0]; i < n; i += tidx.tile_dim0) {
			amp_stl_algorithms::iter_swap(first1 + i, first2 + i);
		}

		tidx.barrier.wait_with_tile_static_memory_fence();

		return amp_stl_algorithms::pair<Difference_type<I1>, Difference_type<I2>>(n, n);
	}

	template<typename I, typename P>
	inline I partition(I first, I last, P&& pred)
	{	// TODO: this will not work with ranges that need more than max_tiles tiles to be processed. Properify.
		if (first == last) return last;

		const concurrency::array_view<Difference_type<I>> p_points(_details::Execution_parameters::tile_cnt(last - first));
		const auto compute_domain = _details::Execution_parameters::tiled_domain(last - first);

		amp_stl_algorithms::fill_n(amp_stl_algorithms::begin(p_points), p_points.extent.size(), last - first);

		concurrency::parallel_for_each(compute_domain, [=, pred = std::forward<P>(pred)](auto&& tidx) restrict(amp) {
			const auto n = amp_algorithms::min<>()(tidx.tile_dim0 * 1, (last - first - tidx.tile_origin[0]));

			tile_static Value_type<I> vals[tidx.tile_dim0];
			_copy_single_tile(first + tidx.tile_origin[0], first + tidx.tile_origin[0] + n, vals, tidx);

			auto p_point = _partition_single_tile_unguarded(vals, vals + n, tidx, pred) - vals;

			tile_static Difference_type<I> off;
			if (tidx.tile_origin == tidx.global) {
				if (positive(tidx.tile[0])) {
					off = last - first;
					while (concurrency::atomic_compare_exchange(&p_points[predecessor(tidx.tile[0])], &off, p_points[predecessor(tidx.tile[0])])) {
						off = last - first;
					}
				}
				else {
					off = Difference_type<I>(0);
				}
			}

			tidx.barrier.wait_with_tile_static_memory_fence();

			if (positive(p_point)) {
				if ((off != tidx.tile_origin[0])) {
					auto a = _swap_ranges_single_tile(first + off, first + tidx.tile_origin[0], vals, vals + p_point, tidx);
					_swap_ranges_single_tile(vals, vals + a.second, vals + a.second, vals + p_point, tidx);
				}
				_copy_single_tile(vals, vals + n, first + tidx.tile_origin[0], tidx);
			}

			tidx.barrier.wait_with_tile_static_memory_fence();

			if (tidx.tile_origin == tidx.global) {
				p_points[tidx.tile] = off + p_point;
			}
		});
		return first + p_points[predecessor(p_points.extent.size())];
	}

	template<typename I, typename P>
	inline I partition_point(I first, I last, P&& pred)
	{
		if (first == last) return last;

		// This is NOT OPTIMAL YET. Will be fixed.
		const concurrency::array_view<Difference_type<I>> ridx(1);
		concurrency::parallel_for_each(ridx.extent, [=](auto&&) restrict(amp) { ridx[0] = last - first; });

		const auto compute_domain = _details::Execution_parameters::tiled_domain(last - first);
		concurrency::parallel_for_each(compute_domain, [=, pred = std::forward<P>(pred)](auto&& tidx) restrict(amp) {
			tile_static bool early_out;
			tile_static bool skip;
			for (auto i = tidx.tile_origin[0]; i < (last - first); i += compute_domain.size()) {
				tidx.barrier.wait_with_tile_static_memory_fence();

				const auto n = amp_algorithms::min<>()(tidx.tile_dim0 * 1, last - first - i);
				if (tidx.tile_origin == tidx.global) {
					early_out = ridx[0] < i;
					skip = pred(*(first + i)) && pred(*(first + i + n - 1));
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

				if (early_out) return;
				if (skip) continue;

				tile_static Difference_type<I> r;
				if (tidx.tile_origin == tidx.global) r = last - first;

				tidx.barrier.wait_with_tile_static_memory_fence();

				if ((tidx.local[0] < n) && !pred(*(first + i + tidx.local[0]))) {
					concurrency::atomic_fetch_min(&r, i + tidx.local[0]);
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

				if (r != (last - first)) {
					if (tidx.tile_origin == tidx.global) {
						concurrency::atomic_fetch_min(&ridx[0], r);
					}
					return;
				}
			}
		});

		return first + ridx[0];
	}

	template<typename I, typename P>
	inline bool is_partitioned(I first, I last, P&& pred)
	{
		return last == amp_stl_algorithms::find_if(amp_stl_algorithms::find_if_not(first, last, std::forward<P>(pred)), last, std::forward<P>(pred));
	}

    //----------------------------------------------------------------------------
    // reduce
    //----------------------------------------------------------------------------

	template<typename I, typename Op, int tsz>
	inline decltype(auto) _reduce_single_tile_unguarded(I first, const concurrency::tiled_index<tsz>& tidx, Op&& op) restrict(amp)
	{	// This assumes that it is fed tsz worth of data, otherwise results are undefined.
		const concurrency::array_view<Value_type<I>> d(tsz, &*first); // Hack around lambda capture deficiencies in restrict(amp) contexts.

		amp_algorithms::_details::static_for<half_nonnegative(tsz),
											 0u,
											 amp_algorithms::_details::Inc::div,
											 2u>()([=, op = op](auto&& h) {
			if (tidx.local[0] < h) {
				d[tidx.local[0]] = op(d[tidx.local[0]], d[tidx.local[0] + h]);
			}

			tidx.barrier.wait_with_tile_static_memory_fence();
		});

		return d[0];
	}

    template<typename ConstRandomAccessIterator, typename T, typename BinaryOperation>
    inline T reduce(ConstRandomAccessIterator first, ConstRandomAccessIterator last, T&& identity_element, BinaryOperation&& op)
    {
		if (first == last) return identity_element;

		using Result = amp_stl_algorithms::pair<Value_type<ConstRandomAccessIterator>, unsigned int>;

		const auto compute_domain = _details::Execution_parameters::tiled_domain(last - first);

		const concurrency::array_view<T> res(1);

		static constexpr unsigned int not_done = 0u;
		static constexpr unsigned int done = 1u;
		const concurrency::array_view<Result> tmp(compute_domain / compute_domain.get_tile_extent().size());
		amp_stl_algorithms::fill(amp_stl_algorithms::begin(tmp), amp_stl_algorithms::end(tmp), Result(identity_element, not_done));

		concurrency::parallel_for_each(compute_domain, [=, identity_element = std::forward<T>(identity_element), op = std::forward<BinaryOperation>(op)](auto&& tidx) restrict(amp) {
			tile_static T vals[tidx.tile_dim0];
			vals[tidx.local[0]] = identity_element;
			for (auto i = tidx.tile_origin[0]; i < (last - first); i += compute_domain.size()) {
				const auto n = amp_stl_algorithms::min(tidx.tile_dim0 * 1, last - first - i);

				if (tidx.local[0] < n) {
					vals[tidx.local[0]] = op(vals[tidx.local[0]], *(first + i + tidx.local[0]));
				}
			}

			tidx.barrier.wait_with_tile_static_memory_fence();

			const auto r = _reduce_single_tile_unguarded(vals, tidx, op);

			if (tidx.tile_origin == tidx.global) {
				if (positive(tidx.tile[0])) {
					auto s = done;
					while (!concurrency::atomic_compare_exchange(&tmp[predecessor(tidx.tile[0])].second, &s, tmp[predecessor(tidx.tile[0])].second)) {
						s = done;
					};
				}

				if (tidx.tile[0] != predecessor(tmp.extent.size())) {
					tmp[tidx.tile[0]] = Result(op(tmp[predecessor(tidx.tile[0])].first, r), done * 1);
				}
				else {
					res[0] = op(tmp[predecessor(tidx.tile[0])].first, r);
				}
			}
		});

		return res[0];
    }

    //----------------------------------------------------------------------------
    // remove, remove_if, remove_copy, remove_copy_if
    //----------------------------------------------------------------------------

    template<typename RandomAccessIterator, typename T>
    inline RandomAccessIterator remove(RandomAccessIterator first, RandomAccessIterator last, T&& value)
    {
        return amp_stl_algorithms::remove_if(first, last, [=, value = std::forward<T>(value)](auto&& v) restrict(amp) { return v == value; });
    }

    template<typename RandomAccessIterator, typename UnaryPredicate>
    inline RandomAccessIterator remove_if(RandomAccessIterator first, RandomAccessIterator last, UnaryPredicate&& pred)
    {
        if (first == last) return last;

        static constexpr auto tsz = _details::Execution_parameters::tile_size();
        concurrency::array_view<unsigned int> locks(_details::Execution_parameters::tile_cnt(last - first));
        concurrency::array_view<Difference_type<RandomAccessIterator>> off(1);

        static constexpr unsigned int locked = 1u;
        static constexpr unsigned int unlocked = 0u;
        concurrency::parallel_for_each(locks.extent, [=](auto&& idx) restrict(amp) {
            if (zero(idx[0])) off[0] = Difference_type<RandomAccessIterator>(0);
            locks[idx] = locked;
        });

        const auto compute_domain = _details::Execution_parameters::tiled_domain(last - first);
        concurrency::parallel_for_each(compute_domain, [=, pred = std::forward<UnaryPredicate>(pred)](auto&& tidx) restrict(amp) {
            for (Difference_type<RandomAccessIterator> i = tidx.tile_origin[0]; i < (last - first); i += compute_domain.size()) {
                const auto n = amp_stl_algorithms::min(tidx.tile_dim0 * 1, last - first - i);

				tile_static Value_type<RandomAccessIterator> vals[tidx.tile_dim0];
				_copy_single_tile(first + i, first + i + n, vals, tidx);

				tile_static Difference_type<RandomAccessIterator> tile_off;
				if (tidx.tile_origin == tidx.global) {
					concurrency::atomic_exchange(&locks[i / tsz], unlocked); // Values cached, release.
					tile_off = Difference_type<RandomAccessIterator>(0);
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

				Difference_type<RandomAccessIterator> o(0);
				if ((tidx.local[0] < n) && !pred(vals[tidx.local[0]])) {
					o = concurrency::atomic_fetch_inc(&tile_off);
				}

				tidx.barrier.wait_with_tile_static_memory_fence();

                if (zero(tile_off)) continue;

				tidx.barrier.wait_with_tile_static_memory_fence();

                if (tidx.tile_origin == tidx.global) {
                    tile_off = concurrency::atomic_fetch_add(&off[0], tile_off);
					auto s = unlocked;
					while (!concurrency::atomic_compare_exchange(&locks[tile_off / tsz], &s, locked)) {
                        s = unlocked;
                    }
                }

                tidx.barrier.wait_with_tile_static_memory_fence();

                if ((tidx.local[0] < n) && !pred(vals[tidx.local[0]])) {
					*(first + tile_off + o) = vals[tidx.local[0]];
				}

                tidx.barrier.wait_with_tile_static_memory_fence();

                if (tidx.tile_origin == tidx.global) {
					concurrency::atomic_exchange(&locks[tile_off / tsz], unlocked);
				}
            }
        });
        return first + off[0];
    }

    template<typename ConstRandomAccessIterator,typename RandomAccessIterator, typename T>
    inline RandomAccessIterator remove_copy(ConstRandomAccessIterator first, ConstRandomAccessIterator last, RandomAccessIterator dest_first, T&& value)
    {
        return amp_stl_algorithms::copy_if(first, last, dest_first, [=, value = std::forward<T>(value)](auto&& v) restrict(amp) { return (v != value); });
    }

    template<typename ConstRandomAccessIterator,typename RandomAccessIterator, typename UnaryPredicate>
    inline RandomAccessIterator remove_copy_if(ConstRandomAccessIterator first, ConstRandomAccessIterator last, RandomAccessIterator dest_first, UnaryPredicate&& p)
    {
        return amp_stl_algorithms::copy_if(first, last, dest_first, [=, p = std::forward<UnaryPredicate>(p)](auto&& v) restrict(amp) { return !p(v); });
    }

    //----------------------------------------------------------------------------
    // replace, replace_if, replace_copy, replace_copy_if
    //----------------------------------------------------------------------------

    template<typename RandomAccessIterator, typename T>
    inline void replace(RandomAccessIterator first, RandomAccessIterator last, T&& old_value, T&& new_value)
    {
        amp_stl_algorithms::replace_if(first, last, [=, old_value = std::forward<T>(old_value)](auto&& v) restrict(amp) { return v == old_value; }, std::forward<T>(new_value));
    }

    template<typename RandomAccessIterator, typename UnaryPredicate, typename T>
    inline void replace_if(RandomAccessIterator first, RandomAccessIterator last, UnaryPredicate&& p, T&& new_value)
    {
        if (first == last) return;

        concurrency::parallel_for_each(concurrency::extent<1>(last - first), [=, new_value = std::forward<T>(new_value), p = std::forward<UnaryPredicate>(p)](auto&& idx) restrict(amp) {
            if (p(*(first + idx[0]))) {
				*(first + idx[0]) = new_value;
			}
        });
    }

    template<typename ConstRandomAccessIterator,typename RandomAccessIterator, typename T>
    inline RandomAccessIterator replace_copy(ConstRandomAccessIterator first, ConstRandomAccessIterator last, RandomAccessIterator dest_first, T&& old_value, T&& new_value)
    {
        return amp_stl_algorithms::replace_copy_if(first, last, dest_first, [=, old_value = std::forward<T>(old_value)](auto&& v) restrict(amp) { return (v == old_value); }, std::forward<T>(new_value));
    }

    template<typename ConstRandomAccessIterator,typename RandomAccessIterator, typename UnaryPredicate, typename T>
    inline RandomAccessIterator replace_copy_if(ConstRandomAccessIterator first, ConstRandomAccessIterator last, RandomAccessIterator dest_first, UnaryPredicate&& p, T&& new_value )
    {
        concurrency::parallel_for_each(concurrency::extent<1>(last - first), [=, new_value = std::forward<T>(new_value), p = std::forward<UnaryPredicate>(p)](auto&& idx) restrict(amp) {
            *(dest_first + idx[0]) = p(*(first + idx[0])) ? new_value : *(first + idx[0]);
        });

        return dest_first + (last - first);
    }

    //----------------------------------------------------------------------------
    // reverse, reverse_copy
    //----------------------------------------------------------------------------

    template<typename RandomAccessIterator>
    inline void reverse(RandomAccessIterator first, RandomAccessIterator last)
    {
        if (first == last || first == (last - 1)) return;

        concurrency::parallel_for_each(concurrency::extent<1>(half_nonnegative(last - first)), [=] (auto&& idx) restrict(amp) {
            amp_stl_algorithms::iter_swap(first + idx[0], last - idx[0] - 1);
        });
    }

    template<typename ConstRandomAccessIterator, typename RandomAccessIterator>
    inline RandomAccessIterator reverse_copy(ConstRandomAccessIterator first, ConstRandomAccessIterator last, RandomAccessIterator dest_first)
    {
        if (first == last) return dest_first;

        concurrency::parallel_for_each(concurrency::extent<1>(last - first), [=](auto&& idx) restrict(amp) {
            *(dest_first + idx[0]) = *(last - idx[0] - 1);
        });

        return dest_first + (last - first);
    }

    //----------------------------------------------------------------------------
    // rotate, rotate_copy
    //----------------------------------------------------------------------------

    template<typename I>
    inline I rotate(I first, I middle, I last)
    {
		// TODO: this is nice and clean but single-invocation block-swap might be preferable.
		//	   : First and second reverses can and should be collapsed into a single p_f_e.
        if (first == middle) return last;
        if (middle == last) return first;

        amp_stl_algorithms::reverse(first, middle);
        amp_stl_algorithms::reverse(middle, last);
        amp_stl_algorithms::reverse(first, last);

        return first + (last - mid);
    }
    template<typename ConstRandomAccessIterator, typename RandomAccessIterator>
    inline RandomAccessIterator rotate_copy(ConstRandomAccessIterator first, ConstRandomAccessIterator middle, ConstRandomAccessIterator last, RandomAccessIterator dest_first)
    {
        concurrency::parallel_for_each(concurrency::extent<1>(last - first), [=](auto&& idx) restrict(amp) {
            if (idx[0] < (middle - first)) {
				*(dest_first + idx[0] + (last - middle)) = *(first + idx[0]);
			}
			else {
				*(dest_first + idx[0] - (middle - first)) = *(first + idx[0]);
			}
        });
        return dest_first + (last - first);
    }

	//----------------------------------------------------------------------------
	// inclusive_scan, exclusive_scan
	// inplace_inclusive_scan, inplace_exclusive_scan
	//----------------------------------------------------------------------------

	// These are all non-standard, but scan is fundamental enough to warrant presence

	template<typename I, typename Op, int tsz>
	inline Codomain<Op, Value_type<I>, Value_type<I>> _scan_single_tile_inclusive(I first, const Value_type<I>& identity_element, const concurrency::tiled_index<tsz>& tidx, Op&& op) restrict(amp)
	{
		unsigned int s = 0;
		concurrency::array_view<unsigned int> swapped(1, &s);

		concurrency::array_view<Value_type<I>> d0(tidx.tile_dim0, &*first);

		tile_static Value_type<I> buf[tidx.tile_dim0];
		concurrency::array_view<Value_type<I>> d1(tidx.tile_dim0, buf);

		amp_algorithms::_details::static_for<1u, tidx.tile_dim0, amp_algorithms::_details::Inc::mul, 2u>()([=](auto&& h) {
			const auto& in = one(swapped[0]) ? d1 : d0;
			const auto& out = one(swapped[0]) ? d0 : d1;

			if (h <= tidx.local[0]) {
				out[tidx.local[0]] = op(in[tidx.local[0] - h], in[tidx.local[0]]);
			}
			_copy_single_tile(amp_stl_algorithms::cbegin(in), amp_stl_algorithms::cbegin(in) + h, amp_stl_algorithms::begin(out), tidx);

			swapped[0] = !swapped[0]; // A bit unclean.
		});

		return d0[predecessor(tidx.tile_dim0)];
	}

	template<typename I1, typename I2, typename Op>
	inline I2 inclusive_scan(I1 first, I1 last, I2 dest_first, const Value_type<I1>& identity_element, Op&& op)
	{	// TODO: this does not work when more than max_tiles are needed to process the range.
		if (first == last) return dest_first;

		const auto compute_domain = _details::Execution_parameters::tiled_domain(last - first);

		concurrency::array_view<Difference_type<I1>> done(compute_domain.size() / compute_domain.get_tile_extent().size());
		amp_stl_algorithms::fill_n(amp_stl_algorithms::begin(done), compute_domain.size(), last - first);

		concurrency::parallel_for_each(compute_domain, [=, op = std::forward<Op>(op)](auto&& tidx) restrict(amp) {
			const auto n = amp_stl_algorithms::min(tidx.tile_dim0 * 1, last - first - tidx.tile_origin[0]);

			tile_static Value_type<I1> vals[tidx.tile_dim0];
			_copy_single_tile(first + tidx.tile_origin[0], first + tidx.tile_origin[0] + n, vals, tidx);
			_fill_single_tile(vals + n, vals + tidx.tile_dim0, identity_element, tidx);

			if (positive(tidx.tile[0])) {
				if (tidx.tile_origin == tidx.global) {
					auto t = predecessor(tidx.tile[0]);
					while (!concurrency::atomic_compare_exchange(&done[predecessor(tidx.tile[0])], &t, predecessor(tidx.tile[0]))) {
						t = predecessor(tidx.tile[0]);
					}
					vals[0] = op(vals[0], *(dest_first + predecessor(tidx.tile_origin[0])));
				}
			}

			tidx.barrier.wait_with_tile_static_memory_fence();

			const auto s = _scan_single_tile_inclusive(vals, identity_element, tidx, op);

			if (tidx.tile_origin == tidx.global) {
				*(dest_first + tidx.tile_origin[0] + predecessor(n)) = s;
				done[tidx.tile] = tidx.tile[0];
			}

			_copy_single_tile(vals, vals + predecessor(n), dest_first + tidx.tile_origin[0], tidx);
		});
		return dest_first + (last - first);
	}

	template<typename I1, typename I2, typename Op>
	inline I2 exclusive_scan(I1 first, I1 last, I2 dest_first, const Value_type<I1>& identity_element, Op&& op)
	{
		amp_stl_algorithms::inclusive_scan(first, last - 1, dest_first + 1, identity_element, std::forward<Op>(op));
		concurrency::parallel_for_each(concurrency::extent<1>(1), [=, op = std::forward<Op>(op)](auto&&) restrict(amp) { *dest_first = identity_element; });
		return dest_first + (last - first);
	}

	template<typename I, typename Op>
	inline I inplace_inclusive_scan(I first, I last, const Value_type<I>& identity_element, Op&& op)
	{
		return amp_stl_algorithms::inclusive_scan(first, last, first, identity_element, std::forward<Op>(op));
	}

	template<typename I, typename Op>
	inline I inplace_exclusive_scan(I first, I last, const Value_type<I>& identity_element, Op&& op)
	{
		return amp_stl_algorithms::exclusive_scan(first, last, first, identity_element, std::forward<Op>(op));
	}

	//----------------------------------------------------------------------------
	// exclusive_segmented_scan, inclusive_segmented_scan,
	// inplace_exclusive_segmented_scan, inplace_inclusive_segmented_scan
	//----------------------------------------------------------------------------

    //----------------------------------------------------------------------------
    // search, search_n, binary_search
    //----------------------------------------------------------------------------

	template<typename I, typename T, typename C>
	inline bool binary_search(I first, I last, T&& value, C&& cmp)
	{
		if (first == last) return false;

		const concurrency::array_view<unsigned int> found(1);
		const auto p_point = amp_stl_algorithms::partition_point(first, last, [=, value = std::forward<T>(value), cmp = std::forward<C>(cmp)](auto&& x) restrict(amp) { return cmp(x, value);});
		concurrency::parallel_for_each(found.extent, [=, value = std::forward<T>(value)](auto&&) restrict(amp) { found[0] = *p_point == value; });

		return found[0] != 0u;
	}

	template<typename I, typename T>
	inline bool binary_search(I first, I last, T&& value)
	{
		return amp_stl_algorithms::binary_search(first, last, std::forward<T>(value), amp_algorithms::less<>());
	}

    //----------------------------------------------------------------------------
    // set_difference, set_intersection, set_symetric_distance, set_union
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------
    // shuffle, random_shuffle
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------
    // is_sorted, is_sorted_until, sort, partial_sort, partial_sort_copy, stable_sort
    //----------------------------------------------------------------------------

    template<typename ConstRandomAccessIterator, typename Compare>
    inline ConstRandomAccessIterator is_sorted_until(ConstRandomAccessIterator first, ConstRandomAccessIterator last, Compare&& comp)
    {
        if (first == last) return last;

        const auto it = amp_stl_algorithms::adjacent_find(first, last, [comp = std::forward<Compare>(comp)](auto&& a, auto&& b) restrict(amp) { return !(comp(a, b)); });
        return it == last ? it : std::next(it);
    }

    template<typename ConstRandomAccessIterator>
    inline bool is_sorted(ConstRandomAccessIterator first, ConstRandomAccessIterator last)
    {
        return amp_stl_algorithms::is_sorted_until(first, last, amp_algorithms::less_equal<>()) == last;
    }

    template<typename ConstRandomAccessIterator, typename Compare>
    inline bool is_sorted(ConstRandomAccessIterator first, ConstRandomAccessIterator last, Compare&& comp)
    {
        return amp_stl_algorithms::is_sorted_until(first, last, std::forward<Compare>(comp)) == last;
    }

    template<typename ConstRandomAccessIterator>
    inline ConstRandomAccessIterator is_sorted_until(ConstRandomAccessIterator first, ConstRandomAccessIterator last)
    {
        return amp_stl_algorithms::is_sorted_until(first, last, amp_algorithms::less_equal<>());
    }

	template<typename T, typename C>
	inline constexpr const T& _median_of_3_xy(const T& x, const T& y, const T& z, C&& cmp) restrict(cpu, amp)
	{
		return !cmp(z, y) ? y : amp_stl_algorithms::max(x, z, forward<C>(cmp));
	}

	template<typename T, typename C>
	inline constexpr const T& _median_of_3(const T& x, const T& y, const T& z, C&& cmp) restrict(cpu, amp)
	{
		return cmp(y, x) ? _median_of_3_xy(y, x, z, forward<C>(cmp)) : _median_of_3_xy(x, y, z, forward<C>(cmp));
	}

	template<typename I, typename C>
	inline array_view_iterator<const Value_type<I>> _median_of_3_random_amp(I first, I last, C&& cmp)
	{
		static std::mt19937_64 g;
		std::uniform_int_distribution<Difference_type<I>> d(0, last - first - 1);

		concurrency::array_view<Value_type<I>> m(1);
		concurrency::parallel_for_each(m.extent, [=, cmp = std::forward<C>(cmp), x = d(g), y = d(g), z = d(g)](auto&&) restrict(amp) {
			m[0] = _median_of_3(*(first + x), *(first + y), *(first + z), cmp);
		});

		return m;
	}

	template<typename I, typename T, int tsz>
	void _fill_single_tile(I first, I last, T&& value, const concurrency::tiled_index<tsz>& tidx) restrict(amp)
	{
		for (Difference_type<I> i = tidx.local[0]; i < (last - first); i += tidx.tile_dim0) {
			*(first + i) = forward<T>(value);
		}

		tidx.barrier.wait_with_tile_static_memory_fence();
	}

	template<typename I, typename T, typename C, int tsz>
	inline decltype(auto) _partition_3_way_single_tile(I first, I last, const concurrency::tiled_index<tsz>& tidx, T&& piv, C&& cmp) restrict(amp)
	{	// This will only work if (last - first) <= tidx.tile_dim0.
		Value_type<I> v = (tidx.local[0] < (last - first)) ? *(first + tidx.local[0]) :
						  (std::is_default_constructible<Value_type<I>>::value ? Value_type<I>() : *first);
		Difference_type<I> l(0);
		Difference_type<I> g(0);
		if (tidx.local[0] < (last - first)) {
			if (cmp(v, forward<T>(piv))) ++l;
			if (cmp(forward<T>(piv), v)) ++g;
		}

		tile_static Difference_type<I> e_tile;
		tile_static Difference_type<I> g_tile;
		if (tidx.tile_origin == tidx.global) {
			e_tile = Difference_type<I>(0);
			g_tile = Difference_type<I>(last - first);
		}

		tidx.barrier.wait_with_tile_static_memory_fence();

		if (tidx.local[0] < (last - first)) {
			l = concurrency::atomic_fetch_add(&e_tile, l);
			g = concurrency::atomic_fetch_sub(&g_tile, g);

		}

		tidx.barrier.wait_with_tile_static_memory_fence();

		if (tidx.local[0] < (last - first)) {
			if (cmp(v, forward<T>(piv))) *(first + l++) = v;
			if (cmp(forward<T>(piv), v)) *(first + g-- - 1) = v;
		}
		_fill_single_tile(first + e_tile, first + g_tile, forward<T>(piv), tidx);

		return amp_stl_algorithms::pair<Difference_type<I>, Difference_type<I>>(e_tile, g_tile);
	}

	template<typename I1, typename I2, typename C>
	inline std::pair<I1, I1> _partition_3_way_unguarded(I1 first, I1 last, I2 piv, C&& cmp)
	{	// This does not work for cases in which we need more than max_tiles. Shall be fixed (alongside partition).
		// It also needs to be properified (too long / not abstract enough).
		using Offsets = amp_stl_algorithms::pair<Difference_type<I1>, Difference_type<I1>>;

		const auto compute_domain = _details::Execution_parameters::tiled_domain(last - first);
		const concurrency::array_view<Offsets> eg_off(compute_domain.size() / _details::Execution_parameters::tile_size());
		amp_stl_algorithms::fill_n(amp_stl_algorithms::begin(eg_off), eg_off.extent.size(), Offsets(last - first, last - first));

		concurrency::parallel_for_each(compute_domain, [=, cmp = std::forward<C>(cmp)](auto&& tidx) restrict(amp) {
			const auto n = amp_stl_algorithms::min(tidx.tile_dim0 * 1, last - first - tidx.tile_origin[0]);

			tile_static Value_type<I1> vals[tidx.tile_dim0];
			_copy_single_tile(first + tidx.tile_origin[0], first + tidx.tile_origin[0] + n, vals, tidx);

			auto eg = _partition_3_way_single_tile(vals, vals + n, tidx, *piv, cmp);

			tile_static Offsets tile_eg;
			if (tidx.tile_origin == tidx.global) {
				if (positive(tidx.tile[0])) {
					tile_eg.second = last - first;
					while (concurrency::atomic_compare_exchange(&eg_off[predecessor(tidx.tile[0])].second, &tile_eg.second, eg_off[predecessor(tidx.tile[0])].second)) {
						tile_eg.second = last - first;
					}
					tile_eg.first = eg_off[predecessor(tidx.tile[0])].first;
				}
				else {
					tile_eg = Offsets(0, 0);
				}
			}

			tidx.barrier.wait_with_tile_static_memory_fence();

			if (positive(eg.second)) {
				if (tile_eg.first != tidx.tile_origin[0]) {
					const auto a = _swap_ranges_single_tile(first + tile_eg.first, first + tidx.tile_origin[0], vals, vals + eg.first, tidx);
					const auto b = _swap_ranges_single_tile(first + amp_stl_algorithms::max(tile_eg.first + a.first, tile_eg.second),
															first + tidx.tile_origin[0], vals, vals + eg.second, tidx);
					_reverse_single_tile(vals, vals + eg.second, tidx);
				}
				_copy_single_tile(vals, vals + n, first + tidx.tile_origin[0], tidx);
			}

			tidx.barrier.wait_with_tile_static_memory_fence();

			if (tidx.tile_origin == tidx.global) {
				eg_off[tidx.tile] = Offsets(tile_eg.first + eg.first, tile_eg.second + eg.second);
			}
		});

		return std::make_pair(first + eg_off[predecessor(eg_off.extent.size())].first, first + eg_off[predecessor(eg_off.extent.size())].second);
	}

	template<typename T, typename C>
	inline bool _compare_and_exchange(T&& x, T&& y, C&& cmp) restrict(amp)
	{
		if (cmp(y, x)) {
			amp_stl_algorithms::swap(forward<T>(x), forward<T>(y));
			return true;
		}
		return false;
	}

	template<typename I, typename C, int tsz>
	inline bool _odd_even_sort_single_tile_n(I first, Difference_type<I> n, const concurrency::tiled_index<tsz>& tidx, C&& cmp) restrict(amp)
	{	// TODO: this is unclean and not very nice, but since it is just a placeholder it will stay in during testing.
		Difference_type<I> even_idx = twice(tidx.local[0]);
		Difference_type<I> odd_idx = successor(twice(tidx.local[0]));

		tile_static bool unsorted;
		if (tidx.tile_origin == tidx.global) {
			unsorted = false;
		}

		tile_static unsigned int swaps;
		do {
			tidx.barrier.wait_with_tile_static_memory_fence();

			if (tidx.tile_origin == tidx.global) {
				swaps = 0u;
			}

			tidx.barrier.wait_with_tile_static_memory_fence();

			if (successor(odd_idx) < n) {
				if (_compare_and_exchange(*(first + odd_idx), *(first + successor(odd_idx)), forward<C>(cmp))) {
					concurrency::atomic_fetch_inc(&swaps);
				}
			}

			tidx.barrier.wait_with_tile_static_memory_fence();

			if (successor(even_idx) < n) {
				if (_compare_and_exchange(*(first + even_idx), *(first + successor(even_idx)), forward<C>(cmp))) {
					concurrency::atomic_fetch_inc(&swaps);
				}
			}

			tidx.barrier.wait_with_tile_static_memory_fence();

			if ((tidx.tile_origin == tidx.global) && !unsorted && positive(swaps)) {
				unsorted = true;
			}

			tidx.barrier.wait_with_tile_static_memory_fence();
		} while (positive(swaps));

		return unsorted;
	}

	template<typename I, typename C>
	inline bool _odd_even_sort_tiles_pass(I first, I last, C&& cmp, bool even_pass = false)
	{
		concurrency::array_view<unsigned int> swaps(1);
		concurrency::parallel_for_each(swaps.extent, [=](auto&&) restrict(amp) { swaps[0] = 0u; });

		const auto compute_domain = _details::Execution_parameters::tiled_domain(half_nonnegative(last - first));
		concurrency::parallel_for_each(compute_domain, [=, cmp = std::forward<C>(cmp)](auto&& tidx) restrict(amp) {
			Difference_type<I> t = twice(tidx.tile_origin[0]) + (even_pass ? Difference_type<I>(0) : tidx.tile_dim0);
			while (t < (last - first)) {
				const auto n = amp_stl_algorithms::min(twice(tidx.tile_dim0), last - first - t);

				tile_static Value_type<I> vals[twice(tidx.tile_dim0)];
				_copy_single_tile(first + t, first + t + n, vals, tidx);

				auto was_unsorted = _odd_even_sort_single_tile_n(vals, n, tidx, cmp);

				_copy_single_tile(vals, vals + n, first + t, tidx);

				if ((tidx.tile_origin == tidx.global) && was_unsorted && zero(swaps[0])) {
					concurrency::atomic_fetch_inc(&swaps[0]);
				}

				t += twice(compute_domain.size());
			}
		});

		return zero(swaps[0]);
	}

	template<typename I, typename C>
	inline void _odd_even_sort_tiles_unguarded(I first, I last, C&& cmp)
	{
		bool sorted;
		do {
			sorted = _odd_even_sort_tiles_pass(first, last, std::forward<C>(cmp));
			sorted = sorted && _odd_even_sort_tiles_pass(first, last, std::forward<C>(cmp), true);
		}
		while (!sorted);
	}

	template<typename I, typename C>
	inline void _partition_into_tiles(I first, I last, C&& cmp)
	{	// This is just a proof-of-concept. It is neither fast enough nor an optimal implementation of Quicksort - will be overhauled for release.
		// The three-way partitioning triggers an annoying bug in current IHV drivers, and is therefore temporarily broken and disabled.
		while (_details::Execution_parameters::tile_size() < (last - first)) {
			const auto p = _median_of_3_random_amp(first, last, std::forward<C>(cmp));
			const auto e = amp_stl_algorithms::partition(first, last, [=, cmp = std::forward<C>(cmp)](auto&& x) restrict(amp) { return cmp(x, *p); });//_partition_3_way_unguarded(first, last, _median_of_3_random_amp(first, last, std::forward<C>(cmp)), std::forward<C>(cmp));
			const auto g = amp_stl_algorithms::partition(e, last, [=, cmp = std::forward<C>(cmp)](auto&& x) restrict(amp) { return !cmp(x, *p) && !cmp(*p, x); });

			if ((g - e) == (last - first)) return;

			if ((e - first) < (last - g)) {
				_partition_into_tiles(first, e, std::forward<C>(cmp));
				first = g;
			}
			else {
				_partition_into_tiles(g, last, std::forward<C>(cmp));
				last = e;
			}
		}
	}

	template<typename I, typename C>
	inline void sort(I first, I last, C&& cmp)
	{
		if ((first == last) || (first == (last - 1))) return;

		_partition_into_tiles(first, last, std::forward<C>(cmp));
		if (!one(_details::Execution_parameters::tile_size())) {
			_odd_even_sort_tiles_unguarded(first, last, cmp);
		}

	}

	template<typename I>
	inline void sort(I first, I last)
	{
		return amp_stl_algorithms::sort(first, last, amp_algorithms::less<>());
	}

    //----------------------------------------------------------------------------
    // swap, swap<T, N>, swap_ranges, iter_swap
    //----------------------------------------------------------------------------

    template<typename T>
    void swap(T& a, T& b) restrict(cpu, amp)
    {
        T tmp = amp_algorithms::move(a);
        a = amp_algorithms::move(b);
        b = amp_algorithms::move(tmp);
    }

    // TODO: Are there other overloads of swap() that should be implemented.
    template<typename T, int N>
    inline void swap(T (&a)[N], T (&b)[N]) restrict(cpu, amp)
    {
        for (int i = 0; i != N; ++i) {
            amp_stl_algorithms::swap(a[i], b[i]);
        }
    }

    template<typename RandomAccessIterator1, typename RandomAccessIterator2>
    inline RandomAccessIterator2 swap_ranges(RandomAccessIterator1 first1, RandomAccessIterator1 last1, RandomAccessIterator2 first2)
    {
        concurrency::parallel_for_each(concurrency::extent<1>(last1 - first1), [=] (auto&& idx) restrict(amp) {
            amp_stl_algorithms::iter_swap(first1 + idx[0], first2 + idx[0]);
        });

        return first2 + (last1 - first1);
    }

    template<typename RandomAccessIterator1, typename RandomAccessIterator2>
    inline void iter_swap(RandomAccessIterator1 a, RandomAccessIterator2 b) restrict(cpu, amp)
    {
        amp_stl_algorithms::swap(*a, *b);
    }

    //----------------------------------------------------------------------------
    // transform (Unary)
    //----------------------------------------------------------------------------

    // The "UnaryFunction" functor needs to be callable as "func(ConstRandomAccessIterator::value_type)".
    // The functor needs to be blittable and cannot contain any array, array_view, or textures.

    template<typename ConstRandomAccessIterator,typename RandomAccessIterator, typename UnaryFunction>
    inline RandomAccessIterator transform(ConstRandomAccessIterator first, ConstRandomAccessIterator last, RandomAccessIterator dest_first, UnaryFunction&& func)
    {
        concurrency::parallel_for_each(concurrency::extent<1>(last - first), [=, func = std::forward<UnaryFunction>(func)] (auto&& idx) restrict(amp) {
            *(dest_first + idx[0]) = func(*(first + idx[0]));
        });

        return dest_first;
    }

    // The "BinaryFunction" functor needs to be callable as "func(ConstRandomAccessIterator1::value_type, ConstRandomAccessIterator2::value_type)".
    // The functor needs to be blittable and cannot contain any array, array_view, or textures.

    template<typename ConstRandomAccessIterator1, typename ConstRandomAccessIterator2,typename RandomAccessIterator, typename BinaryFunction>
    inline RandomAccessIterator transform(ConstRandomAccessIterator1 first1, ConstRandomAccessIterator1 last1, ConstRandomAccessIterator2 first2, RandomAccessIterator dest_first, BinaryFunction&& func)
    {
        concurrency::parallel_for_each(concurrency::extent<1>(last1 - first1), [=, func = std::forward<BinaryFunction>(func)] (auto&& idx) restrict(amp) {
            *(dest_first + idx[0]) = func(*(first1 + idx[0]), *(first2 + idx[0]));
        });

        return dest_first;
    }

    //----------------------------------------------------------------------------
    // unique, unique_copy
    //----------------------------------------------------------------------------

	template<typename I, typename P>
	inline I unique(I first, I last, P&& pred)
	{
		if (first == last) return last;

		static constexpr auto tsz = _details::Execution_parameters::tile_size();
		concurrency::array_view<unsigned int> locks(_details::Execution_parameters::tile_cnt(last - first));
		concurrency::array_view<Difference_type<RandomAccessIterator>> off(1);

		static constexpr unsigned int locked = 1u;
		static constexpr unsigned int unlocked = 0u;
		concurrency::parallel_for_each(locks.extent, [=](auto&& idx) restrict(amp) {
			if (zero(idx[0])) off[0] = Difference_type<RandomAccessIterator>(0);
			locks[idx] = locked;
		});

		const auto compute_domain = _details::Execution_parameters::tiled_domain(last - first);

		return first + off[0];

	}

	template<typename I>
	inline I unique(I first, I last)
	{
		return amp_stl_algorithms::unique(first, last, amp_algorithms::equal_to<>());
	}
}// namespace amp_stl_algorithms
