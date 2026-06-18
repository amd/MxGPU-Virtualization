# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT


include ../make/linux/defines.mk

OUTPUT_DIR := $(BUILD_DIR)/amdsmi/test
OUTPUT_DIR_LCOV := $(OUTPUT_DIR)/amdsmi_tests-lcov
UNIT_UTIL_TEST_MK := $(TEST_UNIT_DIR)/smi_util_test.mk
UNIT_TEST_MK := $(TEST_UNIT_DIR)/smi_unit_tests.mk
UNIT_LNX_WRAP_TEST_MK := $(TEST_UNIT_DIR)/smi_lnx_wrapper_tests.mk
INTEGRATION_TEST_MK := $(TEST_INTEGRATION_DIR)/smi_integration_tests.mk

LCOV_VERSION := $(shell lcov --version 2>/dev/null | awk '/LCOV version/ {print $$4}')

ifeq ($(shell printf '%s\n' "2.0" "$(LCOV_VERSION)" | sort -V | head -n1),2.0)
LCOV_OPT := --ignore-errors inconsistent,inconsistent,mismatch,mismatch,gcov,gcov
else
LCOV_OPT :=
endif

.PHONY: all
all: targets

.PHONY: targets
ifeq ($(HOST),esxi)
targets: integration_tests
	@echo "Skipping unit tests for HOST=esxi"
else
targets: util_test unit_tests integration_tests lnx_wrapper_tests
endif

.PHONY: util_test
util_test:
	$(MAKE) -f $(UNIT_UTIL_TEST_MK) AMDSMI_VERSION_MAJOR=$(AMDSMI_VERSION_MAJOR) AMDSMI_VERSION_MINOR=$(AMDSMI_VERSION_MINOR) AMDSMI_VERSION_RELEASE=$(AMDSMI_VERSION_RELEASE)

.PHONY: unit_tests
unit_tests:
	$(MAKE) -f $(UNIT_TEST_MK) AMDSMI_VERSION_MAJOR=$(AMDSMI_VERSION_MAJOR) AMDSMI_VERSION_MINOR=$(AMDSMI_VERSION_MINOR) AMDSMI_VERSION_RELEASE=$(AMDSMI_VERSION_RELEASE)

.PHONY: lnx_wrapper_tests
lnx_wrapper_tests:
	$(MAKE) -f $(UNIT_LNX_WRAP_TEST_MK) AMDSMI_VERSION_MAJOR=$(AMDSMI_VERSION_MAJOR) AMDSMI_VERSION_MINOR=$(AMDSMI_VERSION_MINOR) AMDSMI_VERSION_RELEASE=$(AMDSMI_VERSION_RELEASE)

.PHONY: integration_tests
integration_tests:
	$(MAKE) -f $(INTEGRATION_TEST_MK) AMDSMI_VERSION_MAJOR=$(AMDSMI_VERSION_MAJOR) AMDSMI_VERSION_MINOR=$(AMDSMI_VERSION_MINOR) AMDSMI_VERSION_RELEASE=$(AMDSMI_VERSION_RELEASE)

.PHONY: run
ifeq ($(HOST),esxi)
run: targets
	@echo "Skipping unit/util/lnx_wrapper run."
else
run: targets
	$(MAKE) -f $(UNIT_UTIL_TEST_MK) run AMDSMI_VERSION_MAJOR=$(AMDSMI_VERSION_MAJOR) AMDSMI_VERSION_MINOR=$(AMDSMI_VERSION_MINOR) AMDSMI_VERSION_RELEASE=$(AMDSMI_VERSION_RELEASE)
	$(MAKE) -f $(UNIT_TEST_MK) run AMDSMI_VERSION_MAJOR=$(AMDSMI_VERSION_MAJOR) AMDSMI_VERSION_MINOR=$(AMDSMI_VERSION_MINOR) AMDSMI_VERSION_RELEASE=$(AMDSMI_VERSION_RELEASE)
	$(MAKE) -f $(UNIT_LNX_WRAP_TEST_MK) run AMDSMI_VERSION_MAJOR=$(AMDSMI_VERSION_MAJOR) AMDSMI_VERSION_MINOR=$(AMDSMI_VERSION_MINOR) AMDSMI_VERSION_RELEASE=$(AMDSMI_VERSION_RELEASE)
endif

# ...existing code...

.PHONY: gen_coverage
gen_coverage:
#	Check if the lcov is installed and his version is equal or greater than 1.15
	@if [ -z "$(LCOV_VERSION)" ]; then \
		echo "ERROR: lcov is required to generate code coverage."; exit 1; \
	fi
#	Clean up previous runs
	$(MAKE) GEN_COVERAGE=YES -f $(UNIT_UTIL_TEST_MK) clean
	$(MAKE) GEN_COVERAGE=YES -f $(UNIT_TEST_MK) clean
	$(MAKE) GEN_COVERAGE=YES -f $(UNIT_LNX_WRAP_TEST_MK) clean
#	Run unit tests and generate coverage info
	$(MAKE) GEN_COVERAGE=YES LCOV_OPT="$(LCOV_OPT)" -f $(UNIT_UTIL_TEST_MK) gen_coverage
	$(MAKE) GEN_COVERAGE=YES LCOV_OPT="$(LCOV_OPT)" -f $(UNIT_TEST_MK) gen_coverage
	$(MAKE) GEN_COVERAGE=YES LCOV_OPT="$(LCOV_OPT)" -f $(UNIT_LNX_WRAP_TEST_MK) gen_coverage

#	Filter out aca-decode from each coverage.info
	@lcov --remove $(BUILD_DIR)/amdsmi/test/amdsmi_unit_tests-lcov/coverage.info -o $(BUILD_DIR)/amdsmi/test/amdsmi_unit_tests-lcov/coverage.info $(LCOV_OPT)
	@lcov --remove $(BUILD_DIR)/amdsmi/test/amdsmi_util_test-lcov/coverage.info -o $(BUILD_DIR)/amdsmi/test/amdsmi_util_test-lcov/coverage.info $(LCOV_OPT)
	@lcov --remove $(BUILD_DIR)/amdsmi/test/amdsmi_lnx_wrapper_test-lcov/coverage.info -o $(BUILD_DIR)/amdsmi/test/amdsmi_lnx_wrapper_test-lcov/coverage.info $(LCOV_OPT)

	mkdir -p $(OUTPUT_DIR_LCOV)

	@lcov \
		-a $(BUILD_DIR)/amdsmi/test/amdsmi_unit_tests-lcov/coverage.info \
		-a $(BUILD_DIR)/amdsmi/test/amdsmi_util_test-lcov/coverage.info \
		-a $(BUILD_DIR)/amdsmi/test/amdsmi_lnx_wrapper_test-lcov/coverage.info \
		-o $(OUTPUT_DIR_LCOV)/coverage.info

#	Keep only info about our source files
	@lcov --extract $(OUTPUT_DIR_LCOV)/coverage.info \
		'$(INCLUDE_DIR)/*' \
		'$(SOURCE_DIR)/*' \
			$(LCOV_OPT) \
		-o $(BUILD_DIR)/amdsmi/test/result.info
#	Convert result to html
	@genhtml $(BUILD_DIR)/amdsmi/test/result.info \
		--output-directory $(BUILD_DIR)/amdsmi/test/coverage/
	@echo "\n\n >>> Test coverage available at: file://$(BUILD_DIR)/amdsmi/test/coverage/index.html \n\n"

.PHONY: clean
clean:
	$(MAKE) -f $(UNIT_UTIL_TEST_MK) clean
	$(MAKE) -f $(UNIT_TEST_MK) clean
	$(MAKE) -f $(INTEGRATION_TEST_MK) clean
	$(MAKE) -f $(UNIT_LNX_WRAP_TEST_MK) clean

	$(MAKE) GEN_COVERAGE=YES -f $(UNIT_TEST_MK) clean
	$(MAKE) GEN_COVERAGE=YES -f $(UNIT_UTIL_TEST_MK) clean
	$(MAKE) GEN_COVERAGE=YES -f $(UNIT_LNX_WRAP_TEST_MK) clean

	$(RM) $(OUTPUT_DIR)/result.info $(RM) $(OUTPUT_DIR)/coverage.info
	$(RM) -rf $(OUTPUT_DIR)/coverage
