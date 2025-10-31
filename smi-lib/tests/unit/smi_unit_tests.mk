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

GEN_COVERAGE ?= NO
THREAD_SAFE ?= True

OUTPUT_DIR := $(BUILD_DIR)/amdsmi/test/amdsmi_unit_tests/
ifeq ($(GEN_COVERAGE), YES)
	OUTPUT_DIR := $(BUILD_DIR)/amdsmi/test/amdsmi_unit_tests-lcov
endif

# exclude sys wrapper - replace it with mock
EXCLUDE_LIN_LIB_SRCS := smi_sys_wrapper.c
ifeq ($(AMD_SMI_NIC_SUPPORT), True)
EXCLUDE_LIB_SRCS := amdsmi_nic_stub.c
endif

LIB_SRCS := $(filter-out $(EXCLUDE_LIB_SRCS),$(notdir $(wildcard $(SOURCE_DIR)/*.c)))
LIB_SRCS += $(addprefix $(LIN_HOST_FOLDER)/,$(filter-out $(EXCLUDE_LIN_LIB_SRCS),$(notdir $(wildcard $(LIN_SOURCE_DIR)/*.c))))
ifeq ($(AMD_SMI_NIC_SUPPORT), True)
LIB_SRCS += $(filter-out $(EXCLUDE_LIB_SRCS),$(notdir $(wildcard $(SOURCE_DIR)/nic/*.c)))
endif
SRCS_ACA := $(filter-out $(EXCLUDE_SRCS),$(notdir $(wildcard $(SOURCE_DIR)/aca-decode/*.c)))

TEST_SRCS := smi_test_init.cpp
TEST_SRCS += smi_test_board_info.cpp
TEST_SRCS += smi_test_device.cpp
TEST_SRCS += smi_test_fw_vbios.cpp
TEST_SRCS += smi_test_uuid.cpp
TEST_SRCS += smi_test_device_monitoring.cpp
TEST_SRCS += smi_test_events.cpp
TEST_SRCS += smi_test_vf.cpp
TEST_SRCS += smi_test_dfc_table.cpp
TEST_SRCS += smi_test_driver.cpp
TEST_SRCS += smi_test_ecc.cpp
TEST_SRCS += smi_test_fw_attestation.cpp
TEST_SRCS += smi_test_partition_profile.cpp
TEST_SRCS += smi_test_vf2pf.cpp
TEST_SRCS += smi_test_structures_alignment.cpp
TEST_SRCS += smi_test_xgmi.cpp
TEST_SRCS += smi_test_chiplet_metrics.cpp
TEST_SRCS += smi_test_version.cpp
TEST_SRCS += smi_test_partitions.cpp
TEST_SRCS += smi_test_ras_cper.cpp
TEST_SRCS += smi_test_numa_info.cpp

TEST_SRCS += smi_fake_sys_wrapper.cpp
TEST_SRCS += smi_test_helpers.cpp

ifeq ($(AMD_SMI_NIC_SUPPORT), True)
TEST_SRCS += smi_test_nic.cpp
TEST_SRCS += smi_fake_nic_interface.cpp
else
TEST_SRCS += smi_test_nic_stub.cpp
endif

OBJSC   := $(addprefix $(OUTPUT_DIR)/,$(LIB_SRCS:.c=.c.o))
OBJSC	+= $(addprefix ${OUTPUT_DIR}/,$(SRCS_ACA:.c=.c.o))
OBJSCPP := $(addprefix $(OUTPUT_DIR)/,$(TEST_SRCS:.cpp=.cpp.o))

DEPS := $(OBJSC:.o=.d) $(OBJSCPP:.o=.d)

TARGET := amdsmi_unit_tests

INCLUDE := $(addprefix -I,\
  $(INCLUDE_DIR)\
  $(INTERFACE_DIR)\
  $(LIN_HOST_INCLUDE_DIR)\
  $(GIM_COMS_INCLUDE_DIR)\
  $(INCLUDE_DIR)/aca-decode/\
  $(NIC_INCLUDE_DIR)\
  $(NIC_INTERFACE_DIR)\
  $(SOURCE_DIR)/nic)

CFLAGS   = -std=c11 $(DEFAULT_CFLAGS) $(INCLUDE) -g -D_XOPEN_SOURCE=700
CXXFLAGS = -std=c++17 $(DEFAULT_CXXFLAGS) $(INCLUDE) -g -D_XOPEN_SOURCE=700
CFLAGS_ACA := $(filter-out $(DEFAULT_CFLAGS), $(CFLAGS))

CFLAGS += -DAMDSMI_VERSION_MAJOR=$(AMDSMI_VERSION_MAJOR)
CFLAGS += -DAMDSMI_VERSION_MINOR=$(AMDSMI_VERSION_MINOR)
CFLAGS += -DAMDSMI_VERSION_RELEASE=$(AMDSMI_VERSION_RELEASE)

LDFLAGS = -lgtest -lgtest_main -lgmock -lgmock_main -pthread

ifeq ($(GEN_COVERAGE), YES)
	CFLAGS += --coverage
	CXXFLAGS += --coverage
	LDFLAGS += --coverage
endif

ifeq ($(THREAD_SAFE), True)
	CFLAGS  += -DTHREAD_SAFE
	CXXFLAGS  += -DTHREAD_SAFE

	ifeq ($(THREAD_SANITIZER), True)
		CFLAGS  += -fsanitize=thread
		CXXFLAGS += -fsanitize=thread
		LDFLAGS += -fsanitize=thread
	endif
endif

ifeq ($(ADDRESS_SANITIZER), True)
CFLAGS  += -fsanitize=address,undefined
CXXFLAGS  += -fsanitize=address,undefined
LDFLAGS += -fsanitize=address,undefined
endif

ifeq ($(AMD_SMI_NIC_SUPPORT), True)
	CFLAGS += -DAMD_SMI_NIC_SUPPORT
	CXXFLAGS += -DAMD_SMI_NIC_SUPPORT
endif


vpath %.c $(SOURCE_DIR) $(SOURCE_DIR)/aca-decode/ $(SOURCE_DIR)/nic/
vpath %.cpp $(TEST_UNIT_DIR)


default: $(OUTPUT_DIR)/$(TARGET)

.PHONY: clean
clean:
	$(RM) $(OBJSC) $(OBJSCPP) $(OUTPUT_DIR)/$(TARGET) $(DEPS)
	$(RM) $(OUTPUT_DIR)/coverage.info
	$(RM) $(OUTPUT_DIR)/*.gcno $(OUTPUT_DIR)/*.gcda

.PHONY: run
run: $(OUTPUT_DIR)/$(TARGET)
	$(OUTPUT_DIR)/$(TARGET)

.PHONY: gen_coverage
gen_coverage: run
#	Capture data from test run into coverage.info
	@lcov -b $(SOURCE_DIR) -b $(TEST_UNIT_DIR) --capture $(LCOV_OPT) \
			--directory $(OUTPUT_DIR) \
			--output-file $(OUTPUT_DIR)/coverage.info

	@lcov --remove $(OUTPUT_DIR)/coverage.info "*/aca-decode/*" \
		--output-file $(OUTPUT_DIR)/coverage_filtered.info $(LCOV_OPT)
	@rm -f $(OUTPUT_DIR)/coverage.info
	@mv $(OUTPUT_DIR)/coverage_filtered.info $(OUTPUT_DIR)/coverage.info

-include $(DEPS)

$(OUTPUT_DIR)/%.c.o: %.c Makefile | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -DVERSION_FILE_PATH=$(VERSION_FILE_PATH) -MMD -MP -c $< -o $@

$(OUTPUT_DIR)/aca_%.c.o: $(SOURCE_DIR)/aca-decode/%.c | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -DVERSION_FILE_PATH=$(VERSION_FILE_PATH) -MMD -MP -c $< -o $@

ifeq ($(AMD_SMI_NIC_SUPPORT), True)
$(OUTPUT_DIR)/smi_nic_%.c.o: $(SOURCE_DIR)/nic/%.c | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -DVERSION_FILE_PATH=$(VERSION_FILE_PATH) -MMD -MP -c $< -o $@
endif

$(OUTPUT_DIR)/%.cpp.o: %.cpp Makefile | $(OUTPUT_DIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(OUTPUT_DIR)/$(TARGET): $(OBJSC) $(OBJSCPP)| $(OUTPUT_DIR)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)/$(LIN_HOST_FOLDER)
