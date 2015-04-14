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
* This file contains the entry-point for unit tests.
*---------------------------------------------------------------------------*/

#include <gtest\gtest.h>

#include <amp.h>
#include <exception>
#include <iostream>

int main(int argc, char **argv)
{
	using namespace std;

    try {
//		concurrency::accelerator::set_default(concurrency::accelerator::direct3d_warp);
        ::testing::InitGoogleTest(&argc, argv);
        return RUN_ALL_TESTS();
    }
    catch (exception& ex) {
        cerr << ex.what() << endl;
        cin.get();
    }
}