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
* This file contains the test driver
*---------------------------------------------------------------------------*/
#define NOMINMAX

#include <vector>
#include <algorithm>
#include <iostream>
#include <amp_algorithms.h>

using namespace concurrency;
using namespace amp_algorithms;

// TODO: replace this with a unit test framework
// TODO: Add tests for indexable_view_traits

template <typename T>
void generate_data(std::vector<T> &v)
{
    // Fill the vector with random values
    for (unsigned int i = 0; i < v.size(); ++i) 
    {
        v[i] = (T) rand();
        if ((i % 4) == 0) 
        {
            v[i] = -v[i];
        }
    }
}

// Helper function for floating-point comparison. It combines absolute and relative comparison techniques,
// in order to check if two floating-point are close enough to be considered as equal.
template<typename T>
bool are_almost_equal(T v1, T v2, const T maxAbsoluteDiff, const T maxRelativeDiff)
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

    if (diff < maxRelativeDiff) // relative comparison
    {
        return true;
    }
    else 
    {
        return false;
    }
}


// Compare two floats and return true if they are close to each other.
bool compare(float v1, float v2, 
             const float maxAbsoluteDiff = 0.000005f,
             const float maxRelativeDiff = 0.001f)
{
    return are_almost_equal(v1, v2, maxAbsoluteDiff, maxRelativeDiff);
}

template<typename T>
bool compare(const T &v1, const T &v2)
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

template <typename value_type, typename BinaryFunctor>
void test_reduce(int element_count, BinaryFunctor func, const char *testName)
{
    std::vector<value_type> inVec(element_count);
    generate_data(inVec);

    array_view<const value_type> inArrView(element_count, inVec);
    value_type amp_result = amp_algorithms::reduce(inArrView, func);

    // Now compute the result on the CPU for verification
    value_type cpu_result = inVec[0];
    for (int i = 1; i < element_count; ++i) {
        cpu_result = func(cpu_result, inVec[i]);
    }

    if (!compare(amp_result, cpu_result)) {
        std::cout << testName << ": Failed. Expected: " << cpu_result << ", Actual: " << amp_result << std::endl;
    }
    else {
        std::cout << testName << ": Passed. Expected: " << cpu_result << ", Actual: " << amp_result << std::endl;
    }
}

template <typename T>
void test_functor_view(int element_count)
{
    std::vector<T> inVec(element_count);
    generate_data(inVec);

    array_view<const T> inArrView(element_count, inVec);

    // The next 4 lines use the functor_view together with the reduce algorithm to obtain the 
    // standard deviation of a set of numbers.
    T gpuSum = amp_algorithms::reduce(accelerator().create_view(), inArrView, amp_algorithms::sum<T>());
    T gpuMean = gpuSum / inArrView.extent.size();

    auto funcView = make_indexable_view(inArrView.extent, [inArrView, gpuMean](const concurrency::index<1> &idx) restrict(cpu, amp) {
        return ((inArrView(idx) - gpuMean) * (inArrView(idx) - gpuMean));
    });

    T gpuTotalVariance = amp_algorithms::reduce(funcView, amp_algorithms::sum<T>());
    T gpuStdDev = sqrt(gpuTotalVariance / inArrView.extent.size());

    // Now compute the result on the CPU for verification
    T cpuSum = 0.0;
    for (int i = 0; i < element_count; ++i) {
        cpuSum += inVec[i];
    }

    T cpuMean = cpuSum / element_count;
    T cpuTotalVariance = 0.0;
    for (int i = 0; i < element_count; ++i) {
        cpuTotalVariance += ((inVec[i] - cpuMean) * (inVec[i] - cpuMean));
    }

    T cpuStdDev = sqrt(cpuTotalVariance / element_count);


    if (!compare(gpuStdDev, cpuStdDev)) {
        std::cout << "test_functor_view: Failed. Expected: " << cpuStdDev << ", Actual: " << gpuStdDev << std::endl;
    }
    else {
        std::cout << "test_functor_view: Passed. Expected: " << cpuStdDev << ", Actual: " << gpuStdDev << std::endl;
    }
}

void test_generate()
{
    std::vector<int> vec(1024);
    array_view<int,1> av(1024, vec);
    av.discard_data();

    amp_algorithms::generate(av, [] () restrict(amp) {
        return 7;
    });
    av.synchronize();

    std::for_each(begin(vec), end(vec), [] (int element) {
        assert(element == 7);
    });
}

void test_unary_transform()
{
    const int height = 16;
    const int width = 16;
    const int size = height * width;

    std::vector<int> vec_in(size);
    std::fill(begin(vec_in), end(vec_in), 7);
    array_view<const int, 2> av_in(height, width, vec_in);

    std::vector<int> vec_out(size);
    array_view<int,2> av_out(height, width, vec_out);

    // Test "transform" by doubling the input elements

    amp_algorithms::transform(av_in, av_out, [] (int x) restrict(amp) {
        return 2 * x;
    });
    av_out.synchronize();

    std::for_each(begin(vec_out), end(vec_out), [] (int element) {
        assert(element == 2*7);
    });
}

void test_binary_transform()
{
    const int depth = 16;
    const int height = 16;
    const int width = 16;
    const int size = depth * height * width;

    std::vector<int> vec_in1(size);
    std::fill(begin(vec_in1), end(vec_in1), 343);
    array_view<const int, 3> av_in1(depth, height, width, vec_in1);

    std::vector<int> vec_in2(size);
    std::fill(begin(vec_in2), end(vec_in2), 323);
    array_view<const int, 3> av_in2(depth, height, width, vec_in2);

    std::vector<int> vec_out(size);
    array_view<int, 3> av_out(depth, height, width, vec_out);

    // Test "transform" by adding the two input elements

    amp_algorithms::transform(av_in1, av_in2, av_out, [] (int x1, int x2) restrict(amp) {
        return x1 + x2;
    });
    av_out.synchronize();

    std::for_each(begin(vec_out), end(vec_out), [] (int element) {
        assert(element == 343 + 323);
    });
}

void test_fill()
{
    std::vector<int> vec(1024);
    array_view<int> av(1024, vec);
    av.discard_data();

    amp_algorithms::fill(av, 7);
    av.synchronize();

    std::for_each(begin(vec), end(vec), [] (int element) {
        assert(element == 7);
    });
}

template<typename value_type, typename functor>
value_type get_identity(functor op)
{
    if (typeid(op) == typeid(amp_algorithms::max<value_type>))
    {
        return std::numeric_limits<value_type>::lowest();
    }
    if (typeid(op) == typeid(amp_algorithms::min<value_type>))
    {
        return std::numeric_limits<value_type>::max();
    }
    if (typeid(op) == typeid(amp_algorithms::mul<value_type>))
    {
        return 1;
    }
    if (typeid(op) == typeid(amp_algorithms::bit_and<value_type>))
    {
        return (value_type)(0xFFFFFFFF);
    }
    else
    {
        return 0;
    }
}

enum class scan_type
{
    scan,
    multiscan,
    segmented
};

// Represents compressed flag array for segmented scan
struct bitvector 
{
    bitvector(unsigned int scan_size) : m_scan_size(scan_size)
    {
        data = std::vector<unsigned int>(bits_pad_to_uint(scan_size), 0);
    }

    void generate_data()
    {
        unsigned int flag_counter = 0;
        for (unsigned int idx=0; idx<data.size() && flag_counter < m_scan_size; ++idx)
        {
            unsigned int bag_of_bits = data[idx];
            for(unsigned int offset=0; offset<bit_count<unsigned int>() && flag_counter < m_scan_size; ++offset)
            {
                if (rand() % 13  == 0) // flip the bit to 1
                {
                    bag_of_bits |= 1 << offset;	
                }

                flag_counter++;
            }
            data[idx] = bag_of_bits;
        }
    }

    bool is_new_segment(unsigned int pos, bool backwards)
    {
        // When we encounter flag going backwards it means, 
        // that it is the first element of this segment (last element to be scanned going backwards)
        // for simplification we increment 'pos' and always look for flags behind our current position.
        if (backwards)
        {
            pos = pos + 1;
        }

        unsigned int idx = pos / bit_count<unsigned int>();
        unsigned int offset = pos % bit_count<unsigned int>();

        unsigned int bag_of_bits = data[idx];
        return (1 << offset & bag_of_bits) > 0 ? true : false;
    }

    std::vector<unsigned int> data;
private:
    template<typename T>
    unsigned int bit_count()
    {
        return sizeof(T) * 8;
    }

    unsigned int bits_pad_to_uint(unsigned int bits)
    {
        return (bits + bit_count<unsigned int>() - 1) / bit_count<unsigned int>();
    }


    unsigned int m_scan_size;
};

// A host side verification for scan, multiscan and segmented scan
template <typename T, typename BinaryFunction>
void verify_scan_results(bool backwards, bool exclusive, BinaryFunction op, std::vector<T> &in, std::vector<T> &out, unsigned int scan_size, unsigned int scan_pitch, unsigned int scan_count, bitvector &flags) 
{
    bool passed = true;

    // For each sub-scan
    for (unsigned int current_scan_num=0; current_scan_num<scan_count; ++current_scan_num)
    {
        T expected_scan_result;
        for (unsigned int i=current_scan_num * scan_pitch; i<scan_size; ++i)
        {
            int pos = i; // pos is used to reference into output and input arrays depending on the direction we go from the front or the back
            if (backwards)
            {
                pos = current_scan_num * scan_pitch + scan_size - 1 - i;
            }

            if (i == current_scan_num * scan_pitch || flags.is_new_segment(pos, backwards))
            {
                // Establish first result, either it is identity (for exclusive) or first/last element depending on scan direction for inclusive scan
                if (exclusive)
                {
                    expected_scan_result = get_identity<T>(op);
                }
                else
                {
                    expected_scan_result = in[pos];
                }
            }

            // Inclusive is computed as pre-fix op
            if (!exclusive && i != current_scan_num * scan_pitch)
            {
                expected_scan_result = op(expected_scan_result, in[pos]);
            }

            // Compare results
            if (!compare(out[pos], expected_scan_result))
            {
                std::cout << " Failed. Expected " << expected_scan_result << ", Actual: " << out[pos] << " at index " << pos << std::endl;
                passed = false;
                break;
            }

            // Exclusive is computed as post-fix op
            if (exclusive)
            {
                expected_scan_result = op(expected_scan_result, in[pos]);
            }
        }
    }

    if (passed)
    {
        std::cout << " Passed." << std::endl;
    }
}

template<typename BinaryFunction>
struct get_binary_function_info
{
    std::string name() { return "Unrecognized function"; }
};

template<typename T>
struct get_binary_function_info<amp_algorithms::sum<T>>
{
    std::string name() { return "sum"; }
};

template<typename T>
struct get_binary_function_info<amp_algorithms::min<T>>
{
    std::string name() { return "min"; }
};

template<typename T>
struct get_binary_function_info<amp_algorithms::max<T>>
{
    std::string name() { return "max"; }
};

template<typename T>
struct get_binary_function_info<amp_algorithms::mul<T>>
{
    std::string name() { return "mul"; }
};

template<typename T>
struct get_binary_function_info<amp_algorithms::bit_and<T>>
{
    std::string name() { return "bitwise and"; }
};

template<typename T>
struct get_binary_function_info<amp_algorithms::bit_or<T>>
{
    std::string name() { return "bitwise or"; }
};

template<typename T>
struct get_binary_function_info<amp_algorithms::bit_xor<T>>
{
    std::string name() { return "bitwise xor"; }
};

template<typename T, typename BinaryFunction>
std::string get_extended_test_name(std::string test_name, bool backwards, bool inplace)
{
    std::stringstream postfix;
    postfix << test_name << " ";
    postfix << typeid(T).name() << " ";
    postfix << get_binary_function_info<BinaryFunction>().name();

    postfix << (backwards ? " backward" : " forward");
    postfix << (inplace ? " in-place" : " not-in-place");

    return postfix.str().c_str();
}

template<typename T, typename BinaryFunction>
void test_scan_internal(int column_count, BinaryFunction op, std::string test_name, bool backwards, bool inplace, scan_type test_type = scan_type::scan, unsigned int row_count = 1)
{
    std::cout <<  get_extended_test_name<T, BinaryFunction>(test_name, backwards, inplace);
    srand(2012);

    std::vector<T> in(row_count * column_count);
    generate_data(in);
    std::vector<T> out(row_count * column_count);
    bitvector flags(column_count);

    // Construct scan object
    scan s(column_count, row_count);

    // Convert bool to direction type
    scan_direction direction = backwards ? scan_direction::backward : scan_direction::forward;

    // Run scan
    if (test_type == scan_type::multiscan)
    {
        extent<2> e2(row_count, column_count);
        array<T, 2> input(e2, in.begin());
        array<T, 2> output(e2);

        s.multi_scan_exclusive(input, inplace ? input : output, direction, op);
        copy(inplace ? input : output, out.begin());
    }
    else
    {
        extent<1> e(column_count);
        array<T> input(e, in.begin());
        array<T> output(e);
        bitvector *flags_ptr = nullptr;

        if (test_type == scan_type::scan)
        {
            s.scan_exclusive(input, inplace ? input : output, direction, op);
        }
        else
        {
            flags.generate_data();
            array<unsigned int> input_flags(static_cast<unsigned int>(flags.data.size()), flags.data.begin());
            s.segmented_scan_exclusive(input, inplace ? input : output, input_flags, direction, op);
        }

        copy(inplace ? input : output, out.begin());
    }

    // Now time to verify the results with host-side computation
    verify_scan_results(backwards, /*exclusive=*/true, op, in, out, column_count, column_count, row_count, flags);
}

template<typename T>
void test_scan(bool backwards)
{
    test_scan_internal<T>(10, amp_algorithms::sum<T>(), "Test scan", backwards, /*in_place=*/ false);
    test_scan_internal<T>(11, amp_algorithms::max<T>(), "Test scan", backwards, /*in_place=*/ true);
    test_scan_internal<T>(2 * 1024, amp_algorithms::min<T>(), "Test scan", backwards, /*in_place=*/ false);
    test_scan_internal<T>(2 * 1024 + 1, amp_algorithms::mul<T>(), "Test scan", backwards, /*in_place=*/ false);
}

template<>
void test_scan<unsigned int>(bool backwards)
{
    test_scan_internal<unsigned int>(10, amp_algorithms::sum<unsigned int>(), "Test scan", backwards, /*in_place=*/ true);
    test_scan_internal<unsigned int>(777, amp_algorithms::mul<unsigned int>(), "Test scan", backwards, /*in_place=*/ false);
}

template<typename T>
void test_scan_bitwise_op(bool backwards)
{
    test_scan_internal<T>(77, amp_algorithms::bit_xor<T>(), "Test scan", backwards, /*in_place=*/ true);
    test_scan_internal<T>(66, amp_algorithms::bit_and<T>(), "Test scan", backwards, /*in_place=*/ false);
    test_scan_internal<T>(12345, amp_algorithms::bit_or<T>(), "Test scan", backwards, /*in_place=*/ true);
}

template<typename T>
void test_multiscan(bool backwards)
{
    test_scan_internal<T>(144, amp_algorithms::sum<T>(), "Test multiscan", backwards, /*in_place=*/ false, scan_type::multiscan, 12);
    test_scan_internal<T>(2048, amp_algorithms::sum<T>(), "Test multiscan", backwards, /*in_place=*/ true, scan_type::multiscan, 1024);
    test_scan_internal<T>(3333, amp_algorithms::max<T>(), "Test multiscan", backwards, /*in_place=*/ false, scan_type::multiscan, 3);
    test_scan_internal<T>(128, amp_algorithms::mul<T>(), "Test multiscan", backwards, /*in_place=*/ true, scan_type::multiscan, 8);
    test_scan_internal<T>(127, amp_algorithms::min<T>(), "Test multiscan", backwards, /*in_place=*/ false, scan_type::multiscan, 3);
}

template<>
void test_multiscan<unsigned int>(bool backwards)
{
    test_scan_internal<unsigned int>(144, amp_algorithms::sum<unsigned int>(), "Test multiscan", backwards, /*in_place=*/ false, scan_type::multiscan, 12);
    test_scan_internal<unsigned int>(2048, amp_algorithms::sum<unsigned int>(), "Test multiscan", backwards, /*in_place=*/ true, scan_type::multiscan, 64);
    test_scan_internal<unsigned int>(128, amp_algorithms::mul<unsigned int>(), "Test multiscan", backwards, /*in_place=*/ false, scan_type::multiscan, 8);
}

template<typename T>
void test_multiscan_bitwise_op(bool backwards)
{
    test_scan_internal<T>(8, amp_algorithms::bit_and<T>(), "Test multiscan", backwards, /*in_place=*/ true, scan_type::multiscan, 2);
    test_scan_internal<T>(2048, amp_algorithms::bit_or<T>(), "Test multiscan", backwards, /*in_place=*/ false, scan_type::multiscan, 2);
    test_scan_internal<T>(3072, amp_algorithms::bit_xor<T>(), "Test multiscan",  backwards, /*in_place=*/ true, scan_type::multiscan, 3);
}

template<typename Func>
void try_catch_test(std::string msg, Func f)
{
    try
    {
        f();
        std::cout << "Test scan other: Failed." << msg << std::endl;
    }
    catch (const std::runtime_error)
    {
    }
}

void test_scan_other()
{
    accelerator ref(accelerator::direct3d_ref);
    accelerator_view ref_view = ref.create_view();

    const int elem_count = 10;
    std::vector<unsigned int> in(elem_count);
    for(unsigned int i=0; i<in.size(); ++i)
    {
        in[i] = 1;
    }

    array<unsigned int> input(extent<1>(elem_count), in.begin(), ref_view);
    // use max_scan_size and max_scan_count greater than actual usage
    scan s(2 * elem_count, elem_count, ref_view);

    // 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 -> 
    s.scan_exclusive(input, input);
    // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 -> 
    s.scan_exclusive(input, input, scan_direction::forward, amp_algorithms::sum<unsigned int>());
    // 0, 0, 1, 3, 6, 10, 15, 21, 28, 36 -> 

    unsigned int flg = 8; // 001000 in binary, so our segment is in here: 0, 0, 1, | 3, 6, 10, 15, 21, 28, 36
    array<unsigned int> flags(1, &flg, ref_view);
    s.segmented_scan_exclusive(input, input, flags, scan_direction::backward, amp_algorithms::sum<unsigned int>());
    // 1, 1, 0, 116, 110, 100, 85, 64, 36, 0

    // Copy out
    in = input;

    unsigned int expected_results[elem_count] = {1, 1, 0, 116, 110, 100, 85, 64, 36, 0};
    for(unsigned int i=0; i<in.size(); ++i)
    {
        if (in[i] != expected_results[i])
        {
            std::cout << "Test scan other: Failed. Expected: " << expected_results[i] << " Actual:" << in[i] << " at index: " << i << std::endl;
            break;
        }
    }

    // Try few negative scenarios
    try_catch_test("Expected exception for non-matching accelerator_view in scan object", [&]() {
        scan s2(2 * elem_count, elem_count, accelerator().default_view);
        s2.scan_exclusive(input, input);
    });

    try_catch_test("Expected exception for non-matching accelerator_view in output", [&]() {
        scan s2(2 * elem_count, elem_count, ref_view);
        array<unsigned int> output(elem_count, ref.create_view());
        s2.scan_exclusive(input, output);
    });

    try_catch_test("Expected exception for scan object with max_scan_size < scan_size", [&]() {
        scan s2(elem_count-1, ref_view);
        s2.scan_exclusive(input, input);
    });

    try_catch_test("Expected exception for scan object with max_scan_count == 0", [&]() {
        scan s2(elem_count, 0, ref_view);
    });

    try_catch_test("Expected exception for scan object with max_scan_count < scan_count", [&]() {
        scan s2(elem_count, 1, ref_view);
        array<unsigned int, 2> in2(10, 10);
        s2.multi_scan_exclusive(in2, in2, scan_direction::forward, amp_algorithms::sum<unsigned int>());
    });

    // Check scan binding cleanup
    array_view<const unsigned int> view(input);
    array<unsigned int> output(input.extent, input.accelerator_view);
    parallel_for_each(output.extent, [view, &output] (index<1> idx) restrict(amp) {
        output[idx] = view[idx];
    });
    output.accelerator_view.wait();
}


template<typename T>
void test_segmented(bool backwards)
{
    test_scan_internal<T>(7123127, amp_algorithms::sum<T>(), "Test segmented scan", backwards, /*inplace=*/false, scan_type::segmented);
    test_scan_internal<T>(31, amp_algorithms::mul<T>(), "Test segmented scan", backwards, /*inplace=*/true, scan_type::segmented);
    test_scan_internal<T>(222, amp_algorithms::min<T>(), "Test segmented scan", backwards, /*inplace=*/false, scan_type::segmented);
    test_scan_internal<T>(333, amp_algorithms::max<T>(), "Test segmented scan", backwards, /*inplace=*/true, scan_type::segmented);
}

template<>
void test_segmented<unsigned int>(bool backwards)
{
    test_scan_internal<unsigned int>(7123127, amp_algorithms::sum<unsigned int>(), "Test segmented scan", backwards, /*inplace=*/false, scan_type::segmented);
    test_scan_internal<unsigned int>(111, amp_algorithms::mul<unsigned int>(), "Test segmented scan", backwards, /*inplace=*/true, scan_type::segmented);
}

template<typename T>
void test_segmented_bitwise_op(bool backwards)
{
    test_scan_internal<T>(234, amp_algorithms::bit_and<T>(), "Test segmented scan", backwards, /*inplace=*/false, scan_type::segmented);
    test_scan_internal<T>(432, amp_algorithms::bit_or<T>(), "Test segmented scan", backwards, /*inplace=*/true, scan_type::segmented);
    test_scan_internal<T>(444, amp_algorithms::bit_xor<T>(), "Test segmented scan", backwards, /*inplace=*/true, scan_type::segmented);
}

void test_all_scans()
{
    bool backwards = false;

    for (int b=0; b<2; ++b)
    {
        test_scan<float>(backwards);
        test_scan<unsigned int>(backwards);
        test_scan<int>(backwards);
        test_scan_bitwise_op<int>(backwards);

        test_multiscan<int>(backwards);
        test_multiscan<unsigned int>(backwards);
        test_multiscan<float>(backwards);
        test_multiscan_bitwise_op<int>(backwards);

        test_segmented<int>(backwards);
        test_segmented<unsigned int>(backwards);
        test_segmented<float>(backwards);
        test_segmented_bitwise_op<int>(backwards);

        backwards = !backwards;
    }

    test_scan_other();
}

int main()
{
    try 
    {
        test_reduce<double>(1023 * 1029 * 13, amp_algorithms::sum<double>(), "Test sum reduction double");
        test_reduce<int>(1023 * 1029 * 13, amp_algorithms::min<int>(), "Test min reduction int");
        test_reduce<float>(1023 * 1029 * 5, amp_algorithms::max<float>(), "Test max reduction float");
        test_reduce<float>(5, amp_algorithms::mul<float>(), "Test mul reduction float");

        test_functor_view<float>(1023 * 31);
        test_functor_view<int>(21);
        test_generate();
        test_unary_transform();
        test_binary_transform();
        test_fill();

        test_all_scans();
    }
    catch(const std::exception &e)
    {
        std::cout << e.what() << std::endl;
    }
    system("pause");
    return 0;
}
