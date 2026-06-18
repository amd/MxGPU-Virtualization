# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT

include make/linux/defines.mk
export ASAN_OPTIONS=verify_asan_link_order=0
BUILD_TYPE ?= Release
THREAD_SAFE ?= True
LOGGING ?= False
THREAD_SANITIZER ?= False
ADDRESS_SANITIZER ?= False
CPPTOOL_BUILD ?= False
ENABLE_GCOV ?= False

# Auto-detect UALOE support based on presence of the ualoe-lib/ source tree.
# When present (and netlink libs are installed) we build the real fabric
# telemetry implementation; otherwise we fall back to src/amdsmi_ualoe_stub.c
# which makes the fabric telemetry APIs return AMDSMI_STATUS_NOT_SUPPORTED.
HAS_UALOE := $(if $(wildcard ualoe-lib/ualoe_lib),True,False)
ifeq ($(HAS_UALOE), True)
  HAS_NETLINK := $(shell pkg-config --exists libnl-3.0 libnl-genl-3.0 libmnl 2>/dev/null && echo True)
  ifneq ($(HAS_NETLINK), True)
    $(warning UALOE source tree present but netlink dev packages (libnl-3.0, libnl-genl-3.0, libmnl) not installed; building without fabric telemetry support. Install the netlink dev packages to enable it.)
    HAS_UALOE := False
  endif
endif

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
$(info SMI-Lib NIC support: $(AMD_SMI_NIC_SUPPORT))
$(info SMI-Lib UALOE support (auto-detected): $(HAS_UALOE))
AMDSMI_BUILD_DIR := $(BUILD_DIR)/amdsmi
OUTPUT_DIR := $(AMDSMI_BUILD_DIR)/$(BUILD_TYPE)
PACKAGE_OUTPUT_DIR := $(AMDSMI_BUILD_DIR)/package/$(BUILD_TYPE)/amdsmi

# Configuration tracking - automatically rebuild when config changes
CONFIG_STAMP := $(OUTPUT_DIR)/.config.stamp
CONFIG_VARS := HAS_UALOE=$(HAS_UALOE) BUILD_TYPE=$(BUILD_TYPE) THREAD_SAFE=$(THREAD_SAFE) AMD_SMI_NIC_SUPPORT=$(AMD_SMI_NIC_SUPPORT)

DOCS_OUTPUT_DIR := $(AMDSMI_BUILD_DIR)/docs
DOXYGEN_OUTPUT_DIR := $(DOCS_OUTPUT_DIR)/doxygen
HOOKS_DIR := $(PROJECT_ROOT)/../.git/hooks
HOOKS_SRC_DIR := $(PROJECT_ROOT)/utils/hooks
ifeq ($(PREFIX),)
  PREFIX := /usr/local
endif
ifeq ($(AMD_SMI_NIC_SUPPORT), True)
EXCLUDE_SRCS := amdsmi_nic_stub.c
endif
ifeq ($(HAS_UALOE), True)
EXCLUDE_SRCS += amdsmi_ualoe_stub.c
endif
SRCS := $(filter-out $(EXCLUDE_SRCS),$(notdir $(wildcard $(SOURCE_DIR)/*.c)))
SRCS_ACA := $(filter-out $(EXCLUDE_SRCS),$(notdir $(wildcard $(SOURCE_DIR)/aca-decode/*.c)))
ifeq ($(HAS_UALOE), True)
SRCS_UALOE := $(notdir $(wildcard ualoe-lib/ualoe_lib/*.c))
SRCS_UALOE += $(notdir $(wildcard $(SOURCE_DIR)/ualoe/*.c))
else
SRCS_UALOE :=
endif
SRCS += $(addprefix $(LIN_HOST_FOLDER)/,$(notdir $(wildcard $(LIN_SOURCE_DIR)/*.c)))
ifeq ($(AMD_SMI_NIC_SUPPORT), True)
SRCS += $(filter-out $(EXCLUDE_SRCS),$(notdir $(wildcard $(SOURCE_DIR)/nic/*.c)))
endif
OBJS := $(addprefix ${OUTPUT_DIR}/,$(SRCS:.c=.c.o))
OBJS += $(addprefix ${OUTPUT_DIR}/,$(SRCS_ACA:.c=.c.o))
OBJS += $(addprefix ${OUTPUT_DIR}/,$(SRCS_UALOE:.c=.c.o))
OBJS  += $(GIM_COMS_OBJS)
DEPS := $(OBJS:.o=.d)
LIBNAME := libamdsmi
DYNAMICLIB_TARGET := $(LIBNAME).so
INCLUDE := $(addprefix -I,$(INCLUDE_DIR) $(INCLUDE_DIR)/aca-decode/ $(INTERFACE_DIR) $(LIN_HOST_INCLUDE_DIR) $(GIM_COMS_INCLUDE_DIR) $(NIC_INCLUDE_DIR) $(NIC_INTERFACE_DIR) $(SOURCE_DIR)/nic)
ifeq ($(HAS_UALOE), True)
INCLUDE += $(addprefix -I,ualoe-lib/ualoe_lib ualoe-lib/include $(SOURCE_DIR)/ualoe)
endif
INCLUDE += $(HOST_INCLUDE_FLAGS)
CFLAGS  = $(DEFAULT_CFLAGS) $(INCLUDE) -fPIC
ifeq ($(HAS_UALOE), True)
# Add libnl dependencies for ualoe
CFLAGS += $(shell pkg-config --cflags libnl-3.0 libnl-genl-3.0 libmnl)
CFLAGS += -DENABLE_UALOE
UALOE_LIBS := $(shell pkg-config --libs libnl-3.0 libnl-genl-3.0 libmnl)
else
UALOE_LIBS :=
endif
VERSION_FILE := VERSION
VERSION_MAJOR := $(shell grep 'major=' $(VERSION_FILE) | cut -d '=' -f 2)
VERSION_MINOR := $(shell grep 'minor=' $(VERSION_FILE) | cut -d '=' -f 2)
VERSION_RELEASE := $(shell grep 'release=' $(VERSION_FILE) | cut -d '=' -f 2)
CFLAGS += -DAMDSMI_VERSION_MAJOR=$(VERSION_MAJOR)
CFLAGS += -DAMDSMI_VERSION_MINOR=$(VERSION_MINOR)
CFLAGS += -DAMDSMI_VERSION_RELEASE=$(VERSION_RELEASE)
VERSION_LIB := AMDSMI_VERSION_MAJOR=$(VERSION_MAJOR) AMDSMI_VERSION_MINOR=$(VERSION_MINOR) AMDSMI_VERSION_RELEASE=$(VERSION_RELEASE)
DYNAMICLIB_FLAGS = -Wl,-soname,libamdsmi.so
DYNAMICLIB_FLAGS += -Wl,-rpath,'$$ORIGIN:/usr/local/lib'
DYNAMICLIB_FLAGS += $(HOST_DYNAMIC_FLAGS)
DYNAMICLIB_FLAGS += $(UALOE_LIBS)
GCC_MAJOR := $(shell $(CXX) -dumpversion | cut -d. -f1)
ifeq ($(shell test $(GCC_MAJOR) -lt 9; echo $$?),0)
    DYNAMICLIB_FLAGS += -lstdc++fs
endif

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
	CFLAGS += -coverage -DENABLE_GCOV
	DYNAMICLIB_FLAGS += -lgcov
endif
DEFAULT_DEPS := $(OUTPUT_DIR)/$(DYNAMICLIB_TARGET)
ifeq ($(THREAD_SANITIZER), False)
	DEFAULT_DEPS += package
endif
ifeq ($(CPPTOOL_BUILD), True)
	DEFAULT_DEPS += cpptool_build
endif
ifeq ($(AMD_SMI_NIC_SUPPORT), True)
CFLAGS += -DAMD_SMI_NIC_SUPPORT
DYNAMICLIB_FLAGS += -DAMD_SMI_NIC_SUPPORT
endif
default: $(DEFAULT_DEPS)
ALL_DEPS := default test_run install-hooks
ifeq ($(THREAD_SANITIZER), False)
	ALL_DEPS += examples
endif
# Build everything
.PHONY: all
all: $(ALL_DEPS)
ifeq ($(HAS_UALOE), True)
vpath %.c $(SOURCE_DIR) $(SOURCE_DIR)/aca-decode/ $(SOURCE_DIR)/nic/ $(SOURCE_DIR)/ualoe/ ualoe-lib/ualoe_lib
else
vpath %.c $(SOURCE_DIR) $(SOURCE_DIR)/aca-decode/ $(SOURCE_DIR)/nic/
endif

# Configuration stamp file rule - detects config changes and forces rebuild
$(CONFIG_STAMP): | $(OUTPUT_DIR)
	@echo "$(CONFIG_VARS)" > $@.tmp
	@if ! cmp -s $@.tmp $@ 2>/dev/null; then \
		echo "Configuration changed, forcing rebuild..."; \
		$(RM) -rf $(OBJS) $(DEPS); \
		mv $@.tmp $@; \
	else \
		$(RM) $@.tmp; \
	fi

$(OBJS): Makefile $(CONFIG_STAMP)
-include $(DEPS)
ifeq ($(AMD_SMI_NIC_SUPPORT), True)
$(NIC_DIR)/build/libamdsminic.a:
	$(MAKE) -C $(NIC_DIR)
endif
$(OUTPUT_DIR)/%.c.o: $(SOURCE_DIR)/%.c | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -DBUILD_TYPE=$(BUILD_TYPE) -DVERSION_FILE_PATH=$(VERSION_FILE_PATH) -MMD -MP -c $< -o $@
$(OUTPUT_DIR)/%.c.o: $(SOURCE_DIR)/aca-decode/%.c | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -DBUILD_TYPE=$(BUILD_TYPE) -DVERSION_FILE_PATH=$(VERSION_FILE_PATH) -MMD -MP -c $< -o $@
ifeq ($(HAS_UALOE), True)
$(OUTPUT_DIR)/%.c.o: ualoe-lib/ualoe_lib/%.c | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -DUALOE_NETLINK -DCFG_MOCK -Wno-unused-variable -Wno-unused-parameter -Wno-conversion -Wno-enum-conversion -Wno-sign-compare -Wno-stringop-truncation -DBUILD_TYPE=$(BUILD_TYPE) -DVERSION_FILE_PATH=$(VERSION_FILE_PATH) -MMD -MP -c $< -o $@
$(OUTPUT_DIR)/%.c.o: $(SOURCE_DIR)/ualoe/%.c | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -DBUILD_TYPE=$(BUILD_TYPE) -DVERSION_FILE_PATH=$(VERSION_FILE_PATH) -MMD -MP -c $< -o $@
endif
ifeq ($(AMD_SMI_NIC_SUPPORT), True)
$(OUTPUT_DIR)/%.c.o: $(SOURCE_DIR)/nic/%.c | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -DBUILD_TYPE=$(BUILD_TYPE) -DVERSION_FILE_PATH=$(VERSION_FILE_PATH) -MMD -MP -c $< -o $@
endif
ifeq ($(AMD_SMI_NIC_SUPPORT), True)
SMI_NL_LIB := $(NIC_DIR)/build/nl/build/libnl.a
LIBMNL_AVAILABLE := $(shell pkg-config --exists libmnl 2>/dev/null && echo yes || echo no)
SMI_NIC_LINK_LIBS := $(NIC_DIR)/build/libamdsminic.a $(SMI_NL_LIB)
ifeq ($(LIBMNL_AVAILABLE), yes)
SMI_NIC_LINK_LIBS += -lmnl
endif
SMI_NIC_LINK_LIBS += -ldl
$(OUTPUT_DIR)/$(DYNAMICLIB_TARGET): $(OBJS) $(NIC_DIR)/build/libamdsminic.a | $(OUTPUT_DIR)
	$(CXX) -shared -o $@ $^ $(DYNAMICLIB_FLAGS) $(SMI_NIC_LINK_LIBS)
else
$(OUTPUT_DIR)/$(DYNAMICLIB_TARGET): $(OBJS) | $(OUTPUT_DIR)
	$(CXX) -shared -o $@ $^ $(DYNAMICLIB_FLAGS)
endif
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
	cp $(PY_INTERFACE_DIR)/amdsmi_nic_interface.py $(PACKAGE_OUTPUT_DIR)
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
	$(MAKE) -C $(TEST_DIR)/ $(VERSION_LIB) AMD_SMI_NIC_SUPPORT=$(AMD_SMI_NIC_SUPPORT)
.PHONY: examples
examples: $(OUTPUT_DIR)/$(DYNAMICLIB_TARGET)
	$(MAKE) -C $(EXAMPLES_DIR)/ AMD_SMI_NIC_SUPPORT=$(AMD_SMI_NIC_SUPPORT)
.PHONY: test_clean
test_clean:
	$(MAKE) -C $(TEST_DIR)/ clean AMD_SMI_NIC_SUPPORT=$(AMD_SMI_NIC_SUPPORT)
.PHONY: test_run
test_run: test
	$(MAKE) -C $(TEST_DIR)/ run $(VERSION_LIB) AMD_SMI_NIC_SUPPORT=$(AMD_SMI_NIC_SUPPORT)
.PHONY: gen_coverage
gen_coverage:
	$(MAKE) -C $(TEST_DIR)/ gen_coverage $(VERSION_LIB) AMD_SMI_NIC_SUPPORT=$(AMD_SMI_NIC_SUPPORT)
ifeq ($(CPPTOOL_BUILD), True)
.PHONY: cpptool_build
cpptool_build:
	$(MAKE) -C $(PROJECT_ROOT)/cli/cpp AMD_SMI_NIC_SUPPORT=$(AMD_SMI_NIC_SUPPORT)
endif

.PHONY: install-hooks
install-hooks:
	@if [ -d $(HOOKS_DIR) ]; then \
		echo "Installing git hooks..."; \
		for hook in pre-commit post-rewrite version-check.sh; do \
			if [ -f $(HOOKS_DIR)/$$hook ]; then \
				echo "Removing outdated $$hook hook..."; \
				rm $(HOOKS_DIR)/$$hook; \
			fi; \
			cp $(HOOKS_SRC_DIR)/$$hook $(HOOKS_DIR)/$$hook; \
			chmod +x $(HOOKS_DIR)/$$hook; \
		done; \
		echo "Git hooks installed."; \
	fi

.PHONY: clean
clean:
	$(RM) -rf $(OBJS) $(AMDSMI_BUILD_DIR) $(DEPS) $(CONFIG_STAMP)
	$(MAKE) -C $(TEST_DIR)/ clean AMD_SMI_NIC_SUPPORT=$(AMD_SMI_NIC_SUPPORT)
	$(MAKE) -C $(EXAMPLES_DIR)/ clean AMD_SMI_NIC_SUPPORT=$(AMD_SMI_NIC_SUPPORT)
ifeq ($(AMD_SMI_NIC_SUPPORT), True)
	$(MAKE) -C $(NIC_DIR)/ clean
endif
	@if [ -d $(HOOKS_DIR) ]; then \
		for hook in pre-commit post-rewrite version-check.sh; do \
			if [ -f $(HOOKS_DIR)/$$hook ]; then \
				echo "Removing $$hook hook..."; \
				rm $(HOOKS_DIR)/$$hook; \
			fi; \
		done; \
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
