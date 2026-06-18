# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT


cmake_minimum_required(VERSION 3.16)

project(amd-smi)
include(${CMAKE_SOURCE_DIR}/cmake/smi_cli_defines.cmake)
set(TARGET amd-smi)
set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
set(CMAKE_BUILD_RPATH "$ORIGIN:/usr/local/lib")
set(CMAKE_INSTALL_RPATH "$ORIGIN:/usr/local/lib")
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

if(NOT DEFINED VIRT)
  set(VIRT "HOST")
endif()

if(VIRT STREQUAL "GUEST")
  message(FATAL_ERROR "Cannot build CLI tool for guest on Linux")
endif()

file(GLOB SOURCES ${SOURCE_DIR}/*.cpp ${SOURCE_DIR}/host/*.cpp)
file(GLOB HPPS "${INCLUDE_DIR}/*.h")
list(FILTER HPPS EXCLUDE REGEX ".*smi_cli_api_guest\\.h$")


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

file(GLOB_RECURSE THIRD_PARTY_HPPS "${THIRD_PARTY_INCLUDE_DIR}/*.h")
list(APPEND HPPS ${THIRD_PARTY_HPPS})

add_definitions(-pthread)
add_executable(amd-smi ${SOURCES} ${HPPS})
target_include_directories(${TARGET} PUBLIC ${INCLUDE_DIR})
target_include_directories(${TARGET} PUBLIC ${THIRD_PARTY_INCLUDE_DIR})
target_link_libraries(${TARGET} pthread)
target_link_libraries(${TARGET} stdc++fs)
target_link_libraries(${TARGET} ${CMAKE_DL_LIBS})

# Define installation rules
install(
    TARGETS amd-smi
    RUNTIME DESTINATION bin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_READ WORLD_EXECUTE
)