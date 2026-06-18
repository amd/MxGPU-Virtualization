# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT



BUILD_TYPE ?= Release
THREAD_SAFE ?= True

HEAP_MEMORY_CHECK := False
include ../make/linux/defines.mk

MODUL_NAME := integration_tests
OUTPUT_DIR := $(BUILD_DIR)/amdsmi/$(MODUL_NAME)/$(BUILD_TYPE)
LIB_DIR := $(BUILD_DIR)/amdsmi/$(BUILD_TYPE)
LIB_NAME := libamdsmi.so

TEST_SRCS := smi_integration_tests.cpp
ifeq ($(AMD_SMI_NIC_SUPPORT), True)
TEST_SRCS += smi_integration_nic_tests.cpp
endif

OBJSCPP := $(addprefix $(OUTPUT_DIR)/,$(TEST_SRCS:.cpp=.cpp.o))

DEPS := $(OBJSCPP:.o=.d)

TARGET := amdsmi_integration_tests

INCLUDE := $(addprefix -I,\
  $(INTERFACE_DIR))

CXXFLAGS = -std=c++17 $(DEFAULT_CXXFLAGS) $(INCLUDE) $(CXX_HOST_FLAGS) -pthread
LDPATH = $(addprefix -L,$(BUILD_DIR)/amdsmi/$(BUILD_TYPE))
LDFLAGS = -lgtest -lgtest_main -pthread -lamdsmi
LINK_OBJS := $(OBJSCPP)

ifeq ($(HOST),esxi)
  GTEST_PREFIX ?= /usr/local
  CXXFLAGS = -std=c++17 $(DEFAULT_CXXFLAGS) $(INCLUDE) -I$(GTEST_PREFIX)/include $(CXX_HOST_FLAGS) -pthread
  LDFLAGS := -pthread -lamdsmi -lgtest -lgtest_main
  LDPATH += -L$(GTEST_PREFIX)/lib -L$(GTEST_PREFIX)/lib64
endif

CXXFLAGS += -Wno-sign-compare

GCC_MAJOR := $(shell $(CXX) -dumpversion | cut -d. -f1)
ifeq ($(shell test $(GCC_MAJOR) -lt 9; echo $$?),0)
    LDFLAGS += -lstdc++fs
endif

ifeq ($(BUILD_TYPE), Debug)
	CXXFLAGS += -g
else
	CXXFLAGS += -O2
endif

ifeq ($(THREAD_SAFE), True)
	CXXFLAGS  += -DTHREAD_SAFE

	ifeq ($(THREAD_SANITIZER), True)
		CXXFLAGS += -fsanitize=thread
		LDFLAGS += -fsanitize=thread
	endif
endif

ifeq ($(ADDRESS_SANITIZER), True)
CXXFLAGS  += -fsanitize=address,undefined
LDFLAGS += -fsanitize=address,undefined
endif


ifeq ($(HEAP_MEMORY_CHECK), True)
	LDFLAGS += -ltcmalloc
endif

vpath %.c $(SOURCE_DIR)
vpath %.cpp $(TEST_INTEGRATION_DIR)

default: $(OUTPUT_DIR)/$(TARGET)

.PHONY: clean
clean:
	$(RM) -rf $(BUILD_DIR)/amdsmi/$(MODUL_NAME)

-include $(DEPS)

$(OUTPUT_DIR)/$(LIB_NAME): $(LIB_DIR)/$(LIB_NAME) | $(OUTPUT_DIR)
	cp $(LIB_DIR)/$(LIB_NAME) $(OUTPUT_DIR)

$(OUTPUT_DIR)/%.cpp.o: %.cpp Makefile | $(OUTPUT_DIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(OUTPUT_DIR)/$(TARGET): $(LINK_OBJS) | $(OUTPUT_DIR) $(OUTPUT_DIR)/$(LIB_NAME)
	$(CXX) $(CXX_HOST_FLAGS) -o $@ $(LINK_OBJS) $(LDPATH) $(LDFLAGS)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)
