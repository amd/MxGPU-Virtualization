/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include <amdgv_gpuiov.h>
#include <amdgv.h>
#include <atombios/atomfirmware.h>
#include <atombios/atom.h>
#include <atombios/atombios.h>
#include <amdgv_vbios.h>

#include "navi32_psp.h"
#include "navi32_ip_discovery.h"
#include "navi32_gfx.h"
#include "navi32_sdma.h"
#include "navi32_reset.h"
#include "navi32_powerplay.h"
#include "navi32_nbio.h"
#include "mmhub_v3_0.h"

#include <navi3/MMHUB/mmhub_3_0_0_offset.h>
#include <navi3/MMHUB/mmhub_3_0_0_sh_mask.h>
#include <navi3/GC/gc_11_0_3_offset.h>
#include <navi3/GC/gc_11_0_3_sh_mask.h>
#include <navi3/NBIO/nbio_4_3_0_offset.h>
#include <navi3/NBIO/nbio_4_3_0_sh_mask.h>
#include <navi3/MP/mp_13_0_0_offset.h>
#include <navi3/MP/mp_13_0_0_sh_mask.h>
#include <navi3/HDP/hdp_6_0_0_offset.h>
#include <navi3/HDP/hdp_6_0_0_sh_mask.h>
#include <navi3/OSSSYS/osssys_6_0_0_offset.h>
#include <navi3/OSSSYS/osssys_6_0_0_sh_mask.h>
#include <navi3/SMUIO/smuio_13_0_6_offset.h>
#include <navi3/SMUIO/smuio_13_0_6_sh_mask.h>
#include <navi3/ATHUB/athub_3_0_0_offset.h>
#include <navi3/ATHUB/athub_3_0_0_sh_mask.h>

static const uint32_t this_block = AMDGV_SECURITY_BLOCK;
#define ROM_OFFSET_SHIFT 17 /* ROM OFFSET is 128KB aligned */

#define BUILD_NUM_MAX_LENGTH  8
#define BUILD_DATE_LENGTH 17
#define PART_INFO_MAX_LENGTH 128

static void navi32_vmhub_hook(struct amdgv_adapter *adapt);

static int navi32_vbios_special_version_check(struct amdgv_adapter *adapt, uint8_t *img)
{
	int i, j, k;
	/* Build_num is defined as 8 bytes length. When the build_num is lower than 8 bytes,
	 * there will be 0x00 added in the end until 8 bytes is used, which is not part of the anchor.
	 */
	int build_num_length;
	uint8_t anchor[] = { 0x00, 0x20, 0x20, 0x20, 0x20, 0x20,
			     0x20, 0x20, 0x20, 0x00 };
	uint8_t build_num[BUILD_NUM_MAX_LENGTH + 1] = { '\0' };
	uint8_t part_info[PART_INFO_MAX_LENGTH + 1] = { '\0' };
	uint8_t build_date[BUILD_DATE_LENGTH + 1] = { '\0' };

	/* search for the first 2k vbios space*/
	for (i = 0; i < 2048 - sizeof(anchor); i++) {
		for (j = 0; j < sizeof(anchor) && ((i + j) < 2048); j++) {
			if (anchor[j] != img[i + j])
				break;
		}

		if (j == sizeof(anchor)) {
			AMDGV_INFO("found anchor at %d\n", i);

			for (k = 0; k < PART_INFO_MAX_LENGTH; k++) {
				if (img[i + j + k] == '\\' || img[i + j + k] == 0x0)
					break;
			}

			for (build_num_length = 0; build_num_length < BUILD_NUM_MAX_LENGTH;
				 build_num_length++) {
				if (img[i - BUILD_NUM_MAX_LENGTH + build_num_length] == 0x0)
					break;
			}

			if (k < PART_INFO_MAX_LENGTH) {
				oss_memcpy(build_num, img + i - BUILD_NUM_MAX_LENGTH,
					   build_num_length);
				AMDGV_INFO("build num: %s\n", build_num);
				oss_memcpy(part_info, img + i + j, k);
				AMDGV_INFO("part info: %s\n", part_info);
				oss_memcpy(build_date, img + 0x50, BUILD_DATE_LENGTH);
				AMDGV_INFO("build date: %s\n", build_date);
				return 0;
			}
		}
	}

	return AMDGV_FAILURE;
}

static bool navi32_vbios_need_post(struct amdgv_adapter *adapt)
{
	uint32_t vbios_rd;
	uint32_t vbios_init;

	/* get vbios status from scratch reg */
	vbios_rd = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BIOS_SCRATCH_7));
	vbios_init = vbios_rd & ATOM_ASIC_INIT_COMPLETE;
	AMDGV_DEBUG("VBIOS (status = 0x%08x init=%d)\n", vbios_rd, vbios_init);

	if (vbios_init) {
		vbios_rd &= ~ATOM_ASIC_INIT_COMPLETE;
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BIOS_SCRATCH_7), vbios_rd);

		AMDGV_INFO("ATOM_ASIC_POSTED\n");
		return false;
	}

	AMDGV_INFO("ATOM_ASIC_NEED_POST\n");
	return true;
}

static bool navi32_vbios_smu_fw_loaded(struct amdgv_adapter *adapt)
{
	uint32_t smc_rd;
	uint32_t smc_intr_en;
	const uint32_t MP1_PUBLIC_DOMAIN = 0x03b00000;

	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_PCIE_INDEX2),
	       (MP1_PUBLIC_DOMAIN | (regMP1_FIRMWARE_FLAGS & 0xffffffff)));
	smc_rd = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_PCIE_DATA2));
	smc_intr_en = REG_GET_FIELD(smc_rd, MP1_FIRMWARE_FLAGS, INTERRUPTS_ENABLED);
	AMDGV_DEBUG("SMU (status=0x%x intr_en=%d)\n", smc_rd, smc_intr_en);

	return smc_intr_en ? true : false;
}

static int navi32_vbios_read_rom_from_reg(struct amdgv_adapter *adapt, uint8_t *bios,
					 uint32_t length_bytes)
{
	uint32_t *vbios_rom;
	unsigned int i, length_in_dword;
	uint32_t rom_offset;
	uint32_t reg_addr;

	vbios_rom = (uint32_t *)bios;
	length_in_dword = length_bytes / 4;

	AMDGV_INFO("Reading VBios from ROM\n");

	reg_addr = SOC15_REG_OFFSET(NBIO, 0, regREGS_ROM_OFFSET_CTRL);
	rom_offset = RREG8_SMN(reg_addr * 4);
	rom_offset = (rom_offset << REGS_ROM_OFFSET_CTRL__ROM_OFFSET__SHIFT) & REGS_ROM_OFFSET_CTRL__ROM_OFFSET_MASK;
	rom_offset = rom_offset << ROM_OFFSET_SHIFT;

	WREG32(SOC15_REG_OFFSET(SMUIO, 0, regROM_INDEX), rom_offset);

	for (i = 0; i < length_in_dword; i++) {
		if (adapt->vbios.timeout)
			return AMDGV_FAILURE;
		vbios_rom[i] = RREG32(SOC15_REG_OFFSET(SMUIO, 0, regROM_DATA));
	}
	return 0;
}

static void navi32_program_asic_golden_settings(struct amdgv_adapter *adapt)
{
	uint32_t tmp;

	/* enable disp timer */
	tmp = RREG32(SOC15_REG_OFFSET(SMUIO, 0, regPWR_IH_CONTROL));
	tmp = REG_SET_FIELD(tmp, PWR_IH_CONTROL, DISP_TIMER2_TRIGGER_MASK, 0);
	WREG32(SOC15_REG_OFFSET(SMUIO, 0, regPWR_IH_CONTROL), tmp);

	/* program MMHUB golden settings */
	tmp = RREG32(SOC15_REG_OFFSET(MMHUB, 0, regDAGB0_WRCLI2));
	tmp = REG_SET_FIELD(tmp, DAGB0_WRCLI2, VIRT_CHAN, 2);
	WREG32(SOC15_REG_OFFSET(MMHUB, 0, regDAGB0_WRCLI2), tmp);

	/* program ATHUB golden settings */
	WREG32_RMW(SOC15_REG_OFFSET(ATHUB, 0, regRPB_ARB_CNTL), 0x0000ff00, 0x00000800);
	WREG32_RMW(SOC15_REG_OFFSET(ATHUB, 0, regRPB_ARB_CNTL2), 0x00ff00ff, 0x00080008);

	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_STORM_CLIENT_LIST_CNTL), 0x00040000);
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_INT_FLOOD_CNTL), 8);

	/* Enable Translated Memory Write TLP handling */
	tmp = RREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_DEV0_0_RCC_FEATURES_CONTROL_MISC));
	tmp = REG_SET_FIELD(tmp, RCC_DEV0_0_RCC_FEATURES_CONTROL_MISC, RX_IGNORE_TRANSMWR_UR, 1);
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_DEV0_0_RCC_FEATURES_CONTROL_MISC), tmp);

	tmp = RREG32(SOC15_REG_OFFSET(HDP, 0, regHDP_MISC_CNTL));
	tmp = REG_SET_FIELD(tmp, HDP_MISC_CNTL, FED_ENABLE, 0);
	WREG32(SOC15_REG_OFFSET(HDP, 0, regHDP_MISC_CNTL), tmp);

	tmp = RREG32(SOC15_REG_OFFSET(HDP, 0, regHDP_MISC_CNTL));
	tmp = REG_SET_FIELD(tmp, HDP_MISC_CNTL, EARLY_WRACK_MISSING_PROTECT_ENABLE, 1);
	tmp = REG_SET_FIELD(tmp, HDP_MISC_CNTL, FED_ENABLE, 1);
	tmp = REG_SET_FIELD(tmp, HDP_MISC_CNTL, ATOMIC_FED_ENABLE, 1);
	WREG32(SOC15_REG_OFFSET(HDP, 0, regHDP_MISC_CNTL), tmp);
}

static int navi32_enable_pci_atomic_request(struct amdgv_adapter *adapt)
{
	uint16_t val;
	int pos;

	pos = oss_pci_find_capability(adapt->dev, PCI_CAP_ID__PCIE);

	if (!pos) {
		AMDGV_ERROR("this device does not support capability: %x\n", PCI_CAP_ID__PCIE);
		return AMDGV_FAILURE;
	}

	oss_pci_read_config_word(adapt->dev, pos + PCIE_DEVICE_CONTROL2, &val);
	val |= (PCIE_DEVICE_CONTROL2__ATOMICOP_REQ);
	oss_pci_write_config_word(adapt->dev, pos + PCIE_DEVICE_CONTROL2, val);

	oss_pci_read_config_word(adapt->dev, pos + PCIE_DEVICE_CAP2, &val);
	adapt->pcie_atomic_ops_support_flags = val & (PCIE_DEVICE_CAP2__ATOMIC_COMP32 | PCIE_DEVICE_CAP2__ATOMIC_COMP64);

	AMDGV_INFO("Atomic Request Enabled\n");

	return 0;
}

static int navi32_disable_pci_aer(struct amdgv_adapter *adapt)
{
	int pos;

	pos = oss_pci_find_ext_cap(adapt->dev, PCI_EXT_CAP_ID_ERR);
	if (!pos) {
		AMDGV_ERROR("this device does not support ext capability: %x\n", PCI_EXT_CAP_ID_ERR);
		return AMDGV_FAILURE;
	}
	oss_pci_write_config_dword(adapt->dev, pos + PCI_ERR_UNCOR_MASK, 0xffffffff);
	AMDGV_INFO("Disable pci aer\n");

	return 0;
}

enum NAVI10_DOORBELL_RANGE {
	/* Compute + GFX: 0~255 */
	NAVI10_DOORBELL_KIQ			= 0x000,
	NAVI10_DOORBELL_HIQ			= 0x001,
	NAVI10_DOORBELL_DIQ			= 0x002,
	NAVI10_DOORBELL_MEC_RING0		= 0x003,
	NAVI10_DOORBELL_MEC_RING1		= 0x004,
	NAVI10_DOORBELL_MEC_RING2		= 0x005,
	NAVI10_DOORBELL_MEC_RING3		= 0x006,
	NAVI10_DOORBELL_MEC_RING4		= 0x007,
	NAVI10_DOORBELL_MEC_RING5		= 0x008,
	NAVI10_DOORBELL_MEC_RING6		= 0x009,
	NAVI10_DOORBELL_MEC_RING7		= 0x00A,
	NAVI10_DOORBELL_USERQUEUE_START		= 0x00B,
	NAVI10_DOORBELL_USERQUEUE_END		= 0x08A,
	NAVI10_DOORBELL_GFX_RING0		= 0x08B,
	NAVI10_DOORBELL_GFX_RING1		= 0x08C,
	NAVI10_DOORBELL_MES_RING		= 0x090,
	/* SDMA:256~335*/
	NAVI10_DOORBELL_sDMA_ENGINE0		= 0x100,
	NAVI10_DOORBELL_sDMA_ENGINE1		= 0x10A,
	NAVI10_DOORBELL_sDMA_ENGINE2		= 0x114,
	NAVI10_DOORBELL_sDMA_ENGINE3		= 0x11E,
	/* IH: 376~391 */
	NAVI10_DOORBELL_IH			= 0x178,

	/* VCN engine's doorbell is 32 bit and two VCN ring share one QWORD */
	/* - lower 32 bits for VCN0 and upper 32 bits for VCN1 */
	NAVI10_DOORBELL64_VCN0_1		= 0x188,
	NAVI10_DOORBELL64_VCN2_3		= 0x189,
	NAVI10_DOORBELL64_VCN4_5		= 0x18A,
	NAVI10_DOORBELL64_VCN6_7		= 0x18B,

	NAVI10_DOORBELL64_VCN8_9		= 0x18C,
	NAVI10_DOORBELL64_VCNa_b		= 0x18D,
	NAVI10_DOORBELL64_VCNc_d		= 0x18E,
	NAVI10_DOORBELL64_VCNe_f		= 0x18F,

	NAVI10_DOORBELL_INVALID			= 0xFFFF
};

static void navi32_assign_asic_doorbell_ranges(struct amdgv_adapter *adapt)
{
	uint32_t doorbell_range;
	uint32_t vcn_doorbell_index;
	uint32_t ih_doorbell_index;
	uint32_t sdma_doorbell_index;
	uint32_t sdma_doorbell_size = 20;

	/* set CP doorbell range */
	WREG32_PCIE(SOC15_REG_OFFSET(NBIO, 0, regS2A_DOORBELL_ENTRY_0_CTRL), 0x30000007);
	WREG32_PCIE(SOC15_REG_OFFSET(NBIO, 0, regS2A_DOORBELL_ENTRY_3_CTRL), 0x3000000d);

	/* set SDMA doorbell range */
	sdma_doorbell_index = NAVI10_DOORBELL_sDMA_ENGINE0 << 1;
	doorbell_range = RREG32_PCIE(SOC15_REG_OFFSET(NBIO, 0, regS2A_DOORBELL_ENTRY_2_CTRL));

	doorbell_range = REG_SET_FIELD(doorbell_range,
						S2A_DOORBELL_ENTRY_2_CTRL,
						S2A_DOORBELL_PORT2_ENABLE,
						0x1);
	doorbell_range = REG_SET_FIELD(doorbell_range,
						S2A_DOORBELL_ENTRY_2_CTRL,
						S2A_DOORBELL_PORT2_AWID,
						0xe);
	doorbell_range = REG_SET_FIELD(doorbell_range,
						S2A_DOORBELL_ENTRY_2_CTRL,
						S2A_DOORBELL_PORT2_RANGE_OFFSET,
						sdma_doorbell_index);
	doorbell_range = REG_SET_FIELD(doorbell_range,
						S2A_DOORBELL_ENTRY_2_CTRL,
						S2A_DOORBELL_PORT2_RANGE_SIZE,
						sdma_doorbell_size * 2);
	doorbell_range = REG_SET_FIELD(doorbell_range,
						S2A_DOORBELL_ENTRY_2_CTRL,
						S2A_DOORBELL_PORT2_AWADDR_31_28_VALUE,
						0x3);
	WREG32_PCIE(SOC15_REG_OFFSET(NBIO, 0, regS2A_DOORBELL_ENTRY_2_CTRL), doorbell_range);

	/* set IH doorbell range */
	ih_doorbell_index = NAVI10_DOORBELL_IH << 1;
	doorbell_range = RREG32_PCIE(SOC15_REG_OFFSET(NBIO, 0, regS2A_DOORBELL_ENTRY_1_CTRL));
	doorbell_range = REG_SET_FIELD(doorbell_range,
						S2A_DOORBELL_ENTRY_1_CTRL,
						S2A_DOORBELL_PORT1_ENABLE,
						0x1);
	doorbell_range = REG_SET_FIELD(doorbell_range,
						S2A_DOORBELL_ENTRY_1_CTRL,
						S2A_DOORBELL_PORT1_AWID,
						0x0);
	doorbell_range = REG_SET_FIELD(doorbell_range,
						S2A_DOORBELL_ENTRY_1_CTRL,
						S2A_DOORBELL_PORT1_RANGE_OFFSET,
						ih_doorbell_index);
	doorbell_range = REG_SET_FIELD(doorbell_range,
						S2A_DOORBELL_ENTRY_1_CTRL,
						S2A_DOORBELL_PORT1_RANGE_SIZE,
						2);
	doorbell_range = REG_SET_FIELD(doorbell_range,
						S2A_DOORBELL_ENTRY_1_CTRL,
						S2A_DOORBELL_PORT1_AWADDR_31_28_VALUE,
						0x0);
	WREG32_PCIE(SOC15_REG_OFFSET(NBIO, 0, regS2A_DOORBELL_ENTRY_1_CTRL), doorbell_range);

	/* set VCN0 doorbell range */
	/*
	 * TODO:
	 * We do not know why VCN's doorbell is at 0x300 (which is 0x180 << 1, and NAVI10_DOORBELL_MMSCH is 0x180)
	 * Further debugging is needed, but for now, we will use it
	 */
	vcn_doorbell_index = NAVI10_DOORBELL64_VCN0_1 << 1;
	doorbell_range = RREG32_PCIE(SOC15_REG_OFFSET(NBIO, 0, regS2A_DOORBELL_ENTRY_4_CTRL));
	doorbell_range = REG_SET_FIELD(doorbell_range,
						S2A_DOORBELL_ENTRY_4_CTRL,
						S2A_DOORBELL_PORT4_ENABLE,
						0x1);
	doorbell_range = REG_SET_FIELD(doorbell_range,
						S2A_DOORBELL_ENTRY_4_CTRL,
						S2A_DOORBELL_PORT4_AWID,
						0x4);
	doorbell_range = REG_SET_FIELD(doorbell_range,
						S2A_DOORBELL_ENTRY_4_CTRL,
						S2A_DOORBELL_PORT4_RANGE_OFFSET,
						vcn_doorbell_index);
	doorbell_range = REG_SET_FIELD(doorbell_range,
						S2A_DOORBELL_ENTRY_4_CTRL,
						S2A_DOORBELL_PORT4_RANGE_SIZE,
						8);
	doorbell_range = REG_SET_FIELD(doorbell_range,
						S2A_DOORBELL_ENTRY_4_CTRL,
						S2A_DOORBELL_PORT4_NEED_DEDUCT_RANGE_OFFSET,
						1);
	doorbell_range = REG_SET_FIELD(doorbell_range,
						S2A_DOORBELL_ENTRY_4_CTRL,
						S2A_DOORBELL_PORT4_AWADDR_31_28_VALUE,
						0x4);
	WREG32_PCIE(SOC15_REG_OFFSET(NBIO, 0, regS2A_DOORBELL_ENTRY_4_CTRL), doorbell_range);

	/* setting VCN1 doorbell range is not needed under SRIOV */
}

static void navi32_vmhub_gfxhub_hook(struct amdgv_adapter *adapt)
{
	adapt->vmhub[VM_GFXHUB].fault_cntl = SOC15_REG_OFFSET(GC, 0, regGCVM_L2_PROTECTION_FAULT_CNTL);
	adapt->vmhub[VM_GFXHUB].fault_status = SOC15_REG_OFFSET(GC, 0, regGCVM_L2_PROTECTION_FAULT_STATUS);
	adapt->vmhub[VM_GFXHUB].fault_addr_lo = SOC15_REG_OFFSET(GC, 0, regGCVM_L2_PROTECTION_FAULT_ADDR_LO32);
	adapt->vmhub[VM_GFXHUB].fault_addr_hi = SOC15_REG_OFFSET(GC, 0, regGCVM_L2_PROTECTION_FAULT_ADDR_HI32);
}

static void navi32_vmhub_mmhub_hook(struct amdgv_adapter *adapt)
{
	adapt->vmhub[VM_MMHUB0].fault_cntl = SOC15_REG_OFFSET(MMHUB, 0, regMMVM_L2_PROTECTION_FAULT_CNTL);
	adapt->vmhub[VM_MMHUB0].fault_status = SOC15_REG_OFFSET(MMHUB, 0, regMMVM_L2_PROTECTION_FAULT_STATUS);
	adapt->vmhub[VM_MMHUB0].fault_addr_lo = SOC15_REG_OFFSET(MMHUB, 0, regMMVM_L2_PROTECTION_FAULT_ADDR_LO32);
	adapt->vmhub[VM_MMHUB0].fault_addr_hi = SOC15_REG_OFFSET(MMHUB, 0, regMMVM_L2_PROTECTION_FAULT_ADDR_HI32);
}

static void navi32_vmhub_hook(struct amdgv_adapter *adapt)
{
	navi32_vmhub_gfxhub_hook(adapt);
	navi32_vmhub_mmhub_hook(adapt);
}

static int navi32_vbios_sw_init(struct amdgv_adapter *adapt)
{
	const char *name = "NAVI32";

	adapt->vbios.is_atom_fw = true;
	adapt->fw_load_type = AMDGV_FW_LOAD_BY_GIM;
	adapt->vbios.read_rom_from_reg = navi32_vbios_read_rom_from_reg;
	adapt->vbios.vmhub_hook = navi32_vmhub_hook;
	adapt->vbios.golden_init = NULL;
	adapt->vbios.special_version_check = navi32_vbios_special_version_check;

	adapt->rs64_enable = true;
	adapt->fw_load_engine = AMDGV_FW_LOAD_PSP;

	adapt->pp.smu_lock = oss_mutex_init();
	if (adapt->pp.smu_lock == OSS_INVALID_HANDLE) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_CREATE_MUTEX_FAIL, 0);
		return AMDGV_FAILURE;
	}

	oss_memcpy(adapt->config.name, name, oss_strlen(name));

	adapt->config.caps.supported_fields_flags = 0;
	adapt->config.caps.supported_fields_flags |= AMDGV_MEM_USAGE_FLAG;
	adapt->config.caps.supported_fields_flags |= AMDGV_POWER_GFX_VOLTAGE_FLAG;
	adapt->config.caps.supported_fields_flags |= AMDGV_MM_METRICS_FLAG;
	adapt->config.caps.supported_fields_flags |= AMDGV_MM2_CLOCK_FLAG;

	navi32_ip_discovery_init(adapt);

	return amdgv_vbios_atom_sw_init(adapt);
}

static int navi32_vbios_sw_fini(struct amdgv_adapter *adapt)
{
	if (adapt->pp.smu_lock) {
		oss_mutex_fini(adapt->pp.smu_lock);
		adapt->pp.smu_lock = OSS_INVALID_HANDLE;
	}

	navi32_ip_discovery_fini(adapt);
	amdgv_vbios_atom_sw_fini(adapt);
	return 0;
}

static uint32_t navi32_vbios_read_rom_data(struct amdgv_adapter *adapt, uint32_t offset)
{
	WREG32(SOC15_REG_OFFSET(SMUIO, 0, regROM_INDEX), offset);
	return RREG32(SOC15_REG_OFFSET(SMUIO, 0, regROM_DATA));
}

static uint32_t navi32_atombios_get_fw_offset(struct amdgv_adapter *adapt, uint8_t fw_type)
{
	uint32_t i, fw_offset = 0, total_entries;
	uint16_t offset;
	int index;
	bool ret;
	uint8_t *vbios = adapt->vbios.image;
	struct atom_context *ctx = adapt->vbios.atom_context;
	PSP_DIRECTORY_TABLE_V2_5 *psptable_2_5;
	PSP_DIRECTORY *psp_dir;

	if (!((fw_type == FW_ID_PSP_BL_STAGE1) ||
	      (fw_type == FW_ID_SEC_GASKET) ||
	      (fw_type == FW_ID_SRIOV_SEC_GASKET))) {
		return AMDGV_FAILURE;
	}

	index = GetIndexIntoMasterTable(DATA, PspDirectory);
	ret = amdgv_atom_parse_data_header(ctx, index, NULL, NULL, NULL, &offset);

	if (!ret)
		return AMDGV_FAILURE;

	psptable_2_5 = (PSP_DIRECTORY_TABLE_V2_5 *)(vbios + offset);

	if (psptable_2_5->table_header.ucTableFormatRevision == 1) {
		return AMDGV_FAILURE;
	}

	psp_dir = (PSP_DIRECTORY *)&psptable_2_5->psp_directory;

	/* TotalEntries comes from the VBIOS image; clamp it to the fixed
	 * pspEntry[] capacity to avoid an out-of-bounds read (CWE-125).
	 */
	total_entries = min(psp_dir->Header.TotalEntries,
			    (uint32_t)PSP_DIRECTORY_MAX_ENTRIES);

	for (i = 0; i < total_entries; i++) {
		if (psp_dir->pspEntry[i].u32Type == fw_type) {
			fw_offset = psp_dir->pspEntry[i].Location;
		}
	}

	return fw_offset;
}

static uint32_t navi32_vbios_get_fw_version(struct amdgv_adapter *adapt, uint8_t fw_type, uint32_t *version)
{
	uint32_t offset;
	uint8_t fw_id;
	uint32_t PspDir_offset = 0;
	uint32_t ISHA_offset = 0;
	uint32_t ROM_offset = 0;

	if (version == NULL)
		return AMDGV_FAILURE;

	switch (fw_type) {
	case PSP_DIR_ENTRY_TYPE_PSP_FW_BOOT_LOADER:
		fw_id = FW_ID_PSP_BL_STAGE1;
		break;
	case PSP_DIR_ENTRY_TYPE_SECURITY_GASKET:
		fw_id = FW_ID_SEC_GASKET;
		break;
	case PSP_DIR_ENTRY_TYPE_PSP_SEC_POLICY_STAGE2:
		fw_id = FW_ID_SRIOV_SEC_GASKET;
		break;
	default:
		return AMDGV_FAILURE;
		break;
	}

	offset = navi32_atombios_get_fw_offset(adapt, fw_id);
	if (offset == AMDGV_FAILURE)
		return AMDGV_FAILURE;

	PspDir_offset = navi32_vbios_read_rom_data(adapt, offsetof(struct _IFWI_GENERIC_EFS, PspDirectoryLocation));

	ISHA_offset = navi32_vbios_read_rom_data(adapt, (PspDir_offset  + offsetof(struct _PSP_DIRECTORY, pspEntry[0].Location)));

	ROM_offset = navi32_vbios_read_rom_data(adapt, (ISHA_offset + offsetof(struct _PSP_IMAGE_SLOT_HEADER, u32Location)));

	WREG32(SOC15_REG_OFFSET(SMUIO, 0, regROM_INDEX), offset + VBIOS_FW_VERSION_OFFSET + ROM_offset);
	*version = RREG32(SOC15_REG_OFFSET(SMUIO, 0, regROM_DATA));

	return 0;
}

static int navi32_vbios_print_fw_version(struct amdgv_adapter *adapt, uint8_t fw_type)
{
	uint32_t version = 0xFFFFFFFF;
	int ret;

	if (navi32_vbios_get_fw_version(adapt, fw_type, &version) == 0) {
		amdgv_atombios_print_fw_version(adapt, fw_type, version);
		ret = 0;
	} else {
		AMDGV_ERROR("Cannot get fw version for fw_type(0x%x)\n", fw_type);
		ret = AMDGV_FAILURE;
	}

	return ret;
}

static int navi32_vbios_hw_init(struct amdgv_adapter *adapt)
{
	int ret;
	uint32_t ver, reg_val;
	uint32_t vbios_status;

	/*  Wait for PSP bootloader to be ready
	*    PSP BL will set bit[31] of C2PMSG_33 to 1
	*/
	if (navi32_psp_wait_boot_complete(
		adapt, SOC15_REG_OFFSET_NAME(MP0, 0, regMP0_SMN_C2PMSG_33)) == false) {
		AMDGV_ERROR("TIMEOUT waiting PSP_BL to finish boot process\n");
		ret = AMDGV_FAILURE;
		goto failed;
	}

	if (adapt->emu_type != FULL_EMU) {
		if (!navi32_vbios_need_post(adapt)) {
			ret = AMDGV_FAILURE;
			if (navi32_psp_wait_sos_loaded_status(adapt) && navi32_vbios_smu_fw_loaded(adapt)) {
				ret = amdgv_reset_hw_for_reload(adapt, false);
				if (ret)
					goto failed;
			} else {
				/* C2PMSG_81 != 0: PSP has been loaded but hang.
				* C2PMSG_81 == 0: PSP has not been loaded and we can just do the original initialization path.
				*/
				reg_val = RREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_81));
				if (reg_val != 0)
					goto failed;
			}
		}

		if (!adapt->reset.reset_state) {
			/* read rom */
			ret = amdgv_vbios_read_img(adapt);
			if (ret)
				return ret;
		}

		/* atom parser init */
		ret = amdgv_vbios_atom_hw_init(adapt);
		if (ret)
			goto failed;

		/* post vbios */
		ret = amdgv_atomfirmware_post(adapt, VBIOS_POST_ASIC_INIT);
		if (ret)
			goto failed;
	}

	AMDGV_INFO("VBIOS posted successfully.\n");

	/* BIOS_SCRATCH_7 should be 0x200 after posting */
	vbios_status = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BIOS_SCRATCH_7));
	if (!(vbios_status & ATOM_ASIC_INIT_COMPLETE)) {
		ret = AMDGV_FAILURE;
		AMDGV_ERROR("Invalid VBIOS status after posting.\n");
		goto failed;
	}

	/* print PSP related VBIOS FW version, if we can get them */
	navi32_vbios_print_fw_version(adapt, PSP_DIR_ENTRY_TYPE_PSP_FW_BOOT_LOADER);
	navi32_vbios_print_fw_version(adapt, PSP_DIR_ENTRY_TYPE_SECURITY_GASKET);
	navi32_vbios_print_fw_version(adapt, PSP_DIR_ENTRY_TYPE_PSP_SEC_POLICY_STAGE2);

	adapt->flags |= AMDGV_FLAG_IH_REG_PSP_EN;
	adapt->flags |= AMDGV_FLAG_GC_REG_RLC_EN;

	navi32_vbios_get_fw_version(adapt, PSP_DIR_ENTRY_TYPE_PSP_SEC_POLICY_STAGE2, &ver);

	ret = navi32_discover_ip(adapt);
	if (ret)
		goto failed;

	navi32_vmhub_gfxhub_hook(adapt);
	navi32_vmhub_mmhub_hook(adapt);

	adapt->mc_fb_loc_addr = RREG32(SOC15_REG_OFFSET(MMHUB, 0, regMMMC_VM_FB_LOCATION_BASE));
	adapt->mc_fb_loc_addr =
		REG_GET_FIELD(adapt->mc_fb_loc_addr, MMMC_VM_FB_LOCATION_BASE, FB_BASE);
	adapt->mc_fb_loc_addr <<= MC_VM_FB_LOCATION__FB_ADDRESS__SHIFT;

	adapt->mc_fb_offset = (uint64_t)RREG32_SOC15(GC, 0, regMMMC_VM_FB_OFFSET) << 24;
	AMDGV_INFO("MC base is at 0x%llx, adapt->mc_fb_offset:%llx\n", adapt->mc_fb_loc_addr, adapt->mc_fb_offset);

	adapt->mc_fb_top_addr = RREG32(SOC15_REG_OFFSET(MMHUB, 0, regMMMC_VM_FB_LOCATION_TOP));
	adapt->mc_fb_top_addr =
		REG_GET_FIELD(adapt->mc_fb_top_addr, MMMC_VM_FB_LOCATION_TOP, FB_TOP);
	adapt->mc_fb_top_addr <<= MC_VM_FB_LOCATION__FB_ADDRESS__SHIFT;
	adapt->mc_fb_top_addr |= ((1 << MC_VM_FB_LOCATION__FB_ADDRESS__SHIFT) - 1);

	adapt->mc_agp_loc_addr = adapt->mc_fb_top_addr + 1;
	adapt->mc_agp_top_addr = adapt->mc_agp_loc_addr + AMDGV_AGP_APERTURE_SIZE - 1;
	if (adapt->flags & AMDGV_FLAG_USE_PF) {
		/* No AGP platform */
		adapt->mc_agp_top_addr = adapt->mc_agp_loc_addr = adapt->mc_fb_top_addr;
	}

	adapt->mc_sys_loc_addr = adapt->mc_fb_loc_addr;
	adapt->mc_sys_top_addr = adapt->mc_agp_top_addr;

	AMDGV_DEBUG("FB MC base is at 0x%llx\n", adapt->mc_fb_loc_addr);
	AMDGV_DEBUG("FB MC top is at 0x%llx\n", adapt->mc_fb_top_addr);
	AMDGV_DEBUG("SYS MC base is at 0x%llx\n", adapt->mc_sys_loc_addr);
	AMDGV_DEBUG("SYS MC top is at 0x%llx\n", adapt->mc_sys_top_addr);
	AMDGV_DEBUG("AGP MC base is at 0x%llx\n", adapt->mc_agp_loc_addr);
	AMDGV_DEBUG("AGP MC top is at 0x%llx\n", adapt->mc_agp_top_addr);

	ret = amdgv_atomfirmware_set_fw_usage_fb_guest(adapt);
	if (ret) {
		AMDGV_ERROR("set guest fw usage fb failed!\n");
		goto failed;
	}

	ret = amdgv_atomfirmware_get_vram_info(adapt);
	if (ret) {
		goto failed;
	}

	navi32_nbio_get_vram_vendor(adapt);

	/* MM capability should be retrieved from vbios later */
	adapt->max_mm_bandwidth[AMDGV_HEVC_ENGINE] = 2 << 20;
	adapt->max_mm_bandwidth[AMDGV_HEVC1_ENGINE] = 2 << 20;
	adapt->max_mm_bandwidth[AMDGV_VCN_ENGINE] = 2 << 20;

	navi32_program_asic_golden_settings(adapt);

	navi32_assign_asic_doorbell_ranges(adapt);

	mmhub_v3_0_gart_enable(adapt);

	ret = navi32_enable_pci_atomic_request(adapt);
	if (ret) {
		AMDGV_ERROR("enable pci atomic request failed!\n");
		goto failed;
	}

	ret = navi32_disable_pci_aer(adapt);
	if (ret) {
		AMDGV_ERROR("disable pci aer failed!\n");
		goto failed;
	}

	return 0;

failed:
	amdgv_vbios_atom_hw_fini(adapt);

	return ret;
}

static int navi32_vbios_hw_fini(struct amdgv_adapter *adapt)
{
	amdgv_vbios_atom_hw_fini(adapt);

	if (!adapt->reset.reset_state) {
		mmhub_v3_0_gart_fini(adapt);
		/* hardware will be put to RESET status after called this, so
		put it to the last step of hw_fini */
		if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->prepare_unload)
			adapt->pp.pp_funcs->prepare_unload(adapt);
	}

	return 0;
}

const struct amdgv_init_func navi32_vbios_func = {
	.name = "navi32_vbios_func",
	.sw_init = navi32_vbios_sw_init,
	.sw_fini = navi32_vbios_sw_fini,
	.hw_init = navi32_vbios_hw_init,
	.hw_fini = navi32_vbios_hw_fini,
};
