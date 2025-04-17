/*
 * Copyright (c) 2019-2023 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "amdgv_device.h"
#include "amdgv_error.h"
#include "amdgv_error_internal.h"

#define error_category(comp)                                                                  \
	static const struct error_text                                                        \
		AMDGV_ERROR_##comp##_LIST[AMDGV_ERROR_SUBCODE(AMDGV_ERROR_##comp##_MAX + 1)]

#define add_entry(subcode, arg, level, txt)                                                         \
	{                                                                                               \
		subcode, #subcode, arg, level, AMDGV_ERROR_PRINT_HEADER txt "\n", 0                                   \
	}

#define add_entry_ext(subcode, arg, level, txt)                                                     \
	{                                                                                     	        \
		subcode, #subcode, arg, level, AMDGV_ERROR_PRINT_HEADER txt "\n", AMDGV_GPU_ERROR_FLAG_ERROR_EXT 	    \
	}

#define add_category(comp)                                                                    \
	{                                                                                     \
		AMDGV_ERROR_CATEGORY_##comp, AMDGV_ERROR_##comp##_LIST,                       \
			AMDGV_ERROR_SUBCODE(AMDGV_ERROR_##comp##_MAX + 1)                     \
	}

#define category_not_used()                                                                   \
	{                                                                                     \
		AMDGV_ERROR_CATEGORY_NON_USED, 0, 0                                           \
	}

/*
 * Error subcode entry and description
 *
 * Format:
 * error_category(component) = {
 *     add_entry(subcode, arg_type, description),
 * };
 *
 */
error_category(DRIVER) = {
	add_entry(AMDGV_ERROR_DRIVER_SPIN_LOCK_BUSY, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED,
		  "Spin lock is busy, didn't free after timeout. Still waiting!"),
	add_entry(AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Allocate system memory failed (length: %llu)!"),
	add_entry(AMDGV_ERROR_DRIVER_CREATE_GFX_WORKQUEUE_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Create GFX workqueue failed."),
	add_entry(AMDGV_ERROR_DRIVER_CREATE_MM_WORKQUEUE_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Create MM workqueue failed."),
	add_entry(AMDGV_ERROR_DRIVER_BUFFER_OVERFLOW, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_MED,
		  "Buffer overflowed (number of entry dropped: %llu)."),

	add_entry(AMDGV_ERROR_DRIVER_DEV_INIT_FAIL, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_HIGH, "Failed to init dev(0x%llx)."),
	add_entry(AMDGV_ERROR_DRIVER_CREATE_THREAD_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Cannot create thread."),
	add_entry(AMDGV_ERROR_DRIVER_NO_ACCESS_PCI_REGION, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_HIGH, "Cannot access PCI region."),
	add_entry(AMDGV_ERROR_DRIVER_MMIO_FAIL, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_ERROR_MED,
		  "Cannot map MMIO BAR."),
	add_entry(AMDGV_ERROR_DRIVER_INTERRUPT_INIT_FAIL, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_HIGH, "Failed to enable interrupt vector(%llu)."),

	add_entry(AMDGV_ERROR_DRIVER_INVALID_VALUE, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Invalid value(%lld)."),
	add_entry(AMDGV_ERROR_DRIVER_CREATE_MUTEX_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Cannot create mutex."),
	add_entry(AMDGV_ERROR_DRIVER_CREATE_TIMER_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Cannot create timer."),
	add_entry(AMDGV_ERROR_DRIVER_CREATE_EVENT_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Cannot create synchronization event."),
	add_entry(AMDGV_ERROR_DRIVER_CREATE_SPIN_LOCK_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Cannot create spin lock."),

	add_entry(AMDGV_ERROR_DRIVER_ALLOC_FB_MEM_FAIL, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Allocate FB memory failed (length: %llu)!"),
	add_entry(AMDGV_ERROR_DRIVER_ALLOC_DMA_MEM_FAIL, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Allocate DMA memory failed (length: %llu)!"),
	add_entry(AMDGV_ERROR_DRIVER_NO_FB_MANAGER, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "FB memory manager is not allocated."),
	add_entry(AMDGV_ERROR_DRIVER_HW_INIT_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_HIGH, "Failed to HW init."),
	add_entry(AMDGV_ERROR_DRIVER_SW_INIT_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Failed to SW init."),

	add_entry(AMDGV_ERROR_DRIVER_INIT_CONFIG_ERROR, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Failed to parse init config."),
	add_entry(AMDGV_ERROR_DRIVER_ERROR_LOGGING_FAILED, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Failed to start error logging."),
	add_entry(AMDGV_ERROR_DRIVER_CREATE_RWLOCK_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Cannot create read/write spinlock."),
	add_entry(AMDGV_ERROR_DRIVER_CREATE_RWSEMA_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Cannot create read/write semaphore."),
	add_entry(AMDGV_ERROR_DRIVER_GET_READ_LOCK_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Failed to acquire read spinlock."),

	add_entry(AMDGV_ERROR_DRIVER_GET_WRITE_LOCK_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Failed to acquire write spinlock."),
	add_entry(AMDGV_ERROR_DRIVER_GET_READ_SEMA_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Failed to acquire read semaphore."),
	add_entry(AMDGV_ERROR_DRIVER_GET_WRITE_SEMA_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Failed to acquire write semaphore."),

	add_entry(AMDGV_ERROR_DRIVER_DIAG_DATA_INIT_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Failed to initialize diagnosis data."),
	add_entry(AMDGV_ERROR_DRIVER_DIAG_DATA_MEM_REQ_FAIL, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Failed to request memory for diagnosis data (length: %llu)."),
	add_entry(AMDGV_ERROR_DRIVER_DIAG_DATA_VADDR_REQ_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Failed to get the vaddr for memory block."),
	add_entry(AMDGV_ERROR_DRIVER_DIAG_DATA_BUS_ADDR_REQ_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Failed to get the bus addr for memory block"),

	add_entry(AMDGV_ERROR_DRIVER_HRTIMER_START_FAIL, ERROR_DATA_ARG_NONE,
		AMDGV_ERROR_SEVERITY_ERROR_MED, "Failed to start kernel hrtimer"),
	add_entry(AMDGV_ERROR_DRIVER_CREATE_DRIVER_FILE_FAIL, ERROR_DATA_ARG_NONE,
		AMDGV_ERROR_SEVERITY_ERROR_MED, "Failed to create driver file"),
	add_entry(AMDGV_ERROR_DRIVER_CREATE_DEVICE_FILE_FAIL, ERROR_DATA_ARG_NONE,
		AMDGV_ERROR_SEVERITY_ERROR_MED, "Failed to create device file"),
	add_entry(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, ERROR_DATA_ARG_NONE,
		AMDGV_ERROR_SEVERITY_ERROR_MED, "Failed to create debugfs file"),
	add_entry(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_DIR_FAIL, ERROR_DATA_ARG_NONE,
		AMDGV_ERROR_SEVERITY_ERROR_MED, "Failed to create debugfs dir"),

	add_entry(AMDGV_ERROR_DRIVER_PCI_ENABLE_DEVICE_FAIL, ERROR_DATA_ARG_64,
		AMDGV_ERROR_SEVERITY_ERROR_MED, "Failed to enable pci device (0x%llx)"),
	add_entry(AMDGV_ERROR_DRIVER_FB_MAP_FAIL, ERROR_DATA_ARG_NONE,
		AMDGV_ERROR_SEVERITY_ERROR_HIGH, "Failed to map FB"),
	add_entry(AMDGV_ERROR_DRIVER_DOORBELL_MAP_FAIL, ERROR_DATA_ARG_NONE,
		AMDGV_ERROR_SEVERITY_ERROR_HIGH, "Failed to map DOORBELL"),
	add_entry(AMDGV_ERROR_DRIVER_PCI_REGISTER_DRIVER_FAIL, ERROR_DATA_ARG_NONE,
		AMDGV_ERROR_SEVERITY_ERROR_HIGH, "Failed to register pci driver."),
	add_entry(AMDGV_ERROR_DRIVER_VF_RESIZE_BAR_FAIL, ERROR_DATA_ARG_NONE,
		AMDGV_ERROR_SEVERITY_ERROR_HIGH, "Failed to resize VF BAR."),

	add_entry(AMDGV_ERROR_DRIVER_ALLOC_IOVA_ALIGN_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_WARNING, "Allocate iova method memory failed."),

	add_entry(AMDGV_ERROR_DRIVER_ROM_MAP_FAIL, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_ERROR_MED,
		  "Cannot map ROM BAR."),

	add_entry_ext(AMDGV_ERROR_DRIVER_FULL_ACCESS_TIMEOUT, ERROR_DATA_ARG_THREE_64_EXT,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "VF %llu full access timeout. |start time: %llu| - |end time: %llu|"),

	/* this one is the MAX */
	add_entry(AMDGV_ERROR_DRIVER_MAX, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_INFO,
		  "This is error log collect test for DRIVER component (test count %llu)."),
};

error_category(FW) = {
	add_entry(AMDGV_ERROR_FW_CMD_ALLOC_BUF_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "PSP command alloc buffer failed!"),
	add_entry(AMDGV_ERROR_FW_CMD_BUF_PREP_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "PSP command buffer prepare failed!"),
	add_entry(AMDGV_ERROR_FW_RING_INIT_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Failed to initialize PSP ring."),
	add_entry(AMDGV_ERROR_FW_UCODE_APPLY_SECURITY_POLICY_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Failed to apply PSP security policy."),
	add_entry(AMDGV_ERROR_FW_START_RING_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Failed to start PSP ring."),

	add_entry(AMDGV_ERROR_FW_UCODE_LOAD_FAIL, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_ERROR_MED,
		  "Failed to load FW %llu"),
	add_entry(AMDGV_ERROR_FW_EXIT_FAIL, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_ERROR_MED,
		  "Failed to cleanup PSP."),
	add_entry(AMDGV_ERROR_FW_INIT_FAIL, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_ERROR_MED,
		  "Failed to init PSP."),
	add_entry(AMDGV_ERROR_FW_CMD_SUBMIT_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "PSP command submit failed."),
	add_entry(AMDGV_ERROR_FW_CMD_FENCE_WAIT_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "PSP command fence wait failed."),

	add_entry(AMDGV_ERROR_FW_TMR_LOAD_FAIL, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_ERROR_MED,
		  "Failed to load TMR."),
	add_entry(AMDGV_ERROR_FW_TOC_LOAD_FAIL, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_ERROR_MED,
		  "Failed to load TOC."),
	add_entry(AMDGV_ERROR_FW_RAS_LOAD_FAIL, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_ERROR_MED,
		  "Failed to load RAS."),
	add_entry(AMDGV_ERROR_FW_RAS_UNLOAD_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Failed to unload RAS."),
	add_entry(AMDGV_ERROR_FW_RAS_TA_INVOKE_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "Failed to invoke RAS TA."),
	add_entry(AMDGV_ERROR_FW_RAS_TA_ERR_INJECT_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "Failed to error inject in RAS TA."),
	add_entry(AMDGV_ERROR_FW_RAS_TA_ENABLE_RAS_FEATURE_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "Failed to enable ras feature in RAS TA."),

	add_entry(AMDGV_ERROR_FW_ASD_LOAD_FAIL, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_ERROR_MED,
		  "Failed to load ASD."),
	add_entry(AMDGV_ERROR_FW_ASD_UNLOAD_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Failed to unload ASD."),
	add_entry(AMDGV_ERROR_FW_AUTOLOAD_FAIL, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_ERROR_MED,
		  "Failed to start autoload."),
	add_entry(AMDGV_ERROR_FW_VFGATE_FAIL, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_ERROR_MED,
		  "Failed to execute VF gate command."),
	add_entry(AMDGV_ERROR_FW_XGMI_LOAD_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Failed to load XGMI."),

	add_entry(AMDGV_ERROR_FW_XGMI_UNLOAD_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Failed to unload XGMI."),
	add_entry(AMDGV_ERROR_FW_XGMI_TA_INVOKE_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Failed to invoke XGMI TA."),

	add_entry(AMDGV_ERROR_FW_TMR_INIT_FAIL, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_ERROR_MED,
		  "Failed to init TMR."),
	add_entry(AMDGV_ERROR_FW_NOT_SUPPORTED_FEATURE, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "The feature is not supported."),
	add_entry(AMDGV_ERROR_FW_GET_PSP_TRACELOG_FAIL, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "Failed to Get PSP Trace Log, status=0x%llx."),

	add_entry(AMDGV_ERROR_FW_SET_SNAPSHOT_ADDR_FAIL, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "Failed to set Snapshot address, status=0x%llx."),
	add_entry(AMDGV_ERROR_FW_SNAPSHOT_TRIGGER_FAIL, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "Failed to trigger Snapshot, status=0x%llx."),

	add_entry(AMDGV_ERROR_FW_MIGRATION_GET_PSP_INFO_FAIL, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "Failed to get migration version, status=0x%llx."),
	add_entry(AMDGV_ERROR_FW_MIGRATION_EXPORT_FAIL, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "Failed migration export, status=0x%llx."),
	add_entry(AMDGV_ERROR_FW_MIGRATION_IMPORT_FAIL, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "Failed migration import, status=0x%llx."),

	add_entry(AMDGV_ERROR_FW_BL_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_HIGH, "Timeout waiting for PSP Bootloader to finish GPU boot process"),

	add_entry(AMDGV_ERROR_FW_RAS_BOOT_FAIL, ERROR_DATA_ARG_16_16_32,
			AMDGV_ERROR_SEVERITY_ERROR_HIGH, "Firmware boot failed, socket: %u, aid: %u, fw status is 0x%x"),

	add_entry(AMDGV_ERROR_FW_MAILBOX_ERROR, ERROR_DATA_ARG_16_16_32,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "VF %u PSP mailbox failure. status: %u amdgv_fw_id: %u."),

	add_entry(AMDGV_ERROR_FW_RAS_MEM_TRAIN_FAIL, ERROR_DATA_ARG_16_16_32,
		  AMDGV_ERROR_SEVERITY_ERROR_HIGH, "socket: %u, aid: %u, hbm: %d, memory training failed."),

	add_entry(AMDGV_ERROR_FW_RAS_FW_LOAD_FAIL, ERROR_DATA_ARG_32_32,
		  AMDGV_ERROR_SEVERITY_ERROR_HIGH, "socket: %d, aid: %d, firmware load failed at boot time"),

	add_entry(AMDGV_ERROR_FW_RAS_WAFL_LINK_TRAIN_FAIL, ERROR_DATA_ARG_32_32,
		  AMDGV_ERROR_SEVERITY_ERROR_HIGH, "socket: %d, aid: %d, wafl link training failed"),

	add_entry(AMDGV_ERROR_FW_RAS_XGMI_LINK_TRAIN_FAIL, ERROR_DATA_ARG_32_32,
		  AMDGV_ERROR_SEVERITY_ERROR_HIGH, "socket: %d, aid: %d, xgmi link training failed"),

	add_entry(AMDGV_ERROR_FW_RAS_USR_CP_LINK_TRAIN_FAIL, ERROR_DATA_ARG_32_32,
		  AMDGV_ERROR_SEVERITY_ERROR_HIGH, "socket: %d, aid: %d, usr cp link training failed"),

	add_entry(AMDGV_ERROR_FW_RAS_USR_DP_LINK_TRAIN_FAIL, ERROR_DATA_ARG_32_32,
		  AMDGV_ERROR_SEVERITY_ERROR_HIGH, "socket: %d, aid: %d, usr dp link training failed"),

	add_entry(AMDGV_ERROR_FW_RAS_HBM_MEM_TEST_FAIL, ERROR_DATA_ARG_16_16_32,
		  AMDGV_ERROR_SEVERITY_ERROR_HIGH, "socket: %u, aid: %u, hbm: %d, hbm memory test failed."),

	add_entry(AMDGV_ERROR_FW_RAS_HBM_BIST_TEST_FAIL, ERROR_DATA_ARG_16_16_32,
		  AMDGV_ERROR_SEVERITY_ERROR_HIGH, "socket: %u, aid: %u, hbm: %d, hbm bist test failed."),

	add_entry(AMDGV_ERROR_FW_RAS_BOOT_REG_READ_FAIL, ERROR_DATA_ARG_32_32,
			AMDGV_ERROR_SEVERITY_ERROR_HIGH, "Firmware boot failed, MP0_SMN_C2PMSG_126 read fail, boot error: 0x%x, fw status: 0x%x"),

	/* this one is the MAX */
	add_entry(AMDGV_ERROR_FW_MAX, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_INFO,
		  "This is error log collect test for FW component (test count %llu)."),
};

error_category(RESET) = {
	add_entry(AMDGV_ERROR_RESET_GPU, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_ERROR_LOW,
		  "Issuing Whole GPU reset."),
	add_entry(AMDGV_ERROR_RESET_GPU_FAILED, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_ERROR_HIGH,
		  "Whole GPU reset failed!"),
	add_entry(AMDGV_ERROR_RESET_FLR, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_ERROR_LOW,
		  "Issuing FLR on vf: %u."),
	add_entry(AMDGV_ERROR_RESET_FLR_FAILED, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_ERROR_MED,
		  "FLR failed on vf : %u!"),
	add_entry(AMDGV_ERROR_RESET_GPU_HIVE_FAILED, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_ERROR_HIGH,
			"Unable to perform Whole GPU reset because one or more connected devices are in RMA state."),

	/* this one is the MAX */
	add_entry(AMDGV_ERROR_RESET_MAX, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_INFO,
		  "This is error log collect test for RESET component (test count %llu)."),
};

error_category(IOV) = {
	add_entry(AMDGV_ERROR_IOV_NO_GPU_IOV_CAP, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_HIGH, "No GPU_IOV caps found."),
	add_entry(AMDGV_ERROR_IOV_ASIC_NO_SRIOV_SUPPORT, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_HIGH, "ASIC has no SRIOV support."),
	add_entry(AMDGV_ERROR_IOV_ENABLE_SRIOV_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_HIGH, "Failed to properly enable SRIOV."),
	add_entry(AMDGV_ERROR_IOV_CMD_TIMEOUT, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_ERROR_MED,
		  "Timeout of waiting command complete (%llu us)."),
	add_entry(AMDGV_ERROR_IOV_CMD_ERROR, ERROR_DATA_ARG_32_32, AMDGV_ERROR_SEVERITY_ERROR_MED,
		  "Command Error (cmd: 0x%X, status: 0x%X)."),

	add_entry(AMDGV_ERROR_IOV_INIT_IV_RING_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Init IV ring failed."),
	add_entry(AMDGV_ERROR_IOV_SRIOV_STRIDE_ERROR, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_HIGH, "Only SRIOV stride = 1 is supported."),
	add_entry(AMDGV_ERROR_IOV_WS_SAVE_TIMEOUT, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "SRIOV SAVE failed!"),
	add_entry(AMDGV_ERROR_IOV_WS_IDLE_TIMEOUT, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "SRIOV IDLE failed!"),
	add_entry(AMDGV_ERROR_IOV_WS_RUN_TIMEOUT, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "SRIOV RUN failed!"),
	add_entry(AMDGV_ERROR_IOV_WS_LOAD_TIMEOUT, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "SRIOV LOAD failed!"),
	add_entry(AMDGV_ERROR_IOV_WS_SHUTDOWN_TIMEOUT, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "SRIOV SHUTDOWN failed!"),
	add_entry(AMDGV_ERROR_IOV_WS_ALREADY_SHUTDOWN, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED,
		  "VF is already in SHUTDOWN state, cannot shutdown again."),
	add_entry(AMDGV_ERROR_IOV_WS_INFINITE_LOOP, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW,
		  "Potential Infinite loop detected in WS State Machine."),
	add_entry(AMDGV_ERROR_IOV_WS_REENTRANT_ERROR, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW,
		  "WS State Machine being called from different threads at the same time."),

	/* this one is the MAX */
	add_entry(AMDGV_ERROR_IOV_MAX, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_INFO,
		  "This is error log collect test for IOV component (test count %llu)."),
};

error_category(ECC) = {
	add_entry(AMDGV_ERROR_ECC_UCE, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_ERROR_MED,
		  "Adapter(UMC) occurred %llu uncorrectable ECC error since GPU load."),
	add_entry(AMDGV_ERROR_ECC_CE, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_ERROR_LOW,
		  "Adapter(UMC) occurred %llu correctable ECC error since GPU load."),
	add_entry(
		AMDGV_ERROR_ECC_IN_PF_FB, ERROR_DATA_ARG_32_32, AMDGV_ERROR_SEVERITY_ERROR_HIGH,
		"%u time out of %u threshold - ECC happens within PF critical region, please follow AMD Service Action Guide."),
	add_entry(
		AMDGV_ERROR_ECC_IN_CRI_REG, ERROR_DATA_ARG_32_32, AMDGV_ERROR_SEVERITY_ERROR_HIGH,
		"%u time out of %u threshold - ECC happens within Critical FB region, either TMR or CSA or IPD, please follow AMD Service Action Guide."),
	add_entry(
		AMDGV_ERROR_ECC_IN_VF_CRI, ERROR_DATA_ARG_32_32, AMDGV_ERROR_SEVERITY_ERROR_HIGH,
		"%u time out of %u threshold - ECC happens within VF critical region, please follow AMD Service Action Guide."),
	add_entry(AMDGV_ERROR_ECC_REACH_THD, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_ERROR_HIGH,
		  "ECC record reach threshold %llu pages, please follow AMD Service Action Guide."),
	add_entry(AMDGV_ERROR_ECC_VF_CE, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_ERROR_LOW,
		  "%llu correctable hardware error detected in UMC block."),
	add_entry(
		AMDGV_ERROR_ECC_VF_UE, ERROR_DATA_ARG_32_32, AMDGV_ERROR_SEVERITY_ERROR_MED,
		"%u time out of %u threshold - uncorrectable hardware error detected in UMC block."),
	add_entry(
		AMDGV_ERROR_ECC_IN_SAME_ROW, ERROR_DATA_ARG_32_32, AMDGV_ERROR_SEVERITY_ERROR_HIGH,
		"%u time out of %u threshold - ECC happens within same memory row, please follow AMD Service Action Guide."),

	add_entry(AMDGV_ERROR_ECC_UMC_UE, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_ERROR_MED,
		"%llu uncorrectable hardware error detected in UMC block."),
	add_entry(
		AMDGV_ERROR_ECC_GFX_CE, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_ERROR_LOW,
		"%llu correctable hardware error detected in GFX block."),
	add_entry(
		AMDGV_ERROR_ECC_GFX_UE, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_ERROR_MED,
		"%llu uncorrectable hardware error detected in GFX block."),
	add_entry(
		AMDGV_ERROR_ECC_SDMA_CE, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_ERROR_LOW,
		"%llu correctable hardware error detected in SDMA block."),
	add_entry(
		AMDGV_ERROR_ECC_SDMA_UE, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_ERROR_MED,
		"%llu uncorrectable hardware error detected in SDMA block."),

	add_entry(AMDGV_ERROR_ECC_GFX_CE_TOTAL, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_INFO,
		  "Adapter(GFX) occurred %llu correctable ECC error since GPU load."),
	add_entry(AMDGV_ERROR_ECC_GFX_UE_TOTAL, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_INFO,
		  "Adapter(GFX) occurred %llu uncorrectable ECC error since GPU load."),
	add_entry(AMDGV_ERROR_ECC_SDMA_CE_TOTAL, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_INFO,
		  "Adapter(SDMA) occurred %llu correctable ECC error since GPU load."),
	add_entry(AMDGV_ERROR_ECC_SDMA_UE_TOTAL, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_INFO,
		  "Adapter(SDMA) occurred %llu uncorrectable ECC error since GPU load."),
	add_entry(AMDGV_ERROR_ECC_UMC_CE_TOTAL, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_INFO,
		  "Adapter(UMC) occurred %llu correctable ECC error since GPU load."),
	add_entry(AMDGV_ERROR_ECC_UMC_UE_TOTAL, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_INFO,
		  "Adapter(UMC) occurred %llu uncorrectable ECC error since GPU load."),
	add_entry(
		AMDGV_ERROR_ECC_MMHUB_CE, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_WARNING,
		"%llu correctable hardware error detected in MMHUB block."),
	add_entry(
		AMDGV_ERROR_ECC_MMHUB_UE, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_ERROR_MED,
		"%llu uncorrectable hardware error detected in MMHUB block."),
	add_entry(AMDGV_ERROR_ECC_MMHUB_CE_TOTAL, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_INFO,
		  "Adapter(MMHUB) occurred %llu correctable ECC error since GPU load."),
	add_entry(AMDGV_ERROR_ECC_MMHUB_UE_TOTAL, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_INFO,
		  "Adapter(MMHUB) occurred %llu uncorrectable ECC error since GPU load."),

	add_entry(
		AMDGV_ERROR_ECC_XGMI_WAFL_CE, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_WARNING,
		"%llu correctable hardware error detected in XGMI block."),
	add_entry(
		AMDGV_ERROR_ECC_XGMI_WAFL_UE, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_ERROR_MED,
		"%llu uncorrectable hardware error detected in XGMI block."),
	add_entry(AMDGV_ERROR_ECC_XGMI_WAFL_CE_TOTAL, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_INFO,
		  "Adapter(XGMI) occurred %llu correctable ECC error since GPU load."),
	add_entry(AMDGV_ERROR_ECC_XGMI_WAFL_UE_TOTAL, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_INFO,
		  "Adapter(XGMI) occurred %llu uncorrectable ECC error since GPU load."),

	add_entry(AMDGV_ERROR_ECC_FATAL_ERROR, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_ERROR_MED,
		"GPU detected ECC Fatal Error."),
	add_entry(AMDGV_ERROR_ECC_POISON_CONSUMPTION, ERROR_DATA_ARG_32_32, AMDGV_ERROR_SEVERITY_ERROR_LOW,
		"RAS poison consumption triggered by VF%u, RAS block:%u."),

	add_entry_ext(AMDGV_ERROR_ECC_ACA_DUMP, ERROR_DATA_ARG_FIVE_64_EXT, AMDGV_ERROR_SEVERITY_INFO,
		  "Acceleration Check Architecture event logged:\nSTATUS=0x%016llx\nADDR=0x%016llx\nMISC0=0x%016llx\nIPID=0x%016llx\nSYND=0x%016llx"),
	add_entry(AMDGV_ERROR_ECC_WRONG_SOCKET_ID, ERROR_DATA_ARG_32_32, AMDGV_ERROR_SEVERITY_ERROR_LOW,
		"The socket id reported from ACA is different from the socket id detected by driver. ACA:%u, driver:%u"),
	add_entry(AMDGV_ERROR_ECC_ACA_UNKNOWN_BLOCK_INSTANCE, ERROR_DATA_ARG_16_16_32, AMDGV_ERROR_SEVERITY_ERROR_LOW,
		"Maximum instance from ACA ras block %u exceeds the driver maximum. ACA:%u, driver:%u"),

	add_entry(AMDGV_ERROR_ECC_UNKNOWN_CHIPLET_CE, ERROR_DATA_ARG_16_16_16_16, AMDGV_ERROR_SEVERITY_WARNING,
		"socket: %u, die: %u, %u new correctable hardware errors detected in UNKNOWN Block. %u total UNKNOWN Block correctable ECC errors since GPU load."),
	add_entry(AMDGV_ERROR_ECC_UNKNOWN_CHIPLET_UE, ERROR_DATA_ARG_16_16_16_16, AMDGV_ERROR_SEVERITY_ERROR_MED,
		"socket: %u, die: %u, %u new uncorrectable hardware errors detected in UNKNOWN Block. %u total UNKNOWN Block uncorrectable ECC errors since GPU load."),
	add_entry(AMDGV_ERROR_ECC_UMC_CHIPLET_CE, ERROR_DATA_ARG_16_16_16_16, AMDGV_ERROR_SEVERITY_WARNING,
		"socket: %u, die: %u, %u new correctable hardware errors detected in UMC Block. %u total UMC Block correctable ECC errors since GPU load."),
	add_entry(AMDGV_ERROR_ECC_UMC_CHIPLET_UE, ERROR_DATA_ARG_16_16_16_16, AMDGV_ERROR_SEVERITY_ERROR_MED,
		"socket: %u, die: %u, %u new uncorrectable hardware errors detected in UMC Block. %u total UMC Block uncorrectable ECC errors since GPU load."),
	add_entry(AMDGV_ERROR_ECC_GFX_CHIPLET_CE, ERROR_DATA_ARG_16_16_16_16, AMDGV_ERROR_SEVERITY_WARNING,
		"socket: %u, die: %u, %u new correctable hardware errors detected in GFX Block. %u total GFX Block correctable ECC errors since GPU load."),
	add_entry(AMDGV_ERROR_ECC_GFX_CHIPLET_UE, ERROR_DATA_ARG_16_16_16_16, AMDGV_ERROR_SEVERITY_ERROR_MED,
		"socket: %u, die: %u, %u new uncorrectable hardware errors detected in GFX Block. %u total GFX Block uncorrectable ECC errors since GPU load."),
	add_entry(AMDGV_ERROR_ECC_SDMA_CHIPLET_CE, ERROR_DATA_ARG_16_16_16_16, AMDGV_ERROR_SEVERITY_WARNING,
		"socket: %u, die: %u, %u new correctable hardware errors detected in SDMA Block. %u total SDMA Block correctable ECC errors since GPU load."),
	add_entry(AMDGV_ERROR_ECC_SDMA_CHIPLET_UE, ERROR_DATA_ARG_16_16_16_16, AMDGV_ERROR_SEVERITY_ERROR_MED,
		"socket: %u, die: %u, %u new uncorrectable hardware errors detected in SDMA Block. %u total SDMA Block uncorrectable ECC errors since GPU load."),
	add_entry(AMDGV_ERROR_ECC_MMHUB_CHIPLET_CE, ERROR_DATA_ARG_16_16_16_16, AMDGV_ERROR_SEVERITY_WARNING,
		"socket: %u, die: %u, %u new correctable hardware errors detected in MMHUB Block. %u total MMHUB Block correctable ECC errors since GPU load."),
	add_entry(AMDGV_ERROR_ECC_MMHUB_CHIPLET_UE, ERROR_DATA_ARG_16_16_16_16, AMDGV_ERROR_SEVERITY_ERROR_MED,
		"socket: %u, die: %u, %u new uncorrectable hardware errors detected in MMHUB Block. %u total MMHUB Block uncorrectable ECC errors since GPU load."),
	add_entry(AMDGV_ERROR_ECC_XGMI_WAFL_CHIPLET_CE, ERROR_DATA_ARG_16_16_16_16, AMDGV_ERROR_SEVERITY_WARNING,
		"socket: %u, die: %u, %u new correctable hardware errors detected in XGMI Block. %u total XGMI Block correctable ECC errors since GPU load."),
	add_entry(AMDGV_ERROR_ECC_XGMI_WAFL_CHIPLET_UE, ERROR_DATA_ARG_16_16_16_16, AMDGV_ERROR_SEVERITY_ERROR_MED,
		"socket: %u, die: %u, %u new uncorrectable hardware errors detected in XGMI Block. %u total XGMI Block uncorrectable ECC errors since GPU load."),
	add_entry(AMDGV_ERROR_ECC_BAD_PAGE_ENTRIES_FOUND, ERROR_DATA_ARG_32_32, AMDGV_ERROR_SEVERITY_INFO,
		"driver RAS bad page storage has %u bad page records of %u threshold."),

	add_entry(AMDGV_ERROR_ECC_UMC_DE, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_WARNING,
		"%llu deferred hardware error detected in UMC block."),
	add_entry(AMDGV_ERROR_ECC_UMC_DE_TOTAL, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_INFO,
		  "Adapter(UMC) occurred %llu deferred ECC error since GPU load."),

	add_entry(AMDGV_ERROR_ECC_UNKNOWN, ERROR_DATA_ARG_16_16_32, AMDGV_ERROR_SEVERITY_ERROR_LOW,
		"socket: %u, %u new hardware errors detected in UNKNOWN Block. %u total UNKNOWN Block ECC errors since GPU load."),

	add_entry(AMDGV_ERROR_ECC_EEPROM_REACH_THD, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_ERROR_HIGH,
		  "RAS EEPROM cannot hold anymore bad page records, please RMA this GPU."),

	add_entry(AMDGV_ERROR_ECC_UMC_CHIPLET_DE, ERROR_DATA_ARG_16_16_16_16, AMDGV_ERROR_SEVERITY_WARNING,
		"socket: %u, die: %u, %u new deferred hardware errors detected in UMC Block. %u total UMC Block deferred ECC errors since GPU load."),

	add_entry(AMDGV_ERROR_ECC_UNKNOWN_CHIPLET_DE, ERROR_DATA_ARG_16_16_16_16, AMDGV_ERROR_SEVERITY_ERROR_LOW,
		"socket: %u, die: %u, %u new deferred hardware errors detected in UNKNOWN Block. %u total UNKNOWN Block deferred ECC errors since GPU load."),

	add_entry(AMDGV_ERROR_ECC_EEPROM_CHK_MISMATCH, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_ERROR_LOW,
		"RAS EEPROM Table checksum mismatch detected."),
	add_entry(AMDGV_ERROR_ECC_EEPROM_RESET, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_WARNING,
		"RAS EEPROM Table is reset."),
	add_entry(AMDGV_ERROR_ECC_EEPROM_RESET_FAILED, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_ERROR_LOW,
		"RAS EEPROM Table reset failed."),

	add_entry(AMDGV_ERROR_ECC_EEPROM_APPEND, ERROR_DATA_ARG_32_32, AMDGV_ERROR_SEVERITY_INFO,
		"%u EEPROM records were appended to RAS EEPROM table. There are %u records now."),
	add_entry(AMDGV_ERROR_ECC_THD_CHANGED, ERROR_DATA_ARG_32_32, AMDGV_ERROR_SEVERITY_INFO,
		"RAS EEPROM table bad page threshold has changed. Old threshold = %u. New threshold %u."),
	add_entry(AMDGV_ERROR_ECC_DUP_ENTRIES, ERROR_DATA_ARG_32_32, AMDGV_ERROR_SEVERITY_INFO,
		"Duplicate bad page records detected. Old count = %d, new count = %d. Overwriting RAS EEPROM table."),
	add_entry(AMDGV_ERROR_ECC_EEPROM_WRONG_HDR, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_INFO,
		"Uninitialized RAS EEPROM table detected."),
	add_entry(AMDGV_ERROR_ECC_EEPROM_WRONG_VER, ERROR_DATA_ARG_32_32, AMDGV_ERROR_SEVERITY_ERROR_LOW,
		"Wrong RAS EEPROM table version detected. Table version = 0x%x. Expected version = 0x%x"),
	add_entry(AMDGV_ERROR_ECC_BAD_PAGE_APPEND, ERROR_DATA_ARG_32_32, AMDGV_ERROR_SEVERITY_INFO,
		"%u bad page records were appended to driver RAS bad page storage. There are %u records now."),

	/* this one is the MAX */
	add_entry(AMDGV_ERROR_ECC_MAX, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_INFO,
		  "This is error log collect test for ECC component (test count %llu)."),
};

error_category(PP) = {
	add_entry(AMDGV_ERROR_PP_SET_DPM_POLICY_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Set virtualization GFX DPM policy failed."),
	add_entry(AMDGV_ERROR_PP_ACTIVATE_DPM_POLICY_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Activate virtualization GFX DPM policy failed."),
	add_entry(AMDGV_ERROR_PP_I2C_SLAVE_NOT_PRESENT, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_INFO, "I2C slave at 0x%llX is not present."),
	add_entry(AMDGV_ERROR_PP_THROTTLER_EVENT, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_WARNING, "GPU device throttler event detected: 0x%016llx."),

	/* this one is the MAX */
	add_entry(AMDGV_ERROR_PP_MAX, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_INFO,
		  "This is error log collect test for PP component (test count %llu)."),
};

error_category(SCHED) = {
	add_entry(AMDGV_ERROR_SCHED_WORLD_SWITCH_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "World Switch Failed!"),
	add_entry(AMDGV_ERROR_SCHED_DISABLE_AUTO_HW_SWITCH_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED,
		  "DISABLE_AUTO_HW_SCHEDULING_AND_CONTEXT_SWITCH failed."),
	add_entry(AMDGV_ERROR_SCHED_ENABLE_AUTO_HW_SWITCH_FAIL, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "ENABLE_AUTO_HW_SWITCH failed on block %llu."),
	add_entry(AMDGV_ERROR_SCHED_GFX_SAVE_REG_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "GFX SAVE register failed."),
	add_entry(AMDGV_ERROR_SCHED_GFX_IDLE_REG_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "GFX IDLE register failed."),

	add_entry(AMDGV_ERROR_SCHED_GFX_RUN_REG_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "GFX RUN register failed."),
	add_entry(AMDGV_ERROR_SCHED_GFX_LOAD_REG_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "GFX LOAD register failed."),
	add_entry(AMDGV_ERROR_SCHED_GFX_INIT_REG_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "GFX INIT register failed."),
	add_entry(AMDGV_ERROR_SCHED_MM_SAVE_REG_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "MM SAVE register failed."),
	add_entry(AMDGV_ERROR_SCHED_MM_IDLE_REG_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "MM IDLE register failed."),

	add_entry(AMDGV_ERROR_SCHED_MM_RUN_REG_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "MM RUN register failed."),
	add_entry(AMDGV_ERROR_SCHED_MM_LOAD_REG_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "MM LOAD register failed."),
	add_entry(AMDGV_ERROR_SCHED_MM_INIT_REG_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "MM INIT register failed."),
	add_entry(AMDGV_ERROR_SCHED_INIT_GPU_FAIL, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "INIT_GPU failed on block %llu."),
	add_entry(AMDGV_ERROR_SCHED_RUN_GPU_FAIL, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_ERROR_LOW,
		  "RUN_GPU failed on block %llu."),

	add_entry(AMDGV_ERROR_SCHED_SAVE_GPU_STATE_FAIL, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "SAVE_GPU_STATE failed on block %llu."),
	add_entry(AMDGV_ERROR_SCHED_LOAD_GPU_STATE_FAIL, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "LOAD_GPU_STATE failed on block %llu."),
	add_entry(AMDGV_ERROR_SCHED_IDLE_GPU_FAIL, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "IDLE_GPU failed on block %llu."),
	add_entry(AMDGV_ERROR_SCHED_FINI_GPU_FAIL, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "FINI_GPU failed on block %llu."),
	add_entry(AMDGV_ERROR_SCHED_DEAD_VF, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_ERROR_MED,
		  "VF%llu is not responding."),

	add_entry(AMDGV_ERROR_SCHED_EVENT_QUEUE_FULL, ERROR_DATA_ARG_32_32,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Sched event queue is full. rptr(%u) wptr(%u)."),
	add_entry(AMDGV_ERROR_SCHED_SHUTDOWN_VF_FAIL, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Cannot shutdown VF%u."),
	add_entry(AMDGV_ERROR_SCHED_RESET_VF_NUM_FAIL, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "VF%u is still available in scheduler."),
	add_entry(AMDGV_ERROR_SCHED_IGNORE_EVENT, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_WARNING,
		  "Ignore schedule event 0x%llx."),
	add_entry(AMDGV_ERROR_SCHED_PF_SWITCH_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Switch to PF Failed!"),

	/* this one is the MAX */
	add_entry(AMDGV_ERROR_SCHED_MAX, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_INFO,
		  "This is error log collect test for SCHED component (test count %llu)."),
};

error_category(VF) = {
	add_entry(AMDGV_ERROR_VF_ATOMBIOS_INIT_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "amdgpu_atombios_init failed."),
	add_entry(AMDGV_ERROR_VF_NO_VBIOS, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_ERROR_LOW,
		  "no vBIOS found."),
	add_entry(AMDGV_ERROR_VF_GPU_POST_ERROR, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "gpu post error."),
	add_entry(AMDGV_ERROR_VF_ATOMBIOS_GET_CLOCK_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "amdgpu_atombios_get_clock_info failed."),
	add_entry(AMDGV_ERROR_VF_FENCE_INIT_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "amdgpu_fence_driver_init failed."),

	add_entry(AMDGV_ERROR_VF_AMDGPU_INIT_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "amdgpu_init failed."),
	add_entry(AMDGV_ERROR_VF_IB_INIT_FAIL, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_ERROR_LOW,
		  "IB initialization failed (%d)."),
	add_entry(AMDGV_ERROR_VF_AMDGPU_LATE_INIT_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_HIGH, "amdgpu_late_init failed."),
	add_entry(AMDGV_ERROR_VF_ASIC_RESUME_FAIL, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "asic resume failed (%d)."),
	add_entry(AMDGV_ERROR_VF_GPU_RESET_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "GPU reset failed."),

	/* this one is the MAX */
	add_entry(AMDGV_ERROR_VF_MAX, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_INFO,
		  "This is error log collect test for VF component (test count %llu)."),
};

error_category(VBIOS) = {
	add_entry(AMDGV_ERROR_VBIOS_INVALID, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_ERROR_HIGH,
		  "Invalid VBios."),
	add_entry(AMDGV_ERROR_VBIOS_IMAGE_MISSING, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_HIGH, "VBios image is missing."),
	add_entry(AMDGV_ERROR_VBIOS_CHECKSUM_ERR, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_HIGH, "VBios checksum error."),
	add_entry(AMDGV_ERROR_VBIOS_POST_FAIL, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_ERROR_HIGH,
		  "Post VBIOS failed."),
	add_entry(AMDGV_ERROR_VBIOS_READ_FAIL, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_ERROR_HIGH,
		  "Read VBIOS failed."),

	add_entry(AMDGV_ERROR_VBIOS_READ_IMG_HEADER_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_HIGH, "Read ROM image header failed."),
	add_entry(AMDGV_ERROR_VBIOS_READ_IMG_SIZE_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_HIGH, "ATOM read ROM image size failed."),
	add_entry(AMDGV_ERROR_VBIOS_GET_FW_INFO_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_HIGH, "ATOM get firmware info failed."),
	add_entry(AMDGV_ERROR_VBIOS_GET_TBL_REVISION_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_HIGH, "ATOM get table revision failed."),
	add_entry(AMDGV_ERROR_VBIOS_PARSER_TBL_FAIL, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_HIGH, "ATOM parse table failed! (parser result=%llu)."),

	add_entry(AMDGV_ERROR_VBIOS_IP_DISCOVERY_FAIL, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_HIGH, "IP Discovery Failure"),
	add_entry(AMDGV_ERROR_VBIOS_TIMEOUT, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_ERROR_HIGH,
		  "Read VBios timeout."),
	add_entry(AMDGV_ERROR_VBIOS_IP_DISCOVERY_BINARY_CHECKSUM_FAIL, ERROR_DATA_ARG_32_32,
		  AMDGV_ERROR_SEVERITY_ERROR_HIGH, "IP Discovery Binary checksum failed expected=0x%08x readback=0x%08x"),
	add_entry(AMDGV_ERROR_VBIOS_IP_DISCOVERY_TABLE_CHECKSUM_FAIL, ERROR_DATA_ARG_16_16_32,
		  AMDGV_ERROR_SEVERITY_ERROR_HIGH, "IP Discovery Table %d checksum failed expected=0x%08x readback=0x%08x"),

	/* this one is the MAX */
	add_entry(AMDGV_ERROR_VBIOS_MAX, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_INFO,
		  "This is error log collect test for VBIOS component (test count %llu)."),
};

error_category(GPU) = {
	add_entry(AMDGV_ERROR_GPU_DEVICE_LOST, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_ERROR_HIGH,
		  "Device disappeared from bus."),
	add_entry(AMDGV_ERROR_GPU_NOT_SUPPORTED, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_ERROR_MED,
		  "The device id (0x%llx) is not supported by this LibGV."),
	add_entry(AMDGV_ERROR_GPU_RMA, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_ERROR_HIGH,
		  "GPU is in RMA state."),
	add_entry(AMDGV_ERROR_GPU_NOT_INITIALIZED, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_MED, "Not initialized."),
	add_entry(AMDGV_ERROR_GPU_MMSCH_ABNORMAL_STATE, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_ERROR_MED,
		  "Multimedia virtualization scheduler has entered an abnormal state"),
	add_entry(AMDGV_ERROR_GPU_RLCV_ABNORMAL_STATE, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_ERROR_MED,
		  "Graphics Virtualization Scheduler has entered an abnormal state"),
	add_entry(AMDGV_ERROR_GPU_SDMA_ENGINE_BUSY, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_INFO,
		  "System DMA engine is busy"),
	add_entry(AMDGV_ERROR_GPU_RLC_ENGINE_BUSY, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_INFO,
		  "Run List Controller is busy"),
	add_entry(AMDGV_ERROR_GPU_GC_ENGINE_BUSY, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_INFO,
		  "Graphics-Compute is busy"),
	add_entry(AMDGV_ERROR_GPU_HIVE_RMA, ERROR_DATA_ARG_NONE, AMDGV_ERROR_SEVERITY_ERROR_HIGH,
			"Driver disabled runtime services because it is FB sharing with GPU(s) that are in RMA state."
			" Disable FB sharing or Replace the RMA GPU(s) to restore the hive."),
	/* this one is the MAX */
	add_entry(AMDGV_ERROR_GPU_MAX, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_INFO,
		  "This is error log collect test for GPU component (test count %llu)."),
};

error_category(GUARD) = {
	add_entry(AMDGV_ERROR_GUARD_RESET_FAIL, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_ERROR_MED,
		  "Failed to reset VF%llu guard info."),
	add_entry(AMDGV_ERROR_GUARD_EVENT_OVERFLOW, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_WARNING, "Event 0x%llx overflow."),

	/* this one is the MAX */
	add_entry(AMDGV_ERROR_GUARD_MAX, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_INFO,
		  "This is error log collect test for GUARD component (test count %llu)."),
};

error_category(GPUMON) = {
	add_entry(AMDGV_ERROR_GPUMON_INVALID_OPTION, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "Invalid VF option (%d)."),
	add_entry(AMDGV_ERROR_GPUMON_INVALID_VF_INDEX, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "Invalid VF index (%u)."),
	add_entry(AMDGV_ERROR_GPUMON_INVALID_FB_SIZE, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "Invalid FB size (%uMB)."),
	add_entry(AMDGV_ERROR_GPUMON_NO_SUITABLE_SPACE, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "No suitable space to fit FB size (%uMB)."),
	add_entry(AMDGV_ERROR_GPUMON_NO_AVAILABLE_SLOT, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "No available slot to allocate new VF."),

	add_entry(AMDGV_ERROR_GPUMON_OVERSIZE_ALLOCATION, ERROR_DATA_ARG_32_32,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW,
		  "FB offset (%uMB) plus FB size (%uMB) exceed total FB size."),
	add_entry(AMDGV_ERROR_GPUMON_OVERLAPPING_FB, ERROR_DATA_ARG_32_32,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "Overlapped FB from (%uMB) to (%uMB)."),
	add_entry(AMDGV_ERROR_GPUMON_INVALID_GFX_TIMESLICE, ERROR_DATA_ARG_32_32,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW,
		  "Invalid GFX time slice value (%ums), Max allowance is (%ums)."),
	add_entry(AMDGV_ERROR_GPUMON_INVALID_MM_TIMESLICE, ERROR_DATA_ARG_32_32,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW,
		  "Invalid MM time slice value (%ums), Max allowance is (%ums)."),
	add_entry(AMDGV_ERROR_GPUMON_INVALID_GFX_PART, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "Invalid GFX time partition."),

	add_entry(AMDGV_ERROR_GPUMON_VF_BUSY, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_ERROR_LOW,
		  "VF(%u) has loaded guest driver."),
	add_entry(AMDGV_ERROR_GPUMON_INVALID_VF_NUM, ERROR_DATA_ARG_32_32,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "VF num (%u) exceeds max allowance (%u)."),
	add_entry(AMDGV_ERROR_GPUMON_NOT_SUPPORTED, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "API not supported by GPUMON."),
	add_entry(AMDGV_ERROR_GPUMON_SET_ALREADY, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_WARNING, "Already set to specified mode."),
	add_entry(AMDGV_ERROR_GPUMON_INVALID_MODE, ERROR_DATA_ARG_NONE,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW, "Specified mode is invalid."),

	/* this one is the MAX */
	add_entry(AMDGV_ERROR_GPUMON_MAX, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_INFO,
		  "This is error log collect test for GPUMON component (test count %llu)."),
};

error_category(MMSCH) = {
	add_entry(AMDGV_ERROR_MMSCH_IGNORED_JOB, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_WARNING,
		  "On this VF, there is least 1 job that is ignored on VCN(%u)."),
	add_entry(AMDGV_ERROR_MMSCH_UNSUPPORTED_VCN_FW, ERROR_DATA_ARG_64,
		  AMDGV_ERROR_SEVERITY_ERROR_LOW,
		  "On this VF, VCN(%u) FW does not support MM bandwidth managment."),

	/* this one is the MAX */
	add_entry(AMDGV_ERROR_MMSCH_MAX, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_INFO,
		  "This is error log collect test for GPUMON component (test count %llu)."),
};

error_category(XGMI) = {
	add_entry(AMDGV_ERROR_XGMI_TOPOLOGY_UPDATE_FAILED, ERROR_DATA_ARG_NONE,
			AMDGV_ERROR_SEVERITY_ERROR_HIGH, "Device failed to update XGMI Topology."),
	add_entry(AMDGV_ERROR_XGMI_TOPOLOGY_HW_INIT_UPDATE, ERROR_DATA_ARG_NONE,
			AMDGV_ERROR_SEVERITY_INFO, "Last initialized device in hive initiating XGMI Topology update."),
	add_entry(AMDGV_ERROR_XGMI_TOPOLOGY_UPDATE_DONE, ERROR_DATA_ARG_NONE,
			AMDGV_ERROR_SEVERITY_INFO, "Device successfully updated XGMI Topology."),
	add_entry(AMDGV_ERROR_XGMI_FB_SHARING_SETTING_ERROR, ERROR_DATA_ARG_NONE,
			AMDGV_ERROR_SEVERITY_ERROR_MED, "Device has invalid XGMI FB Sharing setting."),
	add_entry(AMDGV_ERROR_XGMI_FB_SHARING_SETTING_RESET, ERROR_DATA_ARG_NONE,
			AMDGV_ERROR_SEVERITY_ERROR_MED, "XGMI Hive FB Sharing settings have been reset due to errors."),

		/* this one is the MAX */
	add_entry(AMDGV_ERROR_XGMI_MAX, ERROR_DATA_ARG_64, AMDGV_ERROR_SEVERITY_INFO,
		  "This is error log collect test for XGMI component (test count %llu)."),
};

/* Error category */
const struct amdgv_error_info amdgv_error_list[AMDGV_ERROR_CATEGORY_MAX] = {
	category_not_used(),  add_category(DRIVER), add_category(RESET), add_category(SCHED),
	add_category(VBIOS),  add_category(ECC),    add_category(PP),	 add_category(IOV),
	add_category(VF),     add_category(FW),	    add_category(GPU),	 add_category(GUARD),
	add_category(GPUMON), add_category(MMSCH),  add_category(XGMI),
};
