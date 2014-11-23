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
*---------------------------------------------------------------------------*/

#define NOMINMAX

// TODO: Currently calls to amp_algorithms::copy() causes these warnings.

#define _SCL_SECURE_NO_WARNINGS         // (4996) function call with parameters that may be unsafe...
#pragma  warning (disable : 4267)       // Conversion from 'size_t' to 'int', possible loss of data
#pragma  warning (disable : 4244)       // 'argument' : conversion from '__int64' to 'int', possible loss of data

#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

// These are not required includes, they already included by the AMP library headers.
#include <amp.h>
#include <assert.h>
#include <wrl\client.h>
#include <d3d11.h>
#include <d3dcsx.h>
#include <iterator>
#include <sstream>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>
