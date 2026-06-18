# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT


include ../make/linux/defines.mk

THREAD_SAFE ?= True

OUTPUT_DIR := $(BUILD_DIR)/amdsmi/test/amdsmi_util_test
ifeq ($(GEN_COVERAGE), YES)
	OUTPUT_DIR := $(BUILD_DIR)/amdsmi/test/amdsmi_util_test-lcov
endif


# exclude sys wrapper - replace it with mock
EXCLUDE_LIN_LIB_SRCS := smi_sys_wrapper.c
ifeq ($(AMD_SMI_NIC_SUPPORT), True)
EXCLUDE_LIB_SRCS := amdsmi_nic_stub.c
endif

LIB_SRCS := $(filter-out $(EXCLUDE_LIB_SRCS),$(notdir $(wildcard $(SOURCE_DIR)/*.c)))
ifeq ($(AMD_SMI_NIC_SUPPORT), True)
LIB_SRCS += $(filter-out $(EXCLUDE_LIB_SRCS),$(notdir $(wildcard $(SOURCE_DIR)/nic/*.c)))
endif
LIB_SRCS += $(addprefix $(LIN_HOST_FOLDER)/,$(filter-out $(EXCLUDE_LIN_LIB_SRCS),$(notdir $(wildcard $(LIN_SOURCE_DIR)/*.c))))
SRCS_ACA := $(filter-out $(EXCLUDE_SRCS),$(notdir $(wildcard $(SOURCE_DIR)/aca-decode/*.c)))

TEST_SRCS := smi_test_util.cpp
TEST_SRCS += smi_fake_sys_wrapper.cpp
ifeq ($(AMD_SMI_NIC_SUPPORT), True)
TEST_SRCS += smi_fake_nic_interface.cpp
endif

OBJSC   := $(addprefix $(OUTPUT_DIR)/,$(LIB_SRCS:.c=.c.o))
OBJSC	+= $(addprefix ${OUTPUT_DIR}/,$(SRCS_ACA:.c=.c.o))
OBJSCPP := $(addprefix $(OUTPUT_DIR)/,$(TEST_SRCS:.cpp=.cpp.o))

DEPS := $(OBJSC:.o=.d) $(OBJSCPP:.o=.d)

TARGET := amdsmi_util_test

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
CFLAGS_ACA := $(filter-out $(DEFAULT_CFLAGS), $(CFLAGS))
CXXFLAGS = -std=c++17 $(DEFAULT_CXXFLAGS) $(INCLUDE) -g -D_XOPEN_SOURCE=700

CFLAGS += -DAMDSMI_VERSION_MAJOR
CFLAGS += -DAMDSMI_VERSION_MINOR
CFLAGS += -DAMDSMI_VERSION_RELEASE

LDFLAGS = -lgtest -lgtest_main -lgmock -lgmock_main -pthread
ASLR_COMMAND :=

ifeq ($(GEN_COVERAGE), YES)
	CFLAGS += --coverage
	CXXFLAGS += --coverage
	LDFLAGS += --coverage
endif

ifeq ($(THREAD_SAFE), True)
	CFLAGS  += -DTHREAD_SAFE
	CXXFLAGS  += -DTHREAD_SAFE

	ifeq ($(THREAD_SANITIZER), True)
		ASLR_COMMAND := sudo sysctl vm.mmap_rnd_bits=28
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

set_mmap_rnd_bits:
	$(ASLR_COMMAND)

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
CXXFLAGS_ACA := $(filter-out $(DEFAULT_CFLAGS), $(CXXFLAGS)) $(CXX_HOST_FLAGS)

$(OUTPUT_DIR)/%.c.o: %.c Makefile | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -DVERSION_FILE_PATH=$(VERSION_FILE_PATH) -MMD -MP -c $< -o $@

$(OUTPUT_DIR)/aca_%.c.o: $(SOURCE_DIR)/aca-decode/%.c | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -DVERSION_FILE_PATH=$(VERSION_FILE_PATH) -MMD -MP -c $< -o $@

ifeq ($(AMD_SMI_NIC_SUPPORT), True)
$(OUTPUT_DIR)/smi_nic_%.c.o: $(SOURCE_DIR)/nic/%.c | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -DVERSION_FILE_PATH=$(VERSION_FILE_PATH) -MMD -MP -c $< -o $@
endif

$(OUTPUT_DIR)/%.cpp.o: %.cpp Makefile | $(OUTPUT_DIR)
	$(CXX) $(CXXFLAGS_ACA) -MMD -MP -c $< -o $@

$(OUTPUT_DIR)/$(TARGET): $(OBJSC) $(OBJSCPP)| $(OUTPUT_DIR) set_mmap_rnd_bits
	$(CXX) -o $@ $^ $(LDFLAGS)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)/$(LIN_HOST_FOLDER)
