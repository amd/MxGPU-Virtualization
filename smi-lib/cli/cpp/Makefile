#
# Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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

BUILD_DIR := build
BUILD_PLATFORM ?=
BUILD_TYPE ?= release

ifeq ($(BUILD_PLATFORM),windows)
	PLATFORM := windows
else ifeq ($(BUILD_PLATFORM),linux)
	PLATFORM := linux
else ifeq ($(BUILD_PLATFORM),)
	ifeq ($(OS),Windows_NT)
		PLATFORM := windows
	else
		UNAME_S := $(shell uname -s)
		ifeq ($(UNAME_S),Linux)
			PLATFORM := linux
		else
			$(warning Unknown platform)
	endif
endif
else
	$(warning Unknown platform)
endif

CMAKE_PATH := cmake/$(PLATFORM)

ifeq ($(PLATFORM),linux)
	ifneq ($(BUILD_TYPE),release)
		ifneq ($(BUILD_TYPE),debug)
			$(warning Unknown build type)
			override BUILD_TYPE := unknown
		endif
	endif
endif

ifeq ($(PLATFORM),windows)
    ifeq ($(BUILD_TYPE),release)
        override BUILD_TYPE := rel
    else ifeq ($(BUILD_TYPE),debug)
        override BUILD_TYPE := dbg
	else
		$(warning Unknown build type)
		override BUILD_TYPE := unknown
    endif
    OS_TARGET := wNow64a
endif

BUILD_FLAGS := \
	-DBUILD_TYPE=$(BUILD_TYPE)

default: $(CMAKE_PATH)/CMakeLists.txt
ifeq ($(PLATFORM),linux)
	$(MAKE) cmake_config
	cmake --build $(BUILD_DIR) -- -j$(shell nproc)
	$(MAKE) clean_cmake
else ifeq ($(PLATFORM),windows)
	$(MAKE) -C $(CMAKE_PATH) $(OS_TARGET).$(BUILD_TYPE)
endif

cmake_config:
ifeq ($(PLATFORM),linux)
	@cp $(CMAKE_PATH)/CMakeLists.txt .
	cmake -S . -B $(BUILD_DIR) $(BUILD_FLAGS)
else ifeq ($(PLATFORM),windows)
	cmake -S $(CMAKE_PATH) -B $(BUILD_DIR) $(BUILD_FLAGS)
endif

.PHONY: wNow64a.rel wNow64a.dbg
wNow64a.rel:
	$(MAKE) OS_TARGET=wNow64a BUILD_TYPE=rel

wNow64a.dbg:
	$(MAKE) OS_TARGET=wNow64a BUILD_TYPE=dbg

.PHONY: wNxt64a.rel wNxt64a.dbg
wNxt64a.rel:
	$(MAKE) OS_TARGET=wNxt64a BUILD_TYPE=rel

wNxt64a.dbg:
	$(MAKE) OS_TARGET=wNxt64a BUILD_TYPE=dbg

.PHONY: clean_cmake
clean_cmake:
	@rm -f CMakeLists.txt

.PHONE: clean
clean:
	@rm -rf $(BUILD_DIR)
