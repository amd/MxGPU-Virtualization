/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef SMU_V15_0_8_PMFW_H
#define SMU_V15_0_8_PMFW_H

/** @defgroup FixedPointFormats Telemetry Fixed-Point Formats
 * @brief Q-format conventions used in metrics and telemetry fields
 *
 * Fixed-point values use a fixed number of fractional bits. To convert to
 * real units, divide the stored integer by 2^(fractional_bits).
 * - **Q10**: Signed, 10 fractional bits. Real value = stored_value / 1024.
 *   Used for temperature (Celsius) where value can be negative.
 * - **UQ10**: Unsigned Q10. Real value = stored_value / 1024.
 *   Used for power (Watts), frequency (MHz), percentages, bandwidth.
 * - **UQ16**: Unsigned, 16 fractional bits. Real value = stored_value / 65536.
 *   Used for energy (Joules) in 64-bit accumulators.
 * @{
 */
/** @} */

/** @defgroup DPMLevels DPM and Configuration Table Sizes
 * @brief DPM level counts and table dimensions
 *
 * DPM provides automatic voltage-frequency scaling based on workload demands,
 * thermal conditions, and power constraints.
 * @{
 */
#define NUM_UCLK_DPM_LEVELS   4   /**< Memory (HBM) clock DPM levels: frequency states for bandwidth optimization */
#define NUM_XGMI_DPM_LEVELS   2   /**< XGMI link DPM levels: inter-GPU interconnect power/bandwidth states */
#define NUM_PCIE_BITRATES     4   /**< PCIe bitrate table entries: Gen1 to Gen5+ speed configurations */
#define NUM_XGMI_BITRATES     4   /**< XGMI bitrate table entries: link speed configurations */
#define NUM_XGMI_WIDTHS       3   /**< XGMI width configurations: lane count options for power/bandwidth trade-off */
#define NUM_GFX_P2S_TABLES    8   /**< Number of GFX P2S table groups (voltage guardband vs frequency and temperature) */
#define NUM_PSM_DIDT_THRESHOLDS  3    /**< Number of PSM DIDT thresholds (droop detection sensitivity levels for distributed SQ throttler) */
#define NUM_XCD_XVMIN_VMIN_THRESHOLDS 3  /**< XCD Xvmin thresholds: minimum voltage levels for compute die */
#define NUM_AID_XVMIN_VMIN_THRESHOLDS 3  /**< AID Xvmin thresholds: minimum voltage levels for accelerator I/O die */
#define NUM_PPT_POINTS    4  /**< Number of Package Power Tracking (PPT) curve points. */
/** @} */

/** @defgroup ProductInfo FRU Product Information String Lengths
 * @brief FRU product identification string sizes
 * @{
 */
#define PRODUCT_MODEL_NUMBER_LEN      20  /**< Model number string length */
#define PRODUCT_NAME_LEN              64  /**< Product name string length */
#define PRODUCT_SERIAL_LEN            20  /**< Serial number string length */
#define PRODUCT_MANUFACTURER_NAME_LEN 32  /**< Manufacturer name string length */
#define PRODUCT_FRU_ID_LEN            32  /**< FRU ID string length */
/** @} */

/** @defgroup FeatureIDs Power Management Feature Identifiers
 * @brief Feature control IDs for SMC message-based enable/disable operations
 *
 * These identifiers are used by the driver to enable or disable specific power
 * management features via SMC mailbox messages. Each feature represents a
 * distinct power management algorithm or hardware control function.
 * @{
 */
#define FEATURE_ID_DATA_CALCULATION       1   /**< Telemetry data processing: converts raw sensor data to engineering units, computes power/thermal/performance metrics, provides foundation for all PM decisions */
#define FEATURE_ID_DPM_GFXCLK             2   /**< Graphics DPM: primary DVFS for GPU compute domain, adjusts frequency/voltage based on workload, thermal conditions, and power constraints */
#define FEATURE_ID_DPM_FCLK               3   /**< Fabric clock DPM: dynamic frequency scaling for Infinity Fabric interconnect */
#define FEATURE_ID_LATCHUP_CONTROLLER     4   /**< Latchup controller: temperature compensated max voltage limiter for VDDX*/
#define FEATURE_ID_DPM_SPARE_5            5   /**< Reserved for future use */
#define FEATURE_ID_DPM_UCLK               6   /**< Memory clock DPM: UCLK frequency scaling for HBM bandwidth optimization */
#define FEATURE_ID_DPM_SPARE_7            7   /**< Reserved for future use */
#define FEATURE_ID_DPM_XGMI               8   /**< XGMI link DPM: inter-GPU link power/bandwidth management */
#define FEATURE_ID_DS_FCLK                9   /**< Fabric clock Deep Sleep: gates FCLK during idle to reduce leakage power */
#define FEATURE_ID_DS_GFXCLK              10  /**< Graphics clock Deep Sleep: gates GFXCLK during GPU idle for power savings */
#define FEATURE_ID_DS_LCLK                11  /**< Link clock Deep Sleep: gates PCIe/XGMI link clocks during idle periods */
#define FEATURE_ID_DS_MP0CLK              12  /**< MP0 clock Deep Sleep: power gating for security processor */
#define FEATURE_ID_DS_MP1CLK              13  /**< MP1 clock Deep Sleep: power gating for system management unit */
#define FEATURE_ID_DS_MPIOCLK             14  /**< MPIO clock Deep Sleep: gates multi-purpose I/O clocks when idle */
#define FEATURE_ID_DS_SOCCLK              15  /**< SoC clock Deep Sleep: gates SOC domain clocks during idle */
#define FEATURE_ID_DS_VCN                 16  /**< VCN Deep Sleep: power gates video encoder/decoder when idle */
#define FEATURE_ID_PPT                    17  /**< PPT: monitors total socket power, enforces sustained/boost/peak power limits, triggers throttling when limits approached or exceeded */
#define FEATURE_ID_TDC                    18  /**< TDC: monitors VRM current draw, prevents overcurrent conditions, protects power delivery from exceeding electrical specifications */
#define FEATURE_ID_THERMAL                19  /**< Thermal management: multi-sensor temperature monitoring and control, hierarchical thermal zones with independent control policies */
#define FEATURE_ID_SOC_PCC                20  /**< SoC PCC: manages SoC-level power states, selectively enables/gates SOC blocks (PCIe, memory pads, fabric) based on usage and thermal metrics */
#define FEATURE_ID_PROCHOT                21  /**< PROCHOT: responds to external processor hot signal for emergency throttling */
#define FEATURE_ID_XVMIN0_VMIN_AID        22  /**< AID Xvmin0: adaptive voltage margining for AID, dynamically adjusts minimum voltage based on temperature/workload/aging */
#define FEATURE_ID_XVMIN1_DD_AID          23  /**< AID Xvmin1 Droop Detection: monitors voltage droops in AID domain, triggers compensation when droop thresholds exceeded */
#define FEATURE_ID_XVMIN0_VMIN_XCD        24  /**< XCD Xvmin0: adaptive voltage margining for XCD, optimizes power by finding lowest safe operating voltage */
#define FEATURE_ID_XVMIN1_DD_XCD          25  /**< XCD Xvmin1 Droop Detection: monitors voltage droops in XCD domain, coordinates with Clock Stretch Compensation for stability */
#define FEATURE_ID_FW_CTF                 26  /**< Firmware CTF: emergency shutdown when die temperature exceeds critical threshold to prevent hardware damage */
#define FEATURE_ID_MGCG                   27  /**< MGCG: IP-block level clock gating for power savings */
#define FEATURE_ID_PSI7                   28  /**< PSI7 power state: low-power VRM operating mode for improved efficiency */
#define FEATURE_ID_XGMI_PER_LINK_PWR_DOWN 29  /**< XGMI per-link power down: individually power gates unused XGMI links */
#define FEATURE_ID_SPARE_30               30  /**< Reserved for future use */
#define FEATURE_ID_GFX_DC_RTC             31  /**< GFX DC RTC: DC-to-AC voltage adjustment based on workload */
#define FEATURE_ID_SPARE_32               32  /**< Reserved for future use */
#define FEATURE_ID_PRC                    33  /**< PRC: tracks time spent in PCC states, aggregates real-time usage data for granular power management decisions */
#define FEATURE_ID_PSM_DIDT               34  /**< Distributed PSM-based SQ Throttler: local voltage droop control per SQC tile, leverages AVFS PSM counters to stall Shader Queue during droop conditions */
#define FEATURE_ID_PIT                    35  /**< Predictive Instruction Throttler: 2-tier DIDT mitigation (local WGP + global SE), predicts potential droops from instruction power signatures in Shader Queue */
#define FEATURE_ID_DVO                    36  /**< DVO: adds/subtracts voltage offsets in real-time based on load transients, temperature, and clock-stretch feedback */
#define FEATURE_ID_XVMIN_CLKSTOP_DS       37  /**< Xvmin clock stop deep sleep: places shader blocks into deep-sleep based on PSM or Xvmin signals */
#define FEATURE_ID_SPARE_38               38  /**< Reserved for future use */
#define FEATURE_ID_DPM_GL2CLK             39  /**< GL2 cache clock DPM: frequency scaling for L2 cache subsystem */
#define FEATURE_ID_GC_CAC_EDC             40  /**< Graphics Core CAC/EDC: activity counting and error detection for GFX domain power and reliability monitoring */
#define FEATURE_ID_DS_DMABECLK            41  /**< DMABEC clock Deep Sleep: gates DMA back-end controller clock when idle */
#define FEATURE_ID_DS_MPIFOECLK           42  /**< MPIFOE clock Deep Sleep: gates multi-purpose I/O front-end clock */
#define FEATURE_ID_DS_MPRASCLK            43  /**< MPRAS clock Deep Sleep: gates MP RAS clock when idle */
#define FEATURE_ID_DS_MPNHTCLK            44  /**< MPNHT clock Deep Sleep: gates MP NHT clock when idle */
#define FEATURE_ID_DS_FIOCLK              45  /**< FIO clock Deep Sleep: gates fabric I/O clock when idle */
#define FEATURE_ID_DS_DXIOCLK             46  /**< DXIO clock Deep Sleep: gates DXIO (PCIe/XGMI PHY) clock when idle */
#define FEATURE_ID_PCC                    47  /**< VDD PCC: manages power states for VDD domain, fine-grained control over individual compute units based on activity levels */
#define FEATURE_ID_OCP                    48  /**< OCP: monitors and limits current to prevent VRM damage */
#define FEATURE_ID_TRO                    49  /**< TRO: adjusts thermal response based on workload patterns */
#define FEATURE_ID_GL2_CAC_EDC            50  /**< GL2 CAC/EDC: activity counting and throttling for GL2 cache domain */
#define FEATURE_ID_SPARE_51               51  /**< Reserved for future use */
#define FEATURE_ID_GL2_CGCG               52  /**< GL2 CGCG: aggressive clock gating for GL2 cache */
#define FEATURE_ID_XCAC                   53  /**< XCAC: comprehensive activity counting across all domains for accurate power estimation and throttling decisions */
#define FEATURE_ID_DS_GL2CLK              54  /**< GL2 clock Deep Sleep: gates GL2 cache clock during idle for power savings */
#define FEATURE_ID_FCS_VIN_PCC            55  /**< FCS VIN: fast current scaling for voltage input */
#define FEATURE_ID_FCS_VDDX_OCP_WARN      56  /**< FCS VDDX OCP warning: early warning for VDD overcurrent conditions */
#define FEATURE_ID_FCS_PWRBRK             57  /**< PWRBRK: rapid power-limiting mechanism, engages within microseconds to prevent power limit violations and VRM damage */
#define FEATURE_ID_DF_CSTATE              58  /**< Data Fabric C-state: low-power states for fabric when idle */
#define FEATURE_ID_ARO                    59  /**< ARO: optimizes voltage regulator response */
#define FEATURE_ID_POWER_STABILIZATION    60  /**< Feature ID for power stabilization / power floor. */
#define FEATURE_ID_SPARE_61               61  /**< Reserved for future use */
#define FEATURE_ID_OCPWARNRC              62  /**< OCP warning response control: configurable response to overcurrent warnings */
#define FEATURE_ID_XGMI_FOLDING           63  /**< XGMI link folding: reduces active XGMI lanes for power savings during low traffic */
#define FEATURE_ID_SPARE_64               64  /**< Reserved for future use */
#define NUM_FEATURES                      65  /**< Total number of feature slots */
/** @} */

/** @defgroup MGCGFeatures MGCG Feature IDs
 * @brief Sub-feature identifiers for MGCG control
 *
 * MGCG provides IP-block level clock gating for power savings during idle periods.
 * Each IP block can be independently gated to reduce dynamic and leakage power
 * while maintaining rapid wake-up capability.
 * @{
 */
#define WAFL_CG                 0  /**< WAFL clock gating: gates inter-die fabric links */
#define SMU_FUSE_CG_DEEPSLEEP   1  /**< SMU fuse clock gating during deep sleep: reduces SMU power in low-power states */
#define SMUIO_CG                2  /**< SMU I/O clock gating: gates SMU peripheral I/O interfaces */
#define RSMU_MGCG               3  /**< RSMU MGCG: gates remote SMU instances */
#define SMU_CLK_MGCG            4  /**< SMU clock MGCG: gates internal SMU clocks during idle */
#define MP5_CG                  5  /**< MP5 clock gating: gates auxiliary management processor */
#define UMC_CG                  6  /**< UMC clock gating: gates HBM controller when idle */
#define WAFL0_CLK               7  /**< WAFL0 clock control: gating for first WAFL link instance */
#define WAFL1_CLK               8  /**< WAFL1 clock control: gating for second WAFL link instance */
#define VCN_MGCG                9  /**< VCN MGCG: gates video encoder/decoder when not processing */
#define GL2_MGCG                10 /**< GL2 MGCG: gates L2 cache clocks during low activity */
#define MGCG_NUM_FEATURES       11 /**< Total number of MGCG features */
/** @} */

/**
 * @enum PCIE_LINK_SPEED_INDEX_TABLE_e
 * @brief PCIe Link Generation Speed Indices
 *
 * Enumeration of PCIe generation speeds supported by the platform.
 * Used for MPIO PCIe generation speed message handling.
 */
typedef enum {
  PCIE_LINK_SPEED_INDEX_TABLE_GEN1,      /**< PCIe Gen1: 2.5 GT/s */
  PCIE_LINK_SPEED_INDEX_TABLE_GEN2,      /**< PCIe Gen2: 5.0 GT/s */
  PCIE_LINK_SPEED_INDEX_TABLE_GEN3,      /**< PCIe Gen3: 8.0 GT/s */
  PCIE_LINK_SPEED_INDEX_TABLE_GEN4,      /**< PCIe Gen4: 16.0 GT/s */
  PCIE_LINK_SPEED_INDEX_TABLE_GEN5,      /**< PCIe Gen5: 32.0 GT/s */
  PCIE_LINK_SPEED_INDEX_TABLE_GEN6,      /**< PCIe Gen6: 64.0 GT/s */
  PCIE_LINK_SPEED_INDEX_TABLE_GEN6_ESM,  /**< PCIe Gen6 ESM */
  PCIE_LINK_SPEED_INDEX_TABLE_COUNT      /**< Total number of PCIe speed entries */
} PCIE_LINK_SPEED_INDEX_TABLE_e;

typedef enum{
  SYSTEM_TEMP_UBB_FPGA,                     /**< UBB FPGA temperature */
  SYSTEM_TEMP_UBB_FRONT,                    /**< UBB front temperature */
  SYSTEM_TEMP_UBB_BACK,                     /**< UBB back temperature */
  SYSTEM_TEMP_UBB_OAM7,                     /**< UBB OAM7 module temperature */
  SYSTEM_TEMP_UBB_IBC,                      /**< UBB IBC temperature */
  SYSTEM_TEMP_UBB_UFPGA,                    /**< UBB UFPGA temperature */
  SYSTEM_TEMP_UBB_OAM1,                     /**< UBB OAM1 module temperature */
  SYSTEM_TEMP_OAM_0_1_HSC,                  /**< OAM 0-1 Hot-Swap Controller temperature */
  SYSTEM_TEMP_OAM_2_3_HSC,                  /**< OAM 2-3 Hot-Swap Controller temperature */
  SYSTEM_TEMP_OAM_4_5_HSC,                  /**< OAM 4-5 Hot-Swap Controller temperature */
  SYSTEM_TEMP_OAM_6_7_HSC,                  /**< OAM 6-7 Hot-Swap Controller temperature */
  SYSTEM_TEMP_UBB_FPGA_0V72_VR,             /**< UBB FPGA 0.72V voltage regulator temperature */
  SYSTEM_TEMP_UBB_FPGA_3V3_VR,              /**< UBB FPGA 3.3V voltage regulator temperature */
  SYSTEM_TEMP_RETIMER_0_1_2_3_1V2_VR,       /**< Retimers 0-3 1.2V VR temperature */
  SYSTEM_TEMP_RETIMER_4_5_6_7_1V2_VR,       /**< Retimers 4-7 1.2V VR temperature */
  SYSTEM_TEMP_RETIMER_0_1_0V9_VR,           /**< Retimers 0-1 0.9V VR temperature */
  SYSTEM_TEMP_RETIMER_4_5_0V9_VR,           /**< Retimers 4-5 0.9V VR temperature */
  SYSTEM_TEMP_RETIMER_2_3_0V9_VR,           /**< Retimers 2-3 0.9V VR temperature */
  SYSTEM_TEMP_RETIMER_6_7_0V9_VR,           /**< Retimers 6-7 0.9V VR temperature */
  SYSTEM_TEMP_OAM_0_1_2_3_3V3_VR,           /**< OAM 0-3 3.3V VR temperature */
  SYSTEM_TEMP_OAM_4_5_6_7_3V3_VR,           /**< OAM 4-7 3.3V VR temperature */
  SYSTEM_TEMP_IBC_HSC,                      /**< IBC Hot-Swap Controller temperature */
  SYSTEM_TEMP_IBC,                          /**< IBC temperature */
  SYSTEM_TEMP_MAX_ENTRIES   = 32            /**< Maximum system temperature entries */
} SYSTEM_TEMP_e;

/**
 * @enum NODE_TEMP_e
 * @brief Node-Level Temperature Sensor Indices
 *
 * Temperature sensors specific to individual compute node components including
 * retimers, interconnects, and voltage regulators.
 */
typedef enum{
  NODE_TEMP_RETIMER,                        /**< Node retimer temperature */
  NODE_TEMP_IBC_TEMP,                       /**< Node IBC primary temperature */
  NODE_TEMP_IBC_2_TEMP,                     /**< Node IBC secondary temperature */
  NODE_TEMP_VDD18_VR_TEMP,                  /**< 1.8V voltage regulator temperature */
  NODE_TEMP_04_HBM_B_VR_TEMP,               /**< HBM-B 0.4V voltage regulator temperature */
  NODE_TEMP_04_HBM_D_VR_TEMP,               /**< HBM-D 0.4V voltage regulator temperature */
  NODE_TEMP_MAX_TEMP_ENTRIES    = 12        /**< Maximum node temperature entries */
} NODE_TEMP_e;

/**
 * @enum SVI_TEMP_e
 * @brief SVI plane temperature indices
 *
 * Temperature sensors for voltage regulator modules on different SVI planes.
 * Covers compute (X0/X1), HBM memory, XGMI/GTA links, UCIE interconnect,
 * and SoC I/O power planes.
 */
typedef enum {
  SVI_PLANE_VDDCR_X0_TEMP,                  /**< Compute die X0 voltage regulator temperature */
  SVI_PLANE_VDDCR_X1_TEMP,                  /**< Compute die X1 voltage regulator temperature */

  SVI_PLANE_VDDIO_HBM_B_TEMP,               /**< HBM-B I/O voltage regulator temperature */
  SVI_PLANE_VDDIO_HBM_D_TEMP,               /**< HBM-D I/O voltage regulator temperature */
  SVI_PLANE_VDDIO_04_HBM_B_TEMP,            /**< HBM-B 0.4V I/O VR temperature */
  SVI_PLANE_VDDIO_04_HBM_D_TEMP,            /**< HBM-D 0.4V I/O VR temperature */
  SVI_PLANE_VDDCR_HBM_B_TEMP,               /**< HBM-B core voltage regulator temperature */
  SVI_PLANE_VDDCR_HBM_D_TEMP,               /**< HBM-D core voltage regulator temperature */
  SVI_PLANE_VDDCR_075_HBM_B_TEMP,           /**< HBM-B 0.75V core VR temperature */
  SVI_PLANE_VDDCR_075_HBM_D_TEMP,           /**< HBM-D 0.75V core VR temperature */

  SVI_PLANE_VDDIO_11_GTA_A_TEMP,            /**< GTA-A 1.1V I/O voltage regulator temperature */
  SVI_PLANE_VDDIO_11_GTA_C_TEMP,            /**< GTA-C 1.1V I/O voltage regulator temperature */
  SVI_PLANE_VDDAN_075_GTA_A_TEMP,           /**< GTA-A 0.75V analog VR temperature */
  SVI_PLANE_VDDAN_075_GTA_C_TEMP,           /**< GTA-C 0.75V analog VR temperature */

  SVI_PLANE_VDDCR_075_UCIE_TEMP,            /**< UCIE 0.75V core voltage regulator temperature */
  SVI_PLANE_VDDIO_065_UCIEAA_TEMP,          /**< UCIE-AA 0.65V I/O VR temperature */
  SVI_PLANE_VDDIO_065_UCIEAM_A_TEMP,        /**< UCIE-AM-A 0.65V I/O VR temperature */
  SVI_PLANE_VDDIO_065_UCIEAM_C_TEMP,        /**< UCIE-AM-C 0.65V I/O VR temperature */

  SVI_PLANE_VDDCR_SOCIO_A_TEMP,             /**< SoC I/O-A voltage regulator temperature */
  SVI_PLANE_VDDCR_SOCIO_C_TEMP,             /**< SoC I/O-C voltage regulator temperature */

  SVI_PLANE_VDDAN_075_TEMP,                 /**< 0.75V analog voltage regulator temperature */
  SVI_MAX_TEMP_ENTRIES,                     /**< Maximum SVI temperature entries (22) */
} SVI_TEMP_e;

/**
 * @enum SYSTEM_POWER_e
 * @brief System Power Measurement Indices
 *
 * System-level power measurements including baseboard power and thresholds.
 */
typedef enum{
  SYSTEM_POWER_UBB_POWER,                   /**< UBB current power consumption */
  SYSTEM_POWER_UBB_POWER_THRESHOLD,         /**< UBB power threshold limit */
  SYSTEM_POWER_MAX_ENTRIES_WO_RESERVED,     /**< Maximum entries without reserved space */
  SYSTEM_POWER_MAX_ENTRIES  = 4             /**< Total entries with reserved space */
} SYSTEM_POWER_e;

/** @brief Metrics table version identifier for driver compatibility
 * NOT CURRENTLY USED BY DRIVER, BUT INCREMENT FOR ANY UPDATES TO THE TABLE STRUCTURE
*/
#define SMU_METRICS_TABLE_VERSION 0xF

#pragma pack(push, 4)
/**
 * @struct MetricsTable_t
 * @brief Socket-Level Performance and Power Telemetry
 *
 * Comprehensive telemetry structure providing real-time and accumulated metrics
 * for a single GPU socket. The table is maintained by firmware through periodic
 * background updates and on-demand updates when exported to driver/APML.
 *
 * All accumulated (Acc) values are monotonically increasing 64-bit counters that
 * wrap on overflow. Instantaneous values represent current samples at last update.
 *
 * @note Structure is 4-byte aligned via pragma pack
 * @note AccumulationCounter increments each update; use for staleness detection
 * @note Temperature/power/frequency use fixed-point: see @ref FixedPointFormats (Q10, UQ10, UQ16).
 */
typedef struct {
  uint64_t AccumulationCounter;             /**< Incremented every time the accumulator values are updated in this table */

  /** @name Temperature Sensors
   * Temperature measurements in Celsius Q10 format (instantaneous and accumulated)
   * @{
   */
  uint32_t MaxSocketTemperature;            /**< Maximum temperature reported by all on-die thermal sensors on all AIDs, MIDs and XCDs in the socket (Q10 Celsius) */
  uint32_t MaxVrTemperature;                /**< Maximum temperature reported by SVI3 telemetry for all slave addresses (Q10 Celsius) */
  uint32_t HbmTemperature[12];              /**< Temperature reported by each HBM stack in the socket (12 stacks, Q10 Celsius) */
  uint64_t MaxSocketTemperatureAcc;         /**< Accumulated version of MaxSocketTemperature for averaging */
  uint64_t MaxVrTemperatureAcc;             /**< Accumulated version of MaxVrTemperature for averaging */
  uint64_t HbmTemperatureAcc[12];           /**< Accumulated per-stack HBM temperatures for averaging */
  uint32_t MidTemperature[2];               /**< MID temperatures (2 MIDs, Q10 Celsius) */
  uint32_t AidTemperature[2];               /**< AID temperatures (2 AIDs, Q10 Celsius) */
  uint32_t XcdTemperature[8];               /**< XCD temperatures (8 XCDs, Q10 Celsius) */
  /** @} */

  /** @name Power Management
   * Power limits and consumption in Watts (UQ10 format)
   * @{
   */
  uint32_t SocketPowerLimit;                /**< Power limit currently being enforced by the power throttling controller (UQ10 Watts) */
  uint32_t SocketPower;                     /**< Power consumption of all die in the socket (AID+MID+XCD+HBM) (UQ10 Watts) */
  /** @} */

  /** @name Energy Accounting
   * Energy consumption tracking (UQ16 format for Joules)
   * @{
   */
  uint64_t Timestamp;                       /**< Timestamp corresponding to the energy accumulators in 10ns units */
  uint64_t SocketEnergyAcc;                 /**< Energy accumulator of all die in the socket (AID+MID+XCD+HBM) (UQ16 Joules) */
  uint64_t HbmEnergyAcc;                    /**< Energy accumulator of all HBM stacks in the socket (UQ16 Joules) */
  /** @} */

  /** @name Clock Frequencies
   * Frequency measurements in MHz (UQ10 format, instantaneous and accumulated)
   * @{
   */
  uint32_t GfxclkFrequencyLimit;            /**< Minimum GFXCLK frequency limit enforced from the infrastructure controllers (UQ10 MHz) */
  uint32_t FclkFrequency[2];                /**< Effective FCLK frequency per AID (UQ10 MHz) */
  uint32_t UclkFrequency[2];                /**< Effective UCLK frequency per AID (UQ10 MHz) */
  uint64_t GfxclkFrequencyAcc[8];           /**< GFXCLK frequency accumulator for each XCD, for calculating average (UQ10 MHz) */
  uint32_t GfxclkFrequency[8];              /**< Effective GFXCLK frequency per XCD (UQ10 MHz) */
  uint32_t SocclkFrequency[2];              /**< Effective SOCCLK frequency per MID (UQ10 MHz) */
  uint32_t VclkFrequency[4];                /**< Effective VCLK frequency per VCN instance (2 per MID) (UQ10 MHz) */
  uint32_t DclkFrequency[4];                /**< Effective DCLK frequency per VCN instance (2 per MID) (UQ10 MHz) */
  uint32_t LclkFrequency[2];                /**< Effective LCLK frequency per MID (UQ10 MHz) */
  /** @} */

  /** @name XGMI Interconnect
   * High-speed inter-chip link metrics
   * @{
   */
  uint32_t XgmiWidth;                       /**< Current operating XGMI link width (bus width) */
  uint32_t XgmiBitrate;                     /**< Current operating XGMI link bitrate (Gbps, UQ10) */
  uint64_t XgmiReadBandwidthAcc;            /**< XGMI read bandwidth accumulator for links in the local socket (UQ10 GB/sec) */
  uint64_t XgmiWriteBandwidthAcc;           /**< XGMI write bandwidth accumulator for links in the local socket (UQ10 GB/sec) */
  /** @} */

  /** @name Activity Metrics
   * Utilization and bandwidth measurements
   * @{
   */
  uint32_t SocketGfxBusy;                   /**< Average XCD busy for all enabled XCDs in the socket (UQ10, 0-100%) */
  uint32_t DramBandwidthUtilization;        /**< HBM bandwidth utilization for all HBM stacks in the socket (UQ10, 0-100%) */
  uint64_t SocketGfxBusyAcc;                /**< Accumulated value of SocketGfxBusy for averaging */
  uint64_t DramBandwidthAcc;                /**< HBM bandwidth accumulator for all HBM stacks in the socket (UQ10 GB/sec) */
  uint32_t MaxDramBandwidth;                /**< Maximum supported HBM bandwidth for all HBM stacks running at the maximum supported UCLK frequency (UQ10 GB/sec) */
  uint64_t DramBandwidthUtilizationAcc;     /**< Accumulated value of DramBandwidthUtilization for averaging */
  uint64_t PcieBandwidthAcc[2];             /**< PCIe bandwidth accumulator per MID (UQ10 GB/sec) */
  /** @} */

  /** @name Throttling Residency
   * Accumulated iteration counts spent in throttled states
   * @{
   */
  uint64_t ProchotResidencyAcc;             /**< Incremented every iteration PROCHOT is active */
  uint64_t PptResidencyAcc;                 /**< Incremented every iteration the PPT controller is active */
  uint64_t SocketThmResidencyAcc;           /**< Incremented every iteration the socket thermal throttling controller is active */
  uint64_t VrThmResidencyAcc;               /**< Incremented every iteration the VR thermal throttling controller is active */
  uint64_t HbmThmResidencyAcc;              /**< Incremented every iteration the HBM thermal throttling controller is active */
  /** @} */

  /** @name PCIe Metrics
   * PCIe bandwidth and error tracking for reliability monitoring
   * @{
   */
  uint32_t PcieBandwidth[2];                /**< Current PCIe bandwidth per MID (UQ10 GB/sec) */
  uint64_t PCIeL0ToRecoveryCountAcc;        /**< Accumulated count of PCIe L0 to recovery state transitions (link errors) */
  uint64_t PCIenReplayAAcc;                 /**< Accumulated PCIe replay events count */
  uint64_t PCIenReplayARolloverCountAcc;    /**< Accumulated PCIe replay counter rollover events */
  uint64_t PCIeNAKSentCountAcc;             /**< Accumulated PCIe NAK packets sent */
  uint64_t PCIeNAKReceivedCountAcc;         /**< Accumulated PCIe NAK packets received */
  uint64_t PCIeOtherEndRecoveryAcc;         /**< Accumulated PCIe recovery events initiated by remote endpoint */
  /** @} */

  /** @name VCN/JPEG Activity
   * Video encoder/decoder utilization (UQ10 percentage)
   * @{
   */
  uint32_t VcnBusy[4];                      /**< VCN busy percentage per instance (2 per MID, UQ10 0-100%) */
  uint32_t JpegBusy[40];                    /**< JPEG encoder/decoder busy percentage per instance (20 per MID, UQ10 0-100%) */
  /** @} */

  /** @name PCIe Link Status
   * Current PCIe link configuration
   * @{
   */
  uint32_t PCIeLinkSpeed;                   /**< Current PCIe link generation speed (Gen1-Gen6) */
  uint32_t PCIeLinkWidth;                   /**< Current PCIe link width in lanes (x1, x4, x8, x16) */
  /** @} */

  /** @name Per-XCD Activity
   * Individual compute die activity metrics for workload characterization
   * @{
   */
  uint32_t GfxBusy[8];                      /**< Graphics busy percentage per XCD (UQ10 0-100%) */
  uint64_t GfxBusyAcc[8];                   /**< Accumulated graphics busy time per XCD for averaging */
  /** @} */

  /** @name Application Clock Accounting
   * Clock cycles spent below host-requested limits (NVML parity) for throttling analysis
   * @{
   */
  uint64_t GfxclkBelowHostLimitPptAcc[8];   /**< Accumulated cycles below host limit due to PPT throttling per XCD */
  uint64_t GfxclkBelowHostLimitThmAcc[8];   /**< Accumulated cycles below host limit due to thermal throttling per XCD */
  uint64_t GfxclkBelowHostLimitTotalAcc[8]; /**< Total accumulated cycles below host-requested limit per XCD */
  uint64_t GfxclkLowUtilizationAcc[8];      /**< Accumulated cycles at low utilization per XCD (idle or underutilized) */
  /** @} */

} MetricsTable_t;
#pragma pack(pop)

/** @brief System metrics table version identifier for multi-node compatibility */
#define SMU_SYSTEM_METRICS_TABLE_VERSION 0x1

#pragma pack(push, 4)
/**
 * @struct SystemMetricsTable_t
 * @brief Multi-Node System-Level Telemetry
 *
 * Aggregated telemetry structure for system-level management spanning multiple
 * compute nodes. Provides system/node/VR temperature arrays and node power
 * management metrics for node power management.
 *
 * Temperature values are signed 16-bit integers in Celsius. Unused entries are
 * set to 0xFFFF. This table is updated on-demand when exported.
 *
 * @note Structure is 4-byte aligned via pragma pack
 */
typedef struct {
  uint64_t AccumulationCounter;                             /**< Update timestamp for staleness detection */
  uint16_t LabelVersion;                                    /**< Label mapping version, updated on SMC changes */
  uint16_t NodeIdentifier;                                  /**< Unique node identifier pushed by SMC */
  int16_t  SystemTemperatures[SYSTEM_TEMP_MAX_ENTRIES];     /**< System-level temperatures in Celsius (32 entries) */
  int16_t  NodeTemperatures[NODE_TEMP_MAX_TEMP_ENTRIES];    /**< Node-level temperatures in Celsius (12 entries) */
  int16_t  VrTemperatures[SVI_MAX_TEMP_ENTRIES];            /**< Voltage regulator temperatures in Celsius (22 entries) */
  int16_t  spare[7];                                        /**< Reserved for future use */

  /** @name Node Power Management
   * Node power management metrics
   * @{
   */
  uint32_t NodePowerLimit;                  /**< Current node power limit in Watts */
  uint32_t NodePower;                       /**< Current node power consumption in Watts */
  uint32_t GlobalPPTResidencyAcc;           /**< Accumulated global PPT throttling time */
  /** @} */

  uint16_t SystemPower[SYSTEM_POWER_MAX_ENTRIES];           /**< UBB power and threshold (4 entries) */
} SystemMetricsTable_t;
#pragma pack(pop)

/** @brief Virtual Function metrics table version identifier */
#define SMU_VF_METRICS_TABLE_VERSION 0x5

#pragma pack(push, 4)
/**
 * @struct VfMetricsTable_t
 * @brief Virtual function per-partition telemetry
 *
 * Reduced metrics structure for virtualization where multiple VFs share GPU
 * resources. Provides per-partition graphics frequency and activity tracking.
 * Used when GPU is partitioned for SR-IOV.
 *
 * @note Structure is 4-byte aligned via pragma pack
 */
typedef struct {
  uint32_t AccumulationCounter;             /**< Update counter for staleness detection */
  uint32_t InstGfxclk_TargFreq;             /**< Instantaneous graphics clock target frequency */
  uint64_t AccGfxclk_TargFreq;              /**< Accumulated graphics clock target frequency */
  uint64_t AccGfxRsmuDpm_Busy;              /**< Accumulated RSMU DPM busy time */
  uint64_t AccGfxclkBelowHostLimit;         /**< Accumulated cycles below host-requested limit */
} VfMetricsTable_t;
#pragma pack(pop)

/**
 * @struct FRUProductInfo_t
 * @brief FRU product identification
 *
 * Product information strings read from FRU EEPROM via I2C for hardware
 * identification and inventory management.
 *
 * @note Structure is 4-byte aligned
 */
#pragma pack(push, 4)
typedef struct {
  uint8_t  ModelNumber[PRODUCT_MODEL_NUMBER_LEN];           /**< Product model number string */
  uint8_t  Name[PRODUCT_NAME_LEN];                          /**< Product name string */
  uint8_t  Serial[PRODUCT_SERIAL_LEN];                      /**< Serial number string */
  uint8_t  ManufacturerName[PRODUCT_MANUFACTURER_NAME_LEN]; /**< Manufacturer name string */
  uint8_t  FruId[PRODUCT_FRU_ID_LEN];                       /**< FRU identifier string */
} FRUProductInfo_t;
#pragma pack(pop)

/** @brief Static metrics table version identifier */
#define SMU_STATIC_METRICS_TABLE_VERSION 0x1

#pragma pack(push, 4)
/**
 * @struct StaticMetricsTable_t
 * @brief Static Hardware Capabilities and Configuration
 *
 * Static (non-changing) hardware configuration and capability information
 * including product identification, frequency ranges, power limits, thermal
 * thresholds, and serial numbers. This table is populated at initialization
 * and exported on demand.
 *
 * @note Structure is 4-byte aligned via pragma pack
 * @note Temperature in Celsius.
 */
typedef struct {
  /** @name Product Identification
   * @{
   */
  FRUProductInfo_t  ProductInfo;            /**< FRU product information from I2C EEPROM */
  /** @} */

  /** @name Power Limits
   * @{
   */
  uint32_t MaxSocketPowerLimit;             /**< Maximum power limit the power throttling controller is allowed to be configured to (Watts) */
  /** @} */

  /** @name Frequency Ranges
   * Supported frequency ranges
   * @{
   */
  uint32_t MaxGfxclkFrequency;              /**< Maximum GFXCLK frequency supported by the accelerator (MHz) */
  uint32_t MinGfxclkFrequency;              /**< Minimum GFXCLK frequency supported by the accelerator (MHz) */
  uint32_t MaxFclkFrequency;                /**< Maximum FCLK (fabric clock) frequency (MHz) */
  uint32_t MinFclkFrequency;                /**< Minimum FCLK (fabric clock) frequency (MHz) */
  uint32_t MaxGl2clkFrequency;              /**< Maximum GL2 cache clock frequency (MHz) */
  uint32_t MinGl2clkFrequency;              /**< Minimum GL2 cache clock frequency (MHz) */
  uint32_t UclkFrequencyTable[4];           /**< List of supported UCLK frequencies; 0 means state not supported (MHz) */
  uint32_t SocclkFrequency;                 /**< List of supported SOCCLK frequencies (MHz) */
  uint32_t LclkFrequency;                   /**< List of supported LCLK frequencies (MHz) */
  uint32_t VclkFrequency;                   /**< List of supported VCLK frequencies (MHz) */
  uint32_t DclkFrequency;                   /**< List of supported DCLK frequencies (MHz) */
  /** @} */

  /** @name CTF Limits
   * CTF thresholds in Celsius
   * @{
   */
  uint32_t CTFLimit_MID;                    /**< MID CTF threshold */
  uint32_t CTFLimit_AID;                    /**< AID CTF threshold */
  uint32_t CTFLimit_XCD;                    /**< XCD CTF threshold */
  uint32_t CTFLimit_HBM;                    /**< HBM CTF threshold */
  /** @} */

  /** @name Thermal Throttling Limits
   * Thermal throttling activation thresholds in Celsius
   * @{
   */
  uint32_t ThermalLimit_MID;                /**< MID thermal throttling threshold */
  uint32_t ThermalLimit_AID;                /**< AID thermal throttling threshold */
  uint32_t ThermalLimit_XCD;                /**< XCD thermal throttling threshold */
  uint32_t ThermalLimit_HBM;                /**< HBM thermal throttling threshold */
  /** @} */

  /** @name Public Serial Numbers
   * Hardware die serial numbers for identification
   * @{
   */
  uint64_t PublicSerialNumber_MID[2];       /**< MID public serial numbers (2 MIDs) */
  uint64_t PublicSerialNumber_AID[2];       /**< AID public serial numbers (2 AIDs) */
  uint64_t PublicSerialNumber_XCD[8];       /**< XCD public serial numbers (8 XCDs) */
  /** @} */

  /** @name XGMI Capabilities
   * Maximum XGMI link capabilities for inter-chip communication
   * @{
   */
  uint32_t MaxXgmiWidth;                    /**< Maximum supported XGMI link width (bus width) */
  uint32_t MaxXgmiBitrate;                  /**< Maximum supported XGMI bitrate (Gbps) */
  /** @} */

  /** @name Telemetry Configuration
   * @{
   */
  uint32_t InputTelemetryVoltageInmV;       /**< Input telemetry voltage in millivolts */
  /** @} */

  /** @name Firmware and Configuration
   * @{
   */
  uint32_t pldmVersion[2];                  /**< PLDM firmware version (I2C read) */
  /** @} */

  /** @name PPT Configuration
   * PPT configuration range
   * @{
   */
  uint32_t PPT1Max;                         /**< PPT1 maximum limit */
  uint32_t PPT1Min;                         /**< PPT1 minimum limit */
  uint32_t PPT1Default;                     /**< PPT1 default setting */
  /** @} */
} StaticMetricsTable_t;
#pragma pack(pop)

#endif
