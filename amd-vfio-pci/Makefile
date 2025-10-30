#
# Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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

obj-m := amd-vfio-pci.o

amd-vfio-pci-objs := amd-vfio.o

KERNELRELEASE ?= $(shell uname -r)
KERNELDIR ?= /lib/modules/$(KERNELRELEASE)/build

DKMS_PATH ?=  $(realpath $(SRC_PATH)/../dkms)

ccflags-y := -include  $(DKMS_PATH)/config.h
ccflags-y += -I$(SRC_PATH)/../inc
ccflags-y += -Werror -Wmissing-prototypes -Wimplicit-fallthrough=2 -Wno-enum-conversion -Wno-expansion-to-defined

all: dkms/config.h
	$(MAKE) -C $(KERNELDIR) M=$(SRC_PATH) modules

clean:
	cd $(DKMS_PATH); rm -rf autom4te.cache aclocal.m4 config.* configure .*.d
	$(MAKE) -C $(KERNELDIR) M=$(SRC_PATH) clean

dkms/config.h:
	$(DKMS_PATH)/pre-build.sh --with-linux=$(KERNELDIR)

install: all
	$(MAKE) -C $(KERNELDIR) M=$(SRC_PATH) modules_install

.PHONY: all clean install