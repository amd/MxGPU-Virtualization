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


BUILD_TYPE ?= Release
THREAD_SAFE ?= True

HEAP_MEMORY_CHECK := False
include ../defines.mk

MODUL_NAME := integration_tests
OUTPUT_DIR := $(BUILD_DIR)/amdsmi/$(MODUL_NAME)/$(BUILD_TYPE)
LIB_DIR := $(BUILD_DIR)/amdsmi/$(BUILD_TYPE)
LIB_NAME := libamdsmi.so

TEST_SRCS := smi_integration_tests.cpp

OBJSCPP := $(addprefix $(OUTPUT_DIR)/,$(TEST_SRCS:.cpp=.cpp.o))

DEPS := $(OBJSCPP:.o=.d)

TARGET := amdsmi_integration_tests

INCLUDE := $(addprefix -I,\
  $(INTERFACE_DIR))

CXXFLAGS = -std=c++17 $(DEFAULT_CXXFLAGS) $(INCLUDE) -pthread
LDPATH = $(addprefix -L,$(BUILD_DIR)/amdsmi/$(BUILD_TYPE))
LDFLAGS = -lgtest -lgtest_main -pthread -lamdsmi

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

$(OUTPUT_DIR)/$(TARGET): $(OBJSCPP)| $(OUTPUT_DIR) $(OUTPUT_DIR)/$(LIB_NAME)
	$(CXX) -o $@ $^ $(LDPATH) $(LDFLAGS)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)
