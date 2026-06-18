# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT

# To ensure not breaking existing Makefile, e.g., gim_user_mode and smi-lib
GIM_COMS_ROOT := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

# (1) Existing Makefile directly includes this Makefile outside gim-coms-lib/
#     Still seeing the contents of previous version's Makefile so unaffected
ifneq ("${CURDIR}","${GIM_COMS_ROOT}")
	GIM_COMS_OBJS = $(GIM_COMS_ROOT)/src/gim_ioctl.o \
			$(GIM_COMS_ROOT)/src/gim_fd_list.o \
			$(GIM_COMS_ROOT)/src/gim_ioctl_msghdr.o
	GIM_COMS_INCLUDE_DIR = $(GIM_COMS_ROOT)/inc
# (2) Updated Makefile will include defines.mk instead and link to the lib
else
	include defines.mk

    default:
		$(MAKE) -f gim-coms-lib.mk

    .PHONY: $(MAKECMDGOALS)
    $(MAKECMDGOALS):
		$(MAKE) -f gim-coms-lib.mk $(MAKECMDGOALS)
endif
