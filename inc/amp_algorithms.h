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
        return ((a < b) ? b : a);
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
typename std::result_of<BinaryFunction(const typename indexable_view_traits<InputIndexableView>::value_type&, const typename indexable_view_traits<InputIndexableView>::value_type&)>::type
reduce(const accelerator_view &accl_view, const InputIndexableView &input_view, const BinaryFunction &binary_op) 
{
	return _details::reduce<512, 10000, InputIndexableView, BinaryFunction>(accl_view, input_view, binary_op);
}

template <typename InputIndexableView, typename BinaryFunction>
typename std::result_of<BinaryFunction(const typename indexable_view_traits<InputIndexableView>::value_type&, const typename indexable_view_traits<InputIndexableView>::value_type&)>::type
reduce(const InputIndexableView &input_view, const BinaryFunction &binary_op) 
{
	return reduce(_details::auto_select_target(), input_view, binary_op);
}

//----------------------------------------------------------------------------
// generate
//----------------------------------------------------------------------------

template <typename OutputIndexableView, typename Generator>
void generate(const accelerator_view &accl_view, OutputIndexableView& output_view, const Generator& generator)
{
	_details::parallel_for_each(accl_view, output_view.extent, [output_view,generator] (index<indexable_view_traits<OutputIndexableView>::rank> idx) restrict(amp) {
		output_view[idx] = generator();
	});
}

template <typename OutputIndexableView, typename Generator>
void generate(OutputIndexableView& output_view, const Generator& generator)
{
	::amp_algorithms::generate(_details::auto_select_target(), output_view, generator);
}

//----------------------------------------------------------------------------
// transform (unary)
//----------------------------------------------------------------------------

template <typename ConstInputIndexableView, typename OutputIndexableView, typename UnaryFunc>
void transform(const accelerator_view &accl_view, const ConstInputIndexableView& input_view, OutputIndexableView& output_view, const UnaryFunc& func)
{
	_details::parallel_for_each(accl_view, output_view.extent, [input_view,output_view,func] (index<indexable_view_traits<OutputIndexableView>::rank> idx) restrict(amp) {
		output_view[idx] = func(input_view[idx]);
	});
}

template <typename ConstInputIndexableView, typename OutputIndexableView, typename UnaryFunc>
void transform(const ConstInputIndexableView& input_view, OutputIndexableView& output_view, const UnaryFunc& func)
{
	::amp_algorithms::transform(_details::auto_select_target(), input_view, output_view, func);
}

//----------------------------------------------------------------------------
// transform (binary)
//----------------------------------------------------------------------------

template <typename ConstInputIndexableView1, typename ConstInputIndexableView2, typename OutputIndexableView, typename BinaryFunc>
void transform(const accelerator_view &accl_view, const ConstInputIndexableView1& input_view1, const ConstInputIndexableView2& input_view2, OutputIndexableView& output_view, const BinaryFunc& func)
{
	_details::parallel_for_each(accl_view, output_view.extent, [input_view1,input_view2,output_view,func] (index<indexable_view_traits<OutputIndexableView>::rank> idx) restrict(amp) {
		output_view[idx] = func(input_view1[idx], input_view2[idx]);
	});
}

template <typename ConstInputIndexableView1, typename ConstInputIndexableView2, typename OutputIndexableView, typename BinaryFunc>
void transform(const ConstInputIndexableView1& input_view1, const ConstInputIndexableView2& input_view2, OutputIndexableView& output_view, const BinaryFunc& func)
{
	::amp_algorithms::transform(_details::auto_select_target(), input_view1, input_view2, output_view, func);
}

//----------------------------------------------------------------------------
// fill
//----------------------------------------------------------------------------

template<typename OutputIndexableView, typename T>
void fill(const accelerator_view &accl_view, OutputIndexableView& output_view, const T& value )
{
	:::amp_algorithms::generate(accl_view, output_view, [value] () restrict(amp) { return value; });
}

template<typename OutputIndexableView, typename T>
void fill(OutputIndexableView& output_view, const T& value )
{
	::amp_algorithms::generate(output_view, [value] () restrict(amp) { return value; });
}


} // namespace amp_algorithms
