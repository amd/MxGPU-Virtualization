#
# Copyright (c) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE

cmake_minimum_required(VERSION 3.16)

project(amd-smi)
include(${CMAKE_SOURCE_DIR}/cmake/smi_cli_defines.cmake)
set(TARGET amd-smi)
set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(PLATFORM linux)
set(TARGET amd-smi)

set(DEFAULT_CFLAGS
    -Wall
    -Wextra
    -Werror
    -Wno-missing-field-initializers
    -Wmissing-prototypes
    -Werror=conversion
    -Wshift-negative-value)
set(DEFAULT_CXXFLAGS
    -Wall
    -Wextra
    -Werror
    -Wno-missing-field-initializers
    -Wmissing-declarations
    -Werror=conversion
    -Wshift-negative-value
    -lstdc++fs)

include_directories(${INCLUDE_DIR} ${INTERFACE_DIR} ${THIRD_PARTY_INCLUDE_DIR})

file(GLOB SOURCES ${SOURCE_DIR}/*.cpp ${SOURCE_DIR}/host/*.cpp)

string(TOLOWER "${BUILD_TYPE}" BUILD_TYPE)
if(NOT (BUILD_TYPE STREQUAL "debug" OR BUILD_TYPE STREQUAL "release"))
  message(
    FATAL_ERROR
      "Unsupported build type: ${BUILD_TYPE}. Only debug and release are allowed."
  )
endif()

set(CMAKE_EXE_LINKER_FLAGS
    "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,relro,-z,noexecstack,-z,noexecheap -Wl,--strip-debug -Wl,--strip-all"
)

file(GLOB_RECURSE HPPS "${INCLUDE_DIR}/*.h")
file(GLOB_RECURSE HPPS "${THIRD_PARTY_INCLUDE_DIR}/*.h")

add_definitions(-pthread)
add_executable(amd-smi ${SOURCES} ${HPPS})
target_include_directories(${TARGET} PUBLIC ${INCLUDE_DIR})
target_include_directories(${TARGET} PUBLIC ${THIRD_PARTY_INCLUDE_DIR})
target_link_libraries(${TARGET} pthread)
target_link_libraries(${TARGET} stdc++fs)
target_link_libraries(${TARGET} ${CMAKE_DL_LIBS})

add_custom_command(
  TARGET amd-smi
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:amd-smi>
          ${CMAKE_SOURCE_DIR}/../../tool/amd-smi)
