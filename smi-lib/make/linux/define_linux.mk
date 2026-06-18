# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT


LIN_HOST_FOLDER              := linux
LIN_SOURCE_DIR               := $(SOURCE_DIR)/$(LIN_HOST_FOLDER)
LIN_HOST_INCLUDE_DIR         := $(INCLUDE_DIR)/$(LIN_HOST_FOLDER)

LIB_INSTALL_PATH             := /usr/local/lib

HOST_INCLUDE_FLAGS :=
HOST_DYNAMIC_FLAGS :=
CXX_HOST_FLAGS :=

CC               = gcc
CXX              = g++
AR               = ar

DEFAULT_CFLAGS   = -Wall -Wextra -Werror \
			-Wno-missing-field-initializers \
			-Wmissing-prototypes \
			-Werror=conversion \
