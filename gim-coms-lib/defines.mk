#
# Copyright (c) 2019-2023 Advanced Micro Devices, Inc. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

GIM_COMS_ROOT         := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
GIM_COMS_BUILD_DIR    := $(GIM_COMS_ROOT)/build
GIM_COMS_SOURCE_DIR   := $(GIM_COMS_ROOT)/src
GIM_COMS_INCLUDE_DIR  := $(GIM_COMS_ROOT)/inc
GIM_COMS_LIB_NAME     := gim-coms-lib

GIM_COMS_MAJOR_VER  = 1
GIM_COMS_MINOR_VER  = 0
GIM_COMS_PATCH_VER  = 0
GIM_COMS_VER        = $(GIM_COMS_MAJOR_VER).$(GIM_COMS_MINOR_VER).$(GIM_COMS_PATCH_VER)

HOST_INCLUDE_FLAGS :=
HOST_FLAGS :=

CC               = gcc
CXX              = g++
AR               = ar

DEFAULT_CFLAGS   = -Wall -Wextra -Werror \
			-Wno-missing-field-initializers \
			-Wmissing-prototypes \
			-Werror=conversion \
