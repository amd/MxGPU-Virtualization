/*
 * Copyright (c) 2019-2024 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef _AMDGV_COMMON_EEPROM_H
#define _AMDGV_COMMON_EEPROM_H

enum GPU_HEALTY_STATUS {
	GPU_HEALTHY_USABLE = 0,
	GPU_RETIRED__ECC_IN_CRITICAL_REGION,
	GPU_RETIRED__ECC_REACH_THRESHOLD,
	GPU_RETIRED__ECC_IN_SAME_MEMORY_ROW,
};

enum amdgv_ras_eeprom_err_type {
	AMDGV_RAS_EEPROM_ERR_PLACE_HOLDER,
	AMDGV_RAS_EEPROM_ERR_RECOVERABLE,
	AMDGV_RAS_EEPROM_ERR_NON_RECOVERABLE
};

#pragma pack(push, 1)
struct amdgv_ras_eeprom_table_header {
	uint32_t header;
	uint32_t version;
	uint32_t first_rec_offset;
	uint32_t tbl_size;
	uint32_t checksum;
};

struct amdgv_ras_eeprom_extra_gpu_info_v2 {
	uint32_t header_version;
	uint16_t ecc_page_threshold;
	uint16_t gpu_healthy_status;
	uint32_t padding[64 - 2];
};

struct amdgv_ras_eeprom_extra_gpu_info_v2_1 {
	uint8_t  rma_status;
	uint8_t  gpu_healthy;
	uint16_t ecc_page_threshold;
	uint32_t padding[64 - 1];
};

/*
 * Represents single table record. Packed to be easily serialized into byte
 * stream.
 */
struct eeprom_table_record {
	union {
		uint64_t address;
		uint64_t offset;
	};

	uint64_t retired_page;
	uint64_t ts;

	enum amdgv_ras_eeprom_err_type err_type;

	union {
		unsigned char bank;
		unsigned char cu;
	};

	unsigned char mem_channel;
	unsigned char mcumc_id;
};
#pragma pack(pop)

struct amdgv_ras_eeprom_control {
	struct amdgv_ras_eeprom_table_header tbl_hdr;
	struct amdgv_ras_eeprom_extra_gpu_info_v2 tbl_egi_v2;
	struct amdgv_ras_eeprom_extra_gpu_info_v2_1 tbl_egi_v2_1;
	uint32_t next_addr;
	unsigned int num_recs;
	mutex_t tbl_mutex;
	bool bus_locked;
	/* true: access to i2c by smu
	 * false: access to i2c by driver directly
	 */
	bool smu_i2c_supported;
	uint32_t tbl_byte_sum;
	uint16_t i2c_address;
	uint16_t i2c_port;
	uint32_t bad_channel_bitmap;

	uint32_t max_record_num;
};

struct i2c_msg {
	uint16_t addr;	/* slave address			*/
	uint16_t flags;
#define I2C_M_WR		0x0000	/* write data, to slave		*/
#define I2C_M_RD		0x0001	/* read data, from slave to master */
					/* I2C_M_RD is guaranteed to be 0x0001! */
#define I2C_M_TEN		0x0010	/* this is a ten bit chip address */
#define I2C_M_DMA_SAFE		0x0200	/* the buffer of this message is DMA safe */
					/* makes only sense in kernelspace */
					/* userspace buffers are copied anyway */
#define I2C_M_RECV_LEN		0x0400	/* length will be first received byte */
#define I2C_M_NO_RD_ACK		0x0800	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_IGNORE_NAK	0x1000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_REV_DIR_ADDR	0x2000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_NOSTART		0x4000	/* if I2C_FUNC_NOSTART */
#define I2C_M_STOP		0x8000	/* if I2C_FUNC_PROTOCOL_MANGLING */
	uint16_t len;		/* msg length				*/
	uint8_t *buf;		/* pointer to msg data			*/
};

#endif