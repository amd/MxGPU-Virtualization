#
# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
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

include ../../defines.mk

BUILD_TYPE ?= Release
HEADER  = -I$(INTERFACE_DIR)
SO_FILE = $(BUILD_DIR)/amdsmi/$(BUILD_TYPE)/libamdsmi.a
FLAGS   = -pthread $(DEFAULT_CFLAGS)
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
	$(CC) monitor.c -o monitor $(HEADER) $(SO_FILE) $(SO_PATH) $(FLAGS) $(DEFS)

.PHONY: clean
clean:
	$(RM) -rf monitor
