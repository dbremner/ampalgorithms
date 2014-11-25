#----------------------------------------------------------------------------
# Copyright (c) Microsoft Corp.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy
# of the License at http://www.apache.org/licenses/LICENSE-2.0
# 
# THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED 
# WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, 
# MERCHANTABLITY OR NON-INFRINGEMENT. 
#
# See the Apache Version 2.0 License for specific language governing 
# permissions and limitations under the License.
#---------------------------------------------------------------------------
# 
# C++ AMP algorithms library.
#
# CMake for C++ AMP Algorithms source
#---------------------------------------------------------------------------

set(SOURCE
   ${SOURCE}
   ${CMAKE_CURRENT_SOURCE_DIR}/examples_stl_amp_algorithms.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/stdafx.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/test_amp_algorithms.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/test_amp_algorithms_bitvector.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/test_amp_algorithms_direct3d_scan.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/test_amp_algorithms_operators.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/test_amp_algorithms_reduce.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/test_amp_algorithms_scan.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/test_amp_algorithms_sort.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/test_amp_stl_algorithms.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/test_amp_stl_algorithms_copy.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/test_amp_stl_algorithms_fill.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/test_amp_stl_algorithms_find.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/test_amp_stl_algorithms_is_sorted.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/test_amp_stl_algorithms_remove.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/test_amp_stl_algorithms_replace.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/test_amp_stl_algorithms_swap.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/test_amp_stl_algorithms_transform.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/test_amp_stl_iterators.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/test_testtools.cpp
   PARENT_SCOPE
)