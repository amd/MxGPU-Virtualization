/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MI200_POWERPLAY_H
#define MI200_POWERPLAY_H

#define AMD_MAX_USEC_TIMEOUT        200000  /* 200 ms */

#define MP0_Private         0x03700000
#define MP0_Public          0x03800000
#define MP0_SRAM            0x03900000
#define MP1_Private         0x03a00000
#define MP1_Public          0x03b00000
#define MP1_SRAM            0x03c00004

#define smnMP1_FIRMWARE_FLAGS	0x03010024


#define PAGE_SIZE           0x1000

#define TOOL_SIZE           0x19000

#define MI200_ENGINECLOCK_HARDMAX 198000

#define PP_FB_SIZE   0x400000
#define PP_FB_START_OFFSET 0x800000

#define SMU13_DRIVER_IF_VERSION 0x9

extern int mi200_powerplay_sw_init(struct amdgv_adapter *adapt);
extern int mi200_powerplay_sw_fini(struct amdgv_adapter *adapt);
extern int mi200_powerplay_hw_init(struct amdgv_adapter *adapt);
extern int mi200_powerplay_hw_fini(struct amdgv_adapter *adapt);

int mi200_smu_13_0_get_fw_loaded_status(struct amdgv_adapter *adapt);
int mi200_mode1_reset(struct amdgv_adapter *adapt);
int mi200_wait_mode1_reset_completion(struct amdgv_adapter *adapt);

int mi200_fru_get_product_info(struct amdgv_adapter *adapt);

#endif
