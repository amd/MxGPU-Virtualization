// SPDX-License-Identifier: MIT
/*
 * Copyright 2025 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifdef __KERNEL__
#include <linux/errno.h>
#else
#include <errno.h>
#endif

#include "amdgv.h"
#include "amdgv_ras_mgr.h"
#include "amdgv_ras_mce.h"

static void __fill_mce_to_aca_bank(struct amdgv_adapter *adapt,
	enum mce_bank_type bank_type, struct oss_mce *m, struct aca_bank_reg *aca_bank)
{
	aca_bank->timestamp = m->time;
	aca_bank->bank_type = bank_type;
	aca_bank->ecc_type = RAS_ERR_TYPE__MCE;
	aca_bank->regs[ACA_REG_IDX__STATUS] = m->status;
	aca_bank->regs[ACA_REG_IDX__ADDR] = m->addr;
	aca_bank->regs[ACA_REG_IDX__MISC0] = m->misc;
	aca_bank->regs[ACA_REG_IDX__IPID] = m->ipid;
	aca_bank->regs[ACA_REG_IDX__SYND] = m->synd;
}

static int amdgv_ras_mce_notifier_v5(struct amdgv_adapter *adapt,
			unsigned int id, struct oss_mce *m)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	struct aca_bank_reg aca_bank = {0};
	struct aca_bank_ecc ecc = {0};
	enum mce_bank_type bank_type;

	if (!adapt->smuio.funcs || !adapt->smuio.funcs->get_socket_id) {
		RAS_DEV_WARN(adapt, "No interface to obtain current device socket ID!\n");
		return 0;
	}

	if (ras_mce_check_bank(ras_mgr->ras_core, MCE_BANK_TYPE_GPU, m->bank)) {
		bank_type = MCE_BANK_TYPE_GPU;
	/* For CPU bank, only the first registered gpu device needs to record bank */
	} else if (ras_mce_check_bank(ras_mgr->ras_core, MCE_BANK_TYPE_CPU, m->bank) &&
			!id) {
		bank_type = MCE_BANK_TYPE_CPU;
	} else {
		RAS_DEV_WARN(adapt, "Unsupported oss_mce bank: %u\n",  m->bank);
		return 0;
	}

	__fill_mce_to_aca_bank(adapt, bank_type, m, &aca_bank);

	if (bank_type == MCE_BANK_TYPE_GPU) {
		if (ras_aca_parse_bank(ras_mgr->ras_core, &aca_bank, &ecc))
			return -RAS_CORE_EINVAL;

		/* GPU device only record bank data that matches its own socket id.*/
		if (adapt->smuio.funcs->get_socket_id(adapt) != ecc.bank_info.socket_id)
			return 0;
	}

	return ras_mce_add_aca_bank(ras_mgr->ras_core, &aca_bank);
}

static int amdgv_ras_mce_notifier(void *dev, unsigned int id, unsigned long val, void *data)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	u32 aca_ip_version = 0;

	if (!data || !ras_mgr)
		return 0;

	if (ras_core_get_ip_version(ras_mgr->ras_core,
				RAS_UNIT_ID_ACA, &aca_ip_version))
		return 0;

	switch (aca_ip_version) {
	case IP_VERSION(5, 0, 0):
		return amdgv_ras_mce_notifier_v5(adapt, id, data);
	default:
		RAS_DEV_WARN(adapt, "Invalid aca ip version:0x%x\n", aca_ip_version);
		break;
	}

	return 0;
}

int amdgv_ras_mce_sw_init(struct amdgv_adapter *adapt)
{
	oss_register_mce_notifier(adapt, amdgv_ras_mce_notifier);

	return 0;
}

int amdgv_ras_mce_sw_fini(struct amdgv_adapter *adapt)
{
	int ret = oss_unregister_mce_notifier(adapt);

	if (ret == -RAS_CORE_ENOENT)
		RAS_DEV_WARN(adapt, "MCE notifier unregister: dev not registered\n");

	return ret;
}
