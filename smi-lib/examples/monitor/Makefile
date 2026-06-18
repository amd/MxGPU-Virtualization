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
DEFS    =

ifeq ($(BUILD_TYPE), Debug)
  FLAGS += -fsanitize=address
endif

# display mode
# cmd: command line
# win: window mode
mode ?= cmd

ifeq ($(mode), win)
DEFS += -D WINDOW_MODE
FLAGS += -lncurses
endif

INCLUDE = $(HEADER) $(SO_PATH) $(SO_FILE)

all:
	$(CC) amdsmi_monitor.c -o amdsmi_monitor $(FLAGS) $(DEFS) $(INCLUDE)

.PHONY: clean
clean:
	$(RM) -rf amdsmi_monitor
