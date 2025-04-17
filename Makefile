#
# Copyright (c) 2014-2019 Advanced Micro Devices, Inc. All rights reserved.
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
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE

SRC_PATH := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))

LIBGV_PATH = $(SRC_PATH)/libgv
GIM_SHIM_PATH = $(SRC_PATH)/gim_shim

SMI_PATH = $(SRC_PATH)/smi-lib/drv/
SMI_SHIM_PATH = $(SRC_PATH)/smi-lib/drv/linux

SMI_LIB_COMMON_PATH=$(SRC_PATH)/smi-lib/inc/common

GIM_COMS_PATH = $(SRC_PATH)/gim-coms-lib
include $(GIM_COMS_PATH)/defines.mk

SYSFS_PATH = $(SRC_PATH)/gim_shim/sysfs

subdir-ccflags-y = -I$(LIBGV_PATH)/inc
subdir-ccflags-y += -include $(SRC_PATH)/dkms/config.h
subdir-ccflags-y += -I$(SMI_PATH)/inc -I$(SMI_SHIM_PATH) -I$(SMI_LIB_COMMON_PATH)
subdir-ccflags-y += -I$(GIM_SHIM_PATH)
subdir-ccflags-y += -I$(SYSFS_PATH)
subdir-ccflags-y += -I$(GIM_COMS_INCLUDE_DIR)

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
subdir-ccflags-y += -I $(KERNELDIR)/include/linux
subdir-ccflags-y += -Werror -Wmissing-prototypes -Wimplicit-fallthrough=2 -Wno-enum-conversion -Wno-expansion-to-defined
subdir-ccflags-y += -Wl,-z,relro,-z,noexecstack,-z,noexecheap -Wl,--strip-debug -Wl,--strip-all

obj-m += gim.o

include $(LIBGV_PATH)/Makefile
gim-objs += $(LIBGV_COMPILE_FILES)

include $(GIM_SHIM_PATH)/Makefile
gim-objs += $(GIM_SHIM_FILES)

include $(SMI_PATH)/Makefile
gim-objs += $(SMI_COMPILE_FILES)

include $(SYSFS_PATH)/Makefile
gim-objs += $(SYSFS_FILES)

all: dkms/config.h
	$(MAKE) -C $(SRC_PATH)/gim-coms-lib
	$(MAKE) -C $(KERNELDIR) M=$(SRC_PATH) modules
	$(MAKE) -C $(SRC_PATH)/smi-lib default

install: dkms/config.h
	$(MAKE) -C $(KERNELDIR) M=$(SRC_PATH) modules_install

dkms/config.h:
	$(SRC_PATH)/dkms/pre-build --with-linux=$(KERNELDIR)

clean:
	cd $(SRC_PATH)/dkms; rm -rf autom4te.cache aclocal.m4 config.* configure .*.d
	$(MAKE) -C $(SRC_PATH)/gim-coms-lib clean
	$(MAKE) -C $(KERNELDIR) M=$(SRC_PATH) clean
	$(MAKE) -C $(SRC_PATH)/smi-lib clean

.PHONY: all install clean
