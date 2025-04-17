
/*
 * Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
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

#include "amdgv_device.h"
#include "amdgv_oss_wrapper.h"
#include "amdgv_mca.h"
#include "amd_cper.h"

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

int amdgv_mca_get_new_banks(struct amdgv_adapter *adapt, enum amdgv_mca_error_type type)
{
	int ret = AMDGV_FAILURE;

	if (adapt->mca.funcs && adapt->mca.funcs->get_new_banks) {
		ret = adapt->mca.funcs->get_new_banks(adapt, type);
	} else {
		AMDGV_ERROR("Cannot get MCA bank info\n");
		ret = AMDGV_FAILURE;
	}

	return ret;
}

int amdgv_mca_reset_block_error_count(struct amdgv_adapter *adapt, enum amdgv_ras_block block)
{
	int ret;

	if (!adapt->mca.funcs) {
		/* For asics that do not support funcs return OK*/
		ret = 0;
	} else if (adapt->mca.funcs->reset_block_error_count) {
		ret = adapt->mca.funcs->reset_block_error_count(adapt, block);
	} else {
		AMDGV_ERROR("Cannot reset MCA block error count\n");
		ret = AMDGV_FAILURE;
	}

	return ret;
}

int amdgv_mca_decode_block(struct amdgv_adapter *adapt, struct mca_bank_entry *bank,
			   enum amdgv_ras_block *block)
{
	int ret;

	if (adapt->mca.funcs && adapt->mca.funcs->decode_block) {
		ret = adapt->mca.funcs->decode_block(adapt, bank, block);
	} else {
		AMDGV_ERROR("Cannot decode MCA Bank\n");
		ret = AMDGV_FAILURE;
	}

	return ret;
}

static int amdgv_mca_count_cache_reset_client(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	struct amdgv_mca_error_count_cache_client *client;
	struct amdgv_mca_error_count_cache *cache;
	int i;

	client = &adapt->mca.count_cache.client[idx_vf];

	for (i = 0; i < AMDGV_RAS_BLOCK__LAST; i++) {
		cache = &client->cache[i];
		cache->ce_count = 0;
		cache->ue_count = 0;
		cache->de_count = 0;
		cache->ce_overflow_count = 0;
		cache->ue_overflow_count = 0;
		cache->de_overflow_count = 0;
	}

	return 0;
}

static int amdgv_mca_count_cache_add_client(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	struct amdgv_mca_error_count_cache_client *client;
	client = &adapt->mca.count_cache.client[idx_vf];

	if ((adapt->mca.vf_policy == AMDGV_RAS_VF_TELEMETRY_DISABLE) &&
	    (idx_vf != AMDGV_PF_IDX))
		return 0;

	client->enabled = true;

	return 0;
}

static int amdgv_mca_count_cache_remove_client(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	struct amdgv_mca_error_count_cache_client *client;

	client = &adapt->mca.count_cache.client[idx_vf];

	amdgv_mca_count_cache_reset_client(adapt, idx_vf);

	client->enabled = false;

	return 0;
}

#define TEST_OVERFLOW(old_cnt, new_cnt) (old_cnt + new_cnt < old_cnt ? true : false)

static int amdgv_mca_count_cache_put_client(struct amdgv_adapter *adapt,
					     uint32_t ce_count,
					     uint32_t ue_count,
					     uint32_t de_count,
					     enum amdgv_ras_block block,
					     uint32_t idx_vf)
{
	struct amdgv_mca_error_count_cache_client *client;
	struct amdgv_mca_error_count_cache *cache;

	client = &adapt->mca.count_cache.client[idx_vf];
	cache = &adapt->mca.count_cache.client[idx_vf].cache[block];


	if (!client->enabled)
		return 0;

	if (TEST_OVERFLOW(cache->ce_count, ce_count))
		cache->ce_overflow_count++;
	if (TEST_OVERFLOW(cache->de_count, de_count))
		cache->de_overflow_count++;
	if (TEST_OVERFLOW(cache->ue_count, ue_count))
		cache->ue_overflow_count++;

	cache->ce_count += ce_count;
	cache->de_count += de_count;
	cache->ue_count += ue_count;

	return 0;
}

int amdgv_mca_count_cache_put(struct amdgv_adapter *adapt,
			       uint32_t ce_count,
			       uint32_t ue_count,
			       uint32_t de_count,
			       enum amdgv_ras_block block)
{
	uint32_t idx_vf;

	if (!(adapt->ecc.ras_cap & BIT(block)))
		return 0;

	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++)
		amdgv_mca_count_cache_put_client(adapt,
			ce_count, ue_count, de_count, block, idx_vf);

	amdgv_mca_count_cache_put_client(adapt,
			ce_count, ue_count, de_count, block, AMDGV_PF_IDX);

	return 0;
}

int amdgv_mca_count_cache_client_get(struct amdgv_adapter *adapt,
				     uint32_t *ce_count,
				     uint32_t *ue_count,
				     uint32_t *de_count,
				     uint32_t *ce_overflow_count,
				     uint32_t *ue_overflow_count,
				     uint32_t *de_overflow_count,
				     enum amdgv_ras_block block,
				     uint32_t idx_vf)
{
	int ret = 0;
	struct amdgv_mca_error_count_cache_client *client;
	struct amdgv_mca_error_count_cache *cache;

	if (!(adapt->ecc.ras_cap & BIT(block)))
		return 0;

	client = &adapt->mca.count_cache.client[idx_vf];
	cache = &adapt->mca.count_cache.client[idx_vf].cache[block];

	if (client->enabled) {
		*ce_count = cache->ce_count;
		*de_count = cache->de_count;
		*ue_count = cache->ue_count;
		*ce_overflow_count = cache->ce_overflow_count;
		*de_overflow_count = cache->de_overflow_count;
		*ue_overflow_count = cache->ue_overflow_count;
	} else {
		ret = AMDGV_FAILURE;
	}

	return ret;
}

int amdgv_mca_count_cache_reset(struct amdgv_adapter *adapt)
{
	int idx_vf;

	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++)
		amdgv_mca_count_cache_reset_client(adapt, idx_vf);

	amdgv_mca_count_cache_reset_client(adapt, AMDGV_PF_IDX);

	return 0;
}

int amdgv_mca_count_cache_init(struct amdgv_adapter *adapt)
{
	/* Always enable PF */
	amdgv_mca_count_cache_add_client(adapt, AMDGV_PF_IDX);

	amdgv_mca_cache_notify_event(adapt,
				     MCA_CACHE_EVENT_HOST_LOAD,
				     AMDGV_PF_IDX, 0);

	return 0;
}

int amdgv_mca_count_cache_fini(struct amdgv_adapter *adapt)
{
	amdgv_mca_count_cache_reset(adapt);

	return 0;
}

/* Update client states depending on policy */
int amdgv_mca_cache_notify_event(struct amdgv_adapter *adapt,
				 enum amdgv_mca_cache_event event,
				 uint32_t idx_vf,
				 uint32_t param)
{
	uint32_t tmp_vf;

	switch (event) {
	case MCA_CACHE_EVENT_HOST_LOAD:
		amdgv_mca_count_cache_add_client(adapt, AMDGV_PF_IDX);
		if (adapt->mca.vf_policy == AMDGV_RAS_VF_TELEMETRY_LOG_ON_HOST_LOAD)
			for (tmp_vf = 0; tmp_vf < adapt->num_vf; tmp_vf++)
				amdgv_mca_count_cache_add_client(adapt, tmp_vf);
		break;
	case MCA_CACHE_EVENT_GUEST_LOAD:
		if (adapt->mca.vf_policy == AMDGV_RAS_VF_TELEMETRY_LOG_ON_GUEST_LOAD)
			amdgv_mca_count_cache_reset_client(adapt, idx_vf);
		amdgv_mca_count_cache_add_client(adapt, idx_vf);
		break;
	case MCA_CACHE_EVENT_NUM_VF_CHANGE:
		if (param > 1) {
			AMDGV_WARN("RAS VF Telemetry is not supported for multi-VF");
			for (tmp_vf = 0; tmp_vf < adapt->num_vf; tmp_vf++)
				amdgv_mca_count_cache_remove_client(adapt, tmp_vf);
			break;
		}
		if (adapt->mca.vf_policy == AMDGV_RAS_VF_TELEMETRY_LOG_ON_HOST_LOAD) {
			for (tmp_vf = 0; tmp_vf < adapt->num_vf; tmp_vf++)
				amdgv_mca_count_cache_remove_client(adapt, tmp_vf);
			for (tmp_vf = 0; tmp_vf < param; tmp_vf++)
				amdgv_mca_count_cache_add_client(adapt, tmp_vf);
		}
		break;
	default:
		AMDGV_WARN("Unsupported MCA cache notify event: %d\n", event);
		break;
	}

	return 0;
}

static enum cper_error_severity amdgv_mca_type_to_cper_sev(struct amdgv_adapter *adapt,
							   enum amdgv_mca_error_type type)
{
	switch (type) {
	case AMDGV_MCA_ERROR_TYPE_UE:
		return CPER_SEV_FATAL;
	case AMDGV_MCA_ERROR_TYPE_CE:
		return CPER_SEV_NON_FATAL_CORRECTED;
	case AMDGV_MCA_ERROR_TYPE_DE:
		return CPER_SEV_NON_FATAL_UNCORRECTED;
	default:
		AMDGV_ERROR("Unknown MCA Type!\n");
		return CPER_SEV_FATAL;
	}
}

static int amdgv_mca_generate_ue_cper(struct amdgv_adapter *adapt,
			       struct mca_bank_entry *bank)
{
	struct cper_hdr *fatal = NULL;
	struct cper_sec_crashdump_reg_data reg_data = { 0 };

	fatal = amdgv_cper_alloc_entry(adapt, AMDGV_CPER_TYPE_FATAL, 1);
	if (!fatal)
		return AMDGV_FAILURE;

	reg_data.status_lo = lower_32_bits(bank->regs[MCA_REG_IDX_STATUS]);
	reg_data.status_hi = upper_32_bits(bank->regs[MCA_REG_IDX_STATUS]);
	reg_data.addr_lo   = lower_32_bits(bank->regs[MCA_REG_IDX_ADDR]);
	reg_data.addr_hi   = upper_32_bits(bank->regs[MCA_REG_IDX_ADDR]);
	reg_data.ipid_lo   = lower_32_bits(bank->regs[MCA_REG_IDX_IPID]);
	reg_data.ipid_hi   = upper_32_bits(bank->regs[MCA_REG_IDX_IPID]);
	reg_data.synd_lo   = lower_32_bits(bank->regs[MCA_REG_IDX_SYND]);
	reg_data.synd_hi   = upper_32_bits(bank->regs[MCA_REG_IDX_SYND]);

	amdgv_cper_entry_fill_hdr(adapt, fatal, AMDGV_CPER_TYPE_FATAL, CPER_SEV_FATAL);
	amdgv_cper_entry_fill_fatal_section(adapt, fatal, 0, reg_data);
	amdgv_cper_commit_entry(adapt, fatal);

	return 0;
}

static int amdgv_mca_generate_ce_cper(struct amdgv_adapter *adapt,
			       struct mca_bank_entry *banks,
			       uint16_t bank_count)
{
	struct cper_hdr *corrected = NULL;
	enum cper_error_severity sev = CPER_SEV_NON_FATAL_CORRECTED;
	uint32_t reg_data[CPER_ACA_REG_COUNT] = { 0 };
	uint32_t i;

	corrected = amdgv_cper_alloc_entry(adapt, AMDGV_CPER_TYPE_RUNTIME, bank_count);
	if (!corrected)
		return AMDGV_FAILURE;

	/* Raise severity if DEs are detected */
	for (i = 0; i < bank_count; i++) {
		if (banks[i].type == AMDGV_MCA_ERROR_TYPE_DE) {
			sev = CPER_SEV_NON_FATAL_UNCORRECTED;
			break;
		}
	}

	amdgv_cper_entry_fill_hdr(adapt, corrected, AMDGV_CPER_TYPE_RUNTIME, sev);

	for (i = 0; i < bank_count; i++) {
		reg_data[CPER_ACA_REG_CTL_LO]    = lower_32_bits(banks[i].regs[MCA_REG_IDX_CTL]);
		reg_data[CPER_ACA_REG_CTL_HI]    = upper_32_bits(banks[i].regs[MCA_REG_IDX_CTL]);
		reg_data[CPER_ACA_REG_STATUS_LO] = lower_32_bits(banks[i].regs[MCA_REG_IDX_STATUS]);
		reg_data[CPER_ACA_REG_STATUS_HI] = upper_32_bits(banks[i].regs[MCA_REG_IDX_STATUS]);
		reg_data[CPER_ACA_REG_ADDR_LO]   = lower_32_bits(banks[i].regs[MCA_REG_IDX_ADDR]);
		reg_data[CPER_ACA_REG_ADDR_HI]   = upper_32_bits(banks[i].regs[MCA_REG_IDX_ADDR]);
		reg_data[CPER_ACA_REG_MISC0_LO]  = lower_32_bits(banks[i].regs[MCA_REG_IDX_MISC0]);
		reg_data[CPER_ACA_REG_MISC0_HI]  = upper_32_bits(banks[i].regs[MCA_REG_IDX_MISC0]);
		reg_data[CPER_ACA_REG_CONFIG_LO] = lower_32_bits(banks[i].regs[MCA_REG_IDX_CONFIG]);
		reg_data[CPER_ACA_REG_CONFIG_HI] = upper_32_bits(banks[i].regs[MCA_REG_IDX_CONFIG]);
		reg_data[CPER_ACA_REG_IPID_LO]   = lower_32_bits(banks[i].regs[MCA_REG_IDX_IPID]);
		reg_data[CPER_ACA_REG_IPID_HI]   = upper_32_bits(banks[i].regs[MCA_REG_IDX_IPID]);
		reg_data[CPER_ACA_REG_SYND_LO]   = lower_32_bits(banks[i].regs[MCA_REG_IDX_SYND]);
		reg_data[CPER_ACA_REG_SYND_HI]   = upper_32_bits(banks[i].regs[MCA_REG_IDX_SYND]);

		amdgv_cper_entry_fill_runtime_section(adapt, corrected, i,
						      amdgv_mca_type_to_cper_sev(adapt, banks[i].type),
						      reg_data, CPER_ACA_REG_COUNT);
	}

	amdgv_cper_commit_entry(adapt, corrected);

	return 0;
}

int amdgv_mca_generate_cpers(struct amdgv_adapter *adapt, enum amdgv_mca_error_type type,
			     struct mca_bank_entry *banks, uint16_t bank_count)
{
	int i = 0;
	int ret = 0;

	if (!bank_count)
		return 0;

	/* Generally expect 1 UE at a time. But in case multiple UEs
	 * Are detected, they must be encoded into seperate CPER entries */
	if (type == AMDGV_MCA_ERROR_TYPE_UE) {
		for (i = 0; i < bank_count; i++) {
			ret = amdgv_mca_generate_ue_cper(adapt, &banks[i]);
			if (ret)
				break;
		}
	} else {
		ret = amdgv_mca_generate_ce_cper(adapt, banks, bank_count);
	}

	return ret;
}

int amdgv_mca_get_fatal_bank_from_cper_section(struct amdgv_adapter *adapt, struct cper_hdr *hdr,
					       uint32_t idx,
					       struct mca_bank_entry *bank)
{
	struct cper_sec_crashdump_reg_data *reg_data = NULL;

	amdgv_cper_get_crashdump(adapt, hdr, idx, &reg_data);

	bank->regs[MCA_REG_IDX_STATUS] |= (uint64_t)reg_data->status_lo;
	bank->regs[MCA_REG_IDX_STATUS] |= (uint64_t)reg_data->status_hi << 32;
	bank->regs[MCA_REG_IDX_ADDR]   |= (uint64_t)reg_data->addr_lo;
	bank->regs[MCA_REG_IDX_ADDR]   |= (uint64_t)reg_data->addr_hi << 32;
	bank->regs[MCA_REG_IDX_IPID]   |= (uint64_t)reg_data->ipid_lo;
	bank->regs[MCA_REG_IDX_IPID]   |= (uint64_t)reg_data->ipid_hi << 32;
	bank->regs[MCA_REG_IDX_SYND]   |= (uint64_t)reg_data->synd_lo;
	bank->regs[MCA_REG_IDX_SYND]   |= (uint64_t)reg_data->synd_hi << 32;

	return 0;
}

int amdgv_mca_get_runtime_bank_from_cper_section(struct amdgv_adapter *adapt, struct cper_hdr *hdr,
						 uint32_t idx,
						 struct mca_bank_entry *bank)
{
	uint32_t *reg_data = NULL;

	amdgv_cper_get_runtime_reg_dump(adapt, hdr, idx, &reg_data);

	bank->regs[MCA_REG_IDX_CTL]    |= (uint64_t)reg_data[CPER_ACA_REG_CTL_LO];
	bank->regs[MCA_REG_IDX_CTL]    |= (uint64_t)reg_data[CPER_ACA_REG_CTL_HI] << 32;
	bank->regs[MCA_REG_IDX_STATUS] |= (uint64_t)reg_data[CPER_ACA_REG_STATUS_LO];
	bank->regs[MCA_REG_IDX_STATUS] |= (uint64_t)reg_data[CPER_ACA_REG_STATUS_HI] << 32;
	bank->regs[MCA_REG_IDX_ADDR]   |= (uint64_t)reg_data[CPER_ACA_REG_ADDR_LO];
	bank->regs[MCA_REG_IDX_ADDR]   |= (uint64_t)reg_data[CPER_ACA_REG_ADDR_HI] << 32;
	bank->regs[MCA_REG_IDX_MISC0]  |= (uint64_t)reg_data[CPER_ACA_REG_MISC0_LO];
	bank->regs[MCA_REG_IDX_MISC0]  |= (uint64_t)reg_data[CPER_ACA_REG_MISC0_HI] << 32;
	bank->regs[MCA_REG_IDX_CONFIG] |= (uint64_t)reg_data[CPER_ACA_REG_CONFIG_LO];
	bank->regs[MCA_REG_IDX_CONFIG] |= (uint64_t)reg_data[CPER_ACA_REG_CONFIG_HI] << 32;
	bank->regs[MCA_REG_IDX_IPID]   |= (uint64_t)reg_data[CPER_ACA_REG_IPID_LO];
	bank->regs[MCA_REG_IDX_IPID]   |= (uint64_t)reg_data[CPER_ACA_REG_IPID_HI] << 32;
	bank->regs[MCA_REG_IDX_SYND]   |= (uint64_t)reg_data[CPER_ACA_REG_SYND_LO];
	bank->regs[MCA_REG_IDX_SYND]   |= (uint64_t)reg_data[CPER_ACA_REG_SYND_HI] << 32;

	return 0;
}

int amdgv_mca_export_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_mca *mca_data)
{
	uint32_t idx_live_data, idx_vf, i, j;
	struct live_info_mca_cache *cache;
	struct live_info_mca_ecc *ecc;

	// Export mca client cache data
	for (idx_live_data = 0; idx_live_data < adapt->num_vf + 1; idx_live_data++) {
		if (idx_live_data >= AMDGV_MAX_VF_LIVE) {
			AMDGV_ERROR("VF MGR export live data error, slot# %u, %u live update slots\n", idx_live_data, AMDGV_MAX_VF_LIVE);
			return AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;
		}

		idx_vf = idx_live_data;
		if (idx_live_data == adapt->num_vf)
			idx_vf = AMDGV_PF_IDX;

		cache = &mca_data->mca_cache[idx_live_data];
		cache->enabled = adapt->mca.count_cache.client[idx_vf].enabled;

		for (i = 0; i < AMDGV_RAS_BLOCK_COUNT; i++) {

			if (i >= AMDGV_LIVE_MAX_RAS_BLOCK) {
				AMDGV_ERROR("VF MGR export live data error, ras block# %u, %u ras blocks\n", i, AMDGV_LIVE_MAX_RAS_BLOCK);
				return AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;
			}

			cache->err_count[i].ce_count =
				adapt->mca.count_cache.client[idx_vf].cache[i].ce_count;
			cache->err_count[i].ue_count =
				adapt->mca.count_cache.client[idx_vf].cache[i].ue_count;
			cache->err_count[i].de_count =
				adapt->mca.count_cache.client[idx_vf].cache[i].de_count;
			cache->err_count[i].ce_overflow_count =
				adapt->mca.count_cache.client[idx_vf].cache[i].ce_overflow_count;
			cache->err_count[i].ue_overflow_count =
				adapt->mca.count_cache.client[idx_vf].cache[i].ue_overflow_count;
			cache->err_count[i].de_overflow_count =
				adapt->mca.count_cache.client[idx_vf].cache[i].de_overflow_count;
		}
	}

	// Export mca ecc block data
	for (i = 0; i < AMDGV_RAS_BLOCK_COUNT; i++) {

		if (i >= AMDGV_LIVE_MAX_RAS_BLOCK) {
			AMDGV_ERROR("VF MGR export live data error, ras block# %u, %u ras blocks\n", i, AMDGV_LIVE_MAX_RAS_BLOCK);
			return AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;
		}

		ecc = &mca_data->mca_ecc[i];
		for (j = 0; j < adapt->mca.max_aid_xcd_num; j++) {

			if (j >= AMDGV_LIVE_MAX_XCD_NUM) {
				AMDGV_ERROR("VF MGR export live data error, xcd# %u, %u xcds\n", j, AMDGV_LIVE_MAX_XCD_NUM);
				return AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;
			}

			ecc->err_count[j].new_ce_count =
				adapt->mca.ecc_block[i].mca_ecc[j].new_ce_count;
			ecc->err_count[j].new_ue_count =
				adapt->mca.ecc_block[i].mca_ecc[j].new_ue_count;
			ecc->err_count[j].new_de_count =
				adapt->mca.ecc_block[i].mca_ecc[j].new_de_count;
			ecc->err_count[j].total_ce_count =
				adapt->mca.ecc_block[i].mca_ecc[j].total_ce_count;
			ecc->err_count[j].total_ue_count =
				adapt->mca.ecc_block[i].mca_ecc[j].total_ue_count;
			ecc->err_count[j].total_de_count =
				adapt->mca.ecc_block[i].mca_ecc[j].total_de_count;
		}
	}

	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}

int amdgv_mca_import_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_mca *mca_data)
{
	uint32_t idx_live_data, idx_vf, i, j;
	struct live_info_mca_cache *cache;
	struct live_info_mca_ecc *ecc;

	// Import mca client cache data
	for (idx_live_data = 0; idx_live_data < adapt->num_vf + 1; idx_live_data++) {
		if (idx_live_data >= AMDGV_MAX_VF_LIVE) {
			AMDGV_ERROR("VF MGR import live data error, slot# %u, %u live update slots\n", idx_live_data, AMDGV_MAX_VF_LIVE);
			return AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;
		}

		idx_vf = idx_live_data;
		if (idx_live_data == adapt->num_vf)
			idx_vf = AMDGV_PF_IDX;

		cache = &mca_data->mca_cache[idx_live_data];
		adapt->mca.count_cache.client[idx_vf].enabled = cache->enabled;

		for (i = 0; i < AMDGV_RAS_BLOCK_COUNT; i++) {

			if (i >= AMDGV_LIVE_MAX_RAS_BLOCK) {
				AMDGV_ERROR("VF MGR import live data error, ras block# %u, %u ras blocks\n", i, AMDGV_LIVE_MAX_RAS_BLOCK);
				return AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;
			}

			adapt->mca.count_cache.client[idx_vf].cache[i].ce_count =
				cache->err_count[i].ce_count;
			adapt->mca.count_cache.client[idx_vf].cache[i].ue_count =
				cache->err_count[i].ue_count;
			adapt->mca.count_cache.client[idx_vf].cache[i].de_count =
				cache->err_count[i].de_count;
			adapt->mca.count_cache.client[idx_vf].cache[i].ce_overflow_count =
				cache->err_count[i].ce_overflow_count;
			adapt->mca.count_cache.client[idx_vf].cache[i].ue_overflow_count =
				cache->err_count[i].ue_overflow_count;
			adapt->mca.count_cache.client[idx_vf].cache[i].de_overflow_count =
				cache->err_count[i].de_overflow_count;
		}
	}

	// Import mca ecc block data
	for (i = 0; i < AMDGV_RAS_BLOCK_COUNT; i++) {

		if (i >= AMDGV_LIVE_MAX_RAS_BLOCK) {
			AMDGV_ERROR("VF MGR import live data error, ras block# %u, %u ras blocks\n", i, AMDGV_LIVE_MAX_RAS_BLOCK);
			return AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;
		}

		ecc = &mca_data->mca_ecc[i];
		for (j = 0; j < adapt->mca.max_aid_xcd_num; j++) {

			if (j >= AMDGV_LIVE_MAX_XCD_NUM) {
				AMDGV_ERROR("VF MGR import live data error, xcd# %u, %u xcds\n", j, AMDGV_LIVE_MAX_XCD_NUM);
				return AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;
			}

			adapt->mca.ecc_block[i].mca_ecc[j].new_ce_count =
				ecc->err_count[j].new_ce_count;
			adapt->mca.ecc_block[i].mca_ecc[j].new_ue_count =
				ecc->err_count[j].new_ue_count;
			adapt->mca.ecc_block[i].mca_ecc[j].new_de_count =
				ecc->err_count[j].new_de_count;
			adapt->mca.ecc_block[i].mca_ecc[j].total_ce_count =
				ecc->err_count[j].total_ce_count;
			adapt->mca.ecc_block[i].mca_ecc[j].total_ue_count =
				ecc->err_count[j].total_ue_count;
			adapt->mca.ecc_block[i].mca_ecc[j].total_de_count =
				ecc->err_count[j].total_de_count;
		}
	}

	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}
