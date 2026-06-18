# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT

export enable_gcov = false

LIBGV_PATH := $(dir $(lastword $(MAKEFILE_LIST)))

LIBGV_CCFLAGS = -D MAJOR_VERSION=0 -D MINOR_VERSION=0 -D SUBMINOR_VERSION=0
LIBGV_CCFLAGS += -D RWLOCK

LIBGV_CCFLAGS += -I $(LIBGV_PATH)/inc -I $(LIBGV_PATH)/core -I $(LIBGV_PATH)/core/hw/common

LIBGV_CCFLAGS += -I $(LIBGV_PATH)/core/hw/AI -I $(LIBGV_PATH)/core/hw/AI/asic_reg
	LIBGV_CCFLAGS += -I $(LIBGV_PATH)/core/hw/AI/mi200

LIBGV_CCFLAGS += -I $(LIBGV_PATH)/core/hw/AI/mi300

LIBGV_CCFLAGS += -I $(LIBGV_PATH)/core/hw/navi3 -I $(LIBGV_PATH)/core/hw/navi3/asic_reg


ifeq ($(enable_gcov), true)
       GCOV_PROFILE := y
       LIBGV_CCFLAGS += -fprofile-update=atomic
endif

include $(LIBGV_PATH)/core/Makefile
LIBGV_COMPILE_FILES += $(addprefix core/,$(LIBGV_CORE_FILES))
