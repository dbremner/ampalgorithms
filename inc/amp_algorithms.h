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

#pragma once

#include <amp.h>
#include <xx_amp_algorithms_impl.h>
#include <amp_indexable_view.h>

namespace amp_algorithms
{
using namespace concurrency;

// Some common binary functions
template <typename T>
class sum
{
public:
    T operator()(const T &a, const T &b) const restrict(cpu, amp)
    {
        return (a + b); 
    }
};


template <typename T>
class max
{
public:
    T operator()(const T &a, const T &b) const restrict(cpu, amp)
    {
        return ((a > b) ? a : b);
    }
};

template <typename T>
class min
{
public:
    T operator()(const T &a, const T &b) const restrict(cpu, amp)
    {
        return ((a < b) ? a : b);
    }
};

// Generic reduction template for binary operators that are commutative and associative
template <typename InputIndexableView, typename BinaryFunction>
auto reduce(const accelerator_view &accl_view, const InputIndexableView &input_view, const BinaryFunction &binary_op) 
    -> decltype((*((BinaryFunction*)NULL))(*((indexable_view_traits<InputIndexableView>::value_type*)NULL), *((indexable_view_traits<InputIndexableView>::value_type*)NULL)))
{
    // The input view must be of rank 1
    static_assert(indexable_view_traits<InputIndexableView>::rank == 1, "The input indexable view must be of rank 1");
    typedef typename decltype((*((BinaryFunction*)NULL))(*((indexable_view_traits<InputIndexableView>::value_type*)NULL), *((indexable_view_traits<InputIndexableView>::value_type*)NULL))) result_type;

    // TODO: The window_width needs to be tuned based on perf tests
    const int window_width = 16;

    int element_count = input_view.extent.size();
    assert((element_count != 0) && (element_count > window_width));

    // The input_view cannot be mutated. So in the first pass we will perform a window_width way reduction
    // into a temporary array_view
    int stride = element_count / window_width;
    int tail_length = element_count % window_width;
    array_view<result_type> temp_view(array<result_type>(stride, accl_view));
    _details::parallel_for_each(accl_view, temp_view.extent, [=] (index<1> idx) restrict(amp)
    {
        result_type temp = input_view(idx);
        for(int i = 1; i < window_width; i++)
        {
            temp = binary_op(temp, input_view(index<1>(idx[0] + (i * stride))));
        }

        // Reduce the tail in cases where the number of elements is not divisible.
        // Note: execution of this section may negatively affect the performance.
        // In production code the problem size passed to the reduction should
        // be a power of the window_width.
        if (idx[0] < tail_length) {
            temp = binary_op(temp, input_view(index<1>((window_width * stride) + idx[0])));
        }

        temp_view(idx) = temp;
    });


    // Now we will perform in-place reduction on temp_view, iteratively till
    // the size falls below the threshold (TODO)
    tail_length = stride % window_width;
    stride = stride / window_width;
    while (stride > 0)
    {
        _details::parallel_for_each(accl_view, temp_view.extent, [=] (index<1> idx) restrict(amp)
        {
            result_type temp = temp_view(index<1>(idx[0]));
            for(int i = 1; i < window_width; i++)
            {
                temp = binary_op(temp, temp_view(index<1>(idx[0] + (i * stride))));
            }

            // Reduce the tail in cases where the number of elements is not divisible.
            // Note: execution of this section may negatively affect the performance.
            // In production code the problem size passed to the reduction should
            // be a power of the window_width.
            if (idx[0] < tail_length) {
                temp = binary_op(temp, temp_view(index<1>((window_width * stride) + idx[0])));
            }

            temp_view(index<1>(idx[0])) = temp;
        });

        tail_length = stride % window_width;
        stride = stride / window_width;
    }

    // Lets reduce the tail

    // Perform any remaining reduction on the CPU.
    std::vector<result_type> result(tail_length);
    copy(temp_view.section(0, tail_length), result.begin());
    temp_view.discard_data();

    assert(tail_length != 0);

    if (tail_length == 1)
    {
        return result[0];
    }
    else
    {
        result_type temp = result[0];
        for (int i = 1; i < tail_length; ++i) 
        {
            temp = binary_op(temp, result[i]);
        }

        return temp;
    }
}

template <typename InputIndexableView, typename BinaryFunction>
auto reduce(const InputIndexableView &input_view, const BinaryFunction &binary_op) 
    -> decltype((*((BinaryFunction*)NULL))(*((indexable_view_traits<InputIndexableView>::value_type*)NULL), *((indexable_view_traits<InputIndexableView>::value_type*)NULL)))
{
   return reduce(_details::auto_select_target(), input_view, binary_op);
}

}

