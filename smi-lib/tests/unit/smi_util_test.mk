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

THREAD_SAFE ?= True

OUTPUT_DIR := $(BUILD_DIR)/amdsmi/test/amdsmi_util_test
ifeq ($(GEN_COVERAGE), YES)
	OUTPUT_DIR := $(BUILD_DIR)/amdsmi/test/amdsmi_util_test-lcov
endif


# exclude sys wrapper - replace it with mock
EXCLUDE_LIN_LIB_SRCS := smi_sys_wrapper.c

LIB_SRCS := $(filter-out $(EXCLUDE_LIB_SRCS),$(notdir $(wildcard $(SOURCE_DIR)/*.c)))
LIB_SRCS += $(addprefix $(LIN_HOST_FOLDER)/,$(filter-out $(EXCLUDE_LIN_LIB_SRCS),$(notdir $(wildcard $(LIN_SOURCE_DIR)/*.c))))


TEST_SRCS := smi_test_util.cpp
TEST_SRCS += smi_fake_sys_wrapper.cpp


OBJSC   := $(addprefix $(OUTPUT_DIR)/,$(LIB_SRCS:.c=.c.o))
OBJSCPP := $(addprefix $(OUTPUT_DIR)/,$(TEST_SRCS:.cpp=.cpp.o))

DEPS := $(OBJSC:.o=.d) $(OBJSCPP:.o=.d)

TARGET := amdsmi_util_test

INCLUDE := $(addprefix -I,\
  $(INCLUDE_DIR)\
  $(INTERFACE_DIR)\
  $(LIN_HOST_INCLUDE_DIR)\
  $(GIM_COMS_INCLUDE_DIR))

CFLAGS   = -std=c11 $(DEFAULT_CFLAGS) $(INCLUDE) -g -D_XOPEN_SOURCE=700
CXXFLAGS = -std=c++17 $(DEFAULT_CXXFLAGS) $(INCLUDE) -g -D_XOPEN_SOURCE=700

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


vpath %.c $(SOURCE_DIR)
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

-include $(DEPS)

$(OUTPUT_DIR)/%.c.o: %.c Makefile | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -DVERSION_FILE_PATH=$(VERSION_FILE_PATH) -MMD -MP -c $< -o $@

$(OUTPUT_DIR)/%.cpp.o: %.cpp Makefile | $(OUTPUT_DIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(OUTPUT_DIR)/$(TARGET): $(OBJSC) $(OBJSCPP)| $(OUTPUT_DIR) set_mmap_rnd_bits
	$(CXX) -o $@ $^ $(LDFLAGS)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)/$(LIN_HOST_FOLDER)
