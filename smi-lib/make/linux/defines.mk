# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT


PROJECT_ROOT		:= $(realpath $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))/../..)
VERSION_FILE_PATH	:= \"$(PROJECT_ROOT)/VERSION\"
BUILD_DIR		:= $(PROJECT_ROOT)/build
SOURCE_DIR		:= $(PROJECT_ROOT)/src
INCLUDE_DIR		:= $(PROJECT_ROOT)/inc
NIC_DIR 		:= $(PROJECT_ROOT)/nic
NIC_INCLUDE_DIR 	:= $(NIC_DIR)/inc
NIC_INTERFACE_DIR	:= $(NIC_DIR)/interface
NIC_SOURCE_DIR		:= $(NIC_DIR)/src
IMPORTED_DIR		:= $(PROJECT_ROOT)/imported
IMPORTED_ACA_DIR	:= $(IMPORTED_DIR)/aca-decode
IMPORTED_ACA_INC_DIR	:= $(IMPORTED_ACA_DIR)/include
IMPORTED_ACA_SRC_DIR	:= $(IMPORTED_ACA_DIR)/src
IMPORTED_UALOE_DIR	:= $(PROJECT_ROOT)/imported/ualoe
IMPORTED_UALOE_SRC_DIR	:= $(IMPORTED_UALOE_DIR)/src
IMPORTED_UALOE_INC_DIR	:= $(IMPORTED_UALOE_DIR)/include
HOST ?= linux
AMD_SMI_NIC_SUPPORT ?= True

ifeq (" ","$(wildcard define_$(HOST).mk"))
$(error "Unsupported host param: $(HOST)")
endif

include $(PROJECT_ROOT)/../gim-coms-lib/Makefile

DEFAULT_CXXFLAGS = -Wall -Wextra -Werror \
			-Wno-missing-field-initializers \
			-Wmissing-declarations \
			-Werror=conversion \

include $(PROJECT_ROOT)/make/$(HOST)/define_$(HOST).mk

INTERFACE_DIR		:= $(PROJECT_ROOT)/interface
EXAMPLES_DIR		:= $(PROJECT_ROOT)/examples
TEST_DIR		:= $(PROJECT_ROOT)/tests
TEST_UNIT_DIR		:= $(TEST_DIR)/unit
TEST_INTEGRATION_DIR	:= $(TEST_DIR)/integration
PY_DIR			:= $(PROJECT_ROOT)/py
PY_INTERFACE_DIR	:= $(PY_DIR)/interface

# Flags supported from GCC version 6
GCC_VER_GE6      := $(shell echo `$(CC) -dumpversion | cut -f1-2 -d.`\>=6 | bc)
ifeq ($(GCC_VER_GE6),1)
	DEFAULT_CFLAGS    += -Wshift-negative-value
	DEFAULT_CXXFLAGS  += -Wshift-negative-value
endif
