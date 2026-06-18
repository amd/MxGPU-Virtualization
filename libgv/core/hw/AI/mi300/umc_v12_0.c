/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "umc_v12_0.h"
#include "amdgv_ras.h"
#include "mi300.h"
#include "amdgv.h"
#include "amdgv_device.h"
#include "mi300_nbio.h"
#include "amdgv_mca.h"
#include "mi300_mca.h"
#include "amdgv_vfmgr.h"
#include "amdgv_ras_eeprom_internal.h"

#define EEPROM_TABLE_VERSION_MI300       0x00021000

#define EEPROM_I2C_TARGET_ADDR_MI300        0xA8
#define EEPROM_I2C_CONTROLLER_PORT_MI300    0

/* Macro to combine two uint32_t variables (lo, hi) into a uint64_t */
#define COMBINE_LO_HI_TO_UINT64(lo, hi) ((uint64_t)(hi) << 32 | (lo))

#define ADDR_TO_PFN(addr) ((addr) >> AMDGV_GPU_PAGE_SHIFT)
#define PFN_TO_ADDR(pfn) ((pfn) << AMDGV_GPU_PAGE_SHIFT)

/* SID0 SID1 R14 R13 bits in NA */
#define HBM3E_ADDRHI_MASK 0xf0000000ULL
#define HBM3E_ADDRHI_SHIFT 28

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

const uint32_t umc_v12_0_channel_idx_tbl[]
	[UMC_V12_0_UMC_INSTANCE_NUM][UMC_V12_0_CHANNEL_INSTANCE_NUM] = {
	{{3,   7,   11,  15,  2,   6,   10,  14},  {1,   5,   9,   13,  0,   4,   8,   12},
	 {19,  23,  27,  31,  18,  22,  26,  30},  {17,  21,  25,  29,  16,  20,  24,  28}},
	{{47,  43,  39,  35,  46,  42,  38,  34},  {45,  41,  37,  33,  44,  40,  36,  32},
	 {63,  59,  55,  51,  62,  58,  54,  50},  {61,  57,  53,  49,  60,  56,  52,  48}},
	{{79,  75,  71,  67,  78,  74,  70,  66},  {77,  73,  69,  65,  76,  72,  68,  64},
	 {95,  91,  87,  83,  94,  90,  86,  82},  {93,  89,  85,  81,  92,  88,  84,  80}},
	{{99,  103, 107, 111, 98,  102, 106, 110}, {97,  101, 105, 109, 96,  100, 104, 108},
	 {115, 119, 123, 127, 114, 118, 122, 126}, {113, 117, 121, 125, 112, 116, 120, 124}}
};

const uint32_t umc_v12_5_channel_idx_tbl[]
	[UMC_V12_0_UMC_INSTANCE_NUM][UMC_V12_0_CHANNEL_INSTANCE_NUM] = {
	{{10,   13,   15,  14,  9,   12,   11,  7},  {6,   4,   5,   8,  1,   2,   3,   0},
	 {28,  27,  31,  30,  24,  29,  25,  21},  {23,  18,  22,  26,  19,  20,  16,  17}},
	{{42,  45,  47,  46,  41,  44,  43,  39},  {38,  36,  37,  40,  33,  34,  35,  32},
	 {60,  59,  63,  62,  56,  61,  57,  53},  {55,  50,  54,  58,  51,  52,  48,  49}},
	{{74,  77,  79,  78,  73,  76,  75,  71},  {70,  68,  69,  72,  65,  66,  67,  64},
	 {92,  91,  95,  94,  88,  93,  89,  85},  {87,  82,  86,  90,  83,  84,  80,  81}},
	{{106,  109, 111, 110, 105,  108, 107, 103}, {102,  100, 101, 104, 97,  98, 99, 96},
	 {124, 123, 127, 126, 120, 125, 121, 117}, {119, 114, 118, 122, 115, 116, 112, 113}}
};


/* mapping of MCA error address to normalized address */
static const uint32_t umc_v12_0_ma2na_mapping[] = {
	0,  5,  6,  8,  9,  14, 12, 13,
	10, 11, 15, 16, 17, 18, 19, 20,
	21, 22, 23, 24, 25, 26, 27, 28,
	24, 7,  29, 30,
};

/* mapping of MCA error address to normalized address for HBM3e */
static const uint32_t umc_v12_5_ma2na_mapping[] = {
	0,  5,  6,  8,  9,  14, 12, 13,
	10, 11, 15, 16, 17, 18, 19, 20,
	21, 22, 23, 24, 25, 26, 27, 28,
	29, 7,  30, 31,
};


bool umc_v12_0_is_deferred_error(struct amdgv_adapter *adapt, uint64_t mc_umc_status)
{
	return (amdgv_ras_is_poison_mode_supported(adapt) &&
	    (REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, Val) == 1) &&
	    (REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, Deferred) == 1));
}

bool umc_v12_0_is_uncorrectable_error(struct amdgv_adapter *adapt,
						      uint64_t mc_umc_status)
{
	if (umc_v12_0_is_deferred_error(adapt, mc_umc_status))
		return false;

	return ((REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, Val) == 1) &&
	    (REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, PCC) == 1 ||
	    REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, UC) == 1 ||
	    REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, TCC) == 1));
}

bool umc_v12_0_is_correctable_error(struct amdgv_adapter *adapt, uint64_t mc_umc_status)
{
	if (umc_v12_0_is_deferred_error(adapt, mc_umc_status))
		return false;

	if (!(REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, Val) == 1))
		return false;

	if (REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, CECC) == 1)
		return true;

	if (REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, UECC) == 1 &&
		REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, UC) == 0)
		return true;

	if ((REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, ErrorCodeExt) == 0x5 ||
		 REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, ErrorCodeExt) == 0xb)  &&
		 !umc_v12_0_is_uncorrectable_error(adapt, mc_umc_status))
		return true;

	return false;
}

static bool umc_v12_0_is_uncorrectable_acparity_error(struct amdgv_adapter *adapt,
						      uint64_t mc_umc_status)
{
	return (REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, ErrorCodeExt) == 4);
}

static bool umc_v12_0_is_uncorrectable_odecc_error(struct amdgv_adapter *adapt,
						      uint64_t mc_umc_status)
{
	return (REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, ErrorCodeExt) == 0x0);
}

static bool umc_v12_0_is_uncorrectable_crc_error(struct amdgv_adapter *adapt,
						      uint64_t mc_umc_status)
{
	return (REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, ErrorCodeExt) == 0xf);
}

static void umc_v12_0_query_ras_error_count(struct amdgv_adapter *adapt,
					    void *ras_error_status)
{
	adapt->mca.funcs->pop_block_error_count(adapt,
						AMDGV_RAS_BLOCK__UMC,
						ras_error_status);
}

static bool umc_v12_0_bit_wise_xor(uint32_t val)
{
	bool result = 0;
	int i;

	for (i = 0; i < 32; i++)
		result = result ^ ((val >> i) & 0x1);

	return result;
}

static int convert_ma_to_nps_pa(struct amdgv_adapter *adapt,
			struct umc_mca_addr *addr_in, struct umc_phy_addr *addr_out,
			enum amdgv_memory_partition_mode nps, bool is_ma)
{
	uint32_t i, na_shift = 8;
	uint64_t soc_pa, na, na_nps, addrhi, addrhi_na;
	uint32_t bank_hash0, bank_hash1, bank_hash2, bank_hash3, col, row;
	uint32_t bank0, bank1, bank2, bank3, bank = 0;
	uint32_t ch_inst = addr_in->ch_inst;
	uint32_t umc_inst = addr_in->umc_inst;
	uint32_t node_inst = addr_in->node_inst;
	uint32_t socket_id = addr_in->socket_id;
	uint32_t channel_index = 0;
	uint64_t err_addr = addr_in->err_addr;
	bool hbm3e = (adapt->vram_info.vram_type == AMDGV_DGPU_VRAM_TYPE__HBM3E) ? 1 : 0;
	uint64_t lfb_size = (hbm3e) ? SOCKET_LFB_SIZE_HBM3E : SOCKET_LFB_SIZE;

	if (node_inst != RAS_INV_AID_NODE) {
		if (ch_inst >= UMC_V12_0_CHANNEL_INSTANCE_NUM ||
			umc_inst >= UMC_V12_0_UMC_INSTANCE_NUM ||
			node_inst >= UMC_V12_0_AID_NUM_MAX ||
			socket_id >= UMC_V12_0_SOCKET_NUM_MAX)
			return TA_RAS_STATUS__ERROR_INVALID_PARAMETER;
	} else {
		if (socket_id >= UMC_V12_0_SOCKET_NUM_MAX ||
			ch_inst >= UMC_V12_0_MAX_CHANNEL_NUM)
			return TA_RAS_STATUS__ERROR_INVALID_PARAMETER;
	}

	if (is_ma) {
		bank_hash0 = (err_addr >> UMC_V12_0_MCA_B0_BIT) & 0x1ULL;
		bank_hash1 = (err_addr >> UMC_V12_0_MCA_B1_BIT) & 0x1ULL;
		bank_hash2 = (err_addr >> UMC_V12_0_MCA_B2_BIT) & 0x1ULL;
		bank_hash3 = (err_addr >> UMC_V12_0_MCA_B3_BIT) & 0x1ULL;
		col = (err_addr >> 1) & 0x1fULL;
		if (hbm3e)
			row = (err_addr >> 10) & 0x7fffULL;
		else
			row = (err_addr >> 10) & 0x3fffULL;

		/* apply bank hash algorithm */
		bank0 =
			bank_hash0 ^ (UMC_V12_0_XOR_EN0 &
			(umc_v12_0_bit_wise_xor(col & UMC_V12_0_COL_XOR0) ^
			(umc_v12_0_bit_wise_xor(row & UMC_V12_0_ROW_XOR0))));
		bank1 =
			bank_hash1 ^ (UMC_V12_0_XOR_EN1 &
			(umc_v12_0_bit_wise_xor(col & UMC_V12_0_COL_XOR1) ^
			(umc_v12_0_bit_wise_xor(row & UMC_V12_0_ROW_XOR1))));
		bank2 =
			bank_hash2 ^ (UMC_V12_0_XOR_EN2 &
			(umc_v12_0_bit_wise_xor(col & UMC_V12_0_COL_XOR2) ^
			(umc_v12_0_bit_wise_xor(row & UMC_V12_0_ROW_XOR2))));
		bank3 =
			bank_hash3 ^ (UMC_V12_0_XOR_EN3 &
			(umc_v12_0_bit_wise_xor(col & UMC_V12_0_COL_XOR3) ^
			(umc_v12_0_bit_wise_xor(row & UMC_V12_0_ROW_XOR3))));

		bank = bank0 | (bank1 << 1) | (bank2 << 2) | (bank3 << 3);
		err_addr &= ~0x3c0ULL;
		err_addr |= (bank << UMC_V12_0_MCA_B0_BIT);
	}

	na_nps = 0x0;
	/* convert mca error address to normalized address */
	if (hbm3e) {
		for (i = 1; i < ARRAY_SIZE(umc_v12_5_ma2na_mapping); i++)
			na_nps |= ((err_addr >> i) & 0x1ULL) << umc_v12_5_ma2na_mapping[i];

		/* It's 2.25GB per channel, R[14:13] in MCA address can't be 0x3,
		* [SID1 SID0 R14 R13] should be converted to original value.
		*/
		addrhi = (na_nps & HBM3E_ADDRHI_MASK) >> HBM3E_ADDRHI_SHIFT;
		addrhi_na = (addrhi >> 2) * 3 + addrhi % 4;
		na_nps &= ~HBM3E_ADDRHI_MASK;
		na_nps |= (addrhi_na << HBM3E_ADDRHI_SHIFT);
	} else {
		for (i = 1; i < ARRAY_SIZE(umc_v12_0_ma2na_mapping); i++)
			na_nps |= ((err_addr >> i) & 0x1ULL) << umc_v12_0_ma2na_mapping[i];
	}

	if (nps == AMDGV_MEMORY_PARTITION_MODE_NPS1)
		na_shift = 8;
	else if (nps == AMDGV_MEMORY_PARTITION_MODE_NPS2)
		na_shift = 9;
	else if (nps == AMDGV_MEMORY_PARTITION_MODE_NPS4)
		na_shift = 10;
	else if (nps == AMDGV_MEMORY_PARTITION_MODE_NPS8)
		na_shift = 11;
	else
		return AMDGV_FAILURE;

	na = ((na_nps >> na_shift) << 8) | (na_nps & 0xff);

	if (node_inst != RAS_INV_AID_NODE) {
		channel_index = (hbm3e) ? umc_v12_5_channel_idx_tbl[node_inst][umc_inst][ch_inst] :
			umc_v12_0_channel_idx_tbl[node_inst][umc_inst][ch_inst];
	}
	else {
		channel_index = ch_inst;
		node_inst = channel_index /
			(UMC_V12_0_UMC_INSTANCE_NUM * UMC_V12_0_CHANNEL_INSTANCE_NUM);
	}

	/* translate umc channel address to soc pa, 3 parts are included */
	soc_pa = ADDR_OF_32KB_BLOCK(na) |
		ADDR_OF_256B_BLOCK(channel_index) |
		OFFSET_IN_256B_BLOCK(na);

	/* calc channel hash based on absolute address */
	soc_pa += socket_id * lfb_size;
	/* the umc channel bits are not original values, they are hashed */
	UMC_V12_0_SET_CHANNEL_HASH(channel_index, soc_pa);
	/* restore pa */
	soc_pa -= socket_id * lfb_size;

	/* get some channel bits from na_nps directly and
	* add nps section offset
	*/
	if (nps == AMDGV_MEMORY_PARTITION_MODE_NPS2) {
		soc_pa &= ~(0x1ULL << UMC_V12_0_PA_CH5_BIT);
		soc_pa |= ((na_nps & 0x100) << 5);
		soc_pa += (node_inst >> 1) * (lfb_size >> 1);
	} else if (nps == AMDGV_MEMORY_PARTITION_MODE_NPS4) {
		soc_pa &= ~(0x3ULL << UMC_V12_0_PA_CH4_BIT);
		soc_pa |= ((na_nps & 0x300) << 4);
		soc_pa += node_inst * (lfb_size >> 2);
	} else if (nps == AMDGV_MEMORY_PARTITION_MODE_NPS8) {
		soc_pa &= ~(0x7ULL << UMC_V12_0_PA_CH4_BIT);
		soc_pa |= ((na_nps & 0x700) << 4);
		soc_pa += node_inst * (lfb_size >> 2) +
			(channel_index >> 4) * (lfb_size >> 3);
	}

	addr_out->pa = soc_pa;
	addr_out->bank = bank;
	addr_out->channel_idx = channel_index;

	return 0;
}

static int get_nps_pa_mask_bits(struct amdgv_adapter *adapt,
			enum amdgv_memory_partition_mode nps,
			uint32_t *loop_bits, uint32_t num,
			uint32_t *flip_row_bit, uint32_t *r13_in_pa)
{
	uint32_t vram_type = adapt->vram_info.vram_type;

	if (!loop_bits || !flip_row_bit || !r13_in_pa || (num < UMC_V12_0_RETIRE_LOOP_BITS))
		return AMDGV_FAILURE;

	loop_bits[0] = UMC_V12_0_PA_C2_BIT;
	loop_bits[1] = UMC_V12_0_PA_C3_BIT;
	loop_bits[2] = UMC_V12_0_PA_C4_BIT;
	loop_bits[3] = UMC_V12_0_PA_R13_BIT;
	*flip_row_bit = 13;
	*r13_in_pa = UMC_V12_0_PA_R13_BIT;

	if (nps == AMDGV_MEMORY_PARTITION_MODE_NPS2) {
		loop_bits[0] = UMC_V12_0_PA_CH5_BIT;
		loop_bits[1] = UMC_V12_0_PA_C2_BIT;
		loop_bits[2] = UMC_V12_0_PA_B1_BIT;
		*r13_in_pa = UMC_V12_0_PA_R12_BIT;
	} else if (nps == AMDGV_MEMORY_PARTITION_MODE_NPS4) {
		loop_bits[0] = UMC_V12_0_PA_CH4_BIT;
		loop_bits[1] = UMC_V12_0_PA_CH5_BIT;
		loop_bits[2] = UMC_V12_0_PA_B0_BIT;
		*r13_in_pa = UMC_V12_0_PA_R11_BIT;
	}

	switch (vram_type) {
	case AMDGV_DGPU_VRAM_TYPE__HBM3:
		/* other nps modes are taken as nps1 */
		if (nps == AMDGV_MEMORY_PARTITION_MODE_NPS2)
			loop_bits[3] = UMC_V12_0_PA_R12_BIT;
		else if (nps == AMDGV_MEMORY_PARTITION_MODE_NPS4)
			loop_bits[3] = UMC_V12_0_PA_R11_BIT;
		break;

	case AMDGV_DGPU_VRAM_TYPE__HBM3E:
		loop_bits[3] = UMC_V12_0_PA_R12_BIT;
		*flip_row_bit = 12;

		if (nps == AMDGV_MEMORY_PARTITION_MODE_NPS2)
			loop_bits[3] = UMC_V12_0_PA_R11_BIT;
		else if (nps == AMDGV_MEMORY_PARTITION_MODE_NPS4)
			loop_bits[3] = UMC_V12_0_PA_R10_BIT;
		break;

	default:
		AMDGV_WARN("Unknown HBM type, set RAS retire loop bits to the value in NPS1 mode.\n");
		break;
	}

	return 0;
}

static uint64_t clear_nps_pa_mask_bits(struct amdgv_adapter *adapt,
		uint64_t pa, enum amdgv_memory_partition_mode nps, bool zero_pfn_ok)
{
	uint32_t loop_bits[UMC_V12_0_RETIRE_LOOP_BITS] = {0};
	uint32_t flip_row_bit, r13_in_pa;
	uint64_t mask_pa;
	int i;

	get_nps_pa_mask_bits(adapt, nps, loop_bits, ARRAY_SIZE(loop_bits), &flip_row_bit, &r13_in_pa);

	mask_pa = pa;
	/* clear loop bits in soc physical address */
	for (i = 0; i < UMC_V12_0_RETIRE_LOOP_BITS; i++)
		mask_pa &= ~(0x1ULL << loop_bits[i]);

	if (!zero_pfn_ok && !ADDR_TO_PFN(mask_pa))
		mask_pa |= 0x1ULL << loop_bits[2];

	return mask_pa;
}

static bool check_legacy_record(struct amdgv_adapter *adapt,
			struct eeprom_table_record *record, struct umc_mca_addr *addr_in)
{
	struct umc_phy_addr addr_out;
	uint64_t converted_pfn, record_pfn;
	enum amdgv_memory_partition_mode nps = AMDGV_MEMORY_PARTITION_MODE_NPS1;

	oss_memset(&addr_out, 0, sizeof(addr_out));

	convert_ma_to_nps_pa(adapt, addr_in, &addr_out, nps, false);

	converted_pfn = ADDR_TO_PFN(clear_nps_pa_mask_bits(adapt, addr_out.pa, nps, true));
	record_pfn = ADDR_TO_PFN(clear_nps_pa_mask_bits(adapt, PFN_TO_ADDR(record->retired_page), nps, true));

	return (converted_pfn == record_pfn);
}

static int lookup_bad_pages_in_a_row(struct amdgv_adapter *adapt,
		struct umc_mca_addr *ma, struct umc_phy_addr *pa,
		enum amdgv_memory_partition_mode nps,
		uint64_t *pfns, uint32_t num, bool dump)
{
	uint32_t col, col_lower, row, row_lower, row_high, idx, flip_row_bit, r13_in_pa;
	uint32_t i, loop_bits[UMC_V12_0_RETIRE_LOOP_BITS] = {0};
	uint64_t soc_pa, mask_pa, column, err_addr;
	bool hbm3e = (adapt->vram_info.vram_type == AMDGV_DGPU_VRAM_TYPE__HBM3E) ? 1 : 0;

	get_nps_pa_mask_bits(adapt, nps, loop_bits, ARRAY_SIZE(loop_bits), &flip_row_bit, &r13_in_pa);

	mask_pa = clear_nps_pa_mask_bits(adapt, pa->pa, nps, true);

	err_addr = ma->err_addr;
	/* get column bit 0 and 1 in mca address */
	col_lower = (err_addr >> 1) & 0x3ULL;
	/* extra row bit will be handled later */
	row_lower = (err_addr >> UMC_V12_0_MCA_R0_BIT) & 0x1fffULL;
	row_lower &= ~(0x1ULL << flip_row_bit);

	if (hbm3e) {
		row_high = (mask_pa >> r13_in_pa) & 0x3ULL;
		/* it's 2.25GB in each channel, from MCA address to PA
		 * [R14 R13] is converted if the two bits value are 0x3,
		 * get them from PA instead of MCA address.
		 */
		row_lower |= (row_high << 13);
	}

	idx = 0;
	/* loop for all possibilities of retire bits */
	for (column = 0; column < UMC_V12_0_BAD_PAGE_NUM_PER_CHANNEL; column++) {
		soc_pa = mask_pa;
		for (i = 0; i < UMC_V12_0_RETIRE_LOOP_BITS; i++)
			soc_pa |= (((column >> i) & 0x1ULL) << loop_bits[i]);

		col = ((column & 0x7) << 2) | col_lower;
		/* add row bit 13 */
		row = ((column >> 3) << flip_row_bit) | row_lower;

		if (dump)
			AMDGV_DEBUG("Error Address(PA):0x%-10llx Row:0x%-4x Col:0x%-2x Bank:0x%x Channel:0x%x\n",
				soc_pa, row, col, pa->bank, pa->channel_idx);

		if (pfns && (idx < num))
			pfns[idx++] = ADDR_TO_PFN(soc_pa);
	}

	return idx;
}

static int umc_v12_0_convert_ma_to_pa(struct amdgv_adapter *adapt,
			 struct ras_err_data *err_data, struct mca_err_addr *mca)
{
	struct umc_mca_addr addr_in;
	struct umc_phy_addr addr_out;
	enum amdgv_memory_partition_mode nps;
	uint64_t mask_pa;
	uint32_t ret;

	if (!err_data)
		return AMDGV_FAILURE;

	ret = amdgv_nbio_get_nps_mode(adapt, &nps);
	if (ret)
		return AMDGV_FAILURE;

	oss_memset(&addr_in, 0, sizeof(addr_in));
	oss_memset(&addr_out, 0, sizeof(addr_out));

	addr_in.err_addr = REG_GET_FIELD(mca->mca_addr, MCA_UMC_UMC0_MCUMC_ADDRT0, ErrorAddr);
	addr_in.ch_inst = MCA_IPID_2_UMC_CH(mca->mca_ipid);
	addr_in.umc_inst = MCA_IPID_2_UMC_INST(mca->mca_ipid);
	addr_in.node_inst = MCA_IPID_2_DIE_ID(mca->mca_ipid);
	addr_in.socket_id = MCA_IPID_2_SOCKET_ID(mca->mca_ipid);

	convert_ma_to_nps_pa(adapt, &addr_in, &addr_out, nps, true);

	mask_pa = clear_nps_pa_mask_bits(adapt, addr_out.pa, nps, false);

	amdgv_umc_fill_error_record(adapt, err_data, addr_in.err_addr,
		mask_pa, addr_out.channel_idx, addr_in.umc_inst);

	lookup_bad_pages_in_a_row(adapt, &addr_in, &addr_out, nps, NULL, 0, true);

	return 0;
}

static int convert_eeprom_record_to_mem_addr(struct amdgv_adapter *adapt,
				struct eeprom_table_record *record,
				uint64_t *pa_pfn, uint64_t *pfns, uint32_t num)
{
	struct umc_mca_addr addr_in;
	struct umc_phy_addr addr_out;
	enum amdgv_memory_partition_mode nps;
	enum amdgv_memory_partition_mode save_nps;
	uint64_t mask_pa;
	uint32_t ret;

	ret = amdgv_nbio_get_nps_mode(adapt, &nps);
	if (ret)
		return AMDGV_FAILURE;

	save_nps = get_nps_from_pa(record->retired_page);

	if (oss_atomic_read(&adapt->ecc.in_cross_nps_handling))
		nps = save_nps;

	oss_memset(&addr_in, 0, sizeof(addr_in));
	oss_memset(&addr_out, 0, sizeof(addr_out));

	if (save_nps != nps) {
		addr_in.err_addr = record->address;
		addr_in.ch_inst = record->mem_channel;
		addr_in.umc_inst = record->mcumc_id;
		addr_in.node_inst = RAS_INV_AID_NODE;
		addr_in.socket_id = adapt->xgmi.socket_id;

		convert_ma_to_nps_pa(adapt,
			&addr_in, &addr_out, nps, !check_legacy_record(adapt, record, &addr_in));
		if (pa_pfn) {
			mask_pa = clear_nps_pa_mask_bits(adapt, addr_out.pa, nps, false);
			*pa_pfn = ADDR_TO_PFN(mask_pa);
		}
	} else {
		//no need to care about channel_idx and bank in this situation
		addr_out.pa = PFN_TO_ADDR(set_nps_to_pa(record->retired_page,
				AMDGV_MEMORY_PARTITION_MODE_UNKNOWN));
		if (pa_pfn) {
			*pa_pfn = set_nps_to_pa(record->retired_page,
				AMDGV_MEMORY_PARTITION_MODE_UNKNOWN);
		}
	}

	if (pfns && num)
		ret = lookup_bad_pages_in_a_row(adapt, &addr_in, &addr_out, nps, pfns, num, false);

	return ret;
}

static int umc_v12_0_eeprom_record_to_pa(struct amdgv_adapter *adapt,
				struct eeprom_table_record *record, uint64_t *pa_pfn)
{
	return convert_eeprom_record_to_mem_addr(adapt, record, pa_pfn, NULL, 0);
}

static int umc_v12_0_eeprom_record_to_pages(struct amdgv_adapter *adapt,
		struct eeprom_table_record *record, uint64_t *pfns, uint32_t num)
{
	return convert_eeprom_record_to_mem_addr(adapt, record, NULL, pfns, num);
}

static int umc_v12_0_set_eeprom_record(struct amdgv_adapter *adapt,
		struct eeprom_table_record *record,
		struct amdgv_ras_eeprom_bad_page_info *bp_info)
{
	struct umc_mca_addr addr_in;
	struct umc_phy_addr addr_out;
	enum amdgv_memory_partition_mode nps;
	uint32_t ret;
	uint64_t mca_address, mca_ipid, mask_pa;

	if (!record || !bp_info) {
		AMDGV_ERROR("Missing eeprom record info\n");
		return AMDGV_FAILURE;
	}

	ret = amdgv_nbio_get_nps_mode(adapt, &nps);
	if (ret)
		return AMDGV_FAILURE;

	oss_memset(&addr_in, 0, sizeof(addr_in));
	oss_memset(&addr_out, 0, sizeof(addr_out));

	mca_address = COMBINE_LO_HI_TO_UINT64(bp_info->mca_addr_lo, bp_info->mca_addr_hi);
	mca_ipid = COMBINE_LO_HI_TO_UINT64(bp_info->mca_ipid_lo, bp_info->mca_ipid_hi);
	addr_in.err_addr = REG_GET_FIELD(mca_address, MCA_UMC_UMC0_MCUMC_ADDRT0, ErrorAddr);
	addr_in.ch_inst = MCA_IPID_2_UMC_CH(mca_ipid);
	addr_in.umc_inst = MCA_IPID_2_UMC_INST(mca_ipid);
	addr_in.node_inst = MCA_IPID_2_DIE_ID(mca_ipid);
	addr_in.socket_id = MCA_IPID_2_SOCKET_ID(mca_ipid);

	convert_ma_to_nps_pa(adapt, &addr_in, &addr_out, nps, true);

	mask_pa = clear_nps_pa_mask_bits(adapt, addr_out.pa, nps, false);

	record->retired_page = ADDR_TO_PFN(mask_pa);
	record->mem_channel = addr_out.channel_idx;
	record->mcumc_id = addr_in.umc_inst;
	record->cu = 0;
	record->address = mca_address;
	record->ts = amdgv_ras_eeprom_utc_to_eeprom_format(adapt, (uint64_t)bp_info->timestamp);
	record->err_type = bp_info->severity;

	return 0;
}

static void umc_v12_0_query_error_address(struct amdgv_adapter *adapt,
					  void *ras_error_status)
{
	uint32_t aid;
	struct block_ecc  *umc_ecc;
	struct ras_err_data *err_data = (struct ras_err_data *)ras_error_status;
	uint64_t mc_umc_status;
	struct mca_ecc *mca_umc;
	struct mca_err_addr *mca_err_addr, *tmp;

	if (!err_data->err_addr)
		return;

	adapt->mca.funcs->get_block_raw_ecc(adapt, AMDGV_RAS_BLOCK__UMC, &umc_ecc);

	for (aid = 0; aid < adapt->umc.node_inst_num; aid++) {
		mca_umc = &umc_ecc->mca_ecc[aid];
		if (amdgv_list_empty(&mca_umc->err_addr_list))
			continue;

		amdgv_list_for_each_entry_safe(mca_err_addr, tmp,
			&mca_umc->err_addr_list, struct mca_err_addr, node) {

			mc_umc_status = mca_err_addr->mca_status;
			/* Calculate error address if ue error is detected. Ignore acparity error */
			if (mc_umc_status &&
				(umc_v12_0_is_deferred_error(adapt, mc_umc_status) ||
				(umc_v12_0_is_uncorrectable_error(adapt, mc_umc_status) &&
				 !umc_v12_0_is_uncorrectable_acparity_error(adapt, mc_umc_status) &&
				 !umc_v12_0_is_uncorrectable_odecc_error(adapt, mc_umc_status) &&
				 !umc_v12_0_is_uncorrectable_crc_error(adapt, mc_umc_status)))) {

				uint64_t mca_addr, err_addr, mca_ipid;
				uint32_t InstanceIdHi, InstanceIdLo;

				mca_addr = mca_err_addr->mca_addr;
				mca_ipid = mca_err_addr->mca_ipid;

				err_addr =  REG_GET_FIELD(mca_addr, MCA_UMC_UMC0_MCUMC_ADDRT0, ErrorAddr);
				InstanceIdHi = REG_GET_FIELD(mca_ipid, MCMP1_IPIDT0, InstanceIdHi);
				InstanceIdLo = REG_GET_FIELD(mca_ipid, MCMP1_IPIDT0, InstanceIdLo);

				AMDGV_DEBUG("UMC:IPID:0x%llx, ipid_aid:%d, inst:%d, ch:%d, aid:%d, err_addr:0x%x\n",
					mca_ipid,
					MCA_IPID_HI_2_UMC_AID(InstanceIdHi),
					MCA_IPID_LO_2_UMC_INST(InstanceIdLo),
					MCA_IPID_LO_2_UMC_CH(InstanceIdLo),
					aid, err_addr);

				umc_v12_0_convert_ma_to_pa(adapt, err_data, mca_err_addr);
			}

			mi300_mca_umc_del_err_addr(adapt, mca_umc, mca_err_addr);
		}
	}
}

static void umc_v12_0_query_ras_error_address(struct amdgv_adapter *adapt,
					     void *ras_error_status)
{
	umc_v12_0_query_error_address(adapt, ras_error_status);
}

static void umc_v12_0_get_eeprom_i2c_params(struct amdgv_adapter *adapt,
					struct amdgv_ras_eeprom_control *control)
{
	uint8_t i2c_addr = EEPROM_I2C_TARGET_ADDR_MI300;

	amdgv_atomfirmware_ras_rom_addr(adapt, &i2c_addr);
	control->i2c_address = i2c_addr;

	control->i2c_port = EEPROM_I2C_CONTROLLER_PORT_MI300;
}

static bool umc_v12_0_query_ras_poison_mode(struct amdgv_adapter *adapt)
{
	/*
	 * Force return true, because regUMCCH0_EccCtrl
	 * is not accessible from host side
	 */
	return true;
}

static int umc_v12_0_get_ras_vf_safe_range(struct amdgv_adapter *adapt,
		uint64_t *offset, uint64_t *size, uint32_t idx_vf)
{
	/* future improvement: get safe ranges */
	struct amdgv_vf_device *entry;
	uint64_t reserved_start;

	if (idx_vf == AMDGV_PF_IDX) {
		if (amdgv_vfmgr_configured_vf_num(adapt)) {
			/* All usable FB outside of PF region is considered safe */
			entry = &adapt->array_vf[idx_vf];
			*offset = MBYTES_TO_BYTES(entry->fb_size);
			*size = MBYTES_TO_BYTES(adapt->gpuiov.total_fb_usable) - MBYTES_TO_BYTES(entry->fb_size);
		} else {
			*offset = 0;
			*size = 0;
		}
	} else {
		entry = &adapt->array_vf[idx_vf];
		if (entry->configured) {
			if (entry->xchg.vf_crit_region == GPU_CRIT_REGION_V2) {
				/* right now, just leave as [shared memory region end offset, end of vf fb] */
				*offset = MBYTES_TO_BYTES(entry->fb_offset) + AMDGV_V2_CRIT_REGION_SIZE_BYTES;
				*size =  MBYTES_TO_BYTES(entry->real_fb_size) - AMDGV_V2_CRIT_REGION_SIZE_BYTES;
			} else {
				reserved_start = GET_VF_TABLE_OFFSET_BY_ID(adapt, idx_vf, DATAEXCHANGE) +
				KBYTES_TO_BYTES(GET_VF_TABLE_SIZE_KB_BY_ID(adapt, idx_vf, DATAEXCHANGE));
				/* Add an additional 4MB to critical range
				* to account for guest sw init reservations
				*
				* review how guest can take this into account
				* and add this change later.
				* reserved_start += KBYTES_TO_BYTES(0x1000);
				*/
				*offset = reserved_start + MBYTES_TO_BYTES(entry->fb_offset);
				*size = GET_VF_TABLE_OFFSET_BY_ID(adapt, idx_vf, IPD) - reserved_start;
			}
		} else {
			*offset = 0;
			*size = 0;
		}
	}


	return 0;
}

static int umc_v12_0_get_ras_all_vf_safe_range(struct amdgv_adapter *adapt,
		uint64_t *offset, uint64_t *size, uint32_t max_entry_num)
{
	uint32_t entry_idx = 0;
	uint32_t idx_vf;

	for (idx_vf = 0; idx_vf < adapt->num_vf && idx_vf < max_entry_num; idx_vf++) {
		umc_v12_0_get_ras_vf_safe_range(adapt, &offset[entry_idx], &size[entry_idx], idx_vf);
		entry_idx++;
	}

	if (max_entry_num >= AMDGV_MAX_VF_SLOT) {
		umc_v12_0_get_ras_vf_safe_range(adapt, &offset[entry_idx], &size[entry_idx], AMDGV_PF_IDX);
		entry_idx++;
	}

	return entry_idx;
}

static int umc_12_0_soc_pa_to_bank(struct amdgv_adapter *adapt,
	uint64_t soc_pa,
	struct amdgv_umc_fb_bank_addr *bank_addr)
{

	int channel_hashed = 0;
	int channel_real = 0;
	int channel_reversed = 0;
	int i = 0;

	bank_addr->stack_id = UMC_V12_0_SOC_PA_TO_SID(soc_pa);
	bank_addr->bank_group = 0; /* This is a combination of SID & Bank. Needed?? */
	bank_addr->bank = UMC_V12_0_SOC_PA_TO_BANK(soc_pa);
	bank_addr->row = UMC_V12_0_SOC_PA_TO_ROW(soc_pa);
	bank_addr->column = UMC_V12_0_SOC_PA_TO_COL(soc_pa);

	/* Channel bits 4-6 are hashed. Bruteforce reverse the hash */
	channel_hashed = (soc_pa >> UMC_V12_0_PA_CH4_BIT) & 0x7;

	channel_reversed = 0;
	for (i = 0; i < 8; i++) {
		channel_reversed = 0;
		channel_reversed |= UMC_V12_0_CHANNEL_HASH_CH4((i << 4), soc_pa);
		channel_reversed |= (UMC_V12_0_CHANNEL_HASH_CH5((i << 4), soc_pa) << 1);
		channel_reversed |= (UMC_V12_0_CHANNEL_HASH_CH6((i << 4), soc_pa) << 2);
		if (channel_reversed == channel_hashed) {
			channel_real = ((i << 4)) | ((soc_pa >> UMC_V12_0_PA_CH0_BIT) & 0xf);
		}
	}

	bank_addr->channel = channel_real;
	bank_addr->subchannel = UMC_V12_0_SOC_PA_TO_PC(soc_pa);

	return 0;
}

static int umc_12_0_bank_to_soc_pa(struct amdgv_adapter *adapt,
	struct amdgv_umc_fb_bank_addr bank_addr,
	uint64_t *soc_pa)
{
	uint64_t na = 0;
	uint64_t tmp_pa = 0;
	*soc_pa = 0;

	tmp_pa |= UMC_V12_0_SOC_SID_TO_PA(bank_addr.stack_id);
	tmp_pa |= UMC_V12_0_SOC_BANK_TO_PA(bank_addr.bank);
	tmp_pa |= UMC_V12_0_SOC_ROW_TO_PA(bank_addr.row);
	tmp_pa |= UMC_V12_0_SOC_COL_TO_PA(bank_addr.column);
	tmp_pa |= UMC_V12_0_SOC_CH_TO_PA(bank_addr.channel);
	tmp_pa |= UMC_V12_0_SOC_PC_TO_PA(bank_addr.subchannel);

	/* Get the NA */
	na = ((tmp_pa >> UMC_V12_0_PA_C2_BIT) << UMC_V12_0_NA_C2_BIT);
	na |= tmp_pa & 0xff;

	/* translate umc channel address to soc pa, 3 parts are included */
	tmp_pa = ADDR_OF_32KB_BLOCK(na) |
		ADDR_OF_256B_BLOCK(bank_addr.channel) |
		OFFSET_IN_256B_BLOCK(na);

	/* the umc channel bits are not original values, they are hashed */
	UMC_V12_0_SET_CHANNEL_HASH(bank_addr.channel, tmp_pa);

	*soc_pa = tmp_pa;

	return 0;
}

static void umc_v12_0_set_eeprom_table_version(struct amdgv_adapter *adapt)
{
       struct amdgv_ras_eeprom_control *control = &adapt->eeprom_control;

       control->tbl_hdr.version = EEPROM_TABLE_VERSION_MI300;
}

static int umc_v12_0_pages_in_a_row(struct amdgv_adapter *adapt, uint64_t pa, uint64_t *pfns, int len)
{
	enum amdgv_memory_partition_mode nps;
	struct umc_mca_addr mca_addr = {0};
	struct umc_phy_addr phy_addr = {0};

	if (amdgv_nbio_get_nps_mode(adapt, &nps))
		return AMDGV_FAILURE;

	phy_addr.pa = pa;
	return lookup_bad_pages_in_a_row(adapt, &mca_addr, &phy_addr, nps, pfns, len, false);
}

const struct amdgv_umc_funcs umc_v12_0_funcs = {
	.err_cnt_init = NULL,
	.query_ras_error_count = umc_v12_0_query_ras_error_count,
	.query_ras_error_address = umc_v12_0_query_ras_error_address,
	.query_ras_poison_mode = umc_v12_0_query_ras_poison_mode,
	.get_eeprom_i2c_params = umc_v12_0_get_eeprom_i2c_params,
	.set_eeprom_table_version = umc_v12_0_set_eeprom_table_version,
	.get_ras_vf_safe_range = umc_v12_0_get_ras_all_vf_safe_range,
	.bank_to_soc_pa = umc_12_0_bank_to_soc_pa,
	.soc_pa_to_bank = umc_12_0_soc_pa_to_bank,
	.eeprom_record_to_soc_pa = umc_v12_0_eeprom_record_to_pa,
	.eeprom_record_to_pages = umc_v12_0_eeprom_record_to_pages,
	.set_eeprom_record = umc_v12_0_set_eeprom_record,
	.pages_in_a_row = umc_v12_0_pages_in_a_row,
};

void umc_v12_0_set_umc_funcs(struct amdgv_adapter *adapt)
{
	adapt->umc.max_ras_err_cnt_per_query =
			UMC_V12_0_TOTAL_CHANNEL_NUM(adapt);
	adapt->umc.channel_inst_num = UMC_V12_0_CHANNEL_INSTANCE_NUM;
	adapt->umc.inst_offs = UMC_V12_0_PER_INST_OFFSET;
	adapt->umc.channel_offs = UMC_V12_0_PER_CHANNEL_OFFSET;
	adapt->umc.umc_inst_num = UMC_V12_0_UMC_INSTANCE_NUM;
	adapt->umc.node_inst_num =
		(adapt->umc.node_inst_num + adapt->umc.umc_inst_num - 1)/adapt->umc.umc_inst_num;

	adapt->umc.channel_idx_tbl = &umc_v12_0_channel_idx_tbl[0][0][0];



	adapt->umc.funcs = &umc_v12_0_funcs;
	adapt->umc.supports_ras_eeprom = true;
	if (adapt->opt.use_legacy_eeprom_format)
		adapt->umc.use_legacy_eeprom_format = true;
	adapt->umc.reset_mode = AMDGV_RESET_MODE1;
	adapt->umc.eeprom_version = EEPROM_TABLE_VER_V3;
}
