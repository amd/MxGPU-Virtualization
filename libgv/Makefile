#
# Copyright (c) 2017-2021 Advanced Micro Devices, Inc. All rights reserved.
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

LIBGV_BUILD_DATE := $(shell date +"%Y-%m-%d %H:%M:%S")

LIBGV_PATH := $(dir $(lastword $(MAKEFILE_LIST)))
# version decode: major.minor.subminor
#       major: increase the major number every year
#				2020=>2.0.0 2021=>3.0.0 2022=>4.0.0
#       minor: increase the minor number according to roadmap
#				2.0.0=>2.1.0=>2.2.0=>...=>2.255.0
#       subminor: change the subminor number every week promotion
#				2.0.0=>2.0.1=>2.0.2=>...=>2.0.255
LIBGV_VERSION_FILE = $(LIBGV_PATH)/VERSION

ifneq ("", "$(wildcard $(LIBGV_VERSION_FILE))")
	MAJOR_NUM = $(shell cut -f1 -d. "$(LIBGV_VERSION_FILE)" | head -1)
	MINOR_NUM = $(shell cut -f2 -d. "$(LIBGV_VERSION_FILE)" | head -1)
	SUBMINOR_NUM = $(shell cut -f3 -d. "$(LIBGV_VERSION_FILE)" | cut -f1 -d+ | head -1)
else
	MAJOR_NUM = 0
	MINOR_NUM = 0
	SUBMINOR_NUM = 0
endif

subdir-ccflags-y += -D MAJOR_VERSION=$(MAJOR_NUM) -D MINOR_VERSION=$(MINOR_NUM) \
	-D SUBMINOR_VERSION=$(SUBMINOR_NUM) -D LIBGV_BUILD_DATE='"$(LIBGV_BUILD_DATE)"'
subdir-ccflags-y += -D RWLOCK

INCLUDE_PATH := -I $(LIBGV_PATH)/inc \
		-I $(LIBGV_PATH)/core -I $(LIBGV_PATH)/core/hw/common/

INCLUDE_PATH += -I $(LIBGV_PATH)/core/hw/AI \
		-I $(LIBGV_PATH)/core/hw/AI/asic_reg/

INCLUDE_PATH += -I $(LIBGV_PATH)/core/hw/AI/mi300

subdir-ccflags-y += $(INCLUDE_PATH)

include $(LIBGV_PATH)/core/Makefile

LIBGV_COMPILE_FILES = $(addprefix libgv/,$(LIBGV_FILES))
