/*
 * Copyright (c) 2014-2024 Advanced Micro Devices, Inc. All rights reserved.
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

#include "amdgv.h"
#include "amdgv_device.h"
#include "amdgv_mca.h"
#include "mi300_mca.h"
#include "mi300_smu_ppsmc.h"
#include "mi300_powerplay.h"
#include "umc_v12_0.h"
#include "mi300.h"
#include "mi300_smu_driver_if.h"

#define MI300_MCA_IPID_GFX_XCD0    0x36430400   /* GFX SMNAID XCD 0 */
#define MI300_MCA_IPID_GFX_XCD1    0x38430400   /* GFX SMNAID XCD 1 */
#define MI300_MCA_IPID_SDMA_MMHUB  0x03b30400

#define MI300_MCA_MAX_VALID_MCA_COUNT	(12)

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

static int sdma_err_codes[] = { CODE_SDMA0, CODE_SDMA1, CODE_SDMA2, CODE_SDMA3 };
static int mmhub_err_codes[] = {
	CODE_DAGB0, CODE_DAGB0 + 1, CODE_DAGB0 + 2, CODE_DAGB0 + 3, CODE_DAGB0 + 4, /* DAGB0-4 */
	CODE_EA0, CODE_EA0 + 1, CODE_EA0 + 2, CODE_EA0 + 3, CODE_EA0 + 4,	/* MMEA0-4*/
	CODE_VML2, CODE_VML2_WALKER, CODE_MMCANE,
};

static int mi300_mca_ras_block_to_ue_chiplet_err_code(enum amdgv_ras_block block)
{
	switch (block) {
	case AMDGV_RAS_BLOCK__UMC:
		return AMDGV_ERROR_ECC_UMC_CHIPLET_UE;
	case AMDGV_RAS_BLOCK__GFX:
		return AMDGV_ERROR_ECC_GFX_CHIPLET_UE;
	case AMDGV_RAS_BLOCK__SDMA:
		return AMDGV_ERROR_ECC_SDMA_CHIPLET_UE;
	case AMDGV_RAS_BLOCK__MMHUB:
		return AMDGV_ERROR_ECC_MMHUB_CHIPLET_UE;
	case AMDGV_RAS_BLOCK__XGMI_WAFL:
		return AMDGV_ERROR_ECC_XGMI_WAFL_CHIPLET_UE;
	default:
		return AMDGV_ERROR_ECC_UNKNOWN_CHIPLET_UE;
	}
}

static int mi300_mca_ras_block_to_ce_chiplet_err_code(enum amdgv_ras_block block)
{
	switch (block) {
	case AMDGV_RAS_BLOCK__UMC:
		return AMDGV_ERROR_ECC_UMC_CHIPLET_CE;
	case AMDGV_RAS_BLOCK__GFX:
		return AMDGV_ERROR_ECC_GFX_CHIPLET_CE;
	case AMDGV_RAS_BLOCK__SDMA:
		return AMDGV_ERROR_ECC_SDMA_CHIPLET_CE;
	case AMDGV_RAS_BLOCK__MMHUB:
		return AMDGV_ERROR_ECC_MMHUB_CHIPLET_CE;
	case AMDGV_RAS_BLOCK__XGMI_WAFL:
		return AMDGV_ERROR_ECC_XGMI_WAFL_CHIPLET_CE;
	default:
		return AMDGV_ERROR_ECC_UNKNOWN_CHIPLET_CE;
	}
}

static int mi300_mca_ras_block_to_de_chiplet_err_code(enum amdgv_ras_block block)
{
	switch (block) {
	case AMDGV_RAS_BLOCK__UMC:
		return AMDGV_ERROR_ECC_UMC_CHIPLET_DE;
	default:
		return AMDGV_ERROR_ECC_UNKNOWN_CHIPLET_DE;
	}
}

static int mi300_mca_get_block_ecc_total_count(struct amdgv_adapter *adapt,
					enum amdgv_ras_block block,
					struct ras_err_data *err_data)
{
	uint32_t id;
	struct block_ecc  *block_ecc;

	if (block >= AMDGV_RAS_BLOCK__LAST)
		return AMDGV_FAILURE;

	if (!err_data)
		return AMDGV_FAILURE;

	block_ecc = &adapt->mca.ecc_block[block];

	for (id = 0; id < adapt->mca.max_aid_xcd_num; id++) {
		if ((block != AMDGV_RAS_BLOCK__GFX) && (id >= MI300_MAX_AID_NUM))
			break;
		err_data->ce_count += block_ecc->mca_ecc[id].total_ce_count;
		err_data->ue_count += block_ecc->mca_ecc[id].total_ue_count;
		err_data->de_count += block_ecc->mca_ecc[id].total_de_count;
	}

	return 0;
}

static int mi300_mca_push_chiplet_ecc_err(struct amdgv_adapter *adapt,
					enum amdgv_ras_block block,
					uint32_t socket_id, uint32_t die_id,
					enum amdgv_mca_error_type type,
					uint32_t new_count)
{
	struct ras_err_data err_data = { 0 };

	/* get total error counts from all MCA banks */
	mi300_mca_get_block_ecc_total_count(adapt, block, &err_data);

	switch (type) {
	case AMDGV_MCA_ERROR_TYPE_UE:
		amdgv_mca_count_cache_put(adapt, 0, new_count, 0, block);
		amdgv_put_error(AMDGV_PF_IDX,
				mi300_mca_ras_block_to_ue_chiplet_err_code(block),
				AMDGV_ERROR_16_16_16_16(socket_id,
							die_id,
							new_count,
							err_data.ue_count));
		break;
	case AMDGV_MCA_ERROR_TYPE_CE:
		amdgv_mca_count_cache_put(adapt, new_count, 0, 0, block);
		amdgv_put_error(AMDGV_PF_IDX,
				mi300_mca_ras_block_to_ce_chiplet_err_code(block),
				AMDGV_ERROR_16_16_16_16(socket_id,
							die_id,
							new_count,
							err_data.ce_count));
		break;
	case AMDGV_MCA_ERROR_TYPE_DE:
		amdgv_mca_count_cache_put(adapt, 0, 0, new_count, block);
		amdgv_put_error(AMDGV_PF_IDX,
				mi300_mca_ras_block_to_de_chiplet_err_code(block),
				AMDGV_ERROR_16_16_16_16(socket_id,
							die_id,
							new_count,
							err_data.de_count));
		break;
	default:
		AMDGV_ERROR("Unknown MCA error type\n");
		return AMDGV_FAILURE;
	}

	return 0;
}

static void mi300_mca_umc_add_err_addr(struct amdgv_adapter *adapt,
				struct mca_ecc *mca_ecc,
				struct mca_bank_entry *bank)
{
	struct mca_err_addr *mca_addr, *tmp;
	uint64_t err_addr = bank->regs[MCA_REG_IDX_ADDR];

	/* stop adding new address if it is a duplicate address*/
	amdgv_list_for_each_entry_safe(mca_addr, tmp, &mca_ecc->err_addr_list,
				struct mca_err_addr, node) {
		if (mca_addr->mca_addr == err_addr)
			return;
	}

	mca_addr = oss_alloc_memory(sizeof(*mca_addr));
	if (!mca_addr)
		return;

	AMDGV_INIT_LIST_HEAD(&mca_addr->node);

	mca_addr->mca_status = bank->regs[MCA_REG_IDX_STATUS];
	mca_addr->mca_ipid = bank->regs[MCA_REG_IDX_IPID];
	mca_addr->mca_addr = bank->regs[MCA_REG_IDX_ADDR];

	amdgv_list_add_tail(&mca_addr->node, &mca_ecc->err_addr_list);
}

void mi300_mca_umc_del_err_addr(struct amdgv_adapter *adapt,
				struct mca_ecc *mca_ecc,
				struct mca_err_addr *mca_addr)
{
	amdgv_list_del(&mca_addr->node);
	oss_free_memory(mca_addr);
}

static void mi300_mca_bank_decode_ipid(struct amdgv_adapter *adapt,
				struct mca_bank_entry *bank,
				struct mca_bank_info *info)
{
	uint64_t ipid = bank->regs[MCA_REG_IDX_IPID];
	uint32_t instidhi, instid;

	/* NOTE: All MCA IPID register share the same format,
	* so the driver can share the MCMP1 register header file.
	*/
	info->hwid = REG_GET_FIELD(ipid, MCMP1_IPIDT0, HardwareID);
	info->mcatype = REG_GET_FIELD(ipid, MCMP1_IPIDT0, McaType);

	instidhi = REG_GET_FIELD(ipid, MCMP1_IPIDT0, InstanceIdHi);
	instid = REG_GET_FIELD(ipid, MCMP1_IPIDT0, InstanceIdLo);
	info->aid = ((instidhi >> 2) & 0x03);
	info->socket_id = ((instid & 0x1) << 2) | (instidhi & 0x03);

	if (info->socket_id != adapt->xgmi.socket_id) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_WRONG_SOCKET_ID,
			AMDGV_ERROR_32_32(info->socket_id, adapt->xgmi.socket_id));
	}
}

static void mi300_mca_push_unknown_bank_count(struct amdgv_adapter *adapt)
{
	/* This is a dummy callback. Becacuse driver does not know
	* encoding format for UNKNOWN IPs, it cannot make any assumptions
	* (UE vs CE vs DF). For now just increment a generic counter and
	* report the total number of unknown errors
	*/

	adapt->mca.num_unknown_mca_entries++;
	amdgv_put_error(AMDGV_PF_IDX,
			AMDGV_ERROR_ECC_UNKNOWN,
			AMDGV_ERROR_16_16_32(
			adapt->xgmi.socket_id,
			1,
			adapt->mca.num_unknown_mca_entries));

	return;
}

static void mi300_mca_push_bank_count(struct amdgv_adapter *adapt,
				struct mca_bank_entry *bank,
				enum amdgv_ras_block ras_block)
{
	uint64_t status0;
	struct mca_bank_info  bank_info = {0};
	struct mca_ecc *mca_ecc;
	uint32_t id, instlo;
	uint32_t err_cnt = 0;

	status0 = bank->regs[MCA_REG_IDX_STATUS];
	if (!REG_GET_FIELD(status0, MCMP1_STATUST0, Val))
		return;

	if (!adapt->mca.ecc_block[ras_block].mca_ecc)
		return;

	mi300_mca_bank_decode_ipid(adapt, bank, &bank_info);

	id = bank_info.aid;

	if (ras_block == AMDGV_RAS_BLOCK__GFX) {
		instlo = REG_GET_FIELD(bank->regs[MCA_REG_IDX_IPID], MCMP1_IPIDT0, InstanceIdLo);
		id = bank_info.aid * 2;
		if (instlo == MI300_MCA_IPID_GFX_XCD1)
			id += 1;
	}

	mca_ecc = &adapt->mca.ecc_block[ras_block].mca_ecc[id];
	err_cnt = REG_GET_FIELD(bank->regs[MCA_REG_IDX_MISC0], MCMP1_MISC0T0, ErrCnt);

	if (REG_GET_FIELD(status0, MCMP1_STATUST0, UC) == 1 &&
	REG_GET_FIELD(status0, MCMP1_STATUST0, PCC) == 1) {
		mca_ecc->new_ue_count += 1;
		mca_ecc->total_ue_count += 1;
		mi300_mca_push_chiplet_ecc_err(adapt, ras_block,
			adapt->xgmi.socket_id,
			id, AMDGV_MCA_ERROR_TYPE_UE,
			1);
	} else {
		bank->type = MCA_BANK_ERR_CE_DE_DECODE(bank);
		if (err_cnt) {
			mca_ecc->new_ce_count += err_cnt;
			mca_ecc->total_ce_count += err_cnt;
			mi300_mca_push_chiplet_ecc_err(adapt, ras_block,
				adapt->xgmi.socket_id,
				id, bank->type,
				err_cnt);
		}
	}
}

static void mi300_mca_umc_push_bank_count(struct amdgv_adapter *adapt,
					struct mca_bank_entry *bank)
{
	uint64_t status0;
	uint32_t ext_error_code;
	struct mca_bank_info  bank_info = {0};
	struct mca_ecc *mca_ecc;
	unsigned long err_cnt;

	status0 = bank->regs[MCA_REG_IDX_STATUS];
	if (!REG_GET_FIELD(status0, MCMP1_STATUST0, Val))
		return;

	if (!adapt->mca.ecc_block[AMDGV_RAS_BLOCK__UMC].mca_ecc)
		return;

	mi300_mca_bank_decode_ipid(adapt, bank, &bank_info);

	mca_ecc = &adapt->mca.ecc_block[AMDGV_RAS_BLOCK__UMC].mca_ecc[bank_info.aid];

	ext_error_code = REG_GET_FIELD(bank->regs[MCA_REG_IDX_STATUS], MCMP1_STATUST0, ErrorCodeExt);
	err_cnt = REG_GET_FIELD(bank->regs[MCA_REG_IDX_MISC0], MCMP1_MISC0T0, ErrCnt);

	err_cnt = (ext_error_code == 0) ? err_cnt : 1;

	if (umc_v12_0_is_correctable_error(adapt, status0)) {
		mca_ecc->new_ce_count += err_cnt;
		mca_ecc->total_ce_count += err_cnt;
		bank->type = AMDGV_MCA_ERROR_TYPE_CE;
		mi300_mca_push_chiplet_ecc_err(adapt, AMDGV_RAS_BLOCK__UMC,
			adapt->xgmi.socket_id,
			bank_info.aid, AMDGV_MCA_ERROR_TYPE_CE,
			err_cnt);
	} else if (umc_v12_0_is_deferred_error(adapt, status0)) {
		mca_ecc->new_de_count += err_cnt;
		mca_ecc->total_de_count += err_cnt;
		/* overwrite CE type */
		bank->type = AMDGV_MCA_ERROR_TYPE_DE;
		mi300_mca_umc_add_err_addr(adapt, mca_ecc, bank);
		mi300_mca_push_chiplet_ecc_err(adapt, AMDGV_RAS_BLOCK__UMC,
			adapt->xgmi.socket_id,
			bank_info.aid, AMDGV_MCA_ERROR_TYPE_DE,
			err_cnt);
		adapt->ecc.pending_de_count++;
	} else if (umc_v12_0_is_uncorrectable_error(adapt, status0)) {
		mca_ecc->new_ue_count += err_cnt;
		mca_ecc->total_ue_count += err_cnt;
		/* overwrite DE type */
		bank->type = AMDGV_MCA_ERROR_TYPE_UE;
		mi300_mca_umc_add_err_addr(adapt, mca_ecc, bank);
		mi300_mca_push_chiplet_ecc_err(adapt, AMDGV_RAS_BLOCK__UMC,
			adapt->xgmi.socket_id,
			bank_info.aid, AMDGV_MCA_ERROR_TYPE_UE,
			err_cnt);
	}
}

static void mi300_mca_pcs_xgmi_push_bank_count(struct amdgv_adapter *adapt,
					struct mca_bank_entry *bank)
{
	uint32_t ext_error_code;
	struct mca_ecc *mca_ecc;
	struct mca_bank_info  bank_info = {0};
	uint32_t err_cnt;

	ext_error_code = MCA_REG__STATUS__ERRORCODEEXT(bank->regs[MCA_REG_IDX_STATUS]);

	mi300_mca_bank_decode_ipid(adapt, bank, &bank_info);

	mca_ecc = &adapt->mca.ecc_block[AMDGV_RAS_BLOCK__XGMI_WAFL].mca_ecc[bank_info.aid];
	err_cnt = REG_GET_FIELD(bank->regs[MCA_REG_IDX_MISC0], MCMP1_MISC0T0, ErrCnt);

	if (ext_error_code == 0) {
		mca_ecc->new_ue_count += err_cnt;
		mca_ecc->total_ue_count += err_cnt;
		mi300_mca_push_chiplet_ecc_err(adapt, AMDGV_RAS_BLOCK__XGMI_WAFL,
			adapt->xgmi.socket_id,
			bank_info.aid, AMDGV_MCA_ERROR_TYPE_UE,
			err_cnt);
	} else if (ext_error_code == 6) {
		mca_ecc->new_ce_count += err_cnt;
		mca_ecc->total_ce_count += err_cnt;
		bank->type = MCA_BANK_ERR_CE_DE_DECODE(bank);
		mi300_mca_push_chiplet_ecc_err(adapt, AMDGV_RAS_BLOCK__XGMI_WAFL,
			adapt->xgmi.socket_id,
			bank_info.aid, bank->type,
			err_cnt);
	} else {
		AMDGV_ERROR("Error: XGMI invalid error code:%d\n", ext_error_code);
		return;
	}
}

static void mi300_mca_gfx_push_bank_count(struct amdgv_adapter *adapt,
					  struct mca_bank_entry *bank)
{
	mi300_mca_push_bank_count(adapt, bank, AMDGV_RAS_BLOCK__GFX);
}

static void mi300_mca_sdma_push_bank_count(struct amdgv_adapter *adapt,
					   struct mca_bank_entry *bank)
{
	mi300_mca_push_bank_count(adapt, bank, AMDGV_RAS_BLOCK__SDMA);
}

static void mi300_mca_mmhub_push_bank_count(struct amdgv_adapter *adapt,
					    struct mca_bank_entry *bank)
{
	mi300_mca_push_bank_count(adapt, bank, AMDGV_RAS_BLOCK__MMHUB);
}

static int mi300_mca_parse_error_code(struct amdgv_adapter *adapt,
				      struct mca_bank_entry *bank)
{
	int errcode;

	if (adapt->pp.smu_fw_version >= 0x00555600) {
		errcode = REG_GET_FIELD(bank->regs[MCA_REG_IDX_SYND], MCMP1_SYNDT0, ErrorInformation);
		errcode &= 0xff;
	} else {
		errcode = REG_GET_FIELD(bank->regs[MCA_REG_IDX_STATUS], MCMP1_STATUST0, ErrorCode);
	}

	return errcode;
}

static bool mi300_mca_gfx_is_bank_valid(struct amdgv_adapter *adapt,
					struct mca_bank_entry *bank)
{
	uint32_t instlo;

	instlo = REG_GET_FIELD(bank->regs[MCA_REG_IDX_IPID], MCMP1_IPIDT0, InstanceIdLo);
	instlo &= AMDGV_RAS_GENMASK(31, 1);

	if (instlo == MI300_MCA_IPID_GFX_XCD0 ||
	instlo == MI300_MCA_IPID_GFX_XCD1 ||
	instlo == 0x40430400)
		return true;
	else
		return false;
}

static bool mi300_mca_sdma_is_bank_valid(struct amdgv_adapter *adapt,
					 struct mca_bank_entry *bank)
{
	uint32_t instlo;
	int errcode, i = 0;

	instlo = REG_GET_FIELD(bank->regs[MCA_REG_IDX_IPID], MCMP1_IPIDT0, InstanceIdLo);
	instlo &= AMDGV_RAS_GENMASK(31, 1);

	if (instlo != MI300_MCA_IPID_SDMA_MMHUB)
		return false;

	errcode = mi300_mca_parse_error_code(adapt, bank);
	if (errcode < 0)
		return false;

	for (i = 0; i < ARRAY_SIZE(sdma_err_codes); i++) {
		if (errcode == sdma_err_codes[i])
			return true;
	}

	return false;
}

static bool mi300_mca_mmhub_is_bank_valid(struct amdgv_adapter *adapt,
					  struct mca_bank_entry *bank)
{
	uint32_t instlo;
	int errcode, i = 0;

	instlo = REG_GET_FIELD(bank->regs[MCA_REG_IDX_IPID], MCMP1_IPIDT0, InstanceIdLo);
	instlo &= AMDGV_RAS_GENMASK(31, 1);

	if (instlo != MI300_MCA_IPID_SDMA_MMHUB)
		return false;

	errcode = mi300_mca_parse_error_code(adapt, bank);
	if (errcode < 0)
		return false;

	for (i = 0; i < ARRAY_SIZE(mmhub_err_codes); i++) {
		if (errcode == mmhub_err_codes[i])
			return true;
	}

	return false;
}

#define MI300_MCA_HANDLER_TABLE_SIZE 5

static struct mca_bank_handler bank_handler[MI300_MCA_HANDLER_TABLE_SIZE] = {
	{
		.ip = AMDGV_MCA_IP_UMC,
		.hwid = 0x96,
		.mcatype = 0,
		.block = AMDGV_RAS_BLOCK__UMC,
		.push_bank_count = mi300_mca_umc_push_bank_count,
		.is_bank_valid = NULL,
	},
	{
		.ip = AMDGV_MCA_IP_PCS_XGMI,
		.hwid = 0x50,
		.mcatype = 0x0,
		.block = AMDGV_RAS_BLOCK__XGMI_WAFL,
		.push_bank_count = mi300_mca_pcs_xgmi_push_bank_count,
		.is_bank_valid = NULL,
	},
	/* SMU IP is an aggregate for other IPs. Need further decode with is_bank_valid */
	{
		.ip = AMDGV_MCA_IP_SMU,
		.hwid = 0x1,
		.mcatype = 0x1,
		.block = AMDGV_RAS_BLOCK__GFX,
		.push_bank_count = mi300_mca_gfx_push_bank_count,
		.is_bank_valid = mi300_mca_gfx_is_bank_valid,
	},
	{
		.ip = AMDGV_MCA_IP_SMU,
		.hwid = 0x1,
		.mcatype = 0x1,
		.block = AMDGV_RAS_BLOCK__SDMA,
		.push_bank_count = mi300_mca_sdma_push_bank_count,
		.is_bank_valid = mi300_mca_sdma_is_bank_valid,
	},
	{
		.ip = AMDGV_MCA_IP_SMU,
		.hwid = 0x1,
		.mcatype = 0x1,
		.block = AMDGV_RAS_BLOCK__MMHUB,
		.push_bank_count = mi300_mca_mmhub_push_bank_count,
		.is_bank_valid = mi300_mca_mmhub_is_bank_valid,
	},
};

static struct mca_bank_handler *mi300_mca_get_bank_handler(struct amdgv_adapter *adapt,
							   struct mca_bank_entry *bank)
{
	struct mca_bank_handler *handler = NULL;
	uint16_t mcatype;
	uint64_t hwid;
	int i;

	hwid = REG_GET_FIELD(bank->regs[MCA_REG_IDX_IPID], MCMP1_IPIDT0, HardwareID);
	mcatype = REG_GET_FIELD(bank->regs[MCA_REG_IDX_IPID], MCMP1_IPIDT0, McaType);

	for (i = 0; i < MI300_MCA_HANDLER_TABLE_SIZE; i++) {
		handler = &bank_handler[i];
		if (handler->hwid != hwid || handler->mcatype != mcatype)
			continue;

		/* Found match, no need for further check.*/
		if (!handler->is_bank_valid)
			return handler;

		if (handler->is_bank_valid(adapt, bank))
			return handler;
	}

	return NULL;
}

static int mi300_mca_get_valid_bank_count(struct amdgv_adapter *adapt,
					enum amdgv_mca_error_type type,
					uint32_t *count)
{
	uint32_t msg;
	int ret;

	if (!count)
		return AMDGV_FAILURE;

	switch (type) {
	case AMDGV_MCA_ERROR_TYPE_UE:
		msg = PPSMC_MSG_QueryValidMcaCount;
		break;
	case AMDGV_MCA_ERROR_TYPE_CE:
		msg = PPSMC_MSG_QueryValidMcaCeCount;
		break;
	default:
		return AMDGV_FAILURE;
	}

	ret = mi300_smu_send_msg(adapt, msg, count);
	if (ret) {
		*count = 0;
		return ret;
	}

	return 0;
}

static int mi300_mca_bank_read_reg32(struct amdgv_adapter *adapt,
				enum amdgv_mca_error_type type,
				int idx, int offset, uint32_t *val)
{
	uint32_t msg, param;

	switch (type) {
	case AMDGV_MCA_ERROR_TYPE_UE:
		msg = PPSMC_MSG_McaBankDumpDW;
		break;
	case AMDGV_MCA_ERROR_TYPE_CE:
		msg = PPSMC_MSG_McaBankCeDumpDW;
		break;
	default:
		return AMDGV_FAILURE;
	}

	param = ((idx & 0xffff) << 16) | (offset & 0xfffc);

	return mi300_smu_send_msg_with_param(adapt, msg, param, val);
}

static int mi300_mca_bank_read_reg64(struct amdgv_adapter *adapt,
				enum amdgv_mca_error_type type,
				int idx, int reg_idx, uint64_t *val)
{
	uint32_t data[2] = {0, 0};
	int ret;
	int i, offset;

	if (!val || reg_idx >= MCA_REG_IDX_COUNT)
		return AMDGV_FAILURE;

	offset = reg_idx * 8;
	for (i = 0; i < ARRAY_SIZE(data); i++) {
		ret = mi300_mca_bank_read_reg32(adapt, type, idx, offset + (i << 2), &data[i]);
		if (ret) {
			AMDGV_ERROR("MCA failed to read register[%d], offset:0x%x\n", reg_idx, offset);
			return ret;
		}
	}

	*val = (uint64_t)data[1] << 32 | data[0];

	return 0;
}

static int mi300_mca_push_bank(struct amdgv_adapter *adapt,
				enum amdgv_mca_error_type type,
				int idx,
				struct mca_bank_entry *bank)
{
	int i, ret;
	struct mca_bank_handler *handler;

	/* NOTE: populated all mca register by default */
	for (i = 0; i < MCA_REG_IDX_COUNT; i++) {
		ret = mi300_mca_bank_read_reg64(adapt, type, idx, i, &bank->regs[i]);
		if (ret)
			return ret;
	}

	amdgv_put_error_ext(AMDGV_PF_IDX, AMDGV_ERROR_ECC_ACA_DUMP,
				bank->regs[MCA_REG_IDX_STATUS],
				bank->regs[MCA_REG_IDX_ADDR],
				bank->regs[MCA_REG_IDX_MISC0],
				bank->regs[MCA_REG_IDX_IPID],
				bank->regs[MCA_REG_IDX_SYND]);

	/* UMC CE & DE banks are mixed together.
	* IP Decode will patch the real bank type if nessesary */
	bank->type = type;

	handler = mi300_mca_get_bank_handler(adapt, bank);
	if (handler)
		handler->push_bank_count(adapt, bank);
	else
		mi300_mca_push_unknown_bank_count(adapt);

	return 0;
}

static int mi300_mca_get_new_banks(struct amdgv_adapter *adapt,
				enum amdgv_mca_error_type type)
{
	uint32_t i;
	uint32_t count = 0;
	uint32_t bank_size = sizeof(struct mca_bank_entry) * MI300_MCA_MAX_VALID_MCA_COUNT;
	struct mca_bank_entry *banks = NULL;
	int ret = 0;

	banks = oss_zalloc(bank_size);
	if (!banks) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL, bank_size);
		return AMDGV_FAILURE;
	}

	ret = mi300_mca_get_valid_bank_count(adapt, type, &count);
	if (ret)
		goto exit;

	count = min(count, MI300_MCA_MAX_VALID_MCA_COUNT);
	for (i = 0; i < count; i++) {
		ret = mi300_mca_push_bank(adapt, type, i, &banks[i]);
		if (ret)
			AMDGV_ERROR("Failed to decode MCA Bank %i\n", i);
	}

	if (!ret && count)
		amdgv_mca_generate_cpers(adapt, type, banks, (uint16_t)count);

exit:
	oss_free(banks);

	return ret;
}

static int mi300_mca_get_block_raw_ecc(struct amdgv_adapter *adapt,
				enum amdgv_ras_block block,
				struct block_ecc **block_ecc)
{
	if (block >= AMDGV_RAS_BLOCK__LAST)
		return AMDGV_FAILURE;

	*block_ecc = &adapt->mca.ecc_block[block];
	return 0;
}

static int mi300_mca_pop_block_error_count(struct amdgv_adapter *adapt,
					enum amdgv_ras_block block,
					struct ras_err_data *err_data)
{
	int id;
	uint32_t ce_cnt, ue_cnt, de_cnt;
	struct block_ecc  *block_ecc;

	if (block >= AMDGV_RAS_BLOCK__LAST)
		return AMDGV_FAILURE;

	if (!err_data)
		return AMDGV_FAILURE;

	block_ecc = &adapt->mca.ecc_block[block];

	for (id = 0; id < adapt->mca.max_aid_xcd_num; id++) {

		/* Except for gfx, the id of other ras blocks are less than MI300_MAX_AID_NUM */
		if ((block != AMDGV_RAS_BLOCK__GFX) && (id >= MI300_MAX_AID_NUM))
			break;

		//Query error count
		ce_cnt = block_ecc->mca_ecc[id].new_ce_count;
		ue_cnt = block_ecc->mca_ecc[id].new_ue_count;
		de_cnt = block_ecc->mca_ecc[id].new_de_count;
		err_data->ce_count += ce_cnt;
		err_data->ue_count += ue_cnt;
		err_data->de_count += de_cnt;

		//Clear new ecc count
		block_ecc->mca_ecc[id].new_ce_count = 0;
		block_ecc->mca_ecc[id].new_ue_count = 0;
		block_ecc->mca_ecc[id].new_de_count = 0;
	}

	return 0;
}

static int mi300_mca_reset_block_error_count(struct amdgv_adapter *adapt,
					enum amdgv_ras_block block)
{
	int id = 0;
	struct block_ecc *block_ecc;
	struct mca_ecc *mca_ecc;
	struct mca_err_addr *mca_err_addr, *tmp;

	if (block >= AMDGV_RAS_BLOCK__LAST)
		return AMDGV_FAILURE;

	adapt->mca.num_unknown_mca_entries = 0;

	block_ecc = &adapt->mca.ecc_block[block];

	for (id = 0; id < adapt->mca.max_aid_xcd_num; id++) {
		mca_ecc = &block_ecc->mca_ecc[id];

		mca_ecc->new_ce_count = 0;
		mca_ecc->new_ue_count = 0;
		mca_ecc->new_de_count = 0;
		mca_ecc->total_ce_count = 0;
		mca_ecc->total_ue_count = 0;
		mca_ecc->total_de_count = 0;

		if (!amdgv_list_empty(&mca_ecc->err_addr_list)) {
			amdgv_list_for_each_entry_safe(mca_err_addr, tmp,
				&mca_ecc->err_addr_list, struct mca_err_addr, node) {
				mi300_mca_umc_del_err_addr(adapt, mca_ecc, mca_err_addr);
			}
		}
	}

	amdgv_mca_count_cache_reset(adapt);

	return 0;
}

static int mi300_mca_decode_block(struct amdgv_adapter *adapt, struct mca_bank_entry *bank,
				  enum amdgv_ras_block *block)
{
	struct mca_bank_handler *handler = NULL;

	handler = mi300_mca_get_bank_handler(adapt, bank);
	if (!handler)
		return AMDGV_FAILURE;

	*block = handler->block;

	return 0;
}

static const struct amdgv_funcs mi300_mca_mca_smu_funcs = {
	.set_debug_mode = mi300_smu_mca_set_debug_mode,
	.get_new_banks = mi300_mca_get_new_banks,
	.get_block_raw_ecc = mi300_mca_get_block_raw_ecc,
	.pop_block_error_count = mi300_mca_pop_block_error_count,
	.reset_block_error_count = mi300_mca_reset_block_error_count,
	.decode_block = mi300_mca_decode_block,
};

void mi300_mca_init(struct amdgv_adapter *adapt)
{
	int i = 0;
	int id = 0;
	uint32_t block_ecc_data_size = 0;
	struct amdgv_mca *mca = &adapt->mca;

	mca->max_aid_xcd_num = MI300_MAX_XCD_NUM;
	mca->funcs = &mi300_mca_mca_smu_funcs;
	mca->enabled = true;

	for (i = 0; i < AMDGV_RAS_BLOCK__LAST; i++) {
		block_ecc_data_size = sizeof(struct mca_ecc) * mca->max_aid_xcd_num;
		mca->ecc_block[i].mca_ecc = oss_malloc(block_ecc_data_size);
		if (!mca->ecc_block[i].mca_ecc) {
			int t = 0;

			AMDGV_WARN("Failed to allocate memory for mca ecc!\n");

			for (t = 0; t < i; t++) {
				oss_free(mca->ecc_block[t].mca_ecc);
				mca->ecc_block[t].mca_ecc = NULL;
			}

			break;
		}

		oss_memset(mca->ecc_block[i].mca_ecc, 0, block_ecc_data_size);

		for (id = 0; id < mca->max_aid_xcd_num; id++)
			AMDGV_INIT_LIST_HEAD(&mca->ecc_block[i].mca_ecc[id].err_addr_list);
	}

	amdgv_mca_count_cache_init(adapt);
}

void mi300_mca_fini(struct amdgv_adapter *adapt)
{
	int i = 0;

	adapt->mca.enabled = false;

	amdgv_mca_count_cache_fini(adapt);

	for (i = 0; i < AMDGV_RAS_BLOCK__LAST; i++) {
		if (adapt->mca.ecc_block[i].mca_ecc) {
			oss_free(adapt->mca.ecc_block[i].mca_ecc);
			adapt->mca.ecc_block[i].mca_ecc = NULL;
		}
	}
}
