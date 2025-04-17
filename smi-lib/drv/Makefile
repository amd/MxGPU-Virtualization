#
# Copyright (c) 2017-2021 Advanced Micro Devices, Inc. All rights reserved.
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

SMI_FILES = smi_drv_handshake.o smi_drv_cmd.o smi_drv_core.o smi_drv_utils.o

SMI_COMPILE_FILES  = $(addprefix smi-lib/drv/core/,$(SMI_FILES))
SMI_COMPILE_FILES += smi-lib/drv/linux/smi_drv.o
SMI_COMPILE_FILES += smi-lib/drv/linux/smi_drv_event.o

SMI_COMPILE_FILES_USER_MODE  = $(addprefix smi-lib/drv/core/,$(SMI_FILES))
SMI_COMPILE_FILES_USER_MODE += smi-lib/drv/linux/smi_drv_user_mode.o
