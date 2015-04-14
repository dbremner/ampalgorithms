 /*----------------------------------------------------------------------------
* Copyright © Microsoft Corp.
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
* This file contains the additional utilities to support unit tests.
*---------------------------------------------------------------------------*/

#pragma once
#ifndef _TESTTOOLS_H_BUMPTZI
#define _TESTTOOLS_H_BUMPTZI

#include <amp_algorithms.h>
#include <amp_stl_algorithms.h>
#include <amp_algorithms_type_functions_helpers.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <random>
#include <vector>

//  Define these namespaces and types to pick up poorly specified namespaces and
//  types in library code. This makes the test code more like a real library
//  client which may define conflicting namespaces.

namespace details { };
namespace _details { };
namespace direct3d { };
namespace graphics { };
namespace fast_math { };
namespace precise_math { };

class extent { };
class index { };
class array { };

//  Set USE_REF to use the REF accelerator for all tests. This is useful if tests
//  fail on a particular machine as failure may be due to a driver bug.

namespace testtools
{
    inline void set_default_accelerator(std::wstring test_name)
    {
#if (defined(USE_REF) || defined(_DEBUG))
        std::wstring dev_path = concurrency::accelerator().device_path;
        bool set_ok = concurrency::accelerator::set_default(concurrency::accelerator::direct3d_ref) ||
            (dev_path == concurrency::accelerator::direct3d_ref);

        if (!set_ok)
        {
            std::wstringstream str;
            str << "Unable to set default accelerator to REF. Using "
				<< dev_path << "." << std::endl;
        }
#endif
        concurrency::accelerator().get_default_view().flush();
    }

    //=========================================================================
    //  Helper functions to generate test data of random numbers.
    //=========================================================================

    template<typename T>
    inline int test_array_size()
    {
        int size;
#if _DEBUG
        size = 1023;
#else
        size = 1023 + 1029;
#endif
        if (std::is_integral<T>::value)       size *= 13;
        if (std::is_floating_point<T>::value) size *= 5;

        return size;
}

	template<typename T,
			 typename L = signed char,
			 std::enable_if_t<std::is_integral<T>::value>* = nullptr>
	inline decltype(auto) _generator()
	{
		using Limit_t = amp_stl_algorithms::static_if_t<L,
														std::make_unsigned_t<L>,
														std::is_signed<T>::value>;
 		return std::bind(std::uniform_int_distribution<T>(std::numeric_limits<Limit_t>::min(),
														  std::numeric_limits<Limit_t>::max()),
														  std::mt19937_64());
	}

	template<typename T,
			 typename L = signed char,
			 std::enable_if_t<std::is_floating_point<T>::value>* = nullptr>
	inline decltype(auto) _generator()
	{
		return std::bind(std::uniform_real_distribution<T>(T(std::numeric_limits<L>::min()),
														   T(std::numeric_limits<L>::max())),
														   std::mt19937_64());
	}

    template<typename T>
    inline std::vector<T> generate_data(typename std::vector<T>::size_type sz)
    {
		std::vector<T> r(sz);
		std::generate(std::begin(r), std::end(r), _generator<T>());
		return r;
	}

	//=========================================================================
	//  Custom generator which spawns Musser's median-of-3 killer sequence.
	//=========================================================================

	template<typename T>
	inline std::vector<T> generate_median_of_3_killer(typename std::vector<T>::size_type sz)
	{
		if (sz % 2) return std::vector<T>(); // Only works for even sizes.

		std::vector<T> res(sz);
		const auto h = sz / 2;

		for (decltype(res.size()) i = 0; i != h; ++i) {
			res[i] = T(i + 1 + ((i % 2) ? h : 0));
			res[h + i] = T(2 * (i + 1));
		}

		return res;
	}

    //=========================================================================
    //  CPU based scan implementation for verifyind results from AMP
	//  implementations.
    //=========================================================================

    template <typename InIt, typename OutIt, typename BinaryOp>
    inline void scan_cpu_exclusive(InIt first, InIt last, OutIt dest_first, BinaryOp op)
    {
        *dest_first = typename std::iterator_traits<OutIt>::value_type(0);
		std::transform(first,
					   std::prev(last),
					   dest_first,
					   std::next(dest_first),
					   [=](auto&& x, auto&& y) {
			return op(std::forward<decltype(x)>(x), std::forward<decltype(y)>(y));
		});
    }

    template <typename InIt, typename OutIt, typename BinaryOp>
    inline void scan_cpu_inclusive(InIt first, InIt last, OutIt dest_first, BinaryOp op)
    {
        *dest_first = *first;
		std::transform(std::next(first),
					   last,
					   dest_first,
					   std::next(dest_first),
					   [=](auto&& x, auto&& y) {
			return op(std::forward<decltype(x)>(x), std::forward<decltype(y)>(y));
		});
    }

    //=========================================================================
    //  Comparison.
    //=========================================================================

    // Helper function for floating-point comparison. It combines absolute and
	// relative comparison techniques, in order to check if two floating-point
	// are close enough to be considered as equal.
    template<typename T>
    inline bool are_almost_equal(T v1, T v2, T maxAbsoluteDiff, T maxRelativeDiff)
    {
        // Return quickly if floating-point representations are exactly the same,
        // additionally guard against division by zero when, both v1 and v2 are
		// equal to 0.0f.
        if ((v1 == v2) || (std::fabs(v1 - v2) < maxAbsoluteDiff)) return true;

        T diff(0);

        if (std::fabs(v1) > std::fabs(v2)) {
            diff = std::fabs(v1 - v2) / std::fabs(v1);
        }
        else {
            diff = std::fabs(v2 - v1) / std::fabs(v2);
        }

        return diff < maxRelativeDiff; // relative comparison
    }

    // Compare two floats and return true if they are close to each other.
    inline bool compare(float v1,
						float v2,
						float maxAbsoluteDiff = 0.000005f,
						float maxRelativeDiff = 0.001f)
    {
        return are_almost_equal(v1, v2, maxAbsoluteDiff, maxRelativeDiff);
    }

    template<typename T>
    inline bool compare(const T& v1, const T& v2)
    {
        // This function is constructed in a way that requires T
        // only to define operator< to check for equality
        if (v1 < v2) {
            return false;
        }
        if (v2 < v1) {
            return false;
        }
        return true;
    }

    template<typename StlFunc, typename AmpFunc, typename Data>
    void compare_binary_operator(StlFunc&& stl_func, AmpFunc&& amp_func, Data&& tests)
    {
        for (auto&& p : std::forward<Data>(tests)) {
           EXPECT_EQ(std::forward<StlFunc>(stl_func)(std::forward<decltype(p)>(p).first,
													 std::forward<decltype(p)>(p).second),
					 std::forward<AmpFunc>(amp_func)(std::forward<decltype(p)>(p).first,
													 std::forward<decltype(p)>(p).second));
        }
    }

    template<typename StlFunc, typename AmpFunc, typename Data>
    void compare_unary_operator(StlFunc&& stl_func, AmpFunc&& amp_func, Data&& tests)
    {
        for (auto&& p : std::forward<Data>(tests)) {
            EXPECT_EQ(std::forward<StlFunc>(stl_func)(std::forward<decltype(p)>(p)),
					  std::forward<AmpFunc>(amp_func)(std::forward<decltype(p)>(p)));
        }
    }

    // Compare array_view with other STL containers.
    template<typename T>
    inline decltype(auto) size(const concurrency::array_view<T>& arr)
    {
        return arr.extent.size();
    }

    template<typename T>
    inline decltype(auto) size(const std::vector<T>& arr)
    {
        return arr.size();
    }

    template <typename T1, typename T2>
    inline bool are_equal(const T1& expected,
						  const T2& actual,
						  std::ptrdiff_t expected_size = std::numeric_limits<std::ptrdiff_t>::min())
    {
        if (expected_size == std::numeric_limits<ptrdiff_t>::min()) {
            expected_size = std::distance(begin(expected), end(expected));
            EXPECT_EQ(expected_size, std::distance(amp_stl_algorithms::begin(actual),
												   amp_stl_algorithms::end(actual)));
            if (expected_size != std::distance(amp_stl_algorithms::begin(actual),
											   amp_stl_algorithms::end(actual))) {
                return false;
            }
        }
        if (expected_size == 0) {
            return true;
        }
        bool is_same = true;
        for (decltype(expected_size) i = 0; (i < expected_size) && is_same; ++i) {
            EXPECT_EQ(expected[i], actual[i]);
            if (std::not_equal_to<>()(expected[i], actual[i])) {
                is_same = false;
            }
        }
        return is_same;
    }

    //=========================================================================
    //  Stream output overloads for std::vector, array and array_view.
    //=========================================================================

    // Setting the container width in the output stream modifies the number of
	// elements output from the containers subsequently output in the stream.
	// The following outputs the first 4 elements of data.
    //
    //      std::vector<int> data(12, 1);
    //      cout << container_width(4) << data;

    class container_width
    {
    public:
        explicit container_width(std::size_t width) : m_width(width) { }

    private:
        std::size_t m_width;

        template<typename T, typename Traits>
        inline friend std::basic_ostream<T, Traits>& operator<<(std::basic_ostream<T, Traits>& os,
																const container_width& container)
        {
            os.iword(_details::geti()) = container.m_width;
            return os;
        }
    };

    // TODO: These print two ',' as a delimiter not one. Fix.
    template<typename StrmType, typename Traits, typename T, std::size_t N>
    std::basic_ostream<StrmType, Traits>& operator<<(std::basic_ostream<StrmType, Traits>& os,
													 const std::array<T, N>& vec)
    {
        return os << concurrency::array_view<const T>(vec);
    }

    template<typename StrmType, typename Traits, typename T>
    std::basic_ostream<StrmType, Traits>& operator<<(std::basic_ostream<StrmType, Traits>& os,
													 const std::vector<T>& vec)
    {
		return os << concurrency::array_view<const T>(vec);
    }

    template<typename StrmType, typename Traits, typename T>
    std::basic_ostream<StrmType, Traits>& operator<<(std::basic_ostream<StrmType, Traits>& os,
													 concurrency::array<T>& vec)
    {
		return os << concurrency::array_view<const T>(vec);
    }

    template<typename StrmType, typename Traits, typename T>
    std::basic_ostream<StrmType, Traits>& operator<<(std::basic_ostream<StrmType, Traits>& os,
													 const concurrency::array_view<T>& vec)
    {
        std::copy(vec.data(),
				  vec.data() + std::min<decltype(vec.extent.size())>(_details::get_width(os),
																	 vec.extent.size()),
				  std::ostream_iterator<T,
										StrmType>(os,
											      _details::get_delimiter<typename Traits::char_type>())
				  );
        return os;
    }

    namespace _details
    {
        static inline decltype(auto) geti()
        {
			static auto i = std::ios_base::xalloc();
            return i;
        }

        template<typename STREAM>
        inline decltype(auto) get_width(STREAM& os)
        {
            constexpr std::remove_reference_t<decltype(os.iword(geti()))> default_width = 4;
            decltype(auto) width = os.iword(geti());
            return (width == 0) ? default_width : width;
        }

        template<typename T, std::enable_if_t<std::is_same<T, char>::value ||
			                                  std::is_same<T, wchar_t>::value>* = nullptr>
        inline decltype(auto) get_delimiter()
        {
            static constexpr T delim(',');
            return std::addressof(delim);
        }
    } // namespace _details

    //=========================================================================
    //  Basic performance timing.
    //=========================================================================
	// TODO: this needs to be overhauled and moved to std::chrono before we start
	// performance measurements in earnest.
    inline double elapsed_time(const LARGE_INTEGER& start, const LARGE_INTEGER& end)
    {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        return (double(end.QuadPart) - double(start.QuadPart)) * 1000.0 / double(freq.QuadPart);
    }

    template<typename Func>
    inline double time_func(concurrency::accelerator_view& view, Func&& f)
    {
        //  Ensure that the C++ AMP runtime is initialized.
        concurrency::accelerator::get_all();
        //  Ensure that the C++ AMP kernel has been JITed.
        std::forward<Func>(f)();
        view.wait();

        LARGE_INTEGER start, end;
        QueryPerformanceCounter(&start);
        std::forward<Func>(f)();
        view.wait();
        QueryPerformanceCounter(&end);

        return elapsed_time(start, end);
    }

	template<typename N>
	inline constexpr N compute_sample_size(N pop_sz)
	{	// Returns adequate sample size for a population of pop_sz size. Based on Krejcie, R. V. &
		// Morgan, D. W. (1970) "Determining Sample Size for Research Activities".
	//	static constexpr double chi_sq = 3.84; // 95% confidence level, 1 degree of freedom.
	//	static constexpr double proportion = 0.50; // Maximise sample size.
	//	static constexpr double deg_ac = 0.05; // degree of accuracy.

		return N((3.84 * pop_sz * 0.5 * (1.0 - 0.5)) /
			   (0.05 * 0.05 * (pop_sz - N(1)) + 3.84 * (0.5 * (1.0 - 0.5))));
	}
}; // namespace testtools

class testbase
{
protected:
    testbase()
    {
        testtools::set_default_accelerator(L"stl_algorithms_tests");
        concurrency::accelerator().default_view.wait();
    }
};

template<int _Size = 13>
class stl_algorithms_testbase : public testbase {
protected:
    std::array<int, _Size> input;
    concurrency::array_view<int> input_av;
    std::array<int, _Size> output;
    concurrency::array_view<int> output_av;
    std::array<int, _Size> expected;

    stl_algorithms_testbase() : input_av(input), output_av(output)
    {
        static constexpr std::array<int, _Size> input_data = { 1, 3, 6, 3, 2, 2,
															   7, 8, 2, 9, 2, 10, 2 };
        auto it = std::begin(input);
		while (it != std::end(input)) {
			it = std::copy(std::cbegin(input_data),
						   std::cbegin(input_data) + std::min(_Size, std::end(input) - it),
						   it);
		}
        std::fill_n(std::begin(output), _Size, -1);
        std::fill_n(std::begin(expected), _Size, -1);
    }

    static constexpr int size = _Size;
};

template<typename T>
class greater_than {
private:
    T m_value;
public:
    greater_than(T value = T(0)) : m_value(std::move(value)) {}

    bool operator()(const T& v) const restrict(cpu, amp)
    {
        return m_value < v;
    }
};
#endif // _TESTOOLS_H_BUMPTZI
