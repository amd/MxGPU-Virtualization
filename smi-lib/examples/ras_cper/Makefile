# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT


include ../../make/linux/defines.mk

BUILD_TYPE ?= Release
HEADER  = -I$(INTERFACE_DIR)
SO_PATH = -L$(BUILD_DIR)/amdsmi/$(BUILD_TYPE)/
SO_FILE = -lamdsmi -pthread
FLAGS   = $(DEFAULT_CFLAGS)
FLAGS += -Wl,-rpath,$(LIB_INSTALL_PATH)

ifeq ($(BUILD_TYPE), Debug)
  FLAGS += -fsanitize=address
endif

INCLUDE = $(HEADER) $(SO_PATH) $(SO_FILE)

all:
	$(CC) amdsmi_ras_cper.c -o amdsmi_ras_cper $(FLAGS) $(INCLUDE)

.PHONY: clean
clean:
	$(RM) -rf amdsmi_ras_cper
