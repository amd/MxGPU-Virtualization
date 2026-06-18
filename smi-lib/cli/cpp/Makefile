# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT


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
else ifeq ($(BUILD_PLATFORM),esxi)
	PLATFORM := esxi
	AMD_SMI_NIC_SUPPORT := False
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

ifeq ($(PLATFORM),esxi)
	ifneq ($(BUILD_TYPE),release)
		ifneq ($(BUILD_TYPE),debug)
			$(warning Unknown build type)
			override BUILD_TYPE := unknown
		endif
	endif
endif

BUILD_FLAGS := \
	-DBUILD_TYPE=$(BUILD_TYPE) \
	-DAMD_SMI_NIC_SUPPORT=$(AMD_SMI_NIC_SUPPORT) \
	-DVIRT=$(VIRT)

default:
ifeq ($(PLATFORM),linux)
	$(MAKE) cmake_config
	cmake --build $(BUILD_DIR) -- -j$(shell nproc)
	$(MAKE) clean_cmake
else ifeq ($(PLATFORM),windows)
	$(MAKE) -C $(CMAKE_PATH) $(OS_TARGET).$(BUILD_TYPE) VIRT=$(VIRT)
else ifeq ($(PLATFORM),esxi)
	$(MAKE) -C $(CMAKE_PATH)
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
	@if [ -f $(CMAKE_PATH)/Makefile ]; then $(MAKE) -C $(CMAKE_PATH) clean; fi
