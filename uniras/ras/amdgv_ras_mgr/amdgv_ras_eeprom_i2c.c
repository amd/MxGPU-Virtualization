/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef EXCLUDE_PAGE_RETIRE
#include "amdgv.h"
#include "amdgv_device.h"
#include "amdgv_ras_eeprom.h"
#include "amdgv_ras_eeprom_internal.h"
#include "amdgv_ras_mgr.h"
#include "amdgv_ras_eeprom_i2c.h"
#include "ras_eeprom.h"
#include "amdgv_ras_mp1_v13_0.h"

#define to_amdgv_adapter(x) (container_of(x, struct amdgv_adapter, eeprom_control))

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

#define EEPROM_ADDR_MSB_MASK 0x3ff00
#define EEPROM_ADDRESS_SIZE 0x2
#define EEPROM_TABLE_RECORD_SIZE 24

#define EEPROM_PAGE_BITS   8
#define EEPROM_PAGE_SIZE   (1U << EEPROM_PAGE_BITS)
#define EEPROM_PAGE_MASK   (EEPROM_PAGE_SIZE - 1)

/* typical ras bad page rate is 1 bad page per 100MB VRAM */
#define ESTIMATE_BAD_PAGE_THRESHOLD(size_mb) ((size_mb)/(100))

#define COUNT_BAD_PAGE_THRESHOLD(size) (((size) >> 21) << 4)

/* Reserve 8 physical dram row for possible retirement.
 * In worst cases, it will lose 8 * 2MB memory in vram domain
 */
#define RAS_RESERVED_VRAM_SIZE_DEFAULT	(16ULL << 20)

#define RAS_PAGES_TO_EEPROM_RECORDS(pages, ratio)  div64_u64(pages, ratio)

#define BAD_PAGE_NUM_PER_EEPROM_RECORD_V13  16
#define BAD_PAGE_NUM_PER_EEPROM_RECORD_V15  128

static int ras_eeprom_i2c_config(struct ras_core_context *ras_core,
			struct ras_eeprom_param_config *cfg)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	int amdgv_bad_page_threshold = adapt->opt.bad_page_record_threshold;
	u64 badpages, badpages_per_record = 0;
	uint32_t total_fb_in_mb;
	u32 ip_version = amdgv_ip_version(adapt, MP1_HWIP, 0);

	switch (ip_version) {
	case IP_VERSION(13, 0, 5):
	case IP_VERSION(13, 0, 6):
	case IP_VERSION(13, 0, 10):
	case IP_VERSION(13, 0, 12):
	case IP_VERSION(13, 0, 14):
		badpages_per_record = BAD_PAGE_NUM_PER_EEPROM_RECORD_V13;
		break;
	case IP_VERSION(15, 0, 8):
		badpages_per_record = BAD_PAGE_NUM_PER_EEPROM_RECORD_V15;
		break;
	default:
		RAS_DEV_ERR(adapt, "IP version(0x%x) is not supported!\n", ip_version);
		return -RAS_CORE_ENODATA;
	}

	if (!badpages_per_record) {
		RAS_DEV_ERR(adapt, "EEPROM parameters are not configured!\n");
		return -RAS_CORE_EINVAL;
	}

	cfg->eeprom_ip_version = ip_version;

	cfg->eeprom_i2c_adapter = NULL;
	cfg->eeprom_i2c_addr = MP1_V13_0_EEPROM_I2C_TARGET_ADDR;
	cfg->eeprom_i2c_port = MP1_V13_0_EEPROM_I2C_CONTROLLER_PORT;

	/*
	 * amdgv_bad_page_threshold is used to config
	 * the threshold for the number of bad pages.
	 * -1:  Threshold is set to default value
	 *      Driver will issue a warning message when threshold is reached
	 *      and continue runtime services.
	 * 0:   Disable bad page retirement
	 *      Driver will not retire bad pages
	 *      which is intended for debugging purpose.
	 * -2:  Threshold is determined by a formula
	 *      that assumes 1 bad page per 100M of local memory.
	 *      Driver will continue runtime services when threhold is reached.
	 * 0 < threshold < max number of bad page records in EEPROM,
	 *      A user-defined threshold is set
	 *      Driver will halt runtime services when this custom threshold is reached.
	 */
	if (amdgv_bad_page_threshold == NONSTOP_OVER_THRESHOLD) {
		cfg->work_mode_over_thresh = RAS_WORK_MODE_OVER_THRESH_NORMAL;
		amdgv_gpuiov_get_total_avail_fb_size(adapt, &total_fb_in_mb);
		badpages = ESTIMATE_BAD_PAGE_THRESHOLD(total_fb_in_mb);
	} else if (amdgv_bad_page_threshold == WARN_NONSTOP_OVER_THRESHOLD) {
		cfg->work_mode_over_thresh = RAS_WORK_MODE_OVER_THRESH_STRICT;
		badpages = COUNT_BAD_PAGE_THRESHOLD(RAS_RESERVED_VRAM_SIZE_DEFAULT);
	} else if (!amdgv_bad_page_threshold) {
		cfg->work_mode_over_thresh = RAS_WORK_MODE_OVER_THRESH_DEBUG;
		badpages = 128;
	} else if (amdgv_bad_page_threshold > 0) {
		cfg->work_mode_over_thresh = RAS_WORK_MODE_OVER_THRESH_RMA;
		badpages = amdgv_bad_page_threshold;
	} else {
		RAS_DEV_ERR(adapt, "Invalid amdgv_bad_page_threshold value(%d)\n",
			amdgv_bad_page_threshold);
		return -RAS_CORE_EINVAL;
	}

	/* Convert bad page count to record count as the threshold value */
	cfg->eeprom_record_threshold_count =
		RAS_PAGES_TO_EEPROM_RECORDS(badpages, badpages_per_record);

	return 0;
}

static int ras_eeprom_i2c_transfer(struct ras_core_context *ras_core,
			struct i2c_msg *msg, u32 msg_count)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	struct ras_eeprom_control *eeprom = ras_core->eeprom_mgr.ras_eeprom;
	int ret = 0;

	if (!adapt->pp.pp_funcs || !adapt->pp.pp_funcs->i2c_eeprom_xfer)
		return AMDGV_FAILURE;

	oss_mutex_lock(adapt->smu_i2c_lock);
	ret = adapt->pp.pp_funcs->i2c_eeprom_xfer(adapt,
		eeprom->i2c_port, (struct i2c_msg *)msg, msg_count);
	oss_mutex_unlock(adapt->smu_i2c_lock);

	return ret;
}

static int ras_eeprom_format_i2c_msg(struct ras_core_context *ras_core,
			struct ras_eeprom_control *control,
			struct i2c_msg *msg, unsigned char *buff,
			uint16_t buff_len, uint32_t addr, bool write)
{
	/*
	 * Update bits 16,17 of EEPROM address in I2C address by setting them
	 * to bits 1,2 of Device address byte
	 *
	 * To check if we overflow page boundary
	 * https://www.st.com/resource/en/datasheet/m24m02-dr.pdf sec. 5.1.2
	 */
	msg->addr = control->i2c_address |
			((addr & EEPROM_ADDR_MSB_MASK) >> 15);
	msg->flags = write ? I2C_M_WR : I2C_M_RD;
	msg->len = buff_len; //EEPROM_ADDRESS_SIZE + EEPROM_TABLE_RECORD_SIZE;
	msg->buf = buff;

	/* Insert the EEPROM dest addess, bits 0-15 */
	buff[0] = ((addr >> 8) & 0xff);
	buff[1] = (addr & 0xff);

	return 0;
}

static int ras_eeprom_i2c_xfer(struct ras_core_context *ras_core,
			u32 eeprom_addr, u8 *eeprom_buf, u32 buf_size, bool read)
{
	struct ras_eeprom_control *eeprom = ras_core->eeprom_mgr.ras_eeprom;
	struct i2c_msg msg = { 0 };
	int msg_num = 1;
	uint32_t next_addr;
	u8 *i2c_data_buf;
	u8 *buf_ptr = eeprom_buf;
	int r;
	u16 len;

	for (r = 0; buf_size > 0;
	      buf_size -= len, eeprom_addr += len, buf_ptr += len) {

		if (!read) {
			/* Write the maximum amount of data, without
			 * crossing the device's page boundary, as per
			 * its spec. Partial page writes are allowed,
			 * starting at any location within the page,
			 * so long as the page boundary isn't crossed
			 * over (actually the page pointer rolls
			 * over).
			 *
			 * As per the AT24CM02 EEPROM spec, after
			 * writing into a page, the I2C driver should
			 * terminate the transfer, i.e. in
			 * "i2c_transfer()" below, with a STOP
			 * condition, so that the self-timed write
			 * cycle begins. This is implied for the
			 * "i2c_transfer()" abstraction.
			 */
			len = EEPROM_PAGE_SIZE - (eeprom_addr & EEPROM_PAGE_MASK);
			if (buf_size < len)
				len = buf_size;
		} else {
			/* Reading from the EEPROM has no limitation
			 * on the number of bytes read from the EEPROM
			 * device--they are simply sequenced out.
			 * Keep in mind that i2c_msg.len is u16 type.
			 */
			len = (buf_size < OSS_U16_MAX) ? buf_size : OSS_U16_MAX;
		}

		i2c_data_buf = oss_zalloc(len + EEPROM_ADDRESS_SIZE);
		if (!i2c_data_buf)
			return AMDGV_FAILURE;

		if (!read)
			oss_memcpy(i2c_data_buf + EEPROM_ADDRESS_SIZE, buf_ptr, len);

		next_addr = eeprom_addr - eeprom->i2c_address;
		ras_eeprom_format_i2c_msg(ras_core, eeprom, &msg, i2c_data_buf,
			len + EEPROM_ADDRESS_SIZE, next_addr, !read);

		r = ras_eeprom_i2c_transfer(ras_core, &msg, msg_num);
		if (r != msg_num) {
			oss_free(i2c_data_buf);
			break;
		}

		if (read)
			oss_memcpy(buf_ptr, i2c_data_buf + EEPROM_ADDRESS_SIZE, len);

		oss_free(i2c_data_buf);
	}

	return r < 0 ? r : buf_ptr - eeprom_buf;
}

const struct ras_eeprom_sys_func amdgv_ras_eeprom_i2c_sys_func = {
	.eeprom_i2c_xfer = ras_eeprom_i2c_xfer,
	.get_eeprom_config = ras_eeprom_i2c_config,
};

#endif
