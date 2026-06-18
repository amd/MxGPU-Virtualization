# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT


include ../make/linux/defines.mk

THREAD_SAFE ?= True


OUTPUT_DIR := $(BUILD_DIR)/amdsmi/test/amdsmi_lnx_wrapper_test
ifeq ($(GEN_COVERAGE), YES)
	OUTPUT_DIR := $(BUILD_DIR)/amdsmi/test/amdsmi_lnx_wrapper_test-lcov
endif


LIB_SRCS := smi_sys_wrapper.c

TEST_SRCS := smi_wrapper_unit_tests.cpp
TEST_SRCS += smi_fake_user_gim_ioctl.cpp

OBJSC   := $(addprefix $(OUTPUT_DIR)/,$(LIB_SRCS:.c=.c.o))
OBJSCPP := $(addprefix $(OUTPUT_DIR)/,$(TEST_SRCS:.cpp=.cpp.o))

DEPS := $(OBJSC:.o=.d) $(OBJSCPP:.o=.d)

TARGET := libamdsmi_lnx_wrapper_test

INCLUDE := $(addprefix -I,\
  $(INCLUDE_DIR)\
  $(INTERFACE_DIR)\
  $(LIN_HOST_INCLUDE_DIR)\
  $(GIM_COMS_INCLUDE_DIR))

CFLAGS   = -std=c11 $(DEFAULT_CFLAGS) $(INCLUDE) -g -D_XOPEN_SOURCE=700
CXXFLAGS = -std=c++17 $(DEFAULT_CXXFLAGS) $(INCLUDE) -g -D_XOPEN_SOURCE=700

LDFLAGS = -lgtest -lgtest_main -lgmock -lgmock_main -pthread
LDFLAGS += -Wl,--wrap=poll
LDFLAGS += -Wl,--wrap=read
LDFLAGS += -Wl,--wrap=write
LDFLAGS += -Wl,--wrap=ioctl
LDFLAGS += -Wl,--wrap=open
LDFLAGS += -Wl,--wrap=access

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

vpath %.c $(LIN_SOURCE_DIR)
vpath %.cpp $(TEST_UNIT_DIR)

default: $(OUTPUT_DIR)/$(TARGET)

.PHONY: clean
clean:
	$(RM) $(OBJSC) $(OBJSCPP) $(OUTPUT_DIR)/$(TARGET) $(DEPS)
	$(RM) -rf $(BUILD_DIR)/test
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

	@lcov --remove $(OUTPUT_DIR)/coverage.info \
		--output-file $(OUTPUT_DIR)/coverage_filtered.info $(LCOV_OPT)
	@rm -f $(OUTPUT_DIR)/coverage.info
	@mv $(OUTPUT_DIR)/coverage_filtered.info $(OUTPUT_DIR)/coverage.info

-include $(DEPS)

$(OUTPUT_DIR)/%.c.o: %.c Makefile | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(OUTPUT_DIR)/%.cpp.o: %.cpp Makefile | $(OUTPUT_DIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(OUTPUT_DIR)/$(TARGET): $(OBJSC) $(OBJSCPP)| $(OUTPUT_DIR)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)
