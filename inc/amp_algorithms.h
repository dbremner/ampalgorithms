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

// TODO: Does it really make a lot of sense to declare two namespaces or should everything be flattened into amp_algorithms?
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

    template <typename T = void>
    struct plus : std::binary_function<T, T, T> {
        constexpr T operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return (a + b);
        }
    };
    template<>
    struct plus<void> {
        // This should be corrected once full constexpr support is in place
        template<typename T, typename U>
        constexpr std::common_type_t<T, U> operator()(T&& a, U&& b) const restrict(cpu, amp)
        {
            return a + b;
        }
    };

    template <typename T = void>
    struct minus : std::binary_function<T, T, T> {
        constexpr T operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return (a - b);
        }
    };
    template<>
    struct minus<void> {
        template<typename T, typename U>
        constexpr std::common_type_t<T, U> operator()(T&& a, U&& b) const restrict(cpu, amp)
        {
            return a - b;
        }
    };

    template <typename T = void>
    struct multiplies : std::binary_function<T, T, T> {
        constexpr T operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return (a * b);
        }
    };
    template<>
    struct multiplies<void> {
        template<typename T, typename U>
        constexpr std::common_type_t<T, U> operator()(T&& a, U&& b) const restrict(cpu, amp)
        {
            return a * b;
        }
    };

    template<typename T = void>
    struct divides : std::binary_function<T, T, T> {
        constexpr T operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return a / b;
        }
    };
    template<>
    struct divides<void> {
        template<typename T, typename U>
        constexpr std::common_type_t<T, U> operator()(T&& a, U&& b) const restrict(cpu, amp)
        {
            return a / b;
        }
    };

    template <typename T = void>
    struct modulus : std::binary_function<T, T, T> {
        constexpr T operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return (a % b);
        }
    };
    template<>
    struct modulus<void> {
        template<typename T, typename U>
        constexpr std::common_type_t<T, U> operator()(T&& a, U&& b) const restrict(cpu, amp)
        {
            return a % b;
        }
    };

    template <typename T = void>
    struct negate : std::unary_function<T, T> {
        constexpr T operator()(const T &a) const restrict(cpu, amp)
        {
            return (-a);
        }
    };
    template<>
    struct negate<void> {
        template<typename T>
        decltype(auto) operator()(T&& a) const restrict(cpu, amp)
        {
            return -a;
        }
    };

    //----------------------------------------------------------------------------
    // Additional arithmetic operations with no STL equivalents
    //----------------------------------------------------------------------------
	// TODO: constexpr-esize all of these functions.
	template<typename T, std::enable_if_t<std::is_integral<T>::value>* = nullptr>
	static inline constexpr T static_log2(T&& val) restrict(cpu, amp)
	{
		return (zero(forward<T>(val)) || one(forward<T>(val))) ? T(0) : (T(1) + static_log2(half_nonnegative(forward<T>(val))));
	}

    template<unsigned int N>
    struct static_is_power_of_two
    {
        enum { value = ((static_count_bits<N, bit32>::value == 1) ? TRUE : FALSE) };
    };

    // TODO: Generalize this for other integer types.
    template <typename T>
    inline bool is_power_of_two(T value)
    {
        return count_bits(value) == 1;
    }

    //----------------------------------------------------------------------------
    // Comparison operations
    //----------------------------------------------------------------------------

    template <typename T = void>
    struct equal_to : std::binary_function<T, T, T> {
        constexpr bool operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return (a == b);
        }
    };
    template<>
    struct equal_to<void> {
        template<typename T, typename U>
        constexpr bool operator()(T&& a, U&& b) const restrict(cpu, amp)
        {
            return a == b;
        }
    };

    template <typename T = void>
    struct not_equal_to : std::binary_function<T, T, T> {
        constexpr bool operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return !equal_to<T>()(a, b);
        }
    };
    template<>
    struct not_equal_to<void> {
        template<typename T, typename U>
        /*constexpr*/ bool operator()(T&& a, U&& b) const restrict(cpu, amp)
        {
            return !equal_to<>()(a, b);
        }
    };

    template <typename T = void>
    struct less : std::binary_function<T, T, T> {
        constexpr bool operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return (a < b);
        }
    };
    template<>
    struct less<void> {
        template<typename T, typename U>
        constexpr bool operator()(T&& a, U&& b) const restrict(cpu, amp)
        {
            return a < b;
        }
    };

    template <typename T = void>
    struct less_equal : std::binary_function<T, T, T> {
        constexpr bool operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return !(b < a);
        }
    };
    template<>
    struct less_equal<void> {
        template<typename T, typename U>
        constexpr bool operator()(T&& a, U&& b) const restrict(cpu, amp)
        {
            return !(b < a);
        }
    };

    template <typename T = void>
    struct greater : std::binary_function<T, T, T> {
        constexpr bool operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return !less_equal<T>()(a, b);
        }
    };
    template<>
    struct greater<void> {
        template<typename T, typename U>
        /*constexpr*/ bool operator()(T&& a, U&& b) const restrict(cpu, amp)
        {
            return !less_equal<>()(a, b);
        }
    };

    template <typename T = void>
    struct greater_equal : std::binary_function<T, T, T> {
        constexpr bool operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return !(a < b);
        }
    };
    template<>
    struct greater_equal<void> {
        template<typename T, typename U>
        constexpr bool operator()(T&& a, U&& b) const restrict(cpu, amp)
        {
            return !(a < b);
        }
    };

#ifdef max
#error amp_algorithms encountered a definition of the macro max.
#endif

    template <typename T = void>
    struct max : std::binary_function<T, T, T> {
        constexpr const T& operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return ((b < a) ? a : b);
        }
    };
    template<>
    struct max<void> {
        template<typename T, typename U>
        constexpr const std::remove_reference_t<std::common_type_t<T, U>>& operator()(T&& a, U&& b) const restrict(cpu, amp)
        {
            return b < a ? a : b;
        }
    };

#ifdef min
#error amp_algorithms encountered a definition of the macro min.
#endif

    template <typename T = void>
    struct min : std::binary_function<T, T, T> {
        constexpr const T& operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return ((b < a) ? b : a);
        }
    };
    template<>
    struct min<void> {
        template<typename T, typename U>
        constexpr std::common_type_t<T, U> operator()(T&& a, U&& b) const restrict(cpu, amp)
        {
            return b < a ? b : a;
        };
    };

    //----------------------------------------------------------------------------
    // Logical operations
    //----------------------------------------------------------------------------

    template<class T>
    struct logical_not : std::unary_function<T, T> {
        constexpr bool operator()(const T& a) const restrict(cpu, amp)
        {
            return (!a);
        }
    };

    template<class T>
    struct logical_and : public std::binary_function<T, T, T> {
        constexpr bool operator()(const T& a, const T& b) const restrict(cpu, amp)
        {
            return (a && b);
        }
    };

    template<class T>
    struct logical_or : public std::binary_function<T, T, T> {
        constexpr bool operator()(const T& a, const T& b) const restrict(cpu, amp)
        {
            return (a || b);
        }
    };

    template <typename Predicate>
    class unary_negate
    {
    protected:
        Predicate m_pred;

    public:
        typedef typename Predicate::argument_type argument_type;
        typedef bool result_type;

        explicit unary_negate(Predicate&& pred) : m_pred(std::forward<Predicate>(pred)) {}

        constexpr result_type operator() (const argument_type& a) const restrict(cpu, amp)
        {
            return !m_pred(a);
        }
    };

    template <typename Predicate>
    amp_algorithms::unary_negate<Predicate> not1(Predicate&& pred) restrict(cpu)
    {
        return amp_algorithms::unary_negate<Predicate>(std::forward<Predicate>(pred));
    }
    template <typename Predicate>
    amp_algorithms::unary_negate<Predicate> not1(Predicate&& pred) restrict(amp)
    {
        return amp_algorithms::unary_negate<Predicate>(forward<Predicate>(pred));
    }

    template<typename Predicate>
    class binary_negate
    {
    protected:
        Predicate m_pred;

    public:
        typedef typename Predicate::first_argument_type first_argument_type;
        typedef typename Predicate::second_argument_type second_argument_type;
        typedef bool result_type;

        explicit binary_negate(Predicate&& pred) : m_pred(std::forward<Predicate>(pred)) {}

        constexpr result_type operator()(const first_argument_type& a, const second_argument_type& b) const restrict(cpu, amp)
        {
            return !m_pred(a, b);
        }
    };

    template<typename Predicate>
    amp_algorithms::binary_negate<Predicate> not2(Predicate&& pred) restrict(cpu, amp)
    {
        return amp_algorithms::binary_negate<Predicate>(amp_algorithms::forward<Predicate>(pred));
    }

    //----------------------------------------------------------------------------
    // Bitwise operations
    //----------------------------------------------------------------------------

    template<typename T>
    struct bit_and : std::binary_function<T, T, T> {
        constexpr T operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return (a & b);
        }
    };

    template <typename T>
    struct bit_or : std::binary_function<T, T, T> {
        constexpr T operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return (a | b);
        }
    };

    template <typename T>
    struct bit_xor : std::binary_function<T, T, T> {
        constexpr T operator()(const T &a, const T &b) const restrict(cpu, amp)
        {
            return (a ^ b);
        }
    };

    template <typename T>
    struct bit_not : std::unary_function<T, T> {
        constexpr T operator()(const T &a) const restrict(cpu, amp)
        {
            return (~a);
        }
    };

    //----------------------------------------------------------------------------
    // Additional bitwise operations with no STL equivalent
    //----------------------------------------------------------------------------

    static constexpr unsigned int bit08 = 0x80;
    static constexpr unsigned int bit16 = 0x8000;
    static constexpr unsigned int bit32 = 0x80000000;

    namespace _details
    {
        template<unsigned int N, int MaxBit>
        struct is_bit_set
        {
            enum { value = (N & MaxBit) ? 1 : 0 };
        };
    };

    template<unsigned int N, unsigned int MaxBit = bit32>
    struct static_count_bits
    {
        enum { value = (_details::is_bit_set<N, MaxBit>::value + static_count_bits<N, (MaxBit >> 1)>::value) };
    };

    template<unsigned int N>
    struct static_count_bits<N, 0>
    {
        enum { value = FALSE };
    };

    template<typename T, std::enable_if_t<std::is_integral<T>::value>* = nullptr>
    inline unsigned int count_bits(T value) restrict(cpu)
    {
        value = value - ((value >> 1) & 0x55555555);
        value = (value & 0x33333333) + ((value >> 2) & 0x33333333);
        return (((value + (value >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
    }

	template<typename T, std::enable_if_t<std::is_integral<T>::value>* = nullptr>
	inline unsigned int count_bits(T value) restrict(amp)
	{
		return concurrency::direct3d::countbits(value);
	}

#pragma endregion

#pragma region Byte pack and unpack, padded read and write

    //----------------------------------------------------------------------------
    // Byte pack and unpack
    //----------------------------------------------------------------------------

    template<int index, typename T>
    inline unsigned long pack_byte(const T& value) restrict(cpu, amp)
    {
        assert(value < 256);
        static_assert(index < sizeof(T), "Index out of range.");
        return (static_cast<unsigned long>(value) && 0xFF) << (index * CHAR_BIT);
    }

    template<typename T>
    inline unsigned long pack_byte(const T& value, unsigned index) restrict(cpu, amp)
    {
        //assert(value < 256);
        //assert(index < sizeof(T));
        return (static_cast<unsigned long>(value) && 0xFF) << (index * CHAR_BIT);
    }

    template<int index, typename T>
    inline unsigned int unpack_byte(const T& value) restrict(cpu, amp)
    {
        static_assert(index < sizeof(T), "Index out of range.");
        return (value >> (index * CHAR_BIT)) & 0xFF;
    }

    template<typename T>
    inline unsigned int unpack_byte(const T& value, unsigned index) restrict(cpu, amp)
    {
        //assert(index < sizeof(T));
        return (value >> (index * CHAR_BIT)) & 0xFF;
    }

    template<typename T>
    constexpr unsigned int bit_count() restrict(cpu, amp)
    {
        return sizeof(T) * CHAR_BIT;
    }

    //----------------------------------------------------------------------------
    // container padded_read & padded_write
    //----------------------------------------------------------------------------

    template <typename T, int R>
    inline decltype(auto) padded_read(const concurrency::array_view<T, R>& arr, const concurrency::index<R>& idx) restrict(cpu, amp)
    {
        return arr.extent.contains(idx) ? arr[idx] : T();
    }

    template <typename T>
    inline decltype(auto) padded_read(const concurrency::array_view<T>& arr, int idx) restrict(cpu, amp)
    {
        return padded_read(arr, concurrency::index<1>(idx));
    }

    template <typename T, typename U, int R>
    inline void padded_write(const concurrency::array_view<T, R>& arr, const concurrency::index<R>& idx, U&& value) restrict(cpu, amp)
    {
        if (arr.extent.contains(idx)) {
            arr[idx] = value;
        }
    }

    template <typename T, typename U>
    inline void padded_write(const concurrency::array_view<T>& arr, int idx, U&& value) restrict(cpu, amp)
    {
        padded_write(arr, concurrency::index<1>(idx), value);
    }

    // TODO: Should this return an extent? Better name.
    template <int N, typename InputIndexableView>
    inline int tile_partial_data_size(const InputIndexableView& arr, const tiled_index<N>& tidx) restrict(amp)
    {
        return arr.extent.size() - tidx.tile[0] * tidx.tile_extent[0];
    }

#pragma endregion

    //----------------------------------------------------------------------------
    // bitvector
    //----------------------------------------------------------------------------

    template <typename T>
    class uniform_segments
    {
    private:
        int m_step;

    public:
        uniform_segments(int step) : m_step(step) { }

        bool operator()(const T &i) const
        {
            return (i % m_step == 0);
        }
    };

    struct bitvector
    {
    private:
        unsigned int m_data_size;

    public:
        std::vector<unsigned> data;

#if _MSC_VER >= 1800
        bitvector() = delete;
        ~bitvector() = default;
        bitvector(const bitvector&) = default;
        bitvector(bitvector&&) = default;
        bitvector& operator=(const bitvector&) = default;
        bitvector& operator=(bitvector&&) = default;
#endif
        bitvector(unsigned data_size) : m_data_size(data_size)
        {
            data = std::vector<unsigned>(bits_pad_to_uint(data_size), 0);
        }

        // Initialize bitvector with constant segment width.

        void initialize(int segment_width)
        {
            initialize(uniform_segments<int>(segment_width));
        }

        // Initialize bitvector with custom segment widths.

        template <typename Func>
        void initialize(Func pred)
        {
            unsigned flag_counter = 0;
            for (unsigned idx = 0; idx < data.size() && flag_counter < m_data_size; ++idx)
            {
                unsigned bag_of_bits = data[idx];
                for (unsigned offset = 0; offset < amp_algorithms::bit_count<unsigned>() && flag_counter < m_data_size; ++offset)
                {
                    if (pred(flag_counter))
                    {
                        bag_of_bits |= 1 << offset;
                    }
                    flag_counter++;
                }
                data[idx] = bag_of_bits;
            }
        }

        bool is_bit_set(unsigned pos, amp_algorithms::scan_direction direction = amp_algorithms::scan_direction::forward)
        {
            // When we encounter flag going direction it means,
            // that it is the first element of this segment (last element to be scanned going direction)
            // for simplification we increment 'pos' and always look for flags behind our current position.

            if (direction == amp_algorithms::scan_direction::backward)
            {
                pos++;
            }
            unsigned idx = pos / bit_count<unsigned>();
            unsigned offset = pos % bit_count<unsigned>();
            unsigned bag_of_bits = data[idx];
            return (1 << offset & bag_of_bits) > 0;
        }

    private:
        unsigned bits_pad_to_uint(unsigned bits) const
        {
            return (bits + amp_algorithms::bit_count<unsigned>() - 1) / amp_algorithms::bit_count<unsigned>();
        }
    };

    //----------------------------------------------------------------------------
    // fill
    //----------------------------------------------------------------------------

    template<typename T>
    void fill(const concurrency::accelerator_view &accl_view, const concurrency::array_view<T>& output_view, const T& value)
    {
        ::amp_algorithms::generate(accl_view, output_view, [value]() restrict(amp) { return value; });
    }

    template<typename T>
    void fill(const concurrency::array_view<T>& output_view, const T& value)
    {
        ::amp_algorithms::generate(output_view, [value]() restrict(amp) { return value; });
    }

    //----------------------------------------------------------------------------
    // generate
    //----------------------------------------------------------------------------

    template <typename T, typename Generator>
    void generate(const concurrency::accelerator_view &accl_view, const concurrency::array_view<T>& output_view, Generator&& generator)
    {
        _details::parallel_for_each(accl_view, output_view.extent, [output_view, generator = std::forward<Generator>(generator)](auto&& idx) restrict(amp) {
            output_view[idx] = generator();
        });
    }

    template <typename T, typename Generator>
    void generate(const concurrency::array_view<T>& output_view, Generator&& generator)
    {
        ::amp_algorithms::generate(_details::auto_select_target(), output_view, std::forward<Generator>(generator));
    }

    //----------------------------------------------------------------------------
    // merge_sort
    //----------------------------------------------------------------------------

    // TODO_NOT_IMPLEMENTED: merge_sort
    template <typename T, typename BinaryOperator>
    void merge_sort(const concurrency::accelerator_view& accl_view, const concurrency::array_view<T>& input_view, BinaryOperator&& op)
    {
    }

    template <typename T>
    void merge_sort(const concurrency::accelerator_view& accl_view, const concurrency::array_view<T>& input_view)
    {
        ::amp_algorithms::merge_sort(accl_view, input_view, amp_algorithms::less<T>());
    }

    //----------------------------------------------------------------------------
    // radix_sort
    //----------------------------------------------------------------------------

    template <typename T>
    inline void radix_sort(const concurrency::accelerator_view& accl_view, concurrency::array_view<T>& input_view, concurrency::array_view<T>& output_view)
    {
        static constexpr int bin_width = 2;
        static constexpr int tile_size = 128;
        _details::radix_sort<T, tile_size, bin_width>(accl_view, input_view, output_view);
    }

    // TODO: input_view should be a const.
    template <typename T>
    inline void radix_sort(concurrency::array_view<T>& input_view, concurrency::array_view<T>& output_view)
    {
        radix_sort(_details::auto_select_target(), input_view, output_view);
    }

    template <typename T>
    inline void radix_sort(const concurrency::accelerator_view& accl_view, const concurrency::array_view<T>& input_view)
    {
        radix_sort(accl_view, input_view, input_view);
    }

    template <typename T>
    inline void radix_sort(concurrency::array_view<T>& input_view)
    {
        radix_sort(_details::auto_select_target(), input_view, input_view);
    }

    //----------------------------------------------------------------------------
    // reduce
    //----------------------------------------------------------------------------

    // Generic reduction template for binary operators that are commutative and associative
    template <typename IndexableInputView, typename BinaryFunction>
    inline Value_type<IndexableInputView> reduce(const concurrency::accelerator_view& accl_view, const IndexableInputView& input_view, BinaryFunction&& binary_op)
    {
        constexpr int tile_size = 512;
        return _details::reduce<tile_size, 10000>(accl_view, input_view, std::forward<BinaryFunction>(binary_op));
    }

    template <typename IndexableInputView, typename BinaryFunction>
    inline Value_type<IndexableInputView> reduce(const IndexableInputView& input_view, BinaryFunction&& binary_op)
    {
        return reduce(_details::auto_select_target(), input_view, std::forward<BinaryFunction>(binary_op));
    }

    //----------------------------------------------------------------------------
    // scan
    //----------------------------------------------------------------------------

    // This header needs these enums but they also have to be defined in the _impl header for use by the
    // main STL header, which includes the _impl.
#if !defined(AMP_ALGORITHMS_ENUMS)
#define AMP_ALGORITHMS_ENUMS
    enum class scan_mode : int
    {
        exclusive = 0,
        inclusive = 1
    };

    enum class scan_direction : int
    {
        forward = 0,
        backward = 1
    };
#endif

    template <int TileSize, scan_mode _Mode, typename T, typename _BinaryFunc>
    inline void scan(const concurrency::array_view<T>& input_view, const concurrency::array_view<T>& output_view, _BinaryFunc&& op)
    {
        _details::scan<TileSize, _Mode>(_details::auto_select_target(), input_view, output_view, std::forward<_BinaryFunc>(op));
    }

    template <typename T>
    void scan_exclusive(const concurrency::accelerator_view& accl_view, const concurrency::array_view<T>& input_view, const concurrency::array_view<T>& output_view)
    {
        _details::scan<_details::scan_default_tile_size, amp_algorithms::scan_mode::exclusive>(accl_view, input_view, output_view, amp_algorithms::plus<>());
    }

    template <typename T>
    void scan_exclusive(const concurrency::array_view<T>& input_view, const concurrency::array_view<T>& output_view)
    {
        _details::scan<_details::scan_default_tile_size, amp_algorithms::scan_mode::exclusive>(_details::auto_select_target(), input_view, output_view, amp_algorithms::plus<>());
    }

    template <typename T>
    void scan_inclusive(const concurrency::accelerator_view& accl_view, const concurrency::array_view<T>& input_view, const concurrency::array_view<T>& output_view)
    {
        _details::scan<_details::scan_default_tile_size, amp_algorithms::scan_mode::inclusive>(accl_view, input_view, output_view, amp_algorithms::plus<>());
    }

    template <typename T>
    void scan_inclusive(const concurrency::array_view<T>& input_view, const concurrency::array_view<T>& output_view)
    {
        _details::scan<_details::scan_default_tile_size, amp_algorithms::scan_mode::inclusive>(_details::auto_select_target(), input_view, output_view, amp_algorithms::plus<>());
    }

    //----------------------------------------------------------------------------
    // transform (unary)
    //----------------------------------------------------------------------------

    template <typename T, typename U, int R, typename UnaryFunc>
    void transform(const concurrency::accelerator_view &accl_view, const concurrency::array_view<const T, R>& input_view, const concurrency::array_view<U, R>& output_view, UnaryFunc&& func)
    {
        _details::parallel_for_each(accl_view, output_view.extent, [input_view,output_view,func = std::forward<UnaryFunc>(func)] (auto&& idx) restrict(amp) {
            output_view[idx] = func(input_view[idx]);
        });
    }

    template <typename T, typename U, int R, typename UnaryFunc>
    void transform(const concurrency::array_view<const T, R>& input_view, const concurrency::array_view<U, R>& output_view, UnaryFunc&& func)
    {
        ::amp_algorithms::transform(_details::auto_select_target(), input_view, output_view, std::forward<UnaryFunc>(func));
    }

    //----------------------------------------------------------------------------
    // transform (binary)
    //----------------------------------------------------------------------------

    template <typename T, typename U, typename V, int R, typename BinaryFunc>
    void transform(const concurrency::accelerator_view &accl_view, const concurrency::array_view<T, R>& input_view1, const concurrency::array_view<U, R>& input_view2, const concurrency::array_view<V, R>& output_view, BinaryFunc&& func)
    {
        _details::parallel_for_each(accl_view, output_view.extent, [input_view1,input_view2,output_view,func = std::forward<BinaryFunc>(func)] (auto&& idx) restrict(amp) {
            output_view[idx] = func(input_view1[idx], input_view2[idx]);
        });
    }

    template <typename T, typename U, typename V, int R, typename BinaryFunc>
    void transform(const concurrency::array_view<T, R>& input_view1, const concurrency::array_view<U, R>& input_view2, const concurrency::array_view<V, R>& output_view, BinaryFunc&& func)
    {
        ::amp_algorithms::transform(_details::auto_select_target(), input_view1, input_view2, output_view, std::forward<BinaryFunc>(func));
    }

    //----------------------------------------------------------------------------
    // forward and move for restrict(amp) contexts; should be moved elsewhere.
    //----------------------------------------------------------------------------
    template<typename T>
    constexpr inline T&& forward(std::remove_reference_t<T>& t) restrict(cpu, amp)
    {
        return static_cast<T&&>(t);
    }
    template<typename T>
    constexpr inline T&& forward(std::remove_reference_t<T>&& t) restrict(cpu, amp)
    {
        return static_cast<T&&>(t);
    }

    template<typename T>
    constexpr inline std::remove_reference_t<T>&& move(T&& t) restrict(cpu, amp)
    {
        return static_cast<std::remove_reference_t<T>&&>(t);
    }

} // namespace amp_algorithms
