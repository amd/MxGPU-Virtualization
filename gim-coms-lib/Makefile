#
# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
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
