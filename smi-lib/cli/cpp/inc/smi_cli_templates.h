/* * Copyright (C) 2023-2024 Advanced Micro Devices. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

#include <string>

inline std::string gpuListTemplate{ "GPU: %d \n    BDF: %s \n    UUID: %s\n" };

inline std::string vfListTemplate{ "    VF: %d \n        BDF: %s\n        UUID: %s\n" };

inline std::string vfNestedTemplate{ "GPU: %s \n    VF: %s \n" };

inline std::string versionTemplate{ "VERSION: \n    TOOL_NAME: %s \n    TOOL_VERSION: %s \n    LIB_VERSION: %s \n" };

inline std::string staticAsicTemplate{
	"    ASIC: \n        MARKET_NAME: %s \n        VENDOR_ID: %s \n        VENDOR_NAME: %s \n        SUBVENDOR_ID: %s \n        DEVICE_ID: %s "
	"\n        SUBSYSTEM_ID: %s \n        REV_ID: %s \n        ASIC_SERIAL: %s\n        OAM_ID: %s\n"
};

inline std::string staticDfcHeaderTemplate{
	"    DFC: \n        HEADER: \n            VERSION: %s \n            GART_WR_GUEST_MIN: %s \n            GART_WR_GUEST_MAX: %s \n        DATA: \n"
};

inline std::string staticDfcDataTemplate{
	"            DFC_FW_TYPE: %d \n"
	"            VERIFICATION: %s \n"
	"            CUSTOMER_ORDINAL: %s \n"
};

inline std::string staticDfcDataTemplateEmpty{
	"            DFC_FW_TYPE: \n"
	"            VERIFICATION: \n"
	"            CUSTOMER_ORDINAL: \n"
};

inline std::string staticDfcWhiteListHeaderTemplate{ "            WHITE_LIST:\n" };

inline std::string staticDfcWhiteListElementTemplate{
	"                VERSIONS: \n"
	"                     WHITE_LIST_LATEST: %s\n"
	"                     WHITE_LIST_OLDEST: %s\n"
};

inline std::string staticDfcBlackListHeaderTemplate{ "                     BLACK_LIST:\n" };

inline std::string staticDfcBlackListElementTemplate{
	"                        BLACK_LIST_%d: %s\n"
};

inline std::string staticFbInfoTemplate{
	"    FB_INFO: \n        TOTAL_FB_SIZE: %s %s\n        PF_FB_RESERVED: %s %s"
	"\n        PF_FB_OFFSET: %s %s\n        FB_ALIGNMENT: %s %s"
	"\n        MAX_VF_FB_USABLE: %s %s\n        MIN_VF_FB_USABLE: %s %s\n"
};

inline std::string staticNumVfTemplate{
	"    NUM_VF: \n        SUPPORTED: %s \n        ENABLED: %s\n"
};

inline std::string staticBusTemplate{
	"    BUS: \n        BDF: %s \n        MAX_PCIE_WIDTH: %s\n        MAX_PCIE_SPEED: %s %s\n"
	"        PCIE_INTERFACE_VERSION: %s \n        SLOT_TYPE: %s\n        MAX_PCIE_INTERFACE_VERSION: %s\n"
};

inline std::string staticVbiosTemplate{
	"    VBIOS: \n        NAME: %s \n        BUILD_DATE: %s "
	"\n        PART_NUMBER: %s \n        VERSION: %s\n"
};

inline std::string staticBoardTemplate{
	"    BOARD: \n        MODEL_NUMBER: "
	"%s \n        PRODUCT_SERIAL: %s \n        FRU_ID: %s \n        PRODUCT_NAME: %s "
	"\n        MANUFACTURER_NAME: %s \n"
};

inline std::string staticLimitTemplate{
	"    LIMIT: \n        MAX_POWER: %s %s\n        MIN_POWER: %s %s\n        SOCKET_POWER: %s %s\n"
	"        SLOWDOWN_EDGE_TEMPERATURE: %s %s\n"
	"        SLOWDOWN_HOTSPOT_TEMPERATURE: %s %s\n"
	"        SLOWDOWN_MEM_TEMPERATURE: %s %s\n"
	"        SHUTDOWN_EDGE_TEMPERATURE: %s %s\n"
	"        SHUTDOWN_HOTSPOT_TEMPERATURE: %s %s\n"
	"        SHUTDOWN_MEM_TEMPERATURE: %s %s\n"
};

inline std::string staticRasTemplateHost{
	"    RAS:\n"
	"        EEPROM_VERSION: %s\n"
	"        PARITY_SCHEMA: %s\n"
	"        SINGLE_BIT_SCHEMA: %s\n"
	"        DOUBLE_BIT_SCHEMA: %s\n"
	"        POISON_SCHEMA: %s\n"
	"        BLOCK_STATE: \n"
};

inline std::string staticRasBlockTemplate{"            %s: %s\n" };

inline std::string staticVfTemplate{
	"        FB_OFFSET: %s %s\n        FB_SIZE: %s %s\n        GFX_TIMESLICE: %s %s\n"
};

inline std::string staticProcessIsolate{
	"    PROCESS_ISOLATION: %s \n"
};

inline std::string driverHostInfoTemplate{ "    DRIVER: \n        NAME: %s\n        VERSION: %s\n        DATE: %s\n        MODEL: %s\n" };

inline std::string badPagesTemplate{
	"    BAD_PAGE_%u:\n"
	"        RETIRED_BAD_PAGE: %s\n"
	"        TIMESTAMP: %s\n"
	"        MEM_CHANNEL: %u\n"
	"        MCUMC_ID: %u\n" };

inline std::string fwListTemplate{ "    FW_LIST:\n" };

inline std::string fwListVfTemplate{ "        FW_LIST:\n" };

inline std::string fwTemplate{
	"        FW_%d:\n            FW_ID: %s\n            FW_VERSION: %s\n"
};

inline std::string gpuTemplate{ "GPU: %d\n" };

inline std::string fwErrorRecordListTemplate{ "    ERROR_RECORDS: \n" };

inline std::string fwErrorRecordsTemplate{
	"        ERROR_RECORD:\n             TIMESTAMP: %d\n             VF: %d\n"
	"             NAME: %d\n             STATUS: %d\n"
};

inline std::string metricUsageTemplate{
	"    USAGE:\n"
	"        GFX_ACTIVITY: %s %s\n"
	"        UMC_ACTIVITY: %s %s\n"
	"        MM_ACTIVITY: %s %s\n"};

inline std::string metricVcnUsageDefaultTemplate{
	"        VCN_ACTIVITY: N/A\n"};

inline std::string metricJpegUsageDefaultTemplate{
	"        JPEG_ACTIVITY: N/A\n"};

inline std::string commaTemplate{
	", "};

inline std::string metricVcnUsageHeaderTemplate{
	"        VCN_ACTIVITY: ["};

inline std::string metricVcnUsageTemplate{
	"%s %s"};

inline std::string metricJpegUsageHeaderTemplate{
	"]\n"
	"        JPEG_ACTIVITY: ["};

inline std::string metricJpegUsageTemplate{
	"%s %s"};

inline std::string metricJpegUsageFooterTemplate{
	"]\n"};

inline std::string metricPowerMeasureTemplate{
	"    POWER:\n"
	"        SOCKET_POWER: %s %s\n"
	"        GFX_VOLTAGE: %s %s\n"
	"        SOC_VOLTAGE: %s %s\n"
	"        MEM_VOLTAGE: %s %s\n"
	"        POWER_MANAGEMENT: %s \n" };

inline std::string metricGuestPowerMeasureTemplate{
	"    POWER:\n"
	"        SOCKET_POWER: %s %s\n"
	"        GFX_VOLTAGE: %s %s\n"
	"        SOC_VOLTAGE: %s %s\n"
	"        MEM_VOLTAGE: %s %s\n" };

inline std::string metricClockMeasureHostTemplate{
	"    CLOCK:\n"
	"        GFX:\n"
	"            CLK: %s %s\n"
	"            MIN_CLK: %s %s\n"
	"            MAX_CLK: %s %s\n"
	"            CLK_LOCKED: %s\n"
	"            DEEP_SLEEP: %s\n"
	"        MEM:\n"
	"            CLK: %s %s\n"
	"            MIN_CLK: %s %s\n"
	"            MAX_CLK: %s %s\n"
	"            CLK_LOCKED: %s\n"
	"            DEEP_SLEEP: %s\n" };

inline std::string metricClockMeasureHostHeaderTemplate{"    CLOCK:\n"};

inline std::string metricChipletGfxClockMeasureHostTemplate{
	"        GFX_%d:\n"
	"            CLK: %s \n"
	"            MIN_CLK: %s \n"
	"            MAX_CLK: %s \n"
	"            CLK_LOCKED: %s\n"
	"            DEEP_SLEEP: %s\n"};
inline std::string metricChipletMemClockMeasureHostTemplate{
	"        MEM_%d:\n"
	"            CLK: %s \n"
	"            MIN_CLK: %s \n"
	"            MAX_CLK: %s \n"
	"            CLK_LOCKED: %s\n"
	"            DEEP_SLEEP: %s\n"};

inline std::string metricChipletVCLKClockMeasureHostTemplate{
	"        VCLK_%d:\n"
	"            CLK: %s \n"
	"            MIN_CLK: %s \n"
	"            MAX_CLK: %s \n"
	"            CLK_LOCKED: %s\n"
	"            DEEP_SLEEP: %s\n"};

inline std::string metricChipletDCLKClockMeasureHostTemplate{
	"        DCLK_%d:\n"
	"            CLK: %s \n"
	"            MIN_CLK: %s \n"
	"            MAX_CLK: %s \n"
	"            CLK_LOCKED: %s\n"
	"            DEEP_SLEEP: %s\n"};

inline std::string metricVCLK0ClockMeasureHostTemplate{
	"        VCLK_0: \n"
	"            CLK: %s %s \n"
	"            MIN_CLK: %s %s \n"
	"            MAX_CLK: %s %s \n"
	"            CLK_LOCKED: %s\n"
	"            DEEP_SLEEP: %s\n"};

inline std::string metricVCLK1ClockMeasureHostTemplate{
	"        VCLK_1: \n"
	"            CLK: %s %s \n"
	"            MIN_CLK: %s %s \n"
	"            MAX_CLK: %s %s \n"
	"            CLK_LOCKED: %s\n"
	"            DEEP_SLEEP: %s\n"};

inline std::string metricDCLK0ClockMeasureHostTemplate{
	"        DCLK_0:\n"
	"            CLK: %s %s\n"
	"            MIN_CLK: %s %s\n"
	"            MAX_CLK: %s %s\n"
	"            CLK_LOCKED: %s\n"
	"            DEEP_SLEEP: %s\n"};


inline std::string metricDCLK1ClockMeasureHostTemplate{
	"        DCLK_1:\n"
	"            CLK: %s %s\n"
	"            MIN_CLK: %s %s\n"
	"            MAX_CLK: %s %s\n"
	"            CLK_LOCKED: %s\n"
	"            DEEP_SLEEP: %s\n"};

inline std::string metricThermalMeasureTemplate{
	"    TEMPERATURE:\n"
	"        EDGE: %s %s\n"
	"        HOTSPOT: %s %s\n"
	"        MEM: %s %s\n" };

inline std::string metricEccErrorCountTemplate{
	"    ECC:\n"
	"        TOTAL_CORRECTABLE_COUNT: %s\n"
	"        TOTAL_UNCORRECTABLE_COUNT: %s\n"
	"        TOTAL_DEFERRED_COUNT: %s\n"
	"        CACHE_CORRECTABLE_COUNT: %s\n"
	"        CACHE_UNCORRECTABLE_COUNT: %s\n" };

inline std::string metricEccBlockErrorCountHeaderTemplate{"    ECC_BLOCKS:\n"};

inline std::string metricEccBlockErrorCountTemplate{
	"        %s:\n"
	"            CORRECTABLE_COUNT: %s\n"
	"            UNCORRECTABLE_COUNT: %s\n"
	"            DEFERRED_COUNT: %s\n"};

inline std::string metricScheduleTemplate = {
	"        SCHEDULE:\n"
	"            BOOT_UP_TIME: %s %s\n"
	"            FLR_COUNT: %s\n"
	"            VF_STATE: %s\n"
	"            LAST_BOOT_START: %s\n"
	"            LAST_BOOT_END: %s\n"
	"            LAST_SHUTDOWN_START: %s\n"
	"            LAST_SHUTDOWN_END: %s\n"
	"            SHUTDOWN_TIME: %s %s\n"
	"            LAST_RESET_START: %s\n"
	"            LAST_RESET_END: %s\n"
	"            RESET_TIME: %s %s\n"
	"            ACTIVE_TIME: %s\n"
	"            RUNNING_TIME: %s\n"
	"            TOTAL_ACTIVE_TIME: %s\n"
	"            TOTAL_RUNNING_TIME: %s\n"
};

inline std::string metricGuardTemplate = {
	"        GUARD:\n"
	"            ENABLED: %s\n"
	"            GUARD_INFO:\n%s\n"
};

inline std::string metricGuestDataTemplate = {
	"        GUEST_DATA:\n"
	"            DRIVER_VERSION: %s\n"
	"            FB_USAGE: %s %s\n"
};

inline std::string metricGuardInfoTemplate = {
	"                %s:\n"
	"                    GUARD_STATE: %s\n"
	"                    AMOUNT: %s\n"
	"                    INTERVAL: %s %s\n"
	"                    THRESHOLD: %s\n"
	"                    ACTIVE: %s\n"
};


inline std::string metricPowerEnergyTemplate = {
	"    ENERGY:\n"
	"        TOTAL_ENERGY_CONSUMPTION: %s %s\n"
};

inline std::string memoryPartitioningTemplate = {
	"        MEMORY_PARTITION:\n"
	"             MEMORY_PARTITION_CAPS: %s \n"
	"             CURRENT_PARTITION: %s \n"
};

inline std::string pcieInfoTemplate{
	"    PCIE:\n"
	"        WIDTH: %s \n"
	"        SPEED: %s %s \n"
	"        REPLAY_COUNT: %s \n"
};
inline std::string pcieInfoHostTemplate{
	"    PCIE:\n"
	"        WIDTH: %s \n"
	"        SPEED: %s %s \n"
	"        BANDWIDTH: %s %s\n"
	"        REPLAY_COUNT: %s \n"
	"        L0_TO_RECOVERY_COUNT: %s \n"
	"        REPLAY_ROLL_OVER_COUNT: %s \n"
	"        NAK_SENT_COUNT: %s \n"
	"        NAK_RECEIVED_COUNT: %s \n"
};

inline std::string topologyWeightTemplate{ "WEIGHT_TABLE:\n" };
inline std::string topologyHopsTemplate{ "HOPS_TABLE:\n" };
inline std::string topologyFbSharingTemplate{ "FB_SHARING_TABLE:\n" };
inline std::string topologyLinkTypeTemplate{ "LINK_TYPE_TABLE:\n" };
inline std::string topologyLinkStatusTemplate{ "LINK_STATUS_TABLE:\n" };

inline std::string metricXgmiTemplate{ "    XGMI:\n" };
inline std::string metricXgmiLinkMetricTableTemplate{ "LINK_METRICS_TABLE:\n" };
inline std::string staticVramTemplate{ "    VRAM:\n"
	"        TYPE: %s \n"
	"        VENDOR: %s \n"
	"        SIZE: %s %s \n"
	"        BIT_WIDTH: %s \n" };

inline std::string staticCacheHeaderTemplate{ "    CACHE_INFO:\n" };
inline std::string staticCacheInfoTemplate{
	"        CACHE_%d:\n"
	"            CACHE_PROPERTIES: %s\n"
	"            CACHE_SIZE: %s %s\n"
	"            CACHE_LEVEL: %s\n"
	"            MAX_NUM_CU_SHARED: %s\n"
	"            NUM_CACHE_INSTANCE: %s\n" };

inline std::string staticPolicyHeaderTemplate{
	"    SOC_PSTATE:\n"
	"        NUM_SUPPORTED: %d\n"
	"        CURRENT_ID: %d\n"
	"        POLICIES:\n" };
inline std::string staticPolicyInfoTemplate{
	"            POLICY_ID: %d\n"
	"            POLICY_DESCRIPTION: %s\n" };

inline std::string eventTemplate{ "EVENT_INFO: \n"};
inline std::string eventMessageTemplate{ "GPU: %d \n"
	"    MESSAGE: %s \n"
	"    CATEGORY: %s \n"
	"    DATE: %s \n" };

inline std::string profileTemplate{
	"PROFILE_INFO: \n"
	"    VF_COUNT: %d \n"
	"    PROFILE_CAPS: \n"
	"        COMPUTE: \n"
	"            AVAILABLE: %llu %s\n"
	"            MAX: %llu %s\n"
	"            MIN: %llu %s\n"
	"            OPTIMAL: %llu %s\n"
	"            TOTAL: %llu %s\n"
	"        DECODE: \n"
	"            AVAILABLE: %llu \n"
	"            MAX: %llu \n"
	"            MIN: %llu \n"
	"            OPTIMAL: %llu \n"
	"            TOTAL: %llu \n"
	"        ENCODE: \n"
	"            AVAILABLE: %llu \n"
	"            MAX: %llu \n"
	"            MIN: %llu \n"
	"            OPTIMAL: %llu \n"
	"            TOTAL: %llu \n"
	"        MEMORY: \n"
	"            AVAILABLE: %llu %s\n"
	"            MAX: %llu %s\n"
	"            MIN: %llu %s\n"
	"            OPTIMAL: %llu %s\n"
	"            TOTAL: %llu %s\n"
	"    CURRENT_PROFILE: %s \n"};

inline std::string metricFbUsageTemplate {
	"    FB_USAGE:\n"
	"        FB_TOTAL: %s %s\n"
	"        FB_USED: %s %s\n"};

inline std::string staticPartitionTemplate {
	"    PARTITION: \n"
	"        ACCELERATOR_PARTITION: %s \n"
	"        MEMORY_PARTITION: %s \n"
	"        PARTITION_ID: %s \n"};