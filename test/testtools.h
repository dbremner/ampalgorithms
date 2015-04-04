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

#include <vector>
#include <algorithm>
#include <iostream>

#include <amp_algorithms.h>
#include <amp_stl_algorithms.h>

using namespace concurrency;
using namespace amp_algorithms;
using namespace amp_stl_algorithms;

//  Define these namespaces and types to pick up poorly specified namespaces and types in library code.
//  This makes the test code more like a real library client which may define conflicting namespaces.

namespace details { };
namespace _details { };
namespace direct3d { };
namespace graphics { };
namespace fast_math { };
namespace precise_math { };

class extent { };
class index { };
class array { };

namespace testtools
{
    //===============================================================================
    //  Helper functions to display and set the C++ AMP accelerator.
    //===============================================================================

    inline void to_upper(std::wstring& str)
    {
        std::transform(cbegin(str), cend(str), begin(str), [=](wchar_t c) { return std::toupper(c); });
    }

    inline std::vector<accelerator> get_accelerators(bool show_all = false)
    {
        std::vector<accelerator> accls = accelerator::get_all();
        if (!show_all)
        {
            accls.erase(std::remove_if(accls.begin(), accls.end(), [](accelerator& a)
            {
                return(a.device_path == accelerator::cpu_accelerator);
            }), accls.end());
        }
        std::sort(begin(accls), end(accls), [=](accelerator a, accelerator b)->bool 
        {
            std::wstring dev_a(a.device_path);
            to_upper(dev_a);
            std::wstring dev_b(b.device_path);
            to_upper(dev_b);
            return dev_a < dev_b;
        });
        return accls;
    }

    inline void display_accelerators(const std::vector<accelerator>& accls, bool show_details = false, bool show_all = false)
    {
        if (accls.empty())
        {
            std::wcout << "No accelerators found that are compatible with C++ AMP" << std::endl << std::endl;
            return;
        }

        const std::wstring default_dev_path = accelerator().device_path;

        std::wcout << "Found " << accls.size()
            << " accelerator device(s) that are compatible with C++ AMP:" << std::endl << std::endl;
        int n = 0;
        std::for_each(accls.cbegin(), accls.cend(), [=, &n](const accelerator& a)
        {
            bool is_default = (a.device_path.compare(default_dev_path) == 0);
            float mem_gb =  float(a.dedicated_memory) / (1024.0f * 1024.0f);
            std::wcout << (is_default ? " *" : "  ") << ++n << ": " << a.description;
            if (a.dedicated_memory > 0)
            {
                std::wcout << " ( " << std::fixed << std::setprecision(1) << mem_gb << " GB )";
            }
            std::wcout << std::endl << "       device_path                       = " << a.device_path;
                
            if (show_details)
            {
                std::wcout << std::endl << "       dedicated_memory                  = " << std::fixed << std::setprecision(1) << mem_gb << " GB"
                           << std::endl << "       has_display                       = " << (a.has_display ? "true" : "false")
                           << std::endl << "       is_debug                          = " << (a.is_debug ? "true" : "false")
                           << std::endl << "       is_emulated                       = " << (a.is_emulated ? "true" : "false")
                           << std::endl << "       supports_double_precision         = " << (a.supports_double_precision ? "true" : "false")
                           << std::endl << "       supports_limited_double_precision = " << (a.supports_limited_double_precision ? "true" : "false") << std::endl;
            }
            std::wcout << std::endl;
        });
        std::wcout << std::endl;
        return;
    }

    inline void set_default_accelerator(std::wstring device_path)
    {
        std::wstring dev_path = accelerator().device_path;
        bool set_ok = accelerator::set_default(device_path) || (dev_path == device_path);

        if (!set_ok)
        {
            std::wcout << "Unable to set default accelerator to " << device_path << " . Using " << dev_path << "." << std::endl;
        }
        accelerator().get_default_view().flush();
    }

    //===============================================================================
    //  Helper functions to generate test data of random numbers.
    //===============================================================================

    template <typename T>
    inline void generate_data(std::vector<T> &v)
    {
        srand(2012);    // Set random number seed so tests are reproducible.
        std::generate(begin(v), end(v), [=]{
            int v = rand();
            return static_cast<T>( ((v % 4) == 0) ? -v : v );
        });
    }

    template <>
    inline void generate_data(std::vector<unsigned int> &v)
    {
        srand(2012);    // Set random number seed so tests are reproducible.
        std::generate(begin(v), end(v), [=](){ return (unsigned int) rand(); });
    }

    //===============================================================================
    //  CPU based scan implementation for comparing results from AMP implementations.
    //===============================================================================

    template <typename InIt, typename OutIt, typename BinaryOp>
    inline void scan_cpu_exclusive(InIt first, InIt last, OutIt dest_first, BinaryOp op)
    {
        typedef InIt::value_type T;
        *dest_first = T(0);
        for (int i = 1; i < std::distance(first, last); ++i)
        {
            dest_first[i] = op(dest_first[i - 1], first[i - 1]);
        }
    }

    template <typename InIt, typename OutIt, typename BinaryOp>
    inline void scan_cpu_inclusive(InIt first, InIt last, OutIt dest_first, BinaryOp op)
    {
        typedef InIt::value_type T;
        *dest_first = *first;
        for (int i = 1; i < std::distance(first, last); ++i)
        {
            dest_first[i] = op(dest_first[i - 1], first[i]);
        }
    }
    
    template <int mode, typename InIt, typename OutIt, typename BinaryOp>
    inline void scan_cpu(InIt first, InIt last, OutIt dest_first, BinaryOp op)
    {
        if (mode == static_cast<int>(scan_mode::inclusive))
        {
            scan_cpu_inclusive(first, last, dest_first, op);
        }
        else
        {
            scan_cpu_exclusive(first, last, dest_first, op);
        }
    }

    //===============================================================================
    //  Comparison.
    //===============================================================================

    // Helper function for floating-point comparison. It combines absolute and relative comparison techniques,
    // in order to check if two floating-point are close enough to be considered as equal.
    template<typename T>
    inline bool are_almost_equal(T v1, T v2, const T maxAbsoluteDiff, const T maxRelativeDiff)
    {
        // Return quickly if floating-point representations are exactly the same,
        // additionally guard against division by zero when, both v1 and v2 are equal to 0.0f
        if ((v1 == v2) || (fabs(v1 - v2) < maxAbsoluteDiff))
        {
            return true;
        }

        T diff = (fabs(v1) > fabs(v2)) ? fabs(v1 - v2) / fabs(v1) : fabs(v2 - v1) / fabs(v2);

        return (diff < maxRelativeDiff); // relative comparison
    }

    // Compare two floats and return true if they are close to each other.
    inline bool compare(float v1, float v2, 
        const float maxAbsoluteDiff = 0.00005f,
        const float maxRelativeDiff = 0.0025f)
    {
        return are_almost_equal(v1, v2, maxAbsoluteDiff, maxRelativeDiff);
    }

    template<typename T>
    inline bool compare(const T &v1, const T &v2)
    {
        // This function is constructed in a way that requires T
        // only to define operator< to check for equality

        return !((v1 < v2) || (v2 < v1));
    }

    template<typename T>
    struct is_division : std::false_type
    {};

    template<>
    struct is_division < std::divides<int> > : std::true_type
    {};

    template<>
    struct is_division < std::modulus<int> > : std::true_type
    {};

    template<typename StlFunc, typename T>
    bool is_divide_by_zero(const T& val)
    {
        return (is_division<StlFunc>() && std::is_arithmetic<T>() && (val == T()));
    }

    template<typename StlFunc, typename AmpFunc, typename TIter>
    void compare_binary_operator(StlFunc stl_func, AmpFunc amp_func, TIter first, TIter last)
    {
        std::for_each(first, last, [=](TIter::value_type p) 
        {
            if (!is_divide_by_zero<StlFunc>(p.second))
            {
                EXPECT_EQ(stl_func(p.first, p.second), amp_func(p.first, p.second));
            }
            if (!is_divide_by_zero<StlFunc>(p.first))
            {
                EXPECT_EQ(stl_func(p.second, p.first), amp_func(p.second, p.first));
            }
        });
    }

    template<typename StlFunc, typename AmpFunc, typename T>
    void compare_unary_operator(StlFunc stl_func, AmpFunc amp_func, T test_values)
    {
        for (auto v : test_values)
        {
            EXPECT_EQ(stl_func(v), amp_func(v));
        }
    }

    // Compare array_view with other STL containers.
    template<typename T>
    size_t size(const array_view<T>& arr)
    {
        return arr.extent.size();
    }

    template<typename T>
    size_t size(const std::vector<T>& arr)
    {
        return arr.size();
    }

    template <typename T1, typename T2>
    bool are_equal(const T1& expected, const T2& actual, size_t expected_size = -1)
    {    
        const int output_range = 8;
        if (expected_size == -1)
        {
            expected_size = std::distance(begin(expected), end(expected));
            EXPECT_EQ(expected_size, std::distance(begin(actual), end(actual)));
            if (expected_size != std::distance(begin(actual), end(actual)))
            {
                return false;
            }
        }
        if (expected_size == 0)
        {
            return true;
        }
        bool is_same = true;
        for (int i = 0; (i < static_cast<int>(expected_size) && is_same); ++i)
        {
            EXPECT_EQ(expected[i], actual[i]);
            if (expected[i] != actual[i])
            {
                is_same = false;
            }
        }
        return is_same;
    }

    //===============================================================================
    //  Stream output overloads for std::vector, array and array_view.
    //===============================================================================

    // Setting the container width in the output stream modifies the number of elements output from the containers
    // subsequently output in the stream. The following outputs the first 4 elements of data.
    //
    //      std::vector<int> data(12, 1);
    //      cout << container_width(4) << data;

    class container_width
    {
    public:
        explicit container_width(size_t width) : m_width(width) { }

    private:
        size_t m_width;

        template <class T, class Traits>
        inline friend std::basic_ostream<T, Traits>& operator <<
            (std::basic_ostream<T, Traits>& os, const container_width& container)
        {
            os.iword(_details::geti()) = long(container.m_width);
            return os;
        }
    };

    // TODO: These print two ',' as a delimiter not one. Fix.
    template<typename StrmType, typename Traits, typename T, int N>
    std::basic_ostream<StrmType, Traits>& operator<< (std::basic_ostream<StrmType, Traits>& os, const std::array<T, N>& vec)
    {
        std::copy(std::begin(vec), std::begin(vec) + std::min<size_t>(_details::get_width(os), vec.size()),
            std::ostream_iterator<T, Traits::char_type>(os, _details::get_delimiter<Traits::char_type>()));
        return os;
    }

    template<typename StrmType, typename Traits, typename T>
    std::basic_ostream<StrmType, Traits>& operator<< (std::basic_ostream<StrmType, Traits>& os, const std::vector<T>& vec)
    {
        std::copy(std::begin(vec), std::begin(vec) + std::min<size_t>(_details::get_width(os), vec.size()),
            std::ostream_iterator<T, Traits::char_type>(os, _details::get_delimiter<Traits::char_type>()));
        return os;
    }

    template<typename StrmType, typename Traits, typename T>
    std::basic_ostream<StrmType, Traits>& operator<< (std::basic_ostream<StrmType, Traits>& os, concurrency::array<T, 1>& vec)
    {
        size_t i = std::min<size_t>(_details::get_width(os), vec.extent[0]);
        std::vector<const T> buffer(i);
        copy(vec.section(0, static_cast<int>(i)), std::begin(buffer));
        return os << buffer;
    }

    template<typename StrmType, typename Traits, typename T>
    std::basic_ostream<StrmType, Traits>& operator<< (std::basic_ostream<StrmType, Traits>& os, const concurrency::array_view<T, 1>& vec)
    {
        size_t i = std::min<size_t>(_details::get_width(os), vec.extent[0]);
        std::vector<T> buffer(i);
        copy(vec.section(0, static_cast<int>(i)), std::begin(buffer));
        return os << buffer;
    }

    namespace _details
    {
        inline int geti()
        {
            static int i = std::ios_base::xalloc();
            return i;
        }

        template <typename STREAM>
        inline size_t get_width(STREAM& os)
        {
            const size_t default_width = 4;
            size_t width = os.iword(geti());
            return (width == 0) ? default_width : width;
        }

        template <typename T>
        inline T* get_delimiter()
        {
            assert(false);
            return nullptr;
        }

        template <>
        inline char* get_delimiter()
        {
            static char delim(',');
            return &delim;
        }

        template <>
        inline wchar_t* get_delimiter()
        {
            static wchar_t delim(L',');
            return &delim;
        }
    } // namespace _details

    //===============================================================================
    //  Basic performance timing.
    //===============================================================================

    inline double elapsed_time(const LARGE_INTEGER& start, const LARGE_INTEGER& end)
    {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        return (double(end.QuadPart) - double(start.QuadPart)) * 1000.0 / double(freq.QuadPart);
    }

    template <typename Func>
    double time_func(accelerator_view& view, Func f)
    {
        //  Ensure that the C++ AMP runtime is initialized.
        accelerator::get_all();
        //  Ensure that the C++ AMP kernel has been JITed.
        f();
        view.wait();

        LARGE_INTEGER start, end;
        QueryPerformanceCounter(&start);
        f();
        view.wait();
        QueryPerformanceCounter(&end);

        return elapsed_time(start, end);
    }
}; // namespace test_tools

//===============================================================================
//  Operator test definitions
//===============================================================================

template <typename StlType, typename AmpType>
class OperatorTestDefinition
{
public:
    typedef StlType stl_type;
    typedef AmpType amp_type;
};

//===============================================================================
//  Acceptance test definitions
//===============================================================================

template <typename T, int N, int P = 0>
class TestDefinition {
public:
    typedef T value_type;
    static const int size = N;
    static const int parameter = P;
};

template <typename T, int TS, int N, int P = 0>
class TiledTestDefinition : public TestDefinition < T, N, P >
{
public:
    static const int tile_size = TS;
};

template <typename T, int N, int P = 0>
const int TestDefinition<T, N, P>::size;

template <typename T, int N, int P = 0>
const int TestDefinition<T, N, P>::parameter;

template <typename T, int TS, int N, int P = 0>
const int TiledTestDefinition<T, TS, N, P>::tile_size;

//===============================================================================
//  Test base class
//===============================================================================

class testbase
{
protected:
    testbase()
    {
        accelerator().default_view.wait();
    }

    ~testbase()
    {
        accelerator().default_view.flush();
    }
};

template<int _Size = 13>
class stl_algorithms_testbase : public testbase
{
protected:
    std::array<int, _Size> input;
    array_view<int> input_av;
    std::array<int, _Size> output;
    array_view<int> output_av;
    std::array<int, _Size> expected;

    stl_algorithms_testbase() :
        input_av(concurrency::extent<1>(static_cast<int>(input.size())), input),
        output_av(concurrency::extent<1>(static_cast<int>(output.size())), output)
    {
        const std::array<int, _Size> input_data = { { 1, 3, 6, 3, 2, 2, 7, 8, 2, 9, 2, 10, 2 } };
        int i = 0;
        for (auto& v : input)
        {
            v = input_data[i++ % 13];
        }
        std::fill(begin(output), end(output), -1);
        std::fill(begin(expected), end(expected), -1);
    }

    static const int size = _Size;
};

template <typename T>
class greater_than
{
private:
    T m_value;
public:
    greater_than(const T& value = 0) : m_value(value) {}

    T operator()(const T &v) const restrict(cpu, amp)
    {
        return (v > m_value) ? 1 : 0;
    }
};
