#
# Copyright (c) 2019-2023 Advanced Micro Devices, Inc. All rights reserved.
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

include defines.mk

export ASAN_OPTIONS=verify_asan_link_order=0

BUILD_TYPE ?= Release
THREAD_SAFE ?= True
LOGGING ?= False
THREAD_SANITIZER ?= False
ADDRESS_SANITIZER ?= False
CPPTOOL_BUILD ?= False
ENABLE_GCOV ?= False

ifeq ($(BUILD_TYPE), Debug)
  LOGGING = True
endif

ifeq ($(filter $(BUILD_TYPE), Debug Release), )
  $(error "Unsupported build type: $(BUILD_TYPE). Only Debug and Release are allowed")
endif

ifeq ($(filter $(THREAD_SAFE), True False), )
  $(error "Unsupported thread safe param: $(THREAD_SAFE). Only True and False are allowed")
endif

ifeq ($(filter $(LOGGING), True False), )
  $(error Unsupported logging param: $(LOGGING). Only True and False are allowed)
endif

$(info SMI-LIB BUILD_TYPE: $(BUILD_TYPE))
$(info SMI-LIB THREAD_SAFE: $(THREAD_SAFE))
$(info SMI-Lib LOGGING: $(LOGGING))

AMDSMI_BUILD_DIR := $(BUILD_DIR)/amdsmi
OUTPUT_DIR := $(AMDSMI_BUILD_DIR)/$(BUILD_TYPE)
PACKAGE_OUTPUT_DIR := $(AMDSMI_BUILD_DIR)/package/$(BUILD_TYPE)/amdsmi
DOCS_OUTPUT_DIR := $(AMDSMI_BUILD_DIR)/docs
DOXYGEN_OUTPUT_DIR := $(DOCS_OUTPUT_DIR)/doxygen
HOOKS_DIR=../.git/modules/smi-lib/hooks

ifeq ($(PREFIX),)
  PREFIX := /usr/local
endif

SRCS := $(filter-out $(EXCLUDE_SRCS),$(notdir $(wildcard $(SOURCE_DIR)/*.c)))
SRCS_ACA := $(filter-out $(EXCLUDE_SRCS),$(notdir $(wildcard $(SOURCE_DIR)/aca-decode/*.c)))
SRCS += $(addprefix $(LIN_HOST_FOLDER)/,$(notdir $(wildcard $(LIN_SOURCE_DIR)/*.c)))
OBJS := $(addprefix ${OUTPUT_DIR}/,$(SRCS:.c=.c.o))
OBJS += $(addprefix ${OUTPUT_DIR}/,$(SRCS_ACA:.c=.c.o))

OBJS  += $(GIM_COMS_OBJS)

DEPS := $(OBJS:.o=.d)

LIBNAME := libamdsmi
DYNAMICLIB_TARGET := $(LIBNAME).so

INCLUDE := $(addprefix -I,$(INCLUDE_DIR) $(INCLUDE_DIR)/aca-decode/  $(INTERFACE_DIR) $(LIN_HOST_INCLUDE_DIR) $(GIM_COMS_INCLUDE_DIR))
INCLUDE += $(HOST_INCLUDE_FLAGS)

CFLAGS  = $(DEFAULT_CFLAGS) $(INCLUDE) -fPIC

VERSION_FILE := VERSION
VERSION_MAJOR := $(shell grep 'major=' $(VERSION_FILE) | cut -d '=' -f 2)
VERSION_MINOR := $(shell grep 'minor=' $(VERSION_FILE) | cut -d '=' -f 2)
VERSION_RELEASE := $(shell grep 'release=' $(VERSION_FILE) | cut -d '=' -f 2)

CFLAGS += -DAMDSMI_VERSION_MAJOR=$(VERSION_MAJOR)
CFLAGS += -DAMDSMI_VERSION_MINOR=$(VERSION_MINOR)
CFLAGS += -DAMDSMI_VERSION_RELEASE=$(VERSION_RELEASE)

VERSION_LIB := AMDSMI_VERSION_MAJOR=$(VERSION_MAJOR) AMDSMI_VERSION_MINOR=$(VERSION_MINOR) AMDSMI_VERSION_RELEASE=$(VERSION_RELEASE)

DYNAMICLIB_FLAGS = -Wl,-soname,libamdsmi.so
DYNAMICLIB_FLAGS += -Wl,-rpath,'$$ORIGIN'
DYNAMICLIB_FLAGS += $(HOST_DYNAMIC_FLAGS)

ifeq ($(THREAD_SAFE), True)
  CFLAGS  += -DTHREAD_SAFE -pthread

  ifeq ($(THREAD_SANITIZER), True)
    CFLAGS  += -fsanitize=thread
    DYNAMICLIB_FLAGS += -fsanitize=thread
  endif
endif

ifeq ($(BUILD_TYPE), Debug)
  CFLAGS += -g -fsanitize=address
  DYNAMICLIB_FLAGS += -fsanitize=address
else
  CFLAGS += -O2
endif

ifeq ($(ADDRESS_SANITIZER), True)
CFLAGS  += -fsanitize=address,undefined
DYNAMICLIB_FLAGS += -fsanitize=address,undefined
endif

ifeq ($(LOGGING), True)
	CFLAGS += -D SMI_ENABLE_LOGGING
endif

ifeq ($(BUILD_TYPE), Release)
	CFLAGS  += -Wl,-z,relro,-z,noexecstack,-z,noexecheap -Wl,--strip-debug -Wl,--strip-all
	DYNAMICLIB_FLAGS += -Wl,-z,relro,-z,noexecstack,-z,noexecheap -Wl,--strip-debug -Wl,--strip-all
endif

ifeq ($(ENABLE_GCOV), True)
	CFLAGS += -coverage
	DYNAMICLIB_FLAGS += -lgcov
endif

DEFAULT_DEPS := $(OUTPUT_DIR)/$(DYNAMICLIB_TARGET)
ifeq ($(THREAD_SANITIZER), False)
	DEFAULT_DEPS += package
endif

ifeq ($(CPPTOOL_BUILD), True)
	DEFAULT_DEPS += cpptool_build
endif

default: $(DEFAULT_DEPS)

ALL_DEPS := default test_run install-hooks
ifeq ($(THREAD_SANITIZER), False)
	ALL_DEPS += examples
endif


# Build everything
.PHONY: all
all: $(ALL_DEPS)


vpath %.c $(SOURCE_DIR) $(SOURCE_DIR)/aca-decode/

$(OBJS): Makefile

-include $(DEPS)


$(OUTPUT_DIR)/%.c.o: $(SOURCE_DIR)/%.c | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -DBUILD_TYPE=$(BUILD_TYPE) -DVERSION_FILE_PATH=$(VERSION_FILE_PATH) -MMD -MP -c $< -o $@

$(OUTPUT_DIR)/%.c.o: $(SOURCE_DIR)/aca-decode/%.c | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -DBUILD_TYPE=$(BUILD_TYPE) -DVERSION_FILE_PATH=$(VERSION_FILE_PATH) -MMD -MP -c $< -o $@

$(OUTPUT_DIR)/$(DYNAMICLIB_TARGET): $(OBJS) | $(OUTPUT_DIR)
	$(CC) -shared -o $@ $^ $(DYNAMICLIB_FLAGS)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)/$(LIN_HOST_FOLDER)

$(PACKAGE_OUTPUT_DIR):
	mkdir -p $(PACKAGE_OUTPUT_DIR)

.PHONY: package
package: $(OUTPUT_DIR)/$(DYNAMICLIB_TARGET) | $(PACKAGE_OUTPUT_DIR)
	cp $(OUTPUT_DIR)/$(DYNAMICLIB_TARGET) $(PACKAGE_OUTPUT_DIR)
	cp $(PY_INTERFACE_DIR)/__init__.py $(PACKAGE_OUTPUT_DIR)
	cp $(PY_INTERFACE_DIR)/amdsmi_wrapper.py $(PACKAGE_OUTPUT_DIR)
	cp $(PY_INTERFACE_DIR)/amdsmi_interface.py $(PACKAGE_OUTPUT_DIR)
	cp $(PY_INTERFACE_DIR)/amdsmi_exception.py $(PACKAGE_OUTPUT_DIR)
	echo $(PACKAGE_OUTPUT_DIR)

PYTHONPATH=$(PACKAGE_OUTPUT_DIR)/../ python3 -B $(PY_DIR)/tests/unit/smi_unit_tests.py

.PHONY: python_wrapper
python_wrapper: EXTRACT | $(OUTPUT_DIR)/$(DYNAMICLIB_TARGET)
	cp $(PROJECT_ROOT)/utils/scripts/smi_generator.py $(AMDSMI_BUILD_DIR)/amdsmi_wrapper

	python3 -B $(AMDSMI_BUILD_DIR)/amdsmi_wrapper/smi_generator.py -o $(AMDSMI_BUILD_DIR)/amdsmi_wrapper/amdsmi_wrapper.py \
			-i $(INTERFACE_DIR)/amdsmi.h \
			-l $(OUTPUT_DIR)/$(DYNAMICLIB_TARGET)

EXTRACT:
	mkdir -p $(AMDSMI_BUILD_DIR)/amdsmi_wrapper
	pip3 download ctypeslib2==2.3.2 -d $(PROJECT_ROOT)/dl
	pip3 download clang==10.0.1 -d $(PROJECT_ROOT)/dl
	unzip -o $(PROJECT_ROOT)/dl/ctypeslib2-2.3.2-py3-none-any.whl -d $(AMDSMI_BUILD_DIR)/amdsmi_wrapper
	unzip -o $(PROJECT_ROOT)/dl/clang-10.0.1-py3-none-any.whl -d $(AMDSMI_BUILD_DIR)/amdsmi_wrapper

.PHONY: test
test: $(OUTPUT_DIR)/$(DYNAMICLIB_TARGET)
	$(MAKE) -C $(TEST_DIR)/ $(VERSION_LIB)

.PHONY: examples
examples: $(OUTPUT_DIR)/$(DYNAMICLIB_TARGET)
	$(MAKE) -C $(EXAMPLES_DIR)/

.PHONY: test_clean
test_clean:
	$(MAKE) -C $(TEST_DIR)/ clean

.PHONY: test_run
test_run: test
	$(MAKE) -C $(TEST_DIR)/ run $(VERSION_LIB)

.PHONY: gen_coverage
gen_coverage:
	$(MAKE) -C $(TEST_DIR)/ gen_coverage $(VERSION_LIB)

ifeq ($(CPPTOOL_BUILD), True)
.PHONY: cpptool_build
cpptool_build:
	mkdir -p $(PROJECT_ROOT)/tool/cpptool/build;
	cd $(PROJECT_ROOT)/tool/cpptool/build/; cmake ../; make -j4;
endif

.PHONY: install-hooks
install-hooks:
	@if [ -d $(HOOKS_DIR) ]; then \
		echo "Installing git hooks..."; \
		if [ -f $(HOOKS_DIR)/pre-commit ]; then \
			echo "Removing outdated pre-commit hook..."; \
			rm $(HOOKS_DIR)/pre-commit; \
		fi; \
		cp pre-commit $(HOOKS_DIR)/pre-commit; \
		chmod +x $(HOOKS_DIR)/pre-commit; \
		echo "Git hooks installed."; \
	fi

.PHONY: clean
clean:
	$(RM) -rf $(OBJS) $(AMDSMI_BUILD_DIR) $(DEPS)
	$(MAKE) -C $(TEST_DIR)/ clean
	$(MAKE) -C $(EXAMPLES_DIR)/ clean
	@if [ -f $(HOOKS_DIR)/pre-commit ]; then \
		echo "Removing pre-commit hook..."; \
		rm $(HOOKS_DIR)/pre-commit; \
	fi

install: $(OUTPUT_DIR)/$(DYNAMICLIB_TARGET)
	install -d $(DESTDIR)$(PREFIX)/lib/
	install -m 644 $(OUTPUT_DIR)/$(DYNAMICLIB_TARGET) $(DESTDIR)$(PREFIX)/lib/
	install -d $(DESTDIR)$(PREFIX)/include/
	install -m 644 $(INTERFACE_DIR)/*.h $(DESTDIR)$(PREFIX)/include/

uninstall:
	@if [ -f $(DESTDIR)$(PREFIX)/lib/$(DYNAMICLIB_TARGET) ]; then $(RM) $(DESTDIR)$(PREFIX)/lib/$(DYNAMICLIB_TARGET); fi
	@if [ -f $(DESTDIR)$(PREFIX)/include/amdsmi.h ]; then $(RM) $(DESTDIR)$(PREFIX)/include/amdsmi.h; fi

.PHONY: docs
docs:
	@doxygen --version > /dev/null || (echo "ERROR: doxygen is required to generate documentation. You can install it by typing: sudo apt-get install doxygen"; exit 1)
	python3 -mvenv .venv
	.venv/bin/python -m pip install -r docs/sphinx/requirements.txt
	mkdir -p $(DOXYGEN_OUTPUT_DIR)
	cd $(PROJECT_ROOT)/docs/doxygen && \
	DOXYGEN_INPUT_DIR=$(INTERFACE_DIR) \
	DOXYGEN_OUTPUT_DIR=$(DOXYGEN_OUTPUT_DIR) \
		doxygen Doxyfile
	.venv/bin/sphinx-build -b html docs/ $(DOCS_OUTPUT_DIR)
	@echo "Documentation generated at: $(AMDSMI_BUILD_DIR)/docs/index.html"
