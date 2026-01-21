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
PREFIX ?= /usr/local
AMD_SMI_NIC_SUPPORT ?= True

# Auto-detect VIRT  based on current path
ifeq ($(VIRT),)
	CURRENT_PATH := $(shell pwd)
	ifneq (,$(findstring /drivers/smi_lib/cli/cpp,$(CURRENT_PATH)))
		VIRT := GUEST
	else
		VIRT := HOST
	endif
else
	VIRT ?= HOST
endif

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

# Normalize VIRT value
ifneq ($(VIRT),GUEST)
	override VIRT := HOST
else
	override VIRT := GUEST
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
	-DBUILD_TYPE=$(BUILD_TYPE) \
	-DAMD_SMI_NIC_SUPPORT=$(AMD_SMI_NIC_SUPPORT) \
	-DVIRT=$(VIRT)

default: $(CMAKE_PATH)/CMakeLists.txt
ifeq ($(PLATFORM),linux)
	$(MAKE) cmake_config
	cmake --build $(BUILD_DIR) -- -j$(shell nproc)
	$(MAKE) clean_cmake
else ifeq ($(PLATFORM),windows)
	$(MAKE) -C $(CMAKE_PATH) $(OS_TARGET).$(BUILD_TYPE) VIRT=$(VIRT)
endif

cmake_config:
ifeq ($(PLATFORM),linux)
	@cp $(CMAKE_PATH)/CMakeLists.txt .
	cmake -S . -B $(BUILD_DIR) $(BUILD_FLAGS) -DCMAKE_INSTALL_PREFIX=$(PREFIX)
else ifeq ($(PLATFORM),windows)
	cmake -S $(CMAKE_PATH) -B $(BUILD_DIR) $(BUILD_FLAGS)
endif

.PHONY: wNow64a.rel wNow64a.dbg
wNow64a.rel:
	$(MAKE) -C $(CMAKE_PATH) wNow64a.rel VIRT=$(VIRT)

wNow64a.dbg:
	$(MAKE) -C $(CMAKE_PATH) wNow64a.dbg VIRT=$(VIRT)

.PHONY: wNxt64a.rel wNxt64a.dbg
wNxt64a.rel:
	$(MAKE) -C $(CMAKE_PATH) wNxt64a.rel VIRT=$(VIRT)

wNxt64a.dbg:
	$(MAKE) -C $(CMAKE_PATH) wNxt64a.dbg VIRT=$(VIRT)

.PHONY: clean_cmake
clean_cmake:
	@rm -f CMakeLists.txt

.PHONY: install
install:
ifeq ($(PLATFORM),linux)
	@if [ ! -d build ]; then \
		echo "Error: build directory not found. Run 'make' first."; \
		exit 1; \
	fi
	@echo "Installing using CMake..."
	DESTDIR=$(DESTDIR) cmake --install build --prefix $(PREFIX)
else ifeq ($(PLATFORM),windows)
	@echo "Installation is not supported on Windows."
	@exit 1
endif

.PHONY: uninstall
uninstall:
ifeq ($(PLATFORM),linux)
	@if [ -f build/install_manifest.txt ]; then \
		xargs rm -f < build/install_manifest.txt; \
		echo "Uninstalled successfully."; \
	else \
		echo "Error: install_manifest.txt not found. Nothing to uninstall."; \
		exit 1; \
	fi
else ifeq ($(PLATFORM),windows)
	@echo "Installation is not supported on Windows."
	@exit 1
endif

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)
