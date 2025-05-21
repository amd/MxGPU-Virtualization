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
