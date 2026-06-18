# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT

LIBGV_CORE_PATH = $(LIBGV_PATH)/core

LIBGV_CORE_FILES = amdgv_device.o amdgv_api.o amdgv_gpuiov.o amdgv_irqmgr.o \
	amdgv_mailbox.o amdgv_reset.o amdgv_vfmgr.o amdgv_ws_state.o \
	amdgv_sched_event.o amdgv_sched_reset.o amdgv_sched.o amdgv_sched_switch.o \
	amdgv_vbios.o amdgv_guard.o amdgv_gpumon.o amdgv_gpumon_internal.o \
	amdgv_powerplay_ppatomfwctrl.o amdgv_powerplay_swsmu.o amdgv_misc.o \
	amdgv_notify.o amdgv_memmgr.o amdgv_ecc.o amdgv_error.o amdgv_mes.o \
	amdgv_error_subcode.o amdgv_psp.o amdgv_umc.o amdgv_api_internal.o \
	amdgv_mmsch.o amdgv_gfx.o amdgv_ring.o amdgv_ib.o amdgv_ffbm.o amdgv_mcp.o \
	amdgv_sched_event.o amdgv_gart.o amdgv_xgmi.o amdgv_mca.o amdgv_wb_memory.o \
	amdgv_marketing_name.o amdgv_debug.o amdgv_cper.o amdgv_ras_eeprom.o \
	amdgv_ras_eeprom_internal.o amdgv_live_info.o amdgv_live_migration.o \
	amdgv_dirtybit.o amdgv_diag_data.o amdgv_diag_data_host_drv.o \
	amdgv_diag_data_exclude_list.o amdgv_uni_cmd.o amdgv_sdma.o \
	amdgv_mmhub.o amdgv_gfxhub.o amdgv_nbio.o \
	amdgv_powerplay.o amdgv_sriov_drv.o amdgv_task_barrier.o amdgv_vfmgr_xchg.o amdgv_ual.o

include $(LIBGV_CORE_PATH)/atombios/Makefile
LIBGV_CORE_FILES += $(addprefix atombios/,$(LIBGV_ATOMBIOS_FILES))

include $(LIBGV_CORE_PATH)/hw/common/Makefile
LIBGV_CORE_FILES += $(addprefix hw/common/,$(LIBGV_HW_COMMON_FILES))

include $(LIBGV_CORE_PATH)/hw/AI/Makefile
LIBGV_CORE_FILES += $(addprefix hw/AI/,$(LIBGV_HW_AI_FILES))

include $(LIBGV_CORE_PATH)/hw/navi3/Makefile
LIBGV_CORE_FILES += $(addprefix hw/navi3/,$(LIBGV_HW_NAVI3_FILES))
