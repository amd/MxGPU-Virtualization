# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.16)

project(nl LANGUAGES C)

find_library(LIBMNL_LIBRARY mnl)
find_path(LIBMNL_INCLUDE_DIR libmnl/libmnl.h)

if(LIBMNL_LIBRARY AND LIBMNL_INCLUDE_DIR)
    message(STATUS "libmnl found -- netlink is supported")
    add_library(nl STATIC
        src/nl.c
        src/nl_genl.c
    )
    target_include_directories(nl
        PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE ${LIBMNL_INCLUDE_DIR}
    )
    target_link_libraries(nl PUBLIC ${LIBMNL_LIBRARY})
    target_compile_definitions(nl PUBLIC LIBMNL_INSTALLED)
else()
    message(WARNING
        "libmnl not found. Building without it -- "
        "some NIC functionality and APIs will not work. "
        "Install the libmnl package and reconfigure to enable.")
    add_library(nl STATIC
        src/nl_stub.c
    )
    target_include_directories(nl
        PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
endif()

set_target_properties(nl PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build
)

target_compile_options(nl PRIVATE -Wall -Wextra -Werror -fPIC)
