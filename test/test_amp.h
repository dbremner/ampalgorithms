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

#define NOMINMAX

#include <vector>
#include <algorithm>
#include <iostream>
#include <CppUnitTest.h>


#include <amp_algorithms.h>
#include <amp_stl_algorithms.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
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

//	Set USE_REF to use the REF accelerator for all tests. This is useful if tests fail on a particular machine as
//	failure may be due to a driver bug.

inline void set_default_accelerator()
{
#if defined(USE_REF)
    bool set_ok = accelerator::set_default(accelerator::direct3d_ref);

    if (!set_ok)
    {
        Logger::WriteMessage("Unable to set default accelerator to REF.");
    }
#endif
}

// Helper functions to generate test data of random numbers.
template <typename T>
inline void generate_data(std::vector<T> &v)
{
    srand(2012);    // Set random number seed so tests are reproducable.
    for (unsigned int i = 0; i < v.size(); ++i) 
    {
        v[i] = (T) rand();
        if ((i % 4) == 0) 
        {
            v[i] = -v[i];
        }
    }
}

template <>
inline void generate_data(std::vector<unsigned int> &v)
{
    srand(2012);    // Set random number seed so tests are reproducable.
    std::generate(begin(v), end(v), [=](){ return (unsigned int) rand(); });
}

// Helper function for floating-point comparison. It combines absolute and relative comparison techniques,
// in order to check if two floating-point are close enough to be considered as equal.
template<typename T>
inline bool are_almost_equal(T v1, T v2, const T maxAbsoluteDiff, const T maxRelativeDiff)
{
    // Return quickly if floating-point representations are exactly the same,
    // additionally guard against division by zero when, both v1 and v2 are equal to 0.0f
    if (v1 == v2) 
    {
        return true;
    }
    else if (fabs(v1 - v2) < maxAbsoluteDiff) // absolute comparison
    {
        return true;
    }

    T diff = 0.0f;

    if (fabs(v1) > fabs(v2))
    {
        diff = fabs(v1 - v2) / fabs(v1);
    }
    else
    {
        diff = fabs(v2 - v1) / fabs(v2);
    }

    return (diff < maxRelativeDiff); // relative comparison
}

// Compare two floats and return true if they are close to each other.
inline bool compare(float v1, float v2, 
                    const float maxAbsoluteDiff = 0.000005f,
                    const float maxRelativeDiff = 0.001f)
{
    return are_almost_equal(v1, v2, maxAbsoluteDiff, maxRelativeDiff);
}

template<typename T>
inline bool compare(const T &v1, const T &v2)
{
    // This function is constructed in a way that requires T
    // only to define operator< to check for equality

    if (v1 < v2)
    {
        return false;
    }
    if (v2 < v1)
    {
        return false;
    }
    return true;
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
bool are_equal(const T1& expected, const T2& actual)
{    
    const size_t element_count = std::distance(begin(expected), end(expected));

    if (element_count != std::distance(begin(actual), end(actual)))
        return false;
    
    for (int i = 0; i < int(element_count); ++i)
    {
        if (expected[i] != actual[i])
            return false;
    }
    return true;
}
