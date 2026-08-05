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

#include "amdgv.h"
#include "amdgv_device.h"
#include "amdgv_gpumon_internal.h"
#include "ras_sys.h"
#include "amdgv_ras_mgr.h"
#include "amdgv_ras_cmd.h"
#include "amdgv_ras_gpumon.h"

static int amdgv_ras_gpumon_get_ecc_info(struct amdgv_adapter *adapt,
		struct amdgv_smi_ras_query_if *info)
{
	struct ras_cmd_block_ecc_info_req req = {0};
	struct ras_cmd_block_ecc_info_rsp rsp = {0};
	int ret;

	req.block_id = info->head.block;
	req.subblock_id = info->head.sub_block_index;

	ret = amdgv_ras_mgr_handle_ras_cmd(adapt, RAS_CMD__GET_BLOCK_ECC_STATUS,
			&req, sizeof(req), &rsp, sizeof(rsp));
	if (!ret) {
		info->ce_count = rsp.ce_count;
		info->ue_count = rsp.ue_count;
		info->de_count = rsp.de_count;
	}

	return ret;
}

static int amdgv_ras_gpumon_get_badpage_count(struct amdgv_adapter *adapt, int *bp_cnt)
{
	struct ras_cmd_bad_pages_info_req cmd_input;
	struct ras_cmd_bad_pages_info_rsp *output;
	int ret = 0;

	if (!bp_cnt)
		return -RAS_CORE_EINVAL;

	output = oss_zalloc(sizeof(*output));
	if (!output)
		return -RAS_CORE_ENOMEM;

	oss_memset(&cmd_input, 0, sizeof(cmd_input));

	ret = amdgv_ras_mgr_handle_ras_cmd(adapt, RAS_CMD__GET_BAD_PAGES,
		&cmd_input, sizeof(cmd_input), output, sizeof(*output));
	if (ret)
		goto out;

	*bp_cnt = output->bp_total_cnt;
out:
	oss_free(output);
	return ret;
}

static int amdgv_ras_gpumon_get_badpage_info(struct amdgv_adapter *adapt,
	void *input, struct amdgv_smi_ras_eeprom_table_record *record)
{
	struct ras_cmd_bad_pages_info_req cmd_input;
	struct ras_cmd_bad_pages_info_rsp *output;
	struct ras_cmd_bad_page_record *uniras_rec;
	uint32_t group, pos_in_group;
	uint32_t index;
	int ret = 0;

	if (!input || !record)
		return -RAS_CORE_EINVAL;

	output = oss_zalloc(sizeof(*output));
	if (!output)
		return -RAS_CORE_ENOMEM;

	index = *((uint32_t *)input);
	group = index / RAS_CMD_MAX_BAD_PAGES_PER_GROUP;

	oss_memset(&cmd_input, 0, sizeof(cmd_input));
	cmd_input.group_index = group;

	ret = amdgv_ras_mgr_handle_ras_cmd(adapt, RAS_CMD__GET_BAD_PAGES,
		&cmd_input, sizeof(cmd_input), output, sizeof(*output));
	if (ret)
		goto out;

	pos_in_group = index - group * RAS_CMD_MAX_BAD_PAGES_PER_GROUP;
	uniras_rec = &output->records[pos_in_group];

	record->address = uniras_rec->address;
	record->retired_page = uniras_rec->retired_page;
	record->ts = uniras_rec->ts;
	record->err_type = uniras_rec->err_type;
	record->bank = uniras_rec->bank;
	record->mem_channel = uniras_rec->mem_channel;
	record->mcumc_id = uniras_rec->mcumc_id;

out:
	oss_free(output);
	return ret;
}

static int amdgv_ras_gpumon_get_ras_eeprom_version(struct amdgv_adapter *adapt,
			uint32_t *ras_eeprom_version)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	if (!ras_mgr)
		return -RAS_CORE_EACCES;

	return ras_core_get_eeprom_version(ras_mgr->ras_core, ras_eeprom_version);
}

static int amdgv_ras_gpumon_get_ecc_correction_schema(struct amdgv_adapter *adapt,
		uint32_t *ecc_correction_schema)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);

	if (!ras_mgr)
		return -RAS_CORE_EACCES;

	*ecc_correction_schema |=
		(1 << AMDGV_RAS_ECC_SUPPORT_PARITY) |
		(1 << AMDGV_RAS_ECC_SUPPORT_CORRECTABLE) |
		(1 << AMDGV_RAS_ECC_SUPPORT_UNCORRECTABLE) |
		(1 << AMDGV_RAS_ECC_SUPPORT_POISON);

	return 0;
}

#define EST_SIZE_PER_CPER  0x380  //Byte
/*
 * Minimum buffer size to hint for the next GET_ENTRIES call. Sized to fit one
 * fatal CPER record containing up to 32 error sections (~376 bytes each):
 * 32 * 0x178 = 0x2F00
 */
#define MAX_SIZE_SINGLE_CPER 0x2F00 //Byte
static int amdgv_ras_gpumon_cper_get_count(struct amdgv_adapter *adapt,
		struct amdgv_gpumon_cper *cper)
{
	struct ras_cmd_cper_snapshot_req snapshot_req = {0};
	struct ras_cmd_cper_snapshot_rsp snapshot_rsp = {0};
	uint64_t valid_rptr;
	uint64_t cper_total_size;
	int ret;

	if (!cper)
		return -RAS_CORE_EINVAL;

	ret = amdgv_ras_mgr_handle_ras_cmd(adapt,
			RAS_CMD__GET_CPER_SNAPSHOT,
			&snapshot_req, sizeof(struct ras_cmd_cper_snapshot_req),
			&snapshot_rsp, sizeof(struct ras_cmd_cper_snapshot_rsp));
	if (ret || !snapshot_rsp.total_cper_num) {
		*(cper->get_count.wptr) = 0;
		*(cper->get_count.avail_count) = 0;
		*(cper->get_count.size) = 0;
		return ret;
	}

	valid_rptr = snapshot_rsp.start_cper_id > cper->get_count.rptr ?
		snapshot_rsp.start_cper_id : cper->get_count.rptr;
	if (valid_rptr > snapshot_rsp.latest_cper_id)
		return -RAS_CORE_EINVAL;

	*(cper->get_count.wptr) = snapshot_rsp.latest_cper_id;
	*(cper->get_count.avail_count) = snapshot_rsp.latest_cper_id - valid_rptr;
	cper_total_size = *(cper->get_count.avail_count) * EST_SIZE_PER_CPER;
	if (cper_total_size != 0) {
		*(cper->get_count.size) = cper_total_size > MAX_SIZE_SINGLE_CPER ? cper_total_size : MAX_SIZE_SINGLE_CPER;
	} else {
		*(cper->get_count.size) = 0;
	}

	return 0;
}

static int amdgv_ras_gpumon_cper_get_entries(struct amdgv_adapter *adapt,
		struct amdgv_gpumon_cper *cper)
{
	struct ras_cmd_cper_snapshot_req snapshot_req = {0};
	struct ras_cmd_cper_snapshot_rsp snapshot_rsp = {0};
	struct ras_cmd_cper_record_req record_req = {0};
	struct ras_cmd_cper_record_rsp record_rsp = {0};
	uint64_t valid_rptr;
	uint64_t cper_total_size;
	int ret;

	if (!cper)
		return -RAS_CORE_EINVAL;

	ret = amdgv_ras_mgr_handle_ras_cmd(adapt,
			RAS_CMD__GET_CPER_SNAPSHOT,
			&snapshot_req, sizeof(struct ras_cmd_cper_snapshot_req),
			&snapshot_rsp, sizeof(struct ras_cmd_cper_snapshot_rsp));
	if (ret || !snapshot_rsp.total_cper_num) {
		*(cper->get_entries.write_count) = 0;
		*(cper->get_entries.overflow_count) = 0;
		*(cper->get_entries.left_size) = 0;
		return ret;
	}

	record_req.buf_ptr = (uintptr_t)cper->get_entries.buf;
	record_req.buf_size = cper->get_entries.buf_size;
	valid_rptr = snapshot_rsp.start_cper_id > cper->get_entries.rptr ?
		snapshot_rsp.start_cper_id : cper->get_entries.rptr;
	if (valid_rptr > snapshot_rsp.latest_cper_id)
		return -RAS_CORE_EINVAL;

	record_req.cper_start_id = valid_rptr;
	/* cper_num is u32; clamp to avoid silent truncation on an unusually large ring */
	record_req.cper_num = (uint32_t)min_t(uint64_t,
		snapshot_rsp.latest_cper_id - valid_rptr, (uint64_t)(uint32_t)~0U);
	ret = amdgv_ras_mgr_handle_ras_cmd(adapt, RAS_CMD__GET_CPER_RECORD,
					&record_req, sizeof(struct ras_cmd_cper_record_req),
					&record_rsp, sizeof(struct ras_cmd_cper_record_rsp));
	if (ret)
		return ret;

	*(cper->get_entries.write_count) = record_rsp.real_cper_num;
	cper_total_size = record_rsp.remain_num * EST_SIZE_PER_CPER;
	if (cper_total_size != 0) {
		*(cper->get_entries.left_size) = cper_total_size > MAX_SIZE_SINGLE_CPER ? cper_total_size : MAX_SIZE_SINGLE_CPER;
	} else {
		*(cper->get_entries.left_size) = 0;
	}
	*(cper->get_entries.overflow_count) = snapshot_rsp.start_cper_id > cper->get_entries.rptr ?
		snapshot_rsp.start_cper_id - cper->get_entries.rptr : 0;

	return ret;
}

static int amdgv_ras_gpumon_get_ras_caps(struct amdgv_adapter *adapt,
			struct amdgv_smi_ras_caps *ras_caps)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	struct ras_cmd_get_ras_cap_req req = {0};
	struct ras_cmd_get_ras_cap_rsp rsp = {0};
	int ret;

	if (!ras_mgr || !ras_caps)
		return -RAS_CORE_EACCES;

	ret = amdgv_ras_mgr_handle_ras_cmd(adapt, RAS_CMD__GET_RAS_CAP,
			&req, sizeof(struct ras_cmd_get_ras_cap_req),
			&rsp, sizeof(struct ras_cmd_get_ras_cap_rsp));
	if (ret)
		return ret;

	ras_caps->ras_block_mask = rsp.ras_block_mask;

	ras_caps->ecc_type = rsp.ext_ecc_type;

	return 0;
}

int amdgv_ras_mgr_handle_gpumon_req(struct amdgv_adapter *adapt,
		uint32_t type, void *input, void *output)
{
	switch (type) {
	case GPUMON_GET_ECC_INFO:
		return amdgv_ras_gpumon_get_ecc_info(adapt, input);
	case GPUMON_GET_BAD_PAGE_COUNT:
		return amdgv_ras_gpumon_get_badpage_count(adapt, output);
	case GPUMON_GET_BAD_PAGE_INFO:
		return amdgv_ras_gpumon_get_badpage_info(adapt, input, output);
	case GPUMON_GET_BAD_PAGE_THRESHOLD:
		break;
	case GPUMON_GET_RAS_EEPROM_VERSION:
		return amdgv_ras_gpumon_get_ras_eeprom_version(adapt, output);
	case GPUMON_GET_ECC_CAP:
		return amdgv_ras_gpumon_get_ras_caps(adapt, output);
	case GPUMON_GET_ECC_CORRECTION_SCHEMA:
		return amdgv_ras_gpumon_get_ecc_correction_schema(adapt, output);
	case GPUMON_CPER_GET_COUNT:
		return amdgv_ras_gpumon_cper_get_count(adapt, input);
	case GPUMON_CPER_GET_ENTRIES:
		return amdgv_ras_gpumon_cper_get_entries(adapt, input);
	case GPUMON_GET_RAS_POLICY_INFO:
		break;
	default:
		break;
	}

	return AMDGV_LOG_GPUMON_NOT_SUPPORTED;
}
