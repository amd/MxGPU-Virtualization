/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
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

#include <amdgv_device.h>
#include "mi300_smu_driver_if.h"
#include "mi300_fru.h"

static const uint32_t this_block = AMDGV_POWER_BLOCK;



/* ping remote manager FRU */
static int mi300_fru_ping_rm(struct amdgv_adapter *adapt)
{
	int ret;
	uint8_t cmd_buff[3] = { 0 };
	struct i2c_msg msg = {
		.addr    = I2C_RM_DEV_ADDR,
		.flags   = I2C_M_RD,
		.buf     = cmd_buff,
	};

	/* test RM presence */
	msg.len = I2C_ADDR_SIZE + 1;

	adapt->pp.is_test_smu = true;
	ret = adapt->pp.pp_funcs->i2c_eeprom_xfer(adapt, I2C_CONTROLLER_PORT_0, &msg, 1);
	adapt->pp.is_test_smu = false;

	return ret;
}

static int mi300_fru_read_header(struct amdgv_adapter *adapt, uint16_t header_address,
				  uint16_t *result_address)
{
	int ret;
	uint8_t cmd_buff[3] = { 0 };
	struct i2c_msg msg = {
		.addr    = I2C_RM_DEV_ADDR,
		.flags   = I2C_M_RD,
		.buf     = cmd_buff,
	};

	*result_address = 0;

	cmd_buff[0] = header_address >> 8;
	cmd_buff[1] = header_address & 0xFF;
	msg.len = I2C_ADDR_SIZE + 1;
	ret = adapt->pp.pp_funcs->i2c_eeprom_xfer(adapt, I2C_CONTROLLER_PORT_0, &msg, 1);
	if (ret == AMDGV_FAILURE)
		return AMDGV_FAILURE;

	*result_address = 8 * cmd_buff[2];

	return 0;
}

/*
 * mi300_fru_read_product_info
 *     To read FRU data, we use SMU I2C_PORT_SVD_SCL port
 *
 * @adapt:      LibGV adapter
 * @data_addr:  data address in the slave module
 * @cmd_buff:   pointer to command buffer (inout struct)
 * @data_size:  the size of data read from slave,
 *              rememebr to +1 to include the "size of data" field
 */
static int mi300_fru_read_product_info(struct amdgv_adapter *adapt, uint16_t data_addr,
					uint8_t *cmd_buff, uint8_t *data_size)
{
	int ret;
	uint8_t size;

	struct i2c_msg msg = {
		.addr    = I2C_RM_DEV_ADDR,
		.flags   = I2C_M_RD,
		.buf     = cmd_buff,
		.len      = I2C_ADDR_SIZE + 1,
	};

	*data_size = 0;

	/* first read to get data size */
	cmd_buff[0] = data_addr >> 8;
	cmd_buff[1] = data_addr & 0xFF;
	ret = adapt->pp.pp_funcs->i2c_eeprom_xfer(adapt, I2C_CONTROLLER_PORT_0, &msg, 1);
	if (ret == AMDGV_FAILURE)
		return AMDGV_FAILURE;

	/* The size returned by the i2c is 7bits in size */
	size = cmd_buff[2] & MI300_FRU_PRODUCT_INFO_SIZE_MASK;

	if ((I2C_ADDR_SIZE) + size > I2C_CMD_BUFFER_SIZE)
		return AMDGV_FAILURE;

	/* second read for actual data */
	data_addr += 1;
	cmd_buff[0] = data_addr >> 8;
	cmd_buff[1] = data_addr & 0xFF;
	msg.len = I2C_ADDR_SIZE + size;
	ret = adapt->pp.pp_funcs->i2c_eeprom_xfer(adapt, I2C_CONTROLLER_PORT_0, &msg, 1);
	if (ret == AMDGV_FAILURE)
		return AMDGV_FAILURE;

	*data_size = size;
	return 0;
}

static void mi300_fru_copy_field(struct amdgv_adapter *adapt, enum mi300_fru_product_order id,
					char *src_buf, uint8_t src_size)
{
	uint8_t copy_size = 0;
	char *dest_buf = NULL;

	switch (id) {
	case FRU_PRODUCT_MANUFACTURER:
		dest_buf = adapt->product_info.manufacturer_name;
		copy_size = min(sizeof(adapt->product_info.manufacturer_name) - 1, src_size);
		break;
	case FRU_PRODUCT_NAME:
		dest_buf = adapt->product_info.product_name;
		copy_size = min(sizeof(adapt->product_info.product_name) - 1, src_size);
		break;
	case FRU_PRODUCT_MODEL_NUM:
		dest_buf = adapt->product_info.model_number;
		copy_size = min(sizeof(adapt->product_info.model_number) - 1, src_size);
		break;
	case FRU_PRODUCT_SERIAL:
		dest_buf = adapt->product_info.product_serial;
		copy_size = min(sizeof(adapt->product_info.product_serial) - 1, src_size);
		break;
	case FRU_PRODUCT_FRU_ID:
		dest_buf = adapt->product_info.fru_id;
		copy_size = min(sizeof(adapt->product_info.fru_id) - 1, src_size);
		break;
	default:
		AMDGV_DEBUG("Unknown FRU Field %d\n", id);
		return;
	}

	oss_memcpy(dest_buf, src_buf, copy_size);
	dest_buf[copy_size] = '\0';

	return;
}

int mi300_fru_get_product_info(struct amdgv_adapter *adapt)
{
	int i;
	int ret = 0;

	/*
	 * SMU i2c command buffer (.buf) layout
	 *
	 * 0x0                0x2
	 * |                  |
	 * |   addr to read   |      return data       |
	 * |                  |<---   msg.len - 2   -->|
	 * |<--------------  msg.len  ---------------->|
	 *
	 */
	uint8_t *cmd_buff = adapt->i2c_cmd_buffer;
	uint8_t data_size = 0;
	uint16_t addrptr = 0x0;

	char *string;

	if (cmd_buff == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
			I2C_CMD_BUFFER_SIZE);
		return AMDGV_FAILURE;
	}

	/* ping slave controller to detect presence */
	ret = mi300_fru_ping_rm(adapt);
	if (ret == AMDGV_FAILURE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_PP_I2C_SLAVE_NOT_PRESENT,
				I2C_RM_DEV_ADDR);
		ret = AMDGV_ERROR_PP_I2C_SLAVE_NOT_PRESENT;

		adapt->product_info.valid = false;

		goto clear;
	}

	/*
	 * To read FRU data, we need to read FRU header first to
	 * get the pointer to desired data
	 */
	ret = mi300_fru_read_header(adapt, MI300_FRU_PRODUCT_INFO_HEADER, &addrptr);
	if (ret)
		goto clear;

	/* we have a version, a length and a langurage byte before the actual product data */
	addrptr += MI300_FRU_PRODUCT_INFO_SKIP_ADDR;
	AMDGV_INFO("FRU product addrptr is at 0x%04X\n", addrptr);

	/*
	 * RM FRU data has variable-length fields.
	 * To get desired information, we have to keep reading
	 * until we get all of the data that we want.
	 * We use addrptr to track the address as we go
	 *
	 *
	 * RM FRU data layout at correct data_addr
	 * data_addr+0x0      data_addr+0x1
	 * |                  |
	 * |   size of data   |    actual data    |
	 *                    |<----   size   --->|
	 */
	for (i = 0; i < FRU_PRODUCT_MAX; i++) {
		/* do not read asset tag, some of cards has incorrect data format */
		if (i == FRU_PRODUCT_ASSET_TAG) {
			addrptr += MI300_FRU_PRODUCT_INFO_ASSET_TAG_SIZE;
			continue;
		}

		ret = mi300_fru_read_product_info(adapt, addrptr, cmd_buff, &data_size);
		if (ret)
			goto clear;

		/* NULL terminate the string */
		string = cmd_buff + I2C_ADDR_SIZE;
		string[data_size] = 0;

		mi300_fru_copy_field(adapt, i, string, data_size);

		addrptr += data_size + 1;
	}

	adapt->product_info.valid = true;
	adapt->product_info.visit = true;

clear:
	oss_memset(adapt->i2c_cmd_buffer, 0, I2C_CMD_BUFFER_SIZE);
	return ret;
}
