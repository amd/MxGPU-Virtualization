/*
 * Copyright (C) 2022  Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef MI300_MCA_H
#define MI300_MCA_H

#include "amdgv_mca.h"

#define MI300_MAX_XCD_NUM    8
#define MI300_MAX_AID_NUM    4

#define MCA_REG_FIELD(x, h, l)			(((x) & AMDGV_RAS_GENMASK(h, l)) >> l)
#define MCA_REG__STATUS__VAL(x)			MCA_REG_FIELD(x, 63, 63)
#define MCA_REG__STATUS__OVERFLOW(x)		MCA_REG_FIELD(x, 62, 62)
#define MCA_REG__STATUS__UC(x)			MCA_REG_FIELD(x, 61, 61)
#define MCA_REG__STATUS__EN(x)			MCA_REG_FIELD(x, 60, 60)
#define MCA_REG__STATUS__MISCV(x)		MCA_REG_FIELD(x, 59, 59)
#define MCA_REG__STATUS__ADDRV(x)		MCA_REG_FIELD(x, 58, 58)
#define MCA_REG__STATUS__PCC(x)			MCA_REG_FIELD(x, 57, 57)
#define MCA_REG__STATUS__ERRCOREIDVAL(x)	MCA_REG_FIELD(x, 56, 56)
#define MCA_REG__STATUS__TCC(x)			MCA_REG_FIELD(x, 55, 55)
#define MCA_REG__STATUS__SYNDV(x)		MCA_REG_FIELD(x, 53, 53)
#define MCA_REG__STATUS__CECC(x)		MCA_REG_FIELD(x, 46, 46)
#define MCA_REG__STATUS__UECC(x)		MCA_REG_FIELD(x, 45, 45)
#define MCA_REG__STATUS__DEFERRED(x)		MCA_REG_FIELD(x, 44, 44)
#define MCA_REG__STATUS__POISON(x)		MCA_REG_FIELD(x, 43, 43)
#define MCA_REG__STATUS__SCRUB(x)		MCA_REG_FIELD(x, 40, 40)
#define MCA_REG__STATUS__ERRCOREID(x)		MCA_REG_FIELD(x, 37, 32)
#define MCA_REG__STATUS__ADDRLSB(x)		MCA_REG_FIELD(x, 29, 24)
#define MCA_REG__STATUS__ERRORCODEEXT(x)	MCA_REG_FIELD(x, 21, 16)
#define MCA_REG__STATUS__ERRORCODE(x)		MCA_REG_FIELD(x, 15, 0)


#define MCA_BANK_ERR_CE_DE_DECODE(bank)                                  \
	((MCA_REG__STATUS__POISON((bank)->regs[MCA_REG_IDX_STATUS]) ||   \
	  MCA_REG__STATUS__DEFERRED((bank)->regs[MCA_REG_IDX_STATUS])) ? \
	  AMDGV_MCA_ERROR_TYPE_DE :                                      \
	  AMDGV_MCA_ERROR_TYPE_CE)

struct mca_bank_handler {
	enum amdgv_mca_ip ip;
	uint16_t mcatype;
	uint64_t hwid;
	enum amdgv_ras_block block;

	bool (*is_bank_valid)(struct amdgv_adapter *adapt, struct mca_bank_entry *bank);
	void (*push_bank_count)(struct amdgv_adapter *adapt, struct mca_bank_entry *bank);
};

void mi300_mca_init(struct amdgv_adapter *adapt);
void mi300_mca_fini(struct amdgv_adapter *adapt);

void mi300_mca_umc_del_err_addr(struct amdgv_adapter *adapt,
    struct mca_ecc *mca_ecc, struct mca_err_addr *mca_addr);
#endif
