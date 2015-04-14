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
* This file includes a primitive detection mechanism which identifies the
* native SIMD width for the default accelerator selected by the AMP runtime.
* Note that practically this might not be the hardware width, but rather the
* maximum width that ensures correct execution of cross-lane ops in the absence
* of barriers. The egregious hack that we use to patch this information into
* the library before it gets compiled should be generally ignored, as it will
* be replaced with a robust mechanism.
*---------------------------------------------------------------------------*/

#include <algorithm>
#include <amp.h>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <utility>
#include <vector>


template<int tsz>
inline bool _test_sz()
{	// This is a trivial test, can be fooled. Neither division nor subtraction is commutative,
	// thus ordering is important: if we are running at native SIMD width, the loop within the
	// p_f_e will behave just like the serial loop, since only one lane is active per cycle. If
	// we are not lanes from the "future" will trample over ones from the "past" yielding
	// incorrect results.
	using namespace concurrency;

	array_view<int> r0(1);
	r0[0] = 0;

	concurrency::parallel_for_each(extent<1>(tsz).tile<tsz>(), [=](auto&& tidx) restrict(amp) {
		for (auto i = 0; i != tsz; ++i) {
			if (tidx.local[0] == i) {
				if (i % 2) r0[0] *= tidx.local[0];
				if (!(i % 2)) r0[0] -= tidx.local[0];
			}
		}
	});

	int r1 = 0;
	for (auto i = 0; i != tsz; ++i) {
		if (i % 2) r1 *= i;
		if (!(i % 2)) r1 -= i;
	}

	return r0[0] == r1;
}

template<bool>
inline void _determine_native_simd_width_impl(std::vector<std::pair<std::string, bool>>&)
{
}

template<bool, int sz, int... szs>
inline void _determine_native_simd_width_impl(std::vector<std::pair<std::string, bool>>& sz_vec)
{
	sz_vec.push_back(std::make_pair(std::to_string(sz), _test_sz<sz>()));
	_determine_native_simd_width_impl<true, szs...>(sz_vec);
}

std::string determine_native_simd_width()
{
	using namespace std;

	vector<pair<string, bool>> sz;
	_determine_native_simd_width_impl<true, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024>(sz);

	return find_if(crbegin(sz), crend(sz), [](auto&& x) { return x.second; })->first;
}

int main()
{
	using namespace std;

	try { // This is a rather egregious and quick hack - do not reuse. Even regex would be better.
		static const char p[] = "..\\inc\\native_simd_width.h";
		ifstream inf(p);
		if (inf) {
			static const char s[] = "native_simd_width =";

			string tmp(istreambuf_iterator<char>(inf.rdbuf()), istreambuf_iterator<char>{});
			auto t = search(cbegin(tmp), cend(tmp), cbegin(s), prev(cend(s))); // Ignore terminal null.

			if (t != cend(tmp)) {
				t += prev(cend(s)) - cbegin(s);
				tmp.replace(t, find(t, cend(tmp), ';'), " " + determine_native_simd_width());

				ofstream outf(p, ios_base::trunc);
				if (outf) {
					copy(cbegin(tmp), cend(tmp), ostreambuf_iterator<char>(outf));
				}
				else {
					throw runtime_error("Could not open native_simd_width.h for writing!");
				}

				if (!outf) {
					throw runtime_error("Writing to native_simd_width.h failed!");
				}

				cout << "Established: " << determine_native_simd_width() << " as native SIMD width"
					 << " for accelerator: ";
				wcout << concurrency::accelerator().description << endl;
			}
			else {
				throw runtime_error("tile size parameters not found in native_simd_width.h!");
			}
		}
		else {
			throw runtime_error("native_simd_width.h could not be accessed!");
		}
	}
	catch (exception& ex) {
		cerr << ex.what() << endl;
		cin.get();

		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}