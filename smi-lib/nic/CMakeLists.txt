#
# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.16)

project(amdsminic LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(NIC_INCLUDE_DIR ${PROJECT_ROOT}/inc)
set(NIC_SOURCE_DIR ${PROJECT_ROOT}/src)
set(NIC_BUILD_DIR ${PROJECT_ROOT}/build)
set(NIC_INTERFACE_DIR ${PROJECT_ROOT}/interface)

set(NIC_DEFAULT_CXX_FLAGS "-Wall -Wextra -Werror -Wno-missing-field-initializers -Wmissing-declarations -Werror=conversion -Wshift-negative-value -fPIC")
set(NIC_DEFAULT_CXX_FLAGS "${NIC_DEFAULT_CXX_FLAGS} -Wl,-z,relro,-z,noexecstack,-z,noexecheap -Wl,--strip-debug -Wl,--strip-all")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${NIC_DEFAULT_CXX_FLAGS}")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../nl ${CMAKE_CURRENT_BINARY_DIR}/nl)

find_path(LIBIBVERBS_INCLUDE_DIR infiniband/verbs.h)

file(GLOB CPP_SRCS "${NIC_SOURCE_DIR}/*.cpp")
add_library(amdsminic STATIC ${CPP_SRCS})
set_target_properties(amdsminic PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${NIC_BUILD_DIR})
target_include_directories(amdsminic PUBLIC ${NIC_INCLUDE_DIR} ${NIC_INTERFACE_DIR})
target_link_libraries(amdsminic PRIVATE nl ${CMAKE_DL_LIBS})

# LIBIBVERBS_INSTALLED is defined only when the headers are present
if(LIBIBVERBS_INCLUDE_DIR)
    message(STATUS "libibverbs headers found")
    target_include_directories(amdsminic PRIVATE ${LIBIBVERBS_INCLUDE_DIR})
    target_compile_definitions(amdsminic PRIVATE LIBIBVERBS_INSTALLED)
else()
    message(WARNING
        "libibverbs headers not found. Building without them -- "
        "some NIC features may not be supported. "
        "Install the libibverbs and reconfigure to enable.")
endif()
