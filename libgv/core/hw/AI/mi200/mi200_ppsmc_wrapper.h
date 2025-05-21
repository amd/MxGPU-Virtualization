/*
 * Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef MI200_PPSMC_WRAPPER_H
#define MI200_PPSMC_WRAPPER_H

#include "mi200_ppsmc.h"


// SMU response codes:
#define SMU_13_0_RESULT_OK \
		PPSMC_Result_OK
#define SMU_13_0_RESULT_FAILED \
		PPSMC_Result_Failed
#define SMU_13_0_RESULT_UNKNOWN_CMD \
		PPSMC_Result_UnknownCmd
#define SMU_13_0_RESULT_CMD_REJECTED_PREREQ \
		PPSMC_Result_CmdRejectedPrereq
#define SMU_13_0_RESULT_CMD_REJECTED_BUSY \
		PPSMC_Result_CmdRejectedBusy

// Message defenitions:
#define SMU_13_0_MSG__TEST_MESSAGE \
		PPSMC_MSG_TestMessage
#define SMU_13_0_MSG__GET_SMU_VERSION \
		PPSMC_MSG_GetSmuVersion
#define SMU_13_0_MSG__GET_DRIVER_IF_VERSION \
		PPSMC_MSG_GetDriverIfVersion
#define SMU_13_0_MSG__ENABLE_ALL_SMU_FEATURES \
		PPSMC_MSG_EnableAllSmuFeatures
#define SMU_13_0_MSG__DISABLE_ALL_SMU_FEATURES \
		PPSMC_MSG_DisableAllSmuFeatures
#define SMU_13_0_MSG__ENABLE_SMU_FEATURES_LOW \
		PPSMC_MSG_EnableSmuFeaturesLow
#define SMU_13_0_MSG__ENABLE_SMU_FEATURES_HIGH \
		PPSMC_MSG_EnableSmuFeaturesHigh
#define SMU_13_0_MSG__DISABLE_SMU_FEATURES_LOW \
		PPSMC_MSG_DisableSmuFeaturesLow
#define SMU_13_0_MSG__DISABLE_SMU_FEATURES_HIGH \
		PPSMC_MSG_DisableSmuFeaturesHigh
#define SMU_13_0_MSG__GET_ENABLED_SMU_FEATURES_LOW \
		PPSMC_MSG_GetEnabledSmuFeaturesLow
#define SMU_13_0_MSG__GET_ENABLED_SMU_FEATURES_HIGH \
		PPSMC_MSG_GetEnabledSmuFeaturesHigh
#define SMU_13_0_MSG__SET_DRIVER_DRAM_ADDR_HIGH \
		PPSMC_MSG_SetDriverDramAddrHigh
#define SMU_13_0_MSG__SET_DRIVER_DRAM_ADDR_LOW \
		PPSMC_MSG_SetDriverDramAddrLow
#define SMU_13_0_MSG__SET_TOOLS_DRAM_ADDR_HIGH \
		PPSMC_MSG_SetToolsDramAddrHigh
#define SMU_13_0_MSG__SET_TOOLS_DRAM_ADDR_LOW \
		PPSMC_MSG_SetToolsDramAddrLow
#define SMU_13_0_MSG__TRANSFER_TABLE_SMU2_DRAM \
		PPSMC_MSG_TransferTableSmu2Dram
#define SMU_13_0_MSG__TRANSFER_TABLE_DRAM2_SMU \
		PPSMC_MSG_TransferTableDram2Smu
#define SMU_13_0_MSG__USE_DEFAULT_PPTABLE \
		PPSMC_MSG_UseDefaultPPTable
#define SMU_13_0_MSG__USE_BACKUP_PPTABLE \
		PPSMC_MSG_UseBackupPPTable
#define SMU_13_0_MSG__SET_SYSTEM_VIRTUAL_DRAM_ADDR_HIGH \
		PPSMC_MSG_SetSystemVirtualDramAddrHigh
#define SMU_13_0_MSG__SET_SYSTEM_VIRTUAL_DRAM_ADDR_LOW \
		PPSMC_MSG_SetSystemVirtualDramAddrLow

#define SMU_13_0_MSG__ENTER_BACO \
		PPSMC_MSG_EnterBaco
#define SMU_13_0_MSG__EXIT_BACO \
		PPSMC_MSG_ExitBaco
#define SMU_13_0_MSG__ARM_D3 \
		PPSMC_MSG_ArmD3

#define SMU_13_0_MSG__SET_SOFT_MIN_BY_FREQ \
		PPSMC_MSG_SetSoftMinByFreq
#define SMU_13_0_MSG__SET_SOFT_MAX_BY_FREQ \
		PPSMC_MSG_SetSoftMaxByFreq
#define SMU_13_0_MSG__SET_HARD_MIN_BY_FREQ \
		PPSMC_MSG_SetHardMinByFreq
#define SMU_13_0_MSG__SET_HARD_MAX_BY_FREQ \
		PPSMC_MSG_SetHardMaxByFreq
#define SMU_13_0_MSG__GET_MIN_DPM_FREQ \
		PPSMC_MSG_GetMinDpmFreq
#define SMU_13_0_MSG__GET_MAX_DPM_FREQ \
		PPSMC_MSG_GetMaxDpmFreq
#define SMU_13_0_MSG__GET_DPM_FREQ_BY_INDEX \
		PPSMC_MSG_GetDpmFreqByIndex

#define SMU_13_0_MSG__SET_WORKLOAD_MASK \
		PPSMC_MSG_SetWorkloadMask
#define SMU_13_0_MSG__SET_DF_SWITCH_TYPE \
		PPSMC_MSG_SetDfSwitchType
#define SMU_13_0_MSG__GET_VOLTAGE_BY_DPM \
		PPSMC_MSG_GetVoltageByDpm
#define SMU_13_0_MSG__GET_VOLTAGE_BY_DPM_OVERDRIVE \
		PPSMC_MSG_GetVoltageByDpmOverdrive

#define SMU_13_0_MSG__SET_PPT_LIMIT \
		PPSMC_MSG_SetPptLimit
#define SMU_13_0_MSG__GET_PPT_LIMIT \
		PPSMC_MSG_GetPptLimit

#define SMU_13_0_MSG__POWER_UP_VCN0 \
		PPSMC_MSG_PowerUpVcn0
#define SMU_13_0_MSG__POWER_DOWN_VCN0 \
		PPSMC_MSG_PowerDownVcn0
#define SMU_13_0_MSG__POWER_UP_VCN1 \
		PPSMC_MSG_PowerUpVcn1
#define SMU_13_0_MSG__POWER_DOWN_VCN1 \
		PPSMC_MSG_PowerDownVcn1

#define SMU_13_0_MSG__PREPARE_MP1_FOR_UNLOAD \
		PPSMC_MSG_PrepareMp1ForUnload
#define SMU_13_0_MSG__PREPARE_MP1_FOR_RESET \
		PPSMC_MSG_PrepareMp1ForReset
#define SMU_13_0_MSG__PREPARE_MP1_FOR_SHUTDOWN \
		PPSMC_MSG_PrepareMp1ForShutdown
#define SMU_13_0_MSG__SOFT_RESET \
		PPSMC_MSG_SoftReset

#define SMU_13_0_MSG__RUN_AFLL_BTC \
		PPSMC_MSG_RunAfllBtc
#define SMU_13_0_MSG__RUN_GFX_DC_BTC \
		PPSMC_MSG_RunGfxDcBtc
#define SMU_13_0_MSG__RUN_SOC_DC_BTC \
		PPSMC_MSG_RunSocDcBtc

#define SMU_13_0_MSG__DRAM_LOG_SET_DRAM_ADDR_HIGH \
		PPSMC_MSG_DramLogSetDramAddrHigh
#define SMU_13_0_MSG__DRAM_LOG_SET_DRAM_ADDR_LOW \
		PPSMC_MSG_DramLogSetDramAddrLow
#define SMU_13_0_MSG__DRAM_LOG_SET_DRAM_SIZE \
		PPSMC_MSG_DramLogSetDramSize
#define SMU_13_0_MSG__GET_DEBUG_DATA \
		PPSMC_MSG_GetDebugData

#define SMU_13_0_MSG__WAFL_TEST \
		PPSMC_MSG_WaflTest
#define SMU_13_0_MSG__SET_XGMI_MODE \
		PPSMC_MSG_SetXgmiMode

//Others
#define SMU_13_0_MSG__SET_MEMORY_CHANNEL_ENABLE \
		PPSMC_MSG_SetMemoryChannelEnable

#define SMU_13_0_MESSAGE_COUNT \
		PPSMC_Message_Count

#endif
