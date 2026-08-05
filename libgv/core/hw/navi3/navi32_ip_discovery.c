/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include <amdgv.h>
#include "amdgv_vfmgr.h"
#include "navi32_reg_inc.h"
#include "navi32_psp.h"
#include "navi32_gfx.h"
#include "navi32_ip_discovery.h"
#include "navi32_mmsch.h"
#include "amdgv_sched_internal.h"

static const uint32_t this_block = AMDGV_SECURITY_BLOCK;

#define HW_ID_MAX 300
static const char *hw_id_names[HW_ID_MAX] = {
	[MP1_HWID]		= "MP1",
	[MP2_HWID]		= "MP2",
	[THM_HWID]		= "THM",
	[SMUIO_HWID]		= "SMUIO",
	[FUSE_HWID]		= "FUSE",
	[CLKA_HWID]		= "CLKA",
	[PWR_HWID]		= "PWR",
	[GC_HWID]		= "GC",
	[VCN_HWID]		= "VCN",
	[VCE_HWID]		= "VCE",
	[AUDIO_AZ_HWID]		= "AUDIO_AZ",
	[ACP_HWID]		= "ACP",
	[DCI_HWID]		= "DCI",
	[DMU_HWID]		= "DMU",
	[DCO_HWID]		= "DCO",
	[DIO_HWID]		= "DIO",
	[XDMA_HWID]		= "XDMA",
	[DCEAZ_HWID]		= "DCEAZ",
	[DAZ_HWID]		= "DAZ",
	[SDPMUX_HWID]		= "SDPMUX",
	[NTB_HWID]		= "NTB",
	[IOHC_HWID]		= "IOHC",
	[L2IMU_HWID]		= "L2IMU",
	[MMHUB_HWID]		= "MMHUB",
	[ATHUB_HWID]		= "ATHUB",
	[DBGU_NBIO_HWID]	= "DBGU_NBIO",
	[DFX_HWID]		= "DFX",
	[DBGU0_HWID]		= "DBGU0",
	[DBGU1_HWID]		= "DBGU1",
	[OSSSYS_HWID]		= "OSSSYS",
	[HDP_HWID]		= "HDP",
	[SDMA0_HWID]		= "SDMA0",
	[SDMA1_HWID]		= "SDMA1",
	[SDMA2_HWID]		= "SDMA2",
	[SDMA3_HWID]		= "SDMA3",
	[ISP_HWID]		= "ISP",
	[DBGU_IO_HWID]		= "DBGU_IO",
	[DF_HWID]		= "DF",
	[CLKB_HWID]		= "CLKB",
	[FCH_HWID]		= "FCH",
	[DFX_DAP_HWID]		= "DFX_DAP",
	[L1IMU_PCIE_HWID]	= "L1IMU_PCIE",
	[L1IMU_NBIF_HWID]	= "L1IMU_NBIF",
	[L1IMU_IOAGR_HWID]	= "L1IMU_IOAGR",
	[L1IMU3_HWID]		= "L1IMU3",
	[L1IMU4_HWID]		= "L1IMU4",
	[L1IMU5_HWID]		= "L1IMU5",
	[L1IMU6_HWID]		= "L1IMU6",
	[L1IMU7_HWID]		= "L1IMU7",
	[L1IMU8_HWID]		= "L1IMU8",
	[L1IMU9_HWID]		= "L1IMU9",
	[L1IMU10_HWID]		= "L1IMU10",
	[L1IMU11_HWID]		= "L1IMU11",
	[L1IMU12_HWID]		= "L1IMU12",
	[L1IMU13_HWID]		= "L1IMU13",
	[L1IMU14_HWID]		= "L1IMU14",
	[L1IMU15_HWID]		= "L1IMU15",
	[WAFLC_HWID]		= "WAFLC",
	[FCH_USB_PD_HWID]	= "FCH_USB_PD",
	[PCIE_HWID]		= "PCIE",
	[PCS_HWID]		= "PCS",
	[DDCL_HWID]		= "DDCL",
	[SST_HWID]		= "SST",
	[IOAGR_HWID]		= "IOAGR",
	[NBIF_HWID]		= "NBIF",
	[IOAPIC_HWID]		= "IOAPIC",
	[SYSTEMHUB_HWID]	= "SYSTEMHUB",
	[NTBCCP_HWID]		= "NTBCCP",
	[UMC_HWID]		= "UMC",
	[SATA_HWID]		= "SATA",
	[USB_HWID]		= "USB",
	[CCXSEC_HWID]		= "CCXSEC",
	[XGMI_HWID]		= "XGMI",
	[XGBE_HWID]		= "XGBE",
	[MP0_HWID]		= "MP0",
};

/* hw_id comes from the (firmware/file-sourced) IP-discovery blob and is only
 * container-validated, so it may exceed HW_ID_MAX or point at an unmapped slot.
 * Guard the lookup to avoid an out-of-bounds / NULL %s dereference (CWE-125).
 */
static const char *hwid_name(uint16_t hw_id)
{
	return (hw_id < HW_ID_MAX && hw_id_names[hw_id]) ? hw_id_names[hw_id] : "?";
}

static int hw_id_map[MAX_HWIP] = {
	[GC_HWIP]	= GC_HWID,
	[HDP_HWIP]	= HDP_HWID,
	[SDMA0_HWIP]	= SDMA0_HWID,
	[SDMA1_HWIP]	= SDMA1_HWID,
	[SDMA2_HWIP]	= SDMA2_HWID,
	[SDMA3_HWIP]	= SDMA3_HWID,
	[MMHUB_HWIP]	= MMHUB_HWID,
	[ATHUB_HWIP]	= ATHUB_HWID,
	[NBIO_HWIP]	= NBIF_HWID,
	[MP0_HWIP]	= MP0_HWID,
	[MP1_HWIP]	= MP1_HWID,
	[VCN_HWIP]	= VCN_HWID,
	[VCE_HWIP]	= VCE_HWID,
	[DF_HWIP]	= DF_HWID,
	[DCE_HWIP]	= DCEAZ_HWID,
	[OSSSYS_HWIP]	= OSSSYS_HWID,
	[SMUIO_HWIP]	= SMUIO_HWID,
	[PWR_HWIP]	= PWR_HWID,
	[NBIF_HWIP]	= NBIF_HWID,
	[THM_HWIP]	= THM_HWID,
	[CLK_HWIP]	= CLKA_HWID,
	[UMC_HWIP]	= UMC_HWID,
};

static uint16_t navi32_ip_discovery_get_checksum(uint8_t *data, uint32_t size)
{
	uint16_t checksum = 0;
	uint32_t i;

	for (i = 0; i < size; i++)
		checksum += data[i];

	return checksum;
}

static int navi32_ip_discovery_table_checksum(struct amdgv_adapter *adapt,
					     struct amdgv_ip_discovery_info *copy,
					     enum amdgv_checksum_cmd opt)
{
	int i;
	uint8_t *data;
	uint16_t checksum;
	uint32_t size;
	uint32_t offset;

	for (i = 0; i <= HARVEST_INFO; i++) {
		offset = copy->bhdr->v1.table_list[i].offset;
		size = copy->bhdr->v1.table_list[i].size;

		/* table entry must lie fully within the mapped discovery region */
		if (offset >= AMDGV_IP_DISCOVERY_SIZE ||
		    size > AMDGV_IP_DISCOVERY_SIZE - offset) {
			AMDGV_ERROR("ip_discovery_table[%d]_checksum ERROR - entry out of"
				    " range: offset=%u, size=%u, buffer size=%d",
				    i, offset, size, AMDGV_IP_DISCOVERY_SIZE);
			return AMDGV_FAILURE;
		}

		data = (uint8_t *)copy->data + offset;
		checksum = navi32_ip_discovery_get_checksum(data, size);
		if (opt == UPDATE) {
			copy->bhdr->v1.table_list[i].checksum = checksum;
		} else if (copy->bhdr->v1.table_list[i].checksum != checksum) {
			AMDGV_ERROR("ip_discovery_table[%d]_checksum MISMATCH"
				    " expected=0x%08x readback=0x%08x\n",
				    i, checksum, copy->bhdr->v1.table_list[i].checksum);
			return AMDGV_FAILURE;
		}
	}
	return 0;
}

static int navi32_ip_discovery_binary_checksum(struct amdgv_adapter *adapt,
					      struct amdgv_ip_discovery_info *copy,
					      enum amdgv_checksum_cmd opt)
{
	uint16_t checksum;
	uint32_t offset;

	offset = (uint8_t *)&copy->bhdr->binary_size - (uint8_t *)copy->data;
	checksum = navi32_ip_discovery_get_checksum((uint8_t *)copy->data + offset,
						   copy->bhdr->binary_size - offset);
	if (opt == UPDATE) {
		copy->bhdr->binary_checksum = checksum;
	} else if (copy->bhdr->binary_checksum != checksum) {
		AMDGV_ERROR("ip_discovery_binary_checksum MISMATCH"
			    " expected=0x%08x readback=0x%08x\n",
			    checksum, copy->bhdr->binary_checksum);
		return AMDGV_FAILURE;
	}
	return 0;
}

static int navi32_read_ip_discovery(struct amdgv_adapter *adapt)
{
	unsigned int i, size_dw;
	uint64_t addr;

	addr = amdgv_misc_get_memsize(adapt);
	addr = (addr << 20) - AMDGV_IP_DISCOVERY_OFFSET;

	size_dw = AMDGV_IP_DISCOVERY_SIZE >> 2;
	for (i = 0; i < size_dw; i++, addr += 4)
		adapt->ip_discovery.pf_copy.data[i] = READ_FB32(addr);

	return 0;
}

static void navi32_ip_discovery_map(struct amdgv_adapter *adapt,
				   struct amdgv_ip_discovery_info *copy)
{
	uint8_t *addr;
	uint8_t *base;

	addr = base = (uint8_t *)copy->data;

	copy->bhdr = (struct amdgv_binary_header *)addr;

	addr = base + copy->bhdr->v1.table_list[IP_DISCOVERY].offset;
	copy->ihdr = (struct amdgv_ip_discovery_header *)addr;

	addr = base + copy->bhdr->v1.table_list[GC_INFO].offset;
	copy->gchdr = (struct amdgv_gpu_info_header *)addr;

	addr = base + copy->bhdr->v1.table_list[HARVEST_INFO].offset;
	copy->htbl = (union amdgv_harvest_table *)addr;
}

static int navi32_parse_gc_table(struct amdgv_adapter *adapt)
{
	struct amdgv_ip_discovery_info *pf_copy;
	struct amdgv_gc_info_v1_2 *gc_info;

	pf_copy = &adapt->ip_discovery.pf_copy;

	AMDGV_DEBUG("GC TABLE id: %d\n", pf_copy->gchdr->table_id);
	AMDGV_DEBUG("GC TABLE major: %d\n", pf_copy->gchdr->version_major);
	AMDGV_DEBUG("GC TABLE minor: %d\n", pf_copy->gchdr->version_minor);
	AMDGV_DEBUG("GC TABLE size: %d bytes\n", pf_copy->gchdr->size_bytes);

	gc_info = GET_GC_TABLE_V1_2(pf_copy->gchdr);

	adapt->config.gfx.max_shader_engines = gc_info->gc_num_se;
	adapt->config.gfx.max_cu_per_sh =
		2 * (gc_info->gc_num_wgp0_per_sa + gc_info->gc_num_wgp1_per_sa);
	adapt->config.gfx.max_sh_per_se = gc_info->gc_num_sa_per_se;
	adapt->config.gfx.max_waves_per_simd = gc_info->gc_max_waves_per_simd;
	adapt->config.gfx.wave_size = gc_info->gc_wave_size;

	AMDGV_DEBUG("+gc_num_se          : %d\n", gc_info->gc_num_se);
	AMDGV_DEBUG("+gc_num_wgp0_per_sa : %d\n", gc_info->gc_num_wgp0_per_sa);
	AMDGV_DEBUG("+gc_num_wgp1_per_sa : %d\n", gc_info->gc_num_wgp1_per_sa);
	AMDGV_DEBUG("+gc_num_sa_per_se   : %d\n", gc_info->gc_num_sa_per_se);

	navi32_gfx_atc_ats_invalidate(adapt);

	adapt->config.gfx.active_cu_count = navi32_gfx_cu_count(adapt);
	AMDGV_DEBUG("+gc_num_active_cu   : %d\n", adapt->config.gfx.active_cu_count);

	return 0;
}

static int navi32_parse_harvest_table(struct amdgv_adapter *adapt)
{
	union amdgv_harvest_table *htbl = NULL;
	uint32_t i = 0, umc_harvest_config = 0;
	uint32_t hw_ip;

	/* Let's use IP discovery table for harvest for now */
	for (hw_ip = 0; hw_ip < AMDGV_MAX_MM_ENGINE; hw_ip++) {
		if (adapt->config.mm.count[hw_ip]) {
			switch (hw_ip) {
			case AMDGV_HEVC_ENGINE:
			case AMDGV_VCN_ENGINE:
				AMDGV_DEBUG("MM Engine VCN count: %d\n",
					    adapt->config.mm.count[hw_ip]);
				break;
			case AMDGV_VCE_ENGINE:
				AMDGV_DEBUG("MM Engine VCE count: %d\n",
					    adapt->config.mm.count[hw_ip]);
				break;
			default:
				break;
			}
		}
	}

	htbl = adapt->ip_discovery.pf_copy.htbl;
	/* find harvest umc instance */
	for (i = 0; i < 32; i++) {
		if (htbl->v1.list[i].hw_id == UMC_HWID) {
			adapt->umc.num_umc--;
			umc_harvest_config |=
				1 << htbl->v1.list[i].number_instance;
		} else if (htbl->v1.list[i].hw_id == 0)
			break;
	}

	adapt->umc.active_mask = (((uint64_t)1 << adapt->umc.node_inst_num) - 1) & ~umc_harvest_config;

	return 0;
}

static void navi32_hw_ip_map(struct amdgv_adapter *adapt, struct amdgv_ip *ip)
{
	uint32_t hw_ip, ipn;

	for (hw_ip = 0; hw_ip < MAX_HWIP; hw_ip++) {
		if (hw_id_map[hw_ip] == ip->hw_id && hw_id_map[hw_ip] != 0) {
			ipn = ip->instance_number;
			if (ipn >= HWIP_MAX_INSTANCE) {
				AMDGV_WARN("IP instance %u for hw_ip %u exceeds HWIP_MAX_INSTANCE (%d), skipping\n",
					   ipn, hw_ip, HWIP_MAX_INSTANCE);
				continue;
			}
			adapt->reg_offset[hw_ip][ipn] = ip->base_address;
			switch (ip->hw_id) {
			case VCE_HWID:
				adapt->config.mm.count[AMDGV_VCE_ENGINE]++;
				break;
			case VCN_HWID:
				adapt->config.mm.count[AMDGV_VCN_ENGINE]++;
				break;
			case UMC_HWID:
				adapt->umc.num_umc++;
				/* total umc node instance including harvest one */
				adapt->umc.node_inst_num++;
				break;
			default:
				break;
			}
		}
	}
}

static int navi32_parse_ip_discovery(struct amdgv_adapter *adapt)
{
	struct amdgv_ip_discovery_info *pf_copy;
	struct amdgv_die_header *dhdr;
	int i, j;
	struct amdgv_ip *ip = NULL;
	uint8_t *start;

	pf_copy = &adapt->ip_discovery.pf_copy;

	navi32_ip_discovery_map(adapt, pf_copy);

	if (pf_copy->bhdr->binary_signature != BINARY_SIGNATURE) {
		AMDGV_ERROR("ip_discovery_binary_signature MISMATCH"
			    " expected=0x%08x readback=0x%08x\n",
			    BINARY_SIGNATURE, pf_copy->bhdr->binary_signature);
		return AMDGV_FAILURE;
	}

	/* ihdr is mapped at an untrusted table offset; the discovery header must
	 * fit within the mapped region before any field is dereferenced */
	if (pf_copy->bhdr->v1.table_list[IP_DISCOVERY].offset >= AMDGV_IP_DISCOVERY_SIZE ||
	    sizeof(struct amdgv_ip_discovery_header) >
		    AMDGV_IP_DISCOVERY_SIZE - pf_copy->bhdr->v1.table_list[IP_DISCOVERY].offset) {
		AMDGV_ERROR("ip_discovery_header out of range - offset=%u, buffer size=%d",
			    (uint32_t)pf_copy->bhdr->v1.table_list[IP_DISCOVERY].offset,
			    AMDGV_IP_DISCOVERY_SIZE);
		return AMDGV_FAILURE;
	}

	if (pf_copy->ihdr->signature != DISCOVERY_TABLE_SIGNATURE) {
		AMDGV_ERROR("ip_discovery_table_signature MISMATCH"
			    " expected=0x%08x readback=0x%08x\n",
			    DISCOVERY_TABLE_SIGNATURE, pf_copy->ihdr->signature);
		return AMDGV_FAILURE;
	}

	if (navi32_ip_discovery_binary_checksum(adapt, pf_copy, VERIFY))
		return AMDGV_FAILURE;

	if (navi32_ip_discovery_table_checksum(adapt, pf_copy, VERIFY))
		return AMDGV_FAILURE;

	/* Parse IP discovery table */

	AMDGV_INFO("IP discovery version: 0x%x\n", pf_copy->ihdr->version);
	AMDGV_DEBUG("IP discovery table size: %u bytes\n", pf_copy->ihdr->size);
	AMDGV_DEBUG("IP discovery id: 0x%x\n", pf_copy->ihdr->id);
	AMDGV_DEBUG("IP discovery num dies: %d\n", pf_copy->ihdr->num_dies);

	start = (uint8_t *)pf_copy->data;

	forEachDie(i, dhdr, pf_copy->ihdr, start) {
		AMDGV_DEBUG("Die: %d ID: 0x%x num_ips: %d\n", i, dhdr->die_id, dhdr->num_ips);
		forEachIP(j, ip, dhdr) {
			AMDGV_DEBUG("ip %s [hwid=%d, inst=%d]"
				    "\tv%d.%d rev(%d)\t@%08x\n",
				    hwid_name(ip->hw_id), ip->hw_id, ip->instance_number,
				    ip->major, ip->minor, ip->revision, ip->base_address[0]);
			navi32_hw_ip_map(adapt, ip);
		}
	}

	return 0;
}

static int navi32_ip_discovery_vf_add_harvest_entry(struct amdgv_adapter *adapt,
									struct amdgv_ip_discovery_info *vf_copy,
									uint16_t hw_id, uint8_t number_instance)
{
	union amdgv_harvest_table *htbl = NULL;
	uint32_t i = 0;

	if (vf_copy != NULL && vf_copy->htbl != NULL) {

		htbl = vf_copy->htbl;

		for (i = 0; i < 32; i++) {
			// In order to prevent the insertion of duplicate entries
			if (htbl->v1.list[i].hw_id == hw_id && htbl->v1.list[i].number_instance == number_instance) {
				goto fail;
			} else if (htbl->v1.list[i].hw_id == 0)
				break;
		}

		if (i < 32) {
			htbl->v1.list[i].hw_id = hw_id;
			htbl->v1.list[i].number_instance = number_instance;
			return 0;
		}
	}

fail:
	AMDGV_ERROR("Cannot add entry into harvest table\n");
	return AMDGV_FAILURE;
}

static void navi32_ip_discovery_patch_vf_copy_by_index(struct amdgv_adapter *adapt,
					     struct amdgv_ip_discovery_info *copy, uint32_t idx_vf)
{
	struct amdgv_die_header *dhdr;
	struct amdgv_ip *ip = NULL;
	uint8_t *start;
	int i, j;

	start = (uint8_t *)copy->data;
	forEachDie(i, dhdr, copy->ihdr, start) {
		forEachIP(j, ip, dhdr) {
			switch (ip->hw_id) {
			case VCN_HWID:
				navi32_mmsch_modify_vcn_ip_discovery_revison(adapt, idx_vf, ip->instance_number, &ip->revision);
				break;
			case DMU_HWID:
				navi32_ip_discovery_vf_add_harvest_entry(adapt, copy, ip->hw_id, ip->instance_number);
				break;
			default:
				break;
			}
		}
	}
}

static int navi32_prepare_ip_discovery_vf(struct amdgv_adapter *adapt)
{
	struct amdgv_ip_discovery_info *pf_copy;
	struct amdgv_ip_discovery_info *vf_copy;

	pf_copy = &adapt->ip_discovery.pf_copy;
	vf_copy = &adapt->ip_discovery.vf_copy;

	oss_memcpy((void *)vf_copy->data, (void *)pf_copy->data, AMDGV_IP_DISCOVERY_SIZE);

	navi32_ip_discovery_map(adapt, vf_copy);
	if (navi32_ip_discovery_table_checksum(adapt, vf_copy, UPDATE))
		return AMDGV_FAILURE;

	if (navi32_ip_discovery_binary_checksum(adapt, vf_copy, UPDATE))
		return AMDGV_FAILURE;

	return 0;
}

int navi32_discover_ip(struct amdgv_adapter *adapt)
{
	/* save ASIC name and supported flags and restore it */
	char asic_name[AMDGV_SMI_ASIC_NAME];
	uint32_t supported_flags = adapt->config.caps.supported_fields_flags;
	oss_memcpy(asic_name, adapt->config.name, AMDGV_SMI_ASIC_NAME);

	/* clear IP discovery parsing on init */
	oss_memset(&adapt->config, 0, sizeof(adapt->config));
	adapt->umc.num_umc = 0;
	adapt->umc.node_inst_num = 0;
	adapt->umc.active_mask = 0;

	oss_memcpy(adapt->config.name, asic_name, AMDGV_SMI_ASIC_NAME);
	adapt->config.caps.supported_fields_flags = supported_flags;

	if (navi32_read_ip_discovery(adapt))
		return AMDGV_FAILURE;

	if (navi32_parse_ip_discovery(adapt))
		return AMDGV_FAILURE;

	if (adapt->live_update_state == AMDGV_LIVE_UPDATE_RESTORE)
		if (navi32_parse_gc_table(adapt))
			return AMDGV_FAILURE;

	if (navi32_parse_harvest_table(adapt))
		return AMDGV_FAILURE;

	navi32_prepare_ip_discovery_vf(adapt);
	return 0;
}

int navi32_copy_ip_data_to_vf(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint64_t offset;
	struct amdgv_vf_device *vf;
	struct amdgv_ip_discovery_info vf_copy = {0};
	int ret;

	/* duplicated common VF copy IP discovery data */
	vf_copy.data = (uint32_t *)oss_alloc_memory(AMDGV_IP_DISCOVERY_SIZE);
	if (vf_copy.data == NULL) {
		amdgv_put_log(
			AMDGV_PF_IDX,
			AMDGV_LOG_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
			AMDGV_IP_DISCOVERY_SIZE
		);
		return AMDGV_FAILURE;
	}
	oss_memcpy(vf_copy.data, adapt->ip_discovery.vf_copy.data, AMDGV_IP_DISCOVERY_SIZE);

	/* update pointers in this duplicated vf_copy */
	navi32_ip_discovery_map(adapt, &vf_copy);

	/* patch VF copy according to individual VF index */
	navi32_ip_discovery_patch_vf_copy_by_index(adapt, &vf_copy, idx_vf);
	navi32_ip_discovery_table_checksum(adapt, &vf_copy, UPDATE);
	navi32_ip_discovery_binary_checksum(adapt, &vf_copy, UPDATE);

	vf = &adapt->array_vf[idx_vf];

	if (adapt->ffbm.share_tmr)
		offset = ((uint64_t)vf->fb_size_tmr << 20) - AMDGV_IP_DISCOVERY_OFFSET;
	else
		offset = ((uint64_t)vf->fb_size << 20) - AMDGV_IP_DISCOVERY_OFFSET;

	ret = amdgv_vfmgr_copy_to_vf_fb_abs(adapt, idx_vf, offset, vf_copy.data,
					    AMDGV_IP_DISCOVERY_SIZE);

	oss_free_memory(vf_copy.data);

	return ret;
}

int navi32_ip_discovery_init(struct amdgv_adapter *adapt)
{
	adapt->ip_discovery.copy_to_vf = navi32_copy_ip_data_to_vf;
	adapt->ip_discovery.discover_ip = navi32_discover_ip;
	adapt->ip_discovery.parse_gc_table = navi32_parse_gc_table;

	adapt->ip_discovery.pf_copy.data =
		(uint32_t *)oss_alloc_memory(AMDGV_IP_DISCOVERY_SIZE);
	adapt->ip_discovery.vf_copy.data =
		(uint32_t *)oss_alloc_memory(AMDGV_IP_DISCOVERY_SIZE);

	if (adapt->ip_discovery.pf_copy.data == NULL ||
	    adapt->ip_discovery.vf_copy.data == NULL)
		return AMDGV_FAILURE;
	return 0;
}

void navi32_ip_discovery_fini(struct amdgv_adapter *adapt)
{
	if (adapt->ip_discovery.pf_copy.data) {
		oss_free_memory(adapt->ip_discovery.pf_copy.data);
		adapt->ip_discovery.pf_copy.data = NULL;
	}
	if (adapt->ip_discovery.vf_copy.data) {
		oss_free_memory(adapt->ip_discovery.vf_copy.data);
		adapt->ip_discovery.vf_copy.data = NULL;
	}
}
