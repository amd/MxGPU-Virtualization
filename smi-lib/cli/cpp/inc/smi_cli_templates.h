/* * Copyright (C) 2023-2025 Advanced Micro Devices. All rights reserved.
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
#include <vector>

inline std::string gpuListTemplate{ "GPU: %d \n    BDF: %s \n    UUID: %s\n" };

inline std::string vfListTemplate{ "    VF: %d \n        BDF: %s\n        UUID: %s\n" };

inline std::string vfNestedTemplate{ "GPU: %s \n    VF: %s \n" };

inline std::string nicListTemplate{
	"NIC: %d \n"
	"    BDF: %s \n"
	"    PERMANENT_ADDRESS: %s\n"
	"    PRODUCT_NAME: %s\n"
	"    PART_NUMBER: %s\n"
	"    SERIAL_NUMBER: %s\n"
	"    VENDOR_NAME: %s\n"
};

inline std::string versionTemplate{ "VERSION: \n    TOOL_NAME: %s \n    TOOL_VERSION: %s \n    LIB_VERSION: %s \n    DRIVER_VERSION: %s \n" };

inline std::string staticAsicTemplate{
	"    ASIC: \n        MARKET_NAME: %s \n        VENDOR_ID: %s \n        VENDOR_NAME: %s \n        SUBVENDOR_ID: %s \n        DEVICE_ID: %s "
	"\n        SUBSYSTEM_ID: %s \n        REV_ID: %s \n        ASIC_SERIAL: %s\n        OAM_ID: %s\n        NUM_OF_COMPUTE_UNITS: %s\n"
};

inline std::string nicStaticAsicTemplate{
	"    ASIC: \n"
	"        VENDOR_ID: %s \n"
	"        SUBVENDOR_ID: %s \n"
	"        DEVICE_ID: %s \n"
	"        SUBSYSTEM_ID: %s \n"
	"        REVISION: %s \n"
	"        PERMANENT_ADDRESS: %s \n"
	"        PRODUCT_NAME: %s \n"
	"        PART_NUMBER: %s \n"
	"        SERIAL_NUMBER: %s \n"
	"        VENDOR_NAME: %s \n"
};

inline std::string nicStaticBusTemplate{
	"    BUS: \n"
	"        BDF: %s \n"
	"        MAX_PCIE_WIDTH: %s \n"
	"        MAX_PCIE_SPEED: %s %s\n"
	"        PCIE_INTERFACE_VERSION: %s \n"
	"        SLOT_TYPE: %s \n"
};

inline std::string nicStaticDriverTemplate{
	"    DRIVER: \n"
	"        NAME: %s \n"
	"        VERSION: %s \n"
};

inline std::string nicStaticNumaTemplate{
	"    NUMA: \n"
	"        NODE: %s \n"
	"        AFFINITY: %s\n"
};

inline std::string nicStaticPortHeaderTemplate{ "    PORTS:\n" };

inline std::string nicStaticPortTemplate{
	"        PORT_%u:\n"
	"            BDF: %s\n"
	"            PORT_NUM: %s\n"
	"            TYPE: %s\n"
	"            FLAVOUR: %s\n"
	"            NETDEV: %s\n"
	"            IFINDEX: %s\n"
	"            MAC_ADDRESS: %s\n"
	"            CARRIER: %s\n"
	"            MTU: %s %s\n"
	"            LINK_STATE: %s\n"
	"            LINK_SPEED: %s %s\n"
	"            ACTIVE_FEC: %s\n"
	"            AUTONEG: %s\n"
	"            PAUSE_AUTONEG: %s\n"
	"            PAUSE_RX: %s\n"
	"            PAUSE_TX: %s\n"
};

inline std::string nicStaticRdmaDevHeaderTemplate{ "    RDMA_DEVICES:\n" };

inline std::string nicStaticRdmaDevTemplate{
	"        RDMA_DEVICE_%u:\n"
	"            RDMA_DEV: %s \n"
	"            NODE_GUID: %s \n"
	"            NODE_TYPE: %s \n"
	"            SYS_IMAGE_GUID: %s\n"
	"            FW_VER: %s \n"
	"            PORTS:\n"
};

inline std::string nicStaticRdmaPortTemplate{
	"                PORT_%u:\n"
	"                    NETDEV: %s \n"
	"                    STATE: %s\n"
	"                    RDMA_PORT: %s\n"
	"                    MAX_MTU: %s\n"
	"                    ACTIVE_MTU: %s\n"
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
	"    IFWI: \n        NAME: %s \n        BUILD_DATE: %s "
	"\n        PART_NUMBER: %s \n        VERSION: %s\n        BOOT_FIRMWARE: %s\n"
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
	"        BAD_PAGE_THRESHOLD: %s\n"
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

inline std::string fwVfTemplate{
	"            FW_%d:\n                FW_ID: %s\n                FW_VERSION: %s\n"
};

inline std::string gpuTemplate{ "GPU: %d\n" };

inline std::string nicTemplate{ "NIC: %d\n" };

inline std::string fwErrorRecordListTemplate{ "    ERROR_RECORDS: \n" };

inline std::string fwErrorRecordsTemplate{
	"        ERROR_RECORD:\n             TIMESTAMP: %d\n             VF: %d\n"
	"             NAME: %d\n             STATUS: %d\n"
};

inline std::string metricPerPartitionTemplate{
	"        PER_PARTITION:\n"};

inline std::string AIDTemplate{
	"            AID_%s:\n"};

inline std::string XCPTemplate{
	"            XCP_%s:\n"};


inline std::string activityPerPartitionTemplate{
	"                VCN_ACTIVITY: %s %s\n"};

inline std::string TemperaturePerPartitionTemplate{
        "                TEMPERATURE: %s %s\n"};

inline std::string VCLKPerPartitionTemplate{
	"                CLK_VCLK: %s %s\n"};

inline std::string VCLKMinPerPartitionTemplate{
	"                CLK_VCLK_MIN_LIMIT: %s %s\n"};

inline std::string VCLKMaxPerPartitionTemplate{
	"                CLK_VCLK_MAX_LIMIT: %s %s\n"};

inline std::string DCLKPerPartitionTemplate{
	"                CLK_DCLK_LIMIT: %s %s\n"};

inline std::string DCLKMinPerPartitionTemplate{
	"                CLK_DCLK_MIN_LIMIT: %s %s\n"};

inline std::string DCLKMaxPerPartitionTemplate{
	"                CLK_DCLK_MAX_LIMIT: %s %s\n"};

inline std::string SCLKPerPartitionTemplate{
	"                CLK_SCLK_LIMIT: %s %s\n"};

inline std::string SCLKMinPerPartitionTemplate{
	"                CLK_SCLK_MIN_LIMIT: %s %s\n"};

inline std::string SCLKMaxPerPartitionTemplate{
	"                CLK_SCLK_MAX_LIMIT: %s %s\n"};

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

inline std::string metricJpegUsagePerPartitionHeaderTemplate{
	"                JPEG_ACTIVITY: ["};

inline std::string metricTempPerPartitionHeaderTemplate{
	"                TEMPERATURE: ["};

inline std::string metricHbmTempPerPartitionHeaderTemplate{
	"                HBM_TEMPERATURE: ["};

inline std::string metricGFXCLKPerPartitionHeaderTemplate{
	"                GFX_CLK: ["};

inline std::string metricGFXMinCLKPerPartitionHeaderTemplate{
	"                GFX_MIN_CLK: ["};

inline std::string metricGFXMaxCLKPerPartitionHeaderTemplate{
	"                GFX_MAX_CLK: ["};

inline std::string metricGFXLockedCLKPerPartitionHeaderTemplate{
	"                GFX_CLK_LOCKED: ["};

inline std::string metricGFXUsagePerPartitionHeaderTemplate{
	"                GFX_USAGE: ["};

inline std::string metricJpegUsageTemplate{
	"%s %s"};

inline std::string metricJpegUsagePerPartitionTemplate{
	"%s %s"};

inline std::string metricTempPerPartitionTemplate{
	"%s %s"};

inline std::string metricHbmTempPerPartitionTemplate{
	"%s %s"};

inline std::string GFXPerPartitionTemplate{
	"%s %s"};

inline std::string GFXMinPerPartitionTemplate{
	"%s %s"};

inline std::string GFXMaxPerPartitionTemplate{
	"%s %s"};

inline std::string GFXLockedPerPartitionTemplate{
	"%s"};

inline std::string GFXUsagePerPartitionTemplate{
	"%s %s"};

inline std::string TempXcdPerPartitionTemplate{
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
	"            GUARD_INFO:\n%s"
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

inline std::string topologyCoherentTemplate{ "COHERENT_TABLE:\n" };
inline std::string topologyAtomicsTemplate{ "ATOMICS_TABLE:\n" };
inline std::string topologyDmaTemplate{ "DMA_TABLE:\n" };
inline std::string topologyBiDirectionalTemplate{ "BI_DIRECTIONAL_TABLE:\n" };

inline std::string metricXgmiTemplate{ "    XGMI:\n" };
inline std::string metricXgmiLinkMetricTableTemplate{ "LINK_METRICS_TABLE:\n" };
inline std::string metricXgmiLinkStatusTableTemplate{ "SOURCE_GPU_XGMI_LINK_STATUS:\n" };

inline std::string staticVramTemplate{ "    VRAM:\n"
	"        TYPE: %s \n"
	"        VENDOR: %s \n"
	"        SIZE: %s %s \n"
	"        BIT_WIDTH: %s \n"
	"        MAX_BANDWIDTH: %s %s \n" };

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
	"        NUM_SUPPORTED: %s\n"
	"        CURRENT_ID: %s\n"
	"        POLICIES:\n" };
inline std::string staticPlpdsHeaderTemplate{
	"    XGMI_PLPD:\n"
	"        NUM_SUPPORTED: %d\n"
	"        CURRENT_ID: %d\n"
	"        POLICIES:\n" };
inline std::string staticPolicyInfoTemplate{
	"            POLICY_ID: %s\n"
	"            POLICY_DESCRIPTION: %s\n" };

inline std::string staticVirtualizationModeTemplate{
	"    MODE: %s\n" };

inline std::string staticCpuListTemplate{
	"            CPU_LIST_%d:\n"
	"                BITMASK: %016lx\n"
	"                CORE_RANGE: %s\n"};

inline std::string staticNumaTemplate{
	"    NUMA:\n"
	"        NODE: %d\n"
	"        CPU_AFFINITY:\n%s"
	"        SOCKET_AFFINITY: N/A\n" };

inline std::string staticNumaTemplate_NA{
	"    NUMA:\n"
	"        NODE: %s\n"
	"        CPU_AFFINITY: N/A\n"
	"        SOCKET_AFFINITY: N/A\n" };

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

inline std::string RasCperTemplate {
	    "%-24s %-8d %-24s %-24s %s\n"};

inline std::string setSuccessfullyTemplate {
	"GPU: %d\n"
	"    %s: Successfully set %s to %s\n"};

inline std::string NodeHeaderTemplate {	"NODE: \n"};

inline std::string BaseBoardHeaderTemplate{ "    BASEBOARD:\n" };

inline std::string baseboardSystemTempUbbFpgaTemplate{ "        UBB_FPGA: %s %s\n" };
inline std::string baseboardSystemTempUbbFrontTemplate{ "        UBB_FRONT: %s %s\n" };
inline std::string baseboardSystemTempUbbBackTemplate{ "        UBB_BACK: %s %s\n" };
inline std::string baseboardSystemTempUbbOam7Template{ "        UBB_OAM7: %s %s\n" };
inline std::string baseboardSystemTempUbbIbcTemplate{ "        UBB_IBC: %s %s\n" };
inline std::string baseboardSystemTempUbbUfpgaTemplate{ "        UBB_UFPGA: %s %s\n" };
inline std::string baseboardSystemTempUbbOam1Template{ "        UBB_OAM1: %s %s\n" };
inline std::string baseboardSystemTempOam01HscTemplate{ "        OAM_0_1_HSC: %s %s\n" };
inline std::string baseboardSystemTempOam23HscTemplate{ "        OAM_2_3_HSC: %s %s\n" };
inline std::string baseboardSystemTempOam45HscTemplate{ "        OAM_4_5_HSC: %s %s\n" };
inline std::string baseboardSystemTempOam67HscTemplate{ "        OAM_6_7_HSC: %s %s\n" };
inline std::string baseboardSystemTempUbbFpga0v72VrTemplate{ "        UBB_FPGA_0V72_VR: %s %s\n" };
inline std::string baseboardSystemTempUbbFpga3v3VrTemplate{ "        UBB_FPGA_3V3_VR: %s %s\n" };
inline std::string baseboardSystemTempRetimer01231v2VrTemplate{ "        RETIMER_0_1_2_3_1V2_VR: %s %s\n" };
inline std::string baseboardSystemTempRetimer45671v2VrTemplate{ "        RETIMER_4_5_6_7_1V2_VR: %s %s\n" };
inline std::string baseboardSystemTempRetimer010v9VrTemplate{ "        RETIMER_0_1_0V9_VR: %s %s\n" };
inline std::string baseboardSystemTempRetimer450v9VrTemplate{ "        RETIMER_4_5_0V9_VR: %s %s\n" };
inline std::string baseboardSystemTempRetimer230v9VrTemplate{ "        RETIMER_2_3_0V9_VR: %s %s\n" };
inline std::string baseboardSystemTempRetimer670v9VrTemplate{ "        RETIMER_6_7_0V9_VR: %s %s\n" };
inline std::string baseboardSystemTempOam01233v3VrTemplate{ "        OAM_0_1_2_3_3V3_VR: %s %s\n" };
inline std::string baseboardSystemTempOam45673v3VrTemplate{ "        OAM_4_5_6_7_3V3_VR: %s %s\n" };
inline std::string baseboardSystemTempIbcHscTemplate{ "        IBC_HSC: %s %s\n" };
inline std::string baseboardSystemTempIbcTemplate{ "        IBC: %s %s\n" };

inline std::string GpuBoardHeaderTemplate{ "    GPUBOARD:\n" };

inline std::string gpuboardNodeTempRetimerTemplate{ "        NODE_TEMP_RETIMER: %s %s\n" };
inline std::string gpuboardNodeTempIbcTempTemplate{ "        NODE_TEMP_IBC_TEMP: %s %s\n" };
inline std::string gpuboardNodeTempIbc2TempTemplate{ "        NODE_TEMP_IBC_2_TEMP: %s %s\n" };
inline std::string gpuboardNodeTempVdd18VrTempTemplate{ "        NODE_TEMP_VDD18_VR_TEMP: %s %s\n" };
inline std::string gpuboardNodeTemp04HbmBVrTempTemplate{ "        NODE_TEMP_04_HBM_B_VR_TEMP: %s %s\n" };
inline std::string gpuboardNodeTemp04HbmDVrTempTemplate{ "        NODE_TEMP_04_HBM_D_VR_TEMP: %s %s\n" };
inline std::string gpuboardVrTempVddcrVdd0Template{ "        VR_TEMP_VDDCR_VDD0: %s %s\n" };
inline std::string gpuboardVrTempVddcrVdd1Template{ "        VR_TEMP_VDDCR_VDD1: %s %s\n" };
inline std::string gpuboardVrTempVddcrVdd2Template{ "        VR_TEMP_VDDCR_VDD2: %s %s\n" };
inline std::string gpuboardVrTempVddcrVdd3Template{ "        VR_TEMP_VDDCR_VDD3: %s %s\n" };
inline std::string gpuboardVrTempVddcrSocATemplate{ "        VR_TEMP_VDDCR_SOC_A: %s %s\n" };
inline std::string gpuboardVrTempVddcrSocCTemplate{ "        VR_TEMP_VDDCR_SOC_C: %s %s\n" };
inline std::string gpuboardVrTempVddcrSocioATemplate{ "        VR_TEMP_VDDCR_SOCIO_A: %s %s\n" };
inline std::string gpuboardVrTempVddcrSocioCTemplate{ "        VR_TEMP_VDDCR_SOCIO_C: %s %s\n" };
inline std::string gpuboardVrTempVdd085HbmTemplate{ "        VR_TEMP_VDD_085_HBM: %s %s\n" };
inline std::string gpuboardVrTempVddcr11HbmBTemplate{ "        VR_TEMP_VDDCR_11_HBM_B: %s %s\n" };
inline std::string gpuboardVrTempVddcr11HbmDTemplate{ "        VR_TEMP_VDDCR_11_HBM_D: %s %s\n" };
inline std::string gpuboardVrTempVddUsrTemplate{ "        VR_TEMP_VDD_USR: %s %s\n" };
inline std::string gpuboardVrTempVddio11E32Template{ "        VR_TEMP_VDDIO_11_E32: %s %s\n" };

inline std::string metricPortHeaderTemplate {
	"    PORT: \n"
	"        INDEX: %s\n"
	"        NETDEV: \n"
	"            NAME: %s\n"
	"            STATISTICS: \n"};
inline std::string metricNicPortStatsHeaderTemplate {
	"    PORTS:\n"};

inline std::string metricNicPortTemplate {
	"        PORT_%u:\n"
	"            NETDEV: %s\n"};

inline std::string metricNicVendorStatsHeaderTemplate {
	"            VENDOR_STATISTICS:\n"};

inline std::string metricNicPortStatisticsHeaderTemplate {
	"            STATISTICS:\n"};

inline std::string metricNicRdmaStatsHeaderTemplate {
	"    RDMA_DEVICES:\n"};

inline std::string metricNicRdmaDeviceTemplate {
	"            RDMA_DEVICE_%u:\n"
	"                RDMA_DEV: %s\n"
	"                PORTS:\n"};

inline std::string metricNicRdmaPortTemplate {
	"                    PORT_%u:\n"
	"                        STATISTICS:\n"};

inline std::string rasPolicyTemplate{
	"    POLICY:\n"
	"        MAJOR_VERSION: %s \n"
	"        MINOR_VERSION: %s \n"
	"        DRAM_NON_CRITICAL_REGION_THRESHOLD: %s \n"
	"        DRAM_CRITICAL_REGION_THRESHOLD: %s \n"
};

inline std::string nodePowerManagementTemplate {
	"    POWER_MANAGEMENT: \n"
	"        LIMIT: %s W\n"
	"        STATUS: %s \n"
};
