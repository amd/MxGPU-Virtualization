# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT

LIBGV_CORE_PATH = $(LIBGV_PATH)/core

LIBGV_CORE_FILES = amdgv_device.o amdgv_api.o amdgv_gpuiov.o amdgv_irqmgr.o \
	amdgv_mailbox.o amdgv_reset.o amdgv_vfmgr.o amdgv_ws_state.o \
	amdgv_sched_event.o amdgv_sched_reset.o amdgv_sched.o amdgv_sched_switch.o \
	amdgv_vbios.o amdgv_guard.o amdgv_gpumon.o amdgv_gpumon_internal.o \
	amdgv_powerplay_ppatomfwctrl.o amdgv_powerplay_swsmu.o amdgv_misc.o \
	amdgv_notify.o amdgv_memmgr.o amdgv_ecc.o amdgv_log.o amdgv_mes.o \
	amdgv_log_subcode.o amdgv_psp.o amdgv_umc.o amdgv_api_internal.o \
	amdgv_mmsch.o amdgv_gfx.o amdgv_ring.o amdgv_ib.o amdgv_ffbm.o amdgv_mcp.o \
	amdgv_sched_event.o amdgv_gart.o amdgv_xgmi.o amdgv_mca.o amdgv_wb_memory.o \
	amdgv_marketing_name.o amdgv_debug.o amdgv_cper.o amdgv_ras_eeprom.o \
	amdgv_ras_eeprom_internal.o amdgv_live_info.o amdgv_live_migration.o \
	amdgv_dirtybit.o amdgv_diag_data.o amdgv_diag_data_host_drv.o \
	amdgv_diag_data_exclude_list.o amdgv_uni_cmd.o amdgv_sdma.o \
	amdgv_ip_discovery.o amdgv_mmhub.o amdgv_gfxhub.o amdgv_nbio.o \
	amdgv_powerplay.o amdgv_iovm_drv.o amdgv_task_barrier.o amdgv_vfmgr_xchg.o amdgv_ual.o

include $(LIBGV_CORE_PATH)/atombios/Makefile
LIBGV_CORE_FILES += $(addprefix atombios/,$(LIBGV_ATOMBIOS_FILES))

include $(LIBGV_CORE_PATH)/hw/common/Makefile
LIBGV_CORE_FILES += $(addprefix hw/common/,$(LIBGV_HW_COMMON_FILES))

include $(LIBGV_CORE_PATH)/hw/AI/Makefile
LIBGV_CORE_FILES += $(addprefix hw/AI/,$(LIBGV_HW_AI_FILES))

include $(LIBGV_CORE_PATH)/hw/navi3/Makefile
LIBGV_CORE_FILES += $(addprefix hw/navi3/,$(LIBGV_HW_NAVI3_FILES))

LIBGV_HW_GC_FILES = gfx_v12_1.o gfx_v12_1_clockgating.o gfx_v12_1_misc.o mes_v12_1.o

ifneq ($(findstring -DAMDGV_UNIRAS_SUPPORT,$(subdir-ccflags-y)),)
AMD_GPU_RAS_MGR := amdgv_ras_mgr
AMD_GPU_RAS_PATH := uniras/ras
AMD_GPU_RAS_FULL_PATH := $(LIBGV_PATH)/../uniras/ras
AMD_GPU_RAS_FILES :=
include $(AMD_GPU_RAS_FULL_PATH)/Makefile
endif

LIBGV_HW_GPUIOV_FILES = gpuiov_v9_0.o gpuiov_v9_0_sched.o gpuiov_v9_0_reset.o
#endif

LIBGV_HW_OSSSYS_FILES = ih_v7_1.o
LIBGV_HW_NBIO_FILES = nbio_v6_3_2.o nbio_v6_3_2_mcp.o nbio_v6_3_2_mailbox.o
LIBGV_HW_MMHUB_FILES = mmhub_v4_2_0.o
LIBGV_HW_GHXHUB_FILES = gfxhub_v12_1_0.o
LIBGV_HW_GMC_FILES = gmc_v12_1.o
LIBGV_HW_SDMA_FILES = sdma_v7_1.o
LIBGV_HW_LSDMA_FILES = lsdma_v7_1.o
LIBGV_HW_PSP_FILES = psp_v15_0_8.o psp_v15_0_8_ras_fw.o
LIBGV_HW_SMU_FILES = smu_v15_0_8.o smu_v15_0_8_gpumon.o smu_v15_0_8_pp.o smu_v15_0_8_eeprom.o smuio_v15_0_8.o
LIBGV_HW_VCN_FILES = vcn_v5_0_2.o

LIBGV_CORE_FILES += $(addprefix hwip/gc/,$(LIBGV_HW_GC_FILES))
LIBGV_CORE_FILES += $(addprefix hwip/osssys/,$(LIBGV_HW_OSSSYS_FILES))
LIBGV_CORE_FILES += $(addprefix hwip/nbio/,$(LIBGV_HW_NBIO_FILES))
LIBGV_CORE_FILES += $(addprefix hwip/gpuiov/,$(LIBGV_HW_GPUIOV_FILES))
LIBGV_CORE_FILES += $(addprefix hwip/mmhub/,$(LIBGV_HW_MMHUB_FILES))
LIBGV_CORE_FILES += $(addprefix hwip/gfxhub/,$(LIBGV_HW_GHXHUB_FILES))
LIBGV_CORE_FILES += $(addprefix hwip/gmc/,$(LIBGV_HW_GMC_FILES))
LIBGV_CORE_FILES += $(addprefix hwip/sdma/,$(LIBGV_HW_SDMA_FILES))
LIBGV_CORE_FILES += $(addprefix hwip/lsdma/,$(LIBGV_HW_LSDMA_FILES))
LIBGV_CORE_FILES += $(addprefix hwip/psp/,$(LIBGV_HW_PSP_FILES))
LIBGV_CORE_FILES += $(addprefix hwip/smu/,$(LIBGV_HW_SMU_FILES))
LIBGV_CORE_FILES += $(addprefix hwip/vcn/,$(LIBGV_HW_VCN_FILES))
