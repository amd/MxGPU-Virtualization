/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef navi32_FRU_H
#define navi32_FRU_H

/* SMU I2C interface */
#define I2C_ADDR_SIZE	(0x2)
#define I2C_RM_DEV_ADDR (0x56 << 1)

/*
 * 80 bytes is the minimum recommended size for FRU Product Info section (v1.3)
 * Assume, largest field is maximum 80 bytes.
 */
#define I2C_CMD_BUFFER_SIZE 80

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
