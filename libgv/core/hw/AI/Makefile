#
# Copyright (c) 2017-2023 Advanced Micro Devices, Inc. All rights reserved.
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
# THE SOFTWARE.
LIBGV_HW_AI_PATH = $(LIBGV_CORE_PATH)/hw/AI
LIBGV_HW_MI300_PATH = $(LIBGV_CORE_PATH)/hw/AI/mi300

LIBGV_HW_AI_LOCAL = ai.o
LIBGV_HW_MI300_LOCAL = mi300_gpuiov.o mi300_powerplay.o mi300_fru.o \
        mi300_reset.o mi300_sched.o mi300_vbios.o mi300_vcn.o \
        mi300_reg_init.o mi300_psp.o mi300_misc.o \
        mi300_gpumon.o mi300_mem.o mi300_golden_settings.o mi300_ucode.o \
	mi300_ip_discovery.o mi300_irqmgr.o mmhub_v1_8.o gfxhub_v1_2.o \
	mi300_gfx.o mi300_nbio.o mi300_mailbox.o \
	mi300_mcp.o mi300_xgmi.o mi300_clockgating.o \
	mi300_ecc.o umc_v12_0.o gfx_v9_4_3.o sdma_v4_4_2.o smuio_v13_0_3.o mi300_mca.o
LIBGV_HW_MI308_LOCAL = mi308_ucode.o mi308_mcp.o
LIBGV_HW_MI300_LOCAL += mi300_diag_data.o mi300_diag_data_reg.o

LIBGV_HW_AI_FILES = $(addprefix core/hw/AI/,$(LIBGV_HW_AI_LOCAL))
LIBGV_HW_MI300_FILES = $(addprefix core/hw/AI/mi300/,$(LIBGV_HW_MI300_LOCAL))
LIBGV_HW_MI300_FILES += $(addprefix core/hw/AI/mi300/mi308/,$(LIBGV_HW_MI308_LOCAL))
