/*
 * Copyright (C) 2021 Advanced Micro Devices, Inc. All rights reserved.
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
 * THE SOFTWARE
 */

#ifndef navi32_FRU_H
#define navi32_FRU_H

/* SMU I2C interface */
#define I2C_ADDR_SIZE	(0x2)
#define I2C_RM_DEV_ADDR (0x56 << 1)

/* FRU product information area */
#define NAVI32_FRU_PRODUCT_INFO_HEADER         (0x0004)
#define NAVI32_FRU_PRODUCT_INFO_SKIP_ADDR      (0x3)
#define NAVI32_FRU_PRODUCT_INFO_SIZE_MASK      (0x3F)
#define NAVI32_FRU_PRODUCT_INFO_ASSET_TAG_SIZE (0x3)

/* order of FRU product info, may vary in future */
enum navi32_fru_product_order {
	FRU_PRODUCT_MANUFACTURER = 0,
	FRU_PRODUCT_NAME,
	FRU_PRODUCT_MODEL_NUM,
	FRU_PRODUCT_VERSION,
	FRU_PRODUCT_SERIAL,
	FRU_PRODUCT_ASSET_TAG,
	FRU_PRODUCT_FRU_ID,

	FRU_PRODUCT_MAX
};

int navi32_fru_get_product_info(struct amdgv_adapter *adapt);

#endif
