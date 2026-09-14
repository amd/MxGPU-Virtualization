# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT
#
# Common build fragments for the AMD SMI library build (smi-lib/Makefile):
# version defines plus the compiler/linker flags that are identical across the
# different build variants.
#
# This file intentionally contains NO source selection and NO cli/ references;
# it only assembles flags.
#
# Contract - the including Makefile MUST, before including this file:
#   - include make/linux/defines.mk (provides PROJECT_ROOT, DEFAULT_CFLAGS,
#     HOST_DYNAMIC_FLAGS, CXX, etc.)
#   - set INCLUDE (the -I... search paths for that build variant)
#   - set BUILD_TYPE / THREAD_SAFE / LOGGING / THREAD_SANITIZER /
#     ADDRESS_SANITIZER / ENABLE_GCOV (defaults are fine)
#
# Variant-specific additions (e.g. -DAMD_SMI_NIC_SUPPORT, UALOE flags) are
# appended by the caller AFTER including this file.

LIBNAME           := libamdsmi
DYNAMICLIB_TARGET := $(LIBNAME).so

# --- Version defines (absolute VERSION path so this works from any CWD) ------
VERSION_FILE    := $(PROJECT_ROOT)/VERSION
VERSION_MAJOR   := $(shell grep 'major=' $(VERSION_FILE) | cut -d '=' -f 2)
VERSION_MINOR   := $(shell grep 'minor=' $(VERSION_FILE) | cut -d '=' -f 2)
VERSION_RELEASE := $(shell grep 'release=' $(VERSION_FILE) | cut -d '=' -f 2)
VERSION_LIB     := AMDSMI_VERSION_MAJOR=$(VERSION_MAJOR) AMDSMI_VERSION_MINOR=$(VERSION_MINOR) AMDSMI_VERSION_RELEASE=$(VERSION_RELEASE)

# --- Base compiler flags -----------------------------------------------------
CFLAGS  = $(DEFAULT_CFLAGS) $(INCLUDE) -fPIC
CFLAGS += -DAMDSMI_VERSION_MAJOR=$(VERSION_MAJOR)
CFLAGS += -DAMDSMI_VERSION_MINOR=$(VERSION_MINOR)
CFLAGS += -DAMDSMI_VERSION_RELEASE=$(VERSION_RELEASE)

# --- Base linker flags -------------------------------------------------------
DYNAMICLIB_FLAGS  = -Wl,-soname,$(DYNAMICLIB_TARGET)
DYNAMICLIB_FLAGS += -Wl,-rpath,'$$ORIGIN:/usr/local/lib'
DYNAMICLIB_FLAGS += $(HOST_DYNAMIC_FLAGS)

GCC_MAJOR := $(shell $(CXX) -dumpversion | cut -d. -f1)
ifeq ($(shell test $(GCC_MAJOR) -lt 9; echo $$?),0)
    DYNAMICLIB_FLAGS += -lstdc++fs
endif

# --- Thread safety -----------------------------------------------------------
ifeq ($(THREAD_SAFE), True)
  CFLAGS += -DTHREAD_SAFE -pthread
  ifeq ($(THREAD_SANITIZER), True)
    CFLAGS           += -fsanitize=thread
    DYNAMICLIB_FLAGS += -fsanitize=thread
  endif
endif

# --- Build type --------------------------------------------------------------
ifeq ($(BUILD_TYPE), Debug)
  CFLAGS           += -g -fsanitize=address
  DYNAMICLIB_FLAGS += -fsanitize=address
else
  CFLAGS += -O2
endif

ifeq ($(ADDRESS_SANITIZER), True)
CFLAGS           += -fsanitize=address,undefined
DYNAMICLIB_FLAGS += -fsanitize=address,undefined
endif

ifeq ($(LOGGING), True)
	CFLAGS += -D SMI_ENABLE_LOGGING
endif

ifeq ($(BUILD_TYPE), Release)
	CFLAGS           += -Wl,-z,relro,-z,noexecstack,-z,noexecheap -Wl,--strip-debug -Wl,--strip-all
	DYNAMICLIB_FLAGS += -Wl,-z,relro,-z,noexecstack,-z,noexecheap -Wl,--strip-debug -Wl,--strip-all
endif

ifeq ($(ENABLE_GCOV), True)
	CFLAGS           += -coverage -DENABLE_GCOV
	DYNAMICLIB_FLAGS += -lgcov
endif
