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

cmake_minimum_required (VERSION 2.6)

project (amp_algorithms)

#----------------------------------------------------------------------------
# Flags
#----------------------------------------------------------------------------

set(CMAKE_CXX_FLAGS
  "/DWIN32 /D_WINDOWS /D_CONSOLE /EHsc /W3 /WX"
  CACHE STRING "Compiler flags" FORCE
)

set(CMAKE_CXX_FLAGS_DEBUG
  "/DWIN32 /D_WINDOWS /EHsc /WX"
  CACHE STRING "Debug compiler flags" FORCE
)

set(CMAKE_CXX_FLAGS_RELEASE
  "/DWIN32 /D_WINDOWS /EHsc /WX"
  CACHE STRING "Release compiler flags" FORCE
)

#----------------------------------------------------------------------------
# Google Test
#----------------------------------------------------------------------------

add_subdirectory(gtest-1.7.0)
enable_testing()
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

#----------------------------------------------------------------------------
# AMP Algorithms
#----------------------------------------------------------------------------

add_subdirectory("${PROJECT_SOURCE_DIR}/test")
include_directories("${PROJECT_SOURCE_DIR}/inc")

add_executable(amp_algorithms ${SOURCE} test/gtest_main.cpp)
target_link_libraries(amp_algorithms gtest)

#----------------------------------------------------------------------------
# Tests
#----------------------------------------------------------------------------

add_test(amp_algorithms_tests_padded_read amp_algorithms)
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
                  DEPENDS amp_algorithms.exe)