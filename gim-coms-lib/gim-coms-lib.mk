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

BUILD_TYPE ?= Release

ifeq ($(filter $(BUILD_TYPE), Debug Release), )
  $(error "Unsupported build type: $(BUILD_TYPE). Only Debug and Release are allowed")
endif

$(info GIM-COMS-LIB BUILD_TYPE: $(BUILD_TYPE))

OUTPUT_DIR := $(GIM_COMS_BUILD_DIR)/$(BUILD_TYPE)

SRCS := $(filter-out $(EXCLUDE_SRCS),$(notdir $(wildcard $(GIM_COMS_SOURCE_DIR)/*.c)))
OBJS := $(addprefix ${OUTPUT_DIR}/,$(SRCS:.c=.o))

DEPS := $(OBJS:.o=.d)

LIBNAME := libgim-coms-lib
LIB_TARGET := $(LIBNAME).so.$(GIM_COMS_VER)
LIB_TARGET_MAJOR := $(LIBNAME).so.$(GIM_COMS_MAJOR_VER)
LIB_TARGET_SO := $(LIBNAME).so

INCLUDE := $(addprefix -I,$(GIM_COMS_INCLUDE_DIR))
INCLUDE += $(HOST_INCLUDE_FLAGS)

CFLAGS  = $(DEFAULT_CFLAGS) $(INCLUDE) -fPIC

LIB_FLAGS = -Wl,-soname,$(LIB_TARGET)
LIB_FLAGS += $(HOST_FLAGS)

ifeq ($(BUILD_TYPE), Debug)
  LIB_FLAGS += -fsanitize=address
  CFLAGS += -g -fsanitize=address
else
  CFLAGS += -O2
endif

ALL_DEPS    := $(OUTPUT_DIR)/$(LIB_TARGET)

default: $(ALL_DEPS)

package: default

# Build everything
.PHONY: all
all: default

vpath %.c $(GIM_COMS_SOURCE_DIR)

$(OBJS): Makefile

-include $(DEPS)

$(OUTPUT_DIR)/%.o: %.c | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -DBUILD_TYPE=$(BUILD_TYPE) -MMD -MP -c $< -o $@

$(OUTPUT_DIR)/$(LIB_TARGET): $(OBJS) | $(OUTPUT_DIR)
	$(CC) -shared -o $@ $^ $(LIB_FLAGS)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

.PHONY: clean
clean:
	$(RM) -rf $(OBJS) $(GIM_COMS_BUILD_DIR) $(DEPS)
