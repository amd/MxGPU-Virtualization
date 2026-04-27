#
# Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
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
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

export enable_gcov = false

LIBGV_PATH := $(dir $(lastword $(MAKEFILE_LIST)))

LIBGV_CCFLAGS = -D MAJOR_VERSION=0 -D MINOR_VERSION=0 -D SUBMINOR_VERSION=0 -D LIBGV_BUILD_DATE='"2025-07-24 10:19:16"'
LIBGV_CCFLAGS += -D RWLOCK

LIBGV_CCFLAGS += -I $(LIBGV_PATH)/inc -I $(LIBGV_PATH)/core -I $(LIBGV_PATH)/core/hw/common

LIBGV_CCFLAGS += -I $(LIBGV_PATH)/core/hw/AI -I $(LIBGV_PATH)/core/hw/AI/asic_reg
	LIBGV_CCFLAGS += -I $(LIBGV_PATH)/core/hw/AI/mi200

LIBGV_CCFLAGS += -I $(LIBGV_PATH)/core/hw/AI/mi300

	LIBGV_CCFLAGS += -I $(LIBGV_PATH)/core/hw/navi3 \
			-I $(LIBGV_PATH)/core/hw/navi3/asic_reg/

ifeq ($(enable_gcov), true)
       GCOV_PROFILE := y
       LIBGV_CCFLAGS += -fprofile-update=atomic
endif

include $(LIBGV_PATH)/core/Makefile
LIBGV_COMPILE_FILES += $(addprefix core/,$(LIBGV_CORE_FILES))
