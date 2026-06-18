# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT

SRC_PATH := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))

KERNELDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(SRC_PATH)/dkms/pre-build.sh --with-linux=$(KERNELDIR)
	$(MAKE) -C $(KERNELDIR) M=$(SRC_PATH) modules

clean:
	cd $(SRC_PATH)/dkms; rm -rf aclocal.m4 configure confdefs.h *.cache build_* config.* .*.d
	$(MAKE) -C $(KERNELDIR) M=$(SRC_PATH) clean
	find $(SRC_PATH)/../ \( -name '.*.cmd' -o -name '*.o' -o -name '*.ko' \) -delete
