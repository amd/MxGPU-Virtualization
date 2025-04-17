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

PROJECT_ROOT          := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
VERSION_FILE_PATH     := \"$(PROJECT_ROOT)/VERSION\"
BUILD_DIR             := $(PROJECT_ROOT)/build
SOURCE_DIR            := $(PROJECT_ROOT)/src
INCLUDE_DIR           := $(PROJECT_ROOT)/inc
HOST ?= linux

ifeq (" ","$(wildcard define_$(HOST).mk"))
$(error "Unsupported host param: $(HOST)")
endif

include $(PROJECT_ROOT)/../gim-coms-lib/Makefile

include $(PROJECT_ROOT)/define_$(HOST).mk

INTERFACE_DIR         := $(PROJECT_ROOT)/interface
EXAMPLES_DIR          := $(PROJECT_ROOT)/examples
TEST_DIR              := $(PROJECT_ROOT)/tests
TEST_UNIT_DIR         := $(TEST_DIR)/unit
TEST_INTEGRATION_DIR  := $(TEST_DIR)/integration
PY_DIR                := $(PROJECT_ROOT)/py
PY_INTERFACE_DIR      := $(PY_DIR)/interface

DEFAULT_CXXFLAGS = -Wall -Wextra -Werror \
			-Wno-missing-field-initializers \
			-Wmissing-declarations \
			-Werror=conversion \

# Flags supported from GCC version 6
GCC_VER_GE6      := $(shell echo `$(CC) -dumpversion | cut -f1-2 -d.`\>=6 | bc)
ifeq ($(GCC_VER_GE6),1)
	DEFAULT_CFLAGS    += -Wshift-negative-value
	DEFAULT_CXXFLAGS  += -Wshift-negative-value
endif
