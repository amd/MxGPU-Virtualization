#
# Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
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

include ../defines.mk

OUTPUT_DIR := $(BUILD_DIR)/amdsmi/test
OUTPUT_DIR_LCOV := $(OUTPUT_DIR)/amdsmi_tests-lcov
UNIT_UTIL_TEST_MK := $(TEST_UNIT_DIR)/smi_util_test.mk
UNIT_TEST_MK := $(TEST_UNIT_DIR)/smi_unit_tests.mk
UNIT_LNX_WRAP_TEST_MK := $(TEST_UNIT_DIR)/smi_lnx_wrapper_tests.mk
INTEGRATION_TEST_MK := $(TEST_INTEGRATION_DIR)/smi_integration_tests.mk

LCOV_VERSION := $(shell lcov --version 2>/dev/null | awk '/LCOV version/ {print $$4}')

ifneq ($(shell printf '%s\n' "1.15" "$(LCOV_VERSION)" | sort -V | head -n1),1.15)
	LCOV_OPT :=
else
	LCOV_OPT := --ignore-errors gcov,unused,empty,format,mismatch,inconsistent,gcov,unused,empty,format,mismatch,inconsistent
endif

.PHONY: all
all: targets

.PHONY: targets
targets: util_test unit_tests integration_tests lnx_wrapper_tests

.PHONY: util_test
util_test:
	$(MAKE) -f $(UNIT_UTIL_TEST_MK)

.PHONY: unit_tests
unit_tests:
	$(MAKE) -f $(UNIT_TEST_MK)

.PHONY: lnx_wrapper_tests
lnx_wrapper_tests:
	$(MAKE) -f $(UNIT_LNX_WRAP_TEST_MK)

.PHONY: integration_tests
integration_tests:
	$(MAKE) -f $(INTEGRATION_TEST_MK)

.PHONY: run
run: targets
	$(MAKE) -f $(UNIT_UTIL_TEST_MK) run
	$(MAKE) -f $(UNIT_TEST_MK) run
	$(MAKE) -f $(UNIT_LNX_WRAP_TEST_MK) run

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

	mkdir -p $(OUTPUT_DIR_LCOV)

	@lcov \
		-a $(OUTPUT_DIR)/amdsmi_unit_tests-lcov/coverage.info \
		-a $(OUTPUT_DIR)/amdsmi_util_test-lcov/coverage.info \
		-a $(OUTPUT_DIR)/amdsmi_lnx_wrapper_test-lcov/coverage.info \
		-o $(OUTPUT_DIR_LCOV)/coverage.info

#	Combine lnx wrapper report with amdsmi reports
	@lcov --add $(BUILD_DIR)/amdsmi/test/amdsmi_tests-lcov/coverage.info --add $(BUILD_DIR)/amdsmi/test/amdsmi_lnx_wrapper_test-lcov/coverage.info -o $(BUILD_DIR)/amdsmi/test/coverage.info

#	Keep only info about our source files
	@lcov --extract $(BUILD_DIR)/amdsmi/test/coverage.info \
		'$(INCLUDE_DIR)/*' \
		'$(INTERFACE_DIR)/*' \
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
