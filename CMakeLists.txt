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
# C++ flags
#----------------------------------------------------------------------------

set(CMAKE_CXX_FLAGS
    "$(CMAKE_CXX_FLAGS) /D_CONSOLE /DWIN32 /D_WINDOWS /EHsc /nologo /W3 /WX /Zc:inline"
    CACHE STRING "Common compiler flags" FORCE
)

set(CMAKE_CXX_FLAGS_DEBUG
    "$(CMAKE_CXX_FLAGS_DEBUG) /D_DEBUG /MTd /EHsc /WX /Od /fp:precise"
    CACHE STRING "Debug compiler flags" FORCE
)

set(CMAKE_CXX_FLAGS_RELEASE
    "$(CMAKE_CXX_FLAGS_RELEASE) /EHsc /MT /Oi /Ox /WX /fp:fast"
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

#add_test(amp_algorithms_tests_padded_read amp_algorithms)
add_custom_target(all_tests ALL DEPENDS amp_algorithms)
add_custom_command(TARGET all_tests
    COMMENT "Run tests..."
    POST_BUILD COMMAND ctest -C $<CONFIGURATION> --output-on-failure
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})