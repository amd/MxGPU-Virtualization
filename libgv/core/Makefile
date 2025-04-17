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

LIBGV_CORE_PATH = $(LIBGV_PATH)/core

include $(LIBGV_CORE_PATH)/atombios/Makefile

LIBGV_CORE_LOCAL = amdgv_device.o amdgv_api.o \
	amdgv_gpuiov.o amdgv_irqmgr.o amdgv_mailbox.o \
	amdgv_reset.o amdgv_vfmgr.o amdgv_ws_state.o \
	amdgv_sched_event.o amdgv_sched_reset.o amdgv_sched.o \
	amdgv_sched_switch.o amdgv_vbios.o amdgv_guard.o \
	amdgv_gpumon.o amdgv_gpumon_internal.o amdgv_powerplay_ppatomfwctrl.o amdgv_powerplay_swsmu.o \
	amdgv_misc.o amdgv_notify.o amdgv_memmgr.o amdgv_ecc.o \
	amdgv_error.o amdgv_error_subcode.o \
	amdgv_psp.o amdgv_umc.o \
	amdgv_api_internal.o amdgv_mmsch.o amdgv_gfx.o amdgv_ring.o \
	amdgv_ib.o amdgv_ffbm.o amdgv_mcp.o amdgv_sched_event.o amdgv_gart.o \
	amdgv_xgmi.o amdgv_mca.o amdgv_wb_memory.o \
	amdgv_marketing_name.o amdgv_debug.o amdgv_cper.o

# Support for page retire feature
LIBGV_CORE_LOCAL += amdgv_ras_eeprom.o amdgv_ras_eeprom_internal.o

LIBGV_CORE_LOCAL += amdgv_live_info.o

LIBGV_CORE_LOCAL += amdgv_live_migration.o amdgv_dirtybit.o

LIBGV_CORE_LOCAL += amdgv_diag_data.o amdgv_diag_data_host_drv.o
LIBGV_CORE_LOCAL += amdgv_diag_data_exclude_list.o

LIBGV_CORE_FILES = $(addprefix core/,$(LIBGV_CORE_LOCAL))

LIBGV_FILES = $(LIBGV_CORE_FILES) $(LIBGV_ATOMBIOS_FILES)

include $(LIBGV_CORE_PATH)/hw/AI/Makefile
include $(LIBGV_CORE_PATH)/hw/common/Makefile
LIBGV_FILES += $(LIBGV_HW_AI_FILES)
LIBGV_FILES += $(LIBGV_HW_MI300_FILES)
LIBGV_FILES += $(LIBGV_HW_COMMON_FILES)
