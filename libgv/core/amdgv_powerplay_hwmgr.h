/*
 * Copyright (c) 2017-2020 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef AMDGV_POWERPLAY_HWMGR_H
#define AMDGV_POWERPLAY_HWMGR_H

enum phm_platform_caps {
	PHM_PlatformCaps_AtomBiosPpV1 = 0,
	PHM_PlatformCaps_PowerPlaySupport,
	PHM_PlatformCaps_ACOverdriveSupport,
	PHM_PlatformCaps_BacklightSupport,
	PHM_PlatformCaps_ThermalController,
	PHM_PlatformCaps_BiosPowerSourceControl,
	PHM_PlatformCaps_DisableVoltageTransition,
	PHM_PlatformCaps_DisableEngineTransition,
	PHM_PlatformCaps_DisableMemoryTransition,
	PHM_PlatformCaps_DynamicPowerManagement,
	PHM_PlatformCaps_EnableASPML0s,
	PHM_PlatformCaps_EnableASPML1,
	PHM_PlatformCaps_OD5inACSupport,
	PHM_PlatformCaps_OD5inDCSupport,
	PHM_PlatformCaps_SoftStateOD5,
	PHM_PlatformCaps_NoOD5Support,
	PHM_PlatformCaps_ContinuousHardwarePerformanceRange,
	PHM_PlatformCaps_ActivityReporting,
	PHM_PlatformCaps_EnableBackbias,
	PHM_PlatformCaps_OverdriveDisabledByPowerBudget,
	PHM_PlatformCaps_ShowPowerBudgetWarning,
	PHM_PlatformCaps_PowerBudgetWaiverAvailable,
	PHM_PlatformCaps_GFXClockGatingSupport,
	PHM_PlatformCaps_MMClockGatingSupport,
	PHM_PlatformCaps_AutomaticDCTransition,
	PHM_PlatformCaps_GeminiPrimary,
	PHM_PlatformCaps_MemorySpreadSpectrumSupport,
	PHM_PlatformCaps_EngineSpreadSpectrumSupport,
	PHM_PlatformCaps_StepVddc,
	PHM_PlatformCaps_DynamicPCIEGen2Support,
	PHM_PlatformCaps_SMC,
	PHM_PlatformCaps_FaultyInternalThermalReading,          /* Internal thermal controller reports faulty temperature value when DAC2 is active */
	PHM_PlatformCaps_EnableVoltageControl,                  /* indicates voltage can be controlled */
	PHM_PlatformCaps_EnableSideportControl,                 /* indicates Sideport can be controlled */
	PHM_PlatformCaps_VideoPlaybackEEUNotification,          /* indicates EEU notification of video start/stop is required */
	PHM_PlatformCaps_TurnOffPll_ASPML1,                     /* PCIE Turn Off PLL in ASPM L1 */
	PHM_PlatformCaps_EnableHTLinkControl,                   /* indicates HT Link can be controlled by ACPI or CLMC overridden/automated mode. */
	PHM_PlatformCaps_PerformanceStateOnly,                  /* indicates only performance power state to be used on current system. */
	PHM_PlatformCaps_ExclusiveModeAlwaysHigh,               /* In Exclusive (3D) mode always stay in High state. */
	PHM_PlatformCaps_DisableMGClockGating,                  /* to disable Medium Grain Clock Gating or not */
	PHM_PlatformCaps_DisableMGCGTSSM,                       /* TO disable Medium Grain Clock Gating Shader Complex control */
	PHM_PlatformCaps_UVDAlwaysHigh,                         /* In UVD mode always stay in High state */
	PHM_PlatformCaps_DisablePowerGating,                    /* to disable power gating */
	PHM_PlatformCaps_CustomThermalPolicy,                   /* indicates only performance power state to be used on current system. */
	PHM_PlatformCaps_StayInBootState,                       /* Stay in Boot State, do not do clock/voltage or PCIe Lane and Gen switching (RV7xx and up). */
	PHM_PlatformCaps_SMCAllowSeparateSWThermalState,        /* SMC use separate SW thermal state, instead of the default SMC thermal policy. */
	PHM_PlatformCaps_MultiUVDStateSupport,                  /* Powerplay state table supports multi UVD states. */
	PHM_PlatformCaps_EnableSCLKDeepSleepForUVD,             /* With HW ECOs, we don't need to disable SCLK Deep Sleep for UVD state. */
	PHM_PlatformCaps_EnableMCUHTLinkControl,                /* Enable HT link control by MCU */
	PHM_PlatformCaps_ABM,                                   /* ABM support.*/
	PHM_PlatformCaps_KongThermalPolicy,                     /* A thermal policy specific for Kong */
	PHM_PlatformCaps_SwitchVDDNB,                           /* if the users want to switch VDDNB */
	PHM_PlatformCaps_ULPS,                                  /* support ULPS mode either through ACPI state or ULPS state */
	PHM_PlatformCaps_NativeULPS,                            /* hardware capable of ULPS state (other than through the ACPI state) */
	PHM_PlatformCaps_EnableMVDDControl,                     /* indicates that memory voltage can be controlled */
	PHM_PlatformCaps_ControlVDDCI,                          /* Control VDDCI separately from VDDC. */
	PHM_PlatformCaps_DisableDCODT,                          /* indicates if DC ODT apply or not */
	PHM_PlatformCaps_DynamicACTiming,                       /* if the SMC dynamically re-programs MC SEQ register values */
	PHM_PlatformCaps_EnableThermalIntByGPIO,                /* enable throttle control through GPIO */
	PHM_PlatformCaps_BootStateOnAlert,                      /* Go to boot state on alerts, e.g. on an AC->DC transition. */
	PHM_PlatformCaps_DontWaitForVBlankOnAlert,              /* Do NOT wait for VBLANK during an alert (e.g. AC->DC transition). */
	PHM_PlatformCaps_Force3DClockSupport,                   /* indicates if the platform supports force 3D clock. */
	PHM_PlatformCaps_MicrocodeFanControl,                   /* Fan is controlled by the SMC microcode. */
	PHM_PlatformCaps_AdjustUVDPriorityForSP,
	PHM_PlatformCaps_DisableLightSleep,                     /* Light sleep for evergreen family. */
	PHM_PlatformCaps_DisableMCLS,                           /* MC Light sleep */
	PHM_PlatformCaps_RegulatorHot,                          /* Enable throttling on 'regulator hot' events. */
	PHM_PlatformCaps_BACO,                                  /* Support Bus Alive Chip Off mode */
	PHM_PlatformCaps_DisableDPM,                            /* Disable DPM, supported from Llano */
	PHM_PlatformCaps_DynamicM3Arbiter,                      /* support dynamically change m3 arbitor parameters */
	PHM_PlatformCaps_SclkDeepSleep,                         /* support sclk deep sleep */
	PHM_PlatformCaps_DynamicPatchPowerState,                /* this ASIC supports to patch power state dynamically */
	PHM_PlatformCaps_ThermalAutoThrottling,                 /* enabling auto thermal throttling, */
	PHM_PlatformCaps_SumoThermalPolicy,                     /* A thermal policy specific for Sumo */
	PHM_PlatformCaps_PCIEPerformanceRequest,                /* support to change RC voltage */
	PHM_PlatformCaps_BLControlledByGPU,                     /* support varibright */
	PHM_PlatformCaps_PowerContainment,                      /* support DPM2 power containment (AKA TDP clamping) */
	PHM_PlatformCaps_SQRamping,                             /* support DPM2 SQ power throttle */
	PHM_PlatformCaps_CAC,                                   /* support Capacitance * Activity power estimation */
	PHM_PlatformCaps_NIChipsets,                            /* Northern Island and beyond chipsets */
	PHM_PlatformCaps_TrinityChipsets,                       /* Trinity chipset */
	PHM_PlatformCaps_EvergreenChipsets,                     /* Evergreen family chipset */
	PHM_PlatformCaps_PowerControl,                          /* Cayman and beyond chipsets */
	PHM_PlatformCaps_DisableLSClockGating,                  /* to disable Light Sleep control for HDP memories */
	PHM_PlatformCaps_BoostState,                            /* this ASIC supports boost state */
	PHM_PlatformCaps_UserMaxClockForMultiDisplays,          /* indicates if max memory clock is used for all status when multiple displays are connected */
	PHM_PlatformCaps_RegWriteDelay,                         /* indicates if back to back reg write delay is required */
	PHM_PlatformCaps_NonABMSupportInPPLib,                  /* ABM is not supported in PPLIB, (moved from PPLIB to DAL) */
	PHM_PlatformCaps_GFXDynamicMGPowerGating,               /* Enable Dynamic MG PowerGating on Trinity */
	PHM_PlatformCaps_DisableSMUUVDHandshake,                /* Disable SMU UVD Handshake */
	PHM_PlatformCaps_DTE,                                   /* Support Digital Temperature Estimation */
	PHM_PlatformCaps_W5100Specifc_SmuSkipMsgDTE,            /* This is for the feature requested by David B., and Tonny W.*/
	PHM_PlatformCaps_UVDPowerGating,                        /* enable UVD power gating, supported from Llano */
	PHM_PlatformCaps_UVDDynamicPowerGating,                 /* enable UVD Dynamic power gating, supported from UVD5 */
	PHM_PlatformCaps_VCEPowerGating,                        /* Enable VCE power gating, supported for TN and later ASICs */
	PHM_PlatformCaps_SamuPowerGating,                       /* Enable SAMU power gating, supported for KV and later ASICs */
	PHM_PlatformCaps_UVDDPM,                                /* UVD clock DPM */
	PHM_PlatformCaps_VCEDPM,                                /* VCE clock DPM */
	PHM_PlatformCaps_SamuDPM,                               /* SAMU clock DPM */
	PHM_PlatformCaps_AcpDPM,                                /* ACP clock DPM */
	PHM_PlatformCaps_SclkDeepSleepAboveLow,                 /* Enable SCLK Deep Sleep on all DPM states */
	PHM_PlatformCaps_DynamicUVDState,                       /* Dynamic UVD State */
	PHM_PlatformCaps_WantSAMClkWithDummyBackEnd,            /* Set SAM Clk With Dummy Back End */
	PHM_PlatformCaps_WantUVDClkWithDummyBackEnd,            /* Set UVD Clk With Dummy Back End */
	PHM_PlatformCaps_WantVCEClkWithDummyBackEnd,            /* Set VCE Clk With Dummy Back End */
	PHM_PlatformCaps_WantACPClkWithDummyBackEnd,            /* Set SAM Clk With Dummy Back End */
	PHM_PlatformCaps_OD6inACSupport,                        /* indicates that the ASIC/back end supports OD6 */
	PHM_PlatformCaps_OD6inDCSupport,                        /* indicates that the ASIC/back end supports OD6 in DC */
	PHM_PlatformCaps_EnablePlatformPowerManagement,         /* indicates that Platform Power Management feature is supported */
	PHM_PlatformCaps_SurpriseRemoval,                       /* indicates that surprise removal feature is requested */
	PHM_PlatformCaps_NewCACVoltage,                         /* indicates new CAC voltage table support */
	PHM_PlatformCaps_DiDtSupport,                           /* for dI/dT feature */
	PHM_PlatformCaps_DBRamping,                             /* for dI/dT feature */
	PHM_PlatformCaps_TDRamping,                             /* for dI/dT feature */
	PHM_PlatformCaps_TCPRamping,                            /* for dI/dT feature */
	PHM_PlatformCaps_DBRRamping,                            /* for dI/dT feature */
	PHM_PlatformCaps_DiDtEDCEnable,                         /* for dI/dT feature */
	PHM_PlatformCaps_GCEDC,                                 /* for dI/dT feature */
	PHM_PlatformCaps_PSM,                                   /* for dI/dT feature */
	PHM_PlatformCaps_EnableSMU7ThermalManagement,           /* SMC will manage thermal events */
	PHM_PlatformCaps_FPS,                                   /* FPS support */
	PHM_PlatformCaps_ACP,                                   /* ACP support */
	PHM_PlatformCaps_SclkThrottleLowNotification,           /* SCLK Throttle Low Notification */
	PHM_PlatformCaps_XDMAEnabled,                           /* XDMA engine is enabled */
	PHM_PlatformCaps_UseDummyBackEnd,                       /* use dummy back end */
	PHM_PlatformCaps_EnableDFSBypass,                       /* Enable DFS bypass */
	PHM_PlatformCaps_VddNBDirectRequest,
	PHM_PlatformCaps_PauseMMSessions,
	PHM_PlatformCaps_UnTabledHardwareInterface,             /* Tableless/direct call hardware interface for CI and newer ASICs */
	PHM_PlatformCaps_SMU7,                                  /* indicates that vpuRecoveryBegin without SMU shutdown */
	PHM_PlatformCaps_RevertGPIO5Polarity,                   /* indicates revert GPIO5 plarity table support */
	PHM_PlatformCaps_Thermal2GPIO17,                        /* indicates thermal2GPIO17 table support */
	PHM_PlatformCaps_ThermalOutGPIO,                        /* indicates ThermalOutGPIO support, pin number is assigned by VBIOS */
	PHM_PlatformCaps_DisableMclkSwitchingForFrameLock,      /* Disable memory clock switch during Framelock */
	PHM_PlatformCaps_ForceMclkHigh,                         /* Disable memory clock switching by forcing memory clock high */
	PHM_PlatformCaps_VRHotGPIOConfigurable,                 /* indicates VR_HOT GPIO configurable */
	PHM_PlatformCaps_TempInversion,                         /* enable Temp Inversion feature */
	PHM_PlatformCaps_IOIC3,
	PHM_PlatformCaps_ConnectedStandby,
	PHM_PlatformCaps_EVV,
	PHM_PlatformCaps_EnableLongIdleBACOSupport,
	PHM_PlatformCaps_CombinePCCWithThermalSignal,
	PHM_PlatformCaps_DisableUsingActualTemperatureForPowerCalc,
	PHM_PlatformCaps_StablePState,
	PHM_PlatformCaps_OD6PlusinACSupport,
	PHM_PlatformCaps_OD6PlusinDCSupport,
	PHM_PlatformCaps_ODThermalLimitUnlock,
	PHM_PlatformCaps_ReducePowerLimit,
	PHM_PlatformCaps_ODFuzzyFanControlSupport,
	PHM_PlatformCaps_GeminiRegulatorFanControlSupport,
	PHM_PlatformCaps_ControlVDDGFX,
	PHM_PlatformCaps_BBBSupported,
	PHM_PlatformCaps_DisableVoltageIsland,
	PHM_PlatformCaps_FanSpeedInTableIsRPM,
	PHM_PlatformCaps_GFXClockGatingManagedInCAIL,
	PHM_PlatformCaps_IcelandULPSSWWorkAround,
	PHM_PlatformCaps_FPSEnhancement,
	PHM_PlatformCaps_LoadPostProductionFirmware,
	PHM_PlatformCaps_VpuRecoveryInProgress,
	PHM_PlatformCaps_Falcon_QuickTransition,
	PHM_PlatformCaps_AVFS,
	PHM_PlatformCaps_ClockStretcher,
	PHM_PlatformCaps_TablelessHardwareInterface,
	PHM_PlatformCaps_EnableDriverEVV,
	PHM_PlatformCaps_SPLLShutdownSupport,
	PHM_PlatformCaps_VirtualBatteryState,
	PHM_PlatformCaps_IgnoreForceHighClockRequestsInAPUs,
	PHM_PlatformCaps_DisableMclkSwitchForVR,
	PHM_PlatformCaps_SMU8,
	PHM_PlatformCaps_VRHotPolarityHigh,
	PHM_PlatformCaps_IPS_UlpsExclusive,
	PHM_PlatformCaps_SMCtoPPLIBAcdcGpioScheme,
	PHM_PlatformCaps_GeminiAsymmetricPower,
	PHM_PlatformCaps_OCLPowerOptimization,
	PHM_PlatformCaps_MaxPCIEBandWidth,
	PHM_PlatformCaps_PerfPerWattOptimizationSupport,
	PHM_PlatformCaps_UVDClientMCTuning,
	PHM_PlatformCaps_ODNinACSupport,
	PHM_PlatformCaps_ODNinDCSupport,
	PHM_PlatformCaps_UMDPState,
	PHM_PlatformCaps_AutoWattmanSupport,
	PHM_PlatformCaps_AutoWattmanEnable_CCCState,
	PHM_PlatformCaps_FreeSyncActive,
	PHM_PlatformCaps_EnableShadowPstate,
	PHM_PlatformCaps_customThermalManagement,
	PHM_PlatformCaps_staticFanControl,
	PHM_PlatformCaps_Virtual_System,
	PHM_PlatformCaps_LowestUclkReservedForUlv,
	PHM_PlatformCaps_EnableBoostState,
	PHM_PlatformCaps_AVFSSupport,
	PHM_PlatformCaps_ThermalPolicyDelay,
	PHM_PlatformCaps_CustomFanControlSupport,
	PHM_PlatformCaps_BAMACO,
	PHM_PlatformCaps_Max
};

struct PP_Clocks {
	uint32_t engineClock;
	uint32_t memoryClock;
	uint32_t BusBandwidth;
	uint32_t engineClockInSR;
	uint32_t dcefClock;
	uint32_t dcefClockInSR;
};

#define PHM_MAX_NUM_CAPS_BITS_PER_FIELD (sizeof(uint32_t) * 8)

/* Number of uint32_t entries used by CAPS table */
#define PHM_MAX_NUM_CAPS_ULONG_ENTRIES                                                        \
	((PHM_PlatformCaps_Max + ((PHM_MAX_NUM_CAPS_BITS_PER_FIELD)-1)) /                     \
	 (PHM_MAX_NUM_CAPS_BITS_PER_FIELD))
struct pp_fan_info {
	bool	 bNoFan;
	uint8_t	 ucTachometerPulsesPerRevolution;
	uint32_t ulMinRPM;
	uint32_t ulMaxRPM;
};

struct pp_advance_fan_control_parameters {
	uint16_t  usTMin;                          /* The temperature, in 0.01 centigrades, below which we just run at a minimal PWM. */
	uint16_t  usTMed;                          /* The middle temperature where we change slopes. */
	uint16_t  usTHigh;                         /* The high temperature for setting the second slope. */
	uint16_t  usPWMMin;                        /* The minimum PWM value in percent (0.01% increments). */
	uint16_t  usPWMMed;                        /* The PWM value (in percent) at TMed. */
	uint16_t  usPWMHigh;                       /* The PWM value at THigh. */
	uint8_t   ucTHyst;                         /* Temperature hysteresis. Integer. */
	uint32_t  ulCycleDelay;                    /* The time between two invocations of the fan control routine in microseconds. */
	uint16_t  usTMax;                          /* The max temperature */
	uint8_t   ucFanControlMode;
	uint16_t  usFanPWMMinLimit;
	uint16_t  usFanPWMMaxLimit;
	uint16_t  usFanPWMStep;
	uint16_t  usDefaultMaxFanPWM;
	uint16_t  usFanOutputSensitivity;
	uint16_t  usDefaultFanOutputSensitivity;
	uint16_t  usMaxFanPWM;                     /* The max Fan PWM value for Fuzzy Fan Control feature */
	uint16_t  usFanRPMMinLimit;                /* Minimum limit range in percentage, need to calculate based on minRPM/MaxRpm */
	uint16_t  usFanRPMMaxLimit;                /* Maximum limit range in percentage, usually set to 100% by default */
	uint16_t  usFanRPMStep;                    /* Step increments/decerements, in percent */
	uint16_t  usDefaultMaxFanRPM;              /* The max Fan RPM value for Fuzzy Fan Control feature, default from PPTable */
	uint16_t  usMaxFanRPM;                     /* The max Fan RPM value for Fuzzy Fan Control feature, user defined */
	uint16_t  usFanCurrentLow;                 /* Low current */
	uint16_t  usFanCurrentHigh;                /* High current */
	uint16_t  usFanRPMLow;                     /* Low RPM */
	uint16_t  usFanRPMHigh;                    /* High RPM */
	uint32_t  ulMinFanSCLKAcousticLimit;       /* Minimum Fan Controller SCLK Frequency Acoustic Limit. */
	uint8_t   ucTargetTemperature;             /* Advanced fan controller target temperature. */
	uint8_t   ucMinimumPWMLimit;               /* The minimum PWM that the advanced fan controller can set.  This should be set to the highest PWM that will run the fan at its lowest RPM. */
	uint16_t  usFanGainEdge;                   /* The following is added for Fiji */
	uint16_t  usFanGainHotspot;
	uint16_t  usFanGainLiquid;
	uint16_t  usFanGainVrVddc;
	uint16_t  usFanGainVrMvdd;
	uint16_t  usFanGainPlx;
	uint16_t  usFanGainHbm;
	uint8_t   ucEnableZeroRPM;
	uint8_t   ucFanStopTemperature;
	uint8_t   ucFanStartTemperature;
	uint32_t  ulMaxFanSCLKAcousticLimit;       /* Maximum Fan Controller SCLK Frequency Acoustic Limit. */
	uint32_t  ulTargetGfxClk;
	uint16_t  usZeroRPMStartTemperature;
	uint16_t  usZeroRPMStopTemperature;
};

struct phm_platform_descriptor {
	uint32_t	 platformCaps[PHM_MAX_NUM_CAPS_ULONG_ENTRIES];
	uint32_t	 vbiosInterruptId;
	struct PP_Clocks overdriveLimit;
	struct PP_Clocks clockStep;
	uint32_t	 hardwareActivityPerformanceLevels;
	uint32_t	 minimumClocksReductionPercentage;
	uint32_t	 minOverdriveVDDC;
	uint32_t	 maxOverdriveVDDC;
	uint32_t	 overdriveVDDCStep;
	uint32_t	 hardwarePerformanceLevels;
	uint16_t	 powerBudget;
	uint32_t	 TDPLimit;
	uint32_t	 nearTDPLimit;
	uint32_t	 nearTDPLimitAdjusted;
	uint32_t	 SQRampingThreshold;
	uint32_t	 CACLeakage;
	uint16_t	 TDPODLimit;
	uint32_t	 TDPAdjustment;
	bool		 TDPAdjustmentPolarity;
	uint16_t	 LoadLineSlope;
	uint32_t	 VidMinLimit;
	uint32_t	 VidMaxLimit;
	uint32_t	 VidStep;
	uint32_t	 VidAdjustment;
	bool		 VidAdjustmentPolarity;
};

struct pp_thermal_controller_info {
	uint8_t					 ucType;
	uint8_t					 ucI2cLine;
	uint8_t					 ucI2cAddress;
	struct pp_fan_info			 fanInfo;
	struct pp_advance_fan_control_parameters advanceFanControlParameters;
};

struct phm_odn_performance_level {
	uint32_t clock;
	uint32_t vddc;
	bool	 enabled;
};

struct phm_odn_clock_levels {
	uint32_t size;
	uint32_t options;
	uint32_t flags;
	uint32_t num_of_pl;
	/* variable-sized array, specify by num_of_pl. */
	struct phm_odn_performance_level entries[8];
};

struct phm_ppt_v1_clock_voltage_dependency_record {
	uint32_t clk;
	uint8_t	 vddInd;
	uint8_t	 vddciInd;
	uint8_t	 mvddInd;
	uint16_t vdd_offset;
	uint16_t vddc;
	uint16_t vddgfx;
	uint16_t vddci;
	uint16_t mvdd;
	uint8_t	 phases;
	uint8_t	 cks_enable;
	uint8_t	 cks_voffset;
	uint32_t sclk_offset;
};

struct phm_ppt_v3_information {
	uint8_t uc_thermal_controller_type;

	uint16_t us_small_power_limit1;
	uint16_t us_small_power_limit2;
	uint16_t us_boost_power_limit;

	uint16_t us_od_turbo_power_limit;
	uint16_t us_od_powersave_power_limit;
	uint16_t us_software_shutdown_temp;

	uint32_t *power_saving_clock_max;
	uint32_t *power_saving_clock_min;

	uint32_t *od_settings_max;
	uint32_t *od_settings_min;

	void *smc_pptable;
};

/* Function for setting a platform cap */
static inline void phm_cap_set(uint32_t *caps, enum phm_platform_caps c)
{
	caps[c / PHM_MAX_NUM_CAPS_BITS_PER_FIELD] |=
		(1UL << (c & (PHM_MAX_NUM_CAPS_BITS_PER_FIELD - 1)));
}

static inline void phm_cap_unset(uint32_t *caps, enum phm_platform_caps c)
{
	caps[c / PHM_MAX_NUM_CAPS_BITS_PER_FIELD] &=
		~(1UL << (c & (PHM_MAX_NUM_CAPS_BITS_PER_FIELD - 1)));
}

static inline bool phm_cap_enabled(const uint32_t *caps, enum phm_platform_caps c)
{
	return (0 != (caps[c / PHM_MAX_NUM_CAPS_BITS_PER_FIELD] &
		      (1UL << (c & (PHM_MAX_NUM_CAPS_BITS_PER_FIELD - 1)))));
}

#define PP_CAP(c) phm_cap_enabled(adapt->pp.platform_descriptor.platformCaps, (c))
#endif
