/*
 * Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
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

#include <amdgv_device.h>
#include <amdgv.h>
#include "amdgv_vfmgr.h"
#include "mi300.h"
#include "mi300_psp.h"
#include "mi300_gfx.h"
#include "mi300_ip_discovery.h"
#include "mi300_ecc.h"
#include "amdgv_sched_internal.h"
#include "mi300/NBIO/nbio_7_9_0_offset.h"
#include "mi300/NBIO/nbio_7_9_0_sh_mask.h"

static const uint32_t this_block = AMDGV_SECURITY_BLOCK;

#define mmRCC_CONFIG_MEMSIZE    0xde3
#define mmMP0_SMN_C2PMSG_33     0x16061
#define PSP_C2P_MAILBOX_CLOSED  0xFFFFFFFF
#define IP_LIST_OFFSET (0x8C + 4) /* Die List offset (0x8C) + Die header (4) */
#define HARVEST_TABLE_LEN 32
#define HW_ID_MAX 300
#define MAX(a, b)	((a) > (b) ? (a) : (b))

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
	[MP5_HWID]		= "MP5",
	[SMNCONFIG_HWID]	= "SMNCONFIG",
};

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

static uint16_t mi300_ip_discovery_get_checksum(uint8_t *data, uint32_t size)
{
	uint16_t checksum = 0;
	int i;

	for (i = 0; i < size; i++)
		checksum += data[i];

	return checksum;
}

static int mi300_ip_discovery_table_checksum(struct amdgv_adapter *adapt,
					     struct amdgv_ip_discovery_info *copy,
					     enum amdgv_checksum_cmd opt)
{
	int i;
	uint8_t *data;
	uint16_t checksum;
	uint32_t size;

	for (i = 0; i <= NPS_INFO; i++) {
		data = (uint8_t *)copy->data + copy->bhdr->table_list[i].offset;
		size = copy->bhdr->table_list[i].size;

		checksum = mi300_ip_discovery_get_checksum(data, size);
		if (opt == UPDATE) {
			copy->bhdr->table_list[i].checksum = checksum;
		} else if (copy->bhdr->table_list[i].checksum != checksum) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_VBIOS_IP_DISCOVERY_TABLE_CHECKSUM_FAIL,
				AMDGV_ERROR_16_16_32(i, checksum, copy->bhdr->table_list[i].checksum));
			return AMDGV_FAILURE;
		}
	}
	return 0;
}

static int mi300_ip_discovery_binary_checksum(struct amdgv_adapter *adapt,
					      struct amdgv_ip_discovery_info *copy,
					      enum amdgv_checksum_cmd opt)
{
	uint16_t checksum;
	uint32_t offset;

	/* binary_size claim from the raw data must not exceed buffer length during checksum calculation */
	if (copy->bhdr->binary_size > AMDGV_IP_DISCOVERY_SIZE) {
		AMDGV_ERROR("ip_discovery_binary_checksum binary_size ERROR - greater than buffer size:"
				" binary_size=%d, buffer size=%d", copy->bhdr->binary_size, AMDGV_IP_DISCOVERY_SIZE);
		return AMDGV_FAILURE;
	}

	offset = (uint8_t *)&copy->bhdr->binary_size - (uint8_t *)copy->data;
	checksum = mi300_ip_discovery_get_checksum((uint8_t *)copy->data + offset,
						   copy->bhdr->binary_size - offset);
	if (opt == UPDATE) {
		copy->bhdr->binary_checksum = checksum;
	} else if (copy->bhdr->binary_checksum != checksum) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_VBIOS_IP_DISCOVERY_BINARY_CHECKSUM_FAIL,
			AMDGV_ERROR_32_32(checksum, copy->bhdr->binary_checksum));
		return AMDGV_FAILURE;
	}
	return 0;
}

static int mi300_read_binary_from_file(struct amdgv_adapter *adapt)
{
	AMDGV_DEBUG("Use ip discovery info from file...\n");

	if (oss_get_discovery_binary(adapt->dev, adapt->asic_type, adapt->ip_discovery.pf_copy.data,
			MI300_IP_DISCOVERY_SIZE)) {
		AMDGV_ERROR("Failed to get IP discovery binary from file.\n");
		return AMDGV_FAILURE;
	}
	AMDGV_DEBUG("Got IP discovery binary from file.\n");

	return 0;
}

static int mi300_read_ip_discovery(struct amdgv_adapter *adapt,
	uint32_t offset, uint32_t size)
{
	unsigned int i, size_dw;
	uint64_t addr;

	if (adapt->ip_discovery_load_type == AMDGV_IP_DISCOVERY_LOAD_DIRECT) {
		return mi300_read_binary_from_file(adapt);
	}

	/*  Wait for PSP bootloader to be ready
	 *    PSP BL will set bit[31] of C2PMSG_33 to 1
	 *    only in case when TOS is not loaded
	 */
	oss_msleep(150); /* sleep in case TOS is just starting */
	if (mi300_psp_get_sos_loaded_status(adapt) == 0) {
		if (mi300_psp_wait_for_bootloader_steady(adapt)) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_BL_FAIL, 0);
			return AMDGV_FAILURE;
		}
	}

	addr = (uint64_t)RREG32(mmRCC_CONFIG_MEMSIZE);
	if ((addr == 0) || (addr >= 0x80000)) {
		/* reading all zeros, or >= 512 GB memory size indicates reading error */
		AMDGV_ERROR("Invalid FB Size %llu from RCC_CONFIG_MEMSIZE register\n", addr);
		return AMDGV_FAILURE;
	}

	addr = ((addr << 20) - AMDGV_IP_DISCOVERY_OFFSET) + offset;

	size_dw = (offset + size) >> 2;
	for (i = (offset >> 2); i < size_dw; i++, addr += 4)
		adapt->ip_discovery.pf_copy.data[i] = READ_FB32(addr);

	return 0;
}

static void mi300_ip_discovery_map(struct amdgv_adapter *adapt,
				   struct amdgv_ip_discovery_info *copy)
{
	uint8_t *addr;
	uint8_t *base;

	addr = base = (uint8_t *)copy->data;

	copy->bhdr = (struct amdgv_binary_header *)addr;

	addr = base + copy->bhdr->table_list[IP_DISCOVERY].offset;
	copy->ihdr = (struct amdgv_ip_discovery_header *)addr;

	addr = base + copy->bhdr->table_list[GC].offset;
	copy->gchdr = (struct amdgv_gpu_info_header *)addr;

	addr = base + copy->bhdr->table_list[HARVEST_INFO].offset;
	copy->htbl = (struct amdgv_harvest_table *)addr;

	addr = base + copy->bhdr->table_list[MALL_INFO].offset;
	copy->mhdr = (struct amdgv_mall_info_header *)addr;

	addr = base + copy->bhdr->table_list[NPS_INFO].offset;
	copy->nhdr = (struct amdgv_nps_info_header *)addr;
}

static int mi300_parse_gc_table(struct amdgv_adapter *adapt)
{
	struct amdgv_ip_discovery_info *pf_copy;
	struct amdgv_gc_info_v2_1 *gc_info;

	pf_copy = &adapt->ip_discovery.pf_copy;

	AMDGV_DEBUG("GC TABLE id: %d\n", pf_copy->gchdr->table_id);
	AMDGV_DEBUG("GC TABLE major: %d\n", pf_copy->gchdr->version_major);
	AMDGV_DEBUG("GC TABLE minor: %d\n", pf_copy->gchdr->version_minor);
	AMDGV_DEBUG("GC TABLE size: %d bytes\n", pf_copy->gchdr->size_bytes);

	if (pf_copy->gchdr->version_major != 2 || pf_copy->gchdr->version_minor != 1) {
		AMDGV_WARN("Unsupported GC version!\n");
		return 0;
	}

	gc_info = GET_GC_TABLE_V2_1(pf_copy->gchdr);

	adapt->config.gfx.max_shader_engines = gc_info->gc_num_se;
	adapt->config.gfx.max_cu_per_sh = gc_info->gc_num_cu_per_sh;
	adapt->config.gfx.max_sh_per_se = gc_info->gc_num_sh_per_se;
	adapt->config.gfx.max_waves_per_simd = gc_info->gc_max_waves_per_simd;
	adapt->config.gfx.wave_size = gc_info->gc_wave_size;

	AMDGV_INFO("%s GFX IP: %d.%d\n", adapt->config.name, adapt->config.gfx.major,
		   adapt->config.gfx.minor);
	AMDGV_INFO("+gc_num_se          : %d\n", gc_info->gc_num_se);
	AMDGV_INFO("+gc_num_cu_per_sh : %d\n", gc_info->gc_num_cu_per_sh);
	AMDGV_INFO("+gc_num_sh_per_se : %d\n", gc_info->gc_num_sh_per_se);

	return 0;
}

static int mi300_parse_nps_table(struct amdgv_adapter *adapt)
{
	struct amdgv_ip_discovery_info *pf_copy;
	struct amdgv_nps_info_v1_0 *nps_info;
	uint32_t i;

	pf_copy = &adapt->ip_discovery.pf_copy;

	AMDGV_DEBUG("NPS TABLE id: %d\n", pf_copy->nhdr->table_id);
	AMDGV_DEBUG("NPS TABLE major: %d\n", pf_copy->nhdr->version_major);
	AMDGV_DEBUG("NPS TABLE minor: %d\n", pf_copy->nhdr->version_minor);
	AMDGV_DEBUG("NPS TABLE size: %d bytes\n", pf_copy->nhdr->size_bytes);

	if (pf_copy->nhdr->version_major != 1 || pf_copy->nhdr->version_minor != 0) {
		AMDGV_WARN("Unsupported NPS version %u.%u!\n",
			pf_copy->nhdr->version_major, pf_copy->nhdr->version_minor);
		return 0;
	}

	nps_info = GET_NPS_INFO_V1_0(pf_copy->nhdr);
	AMDGV_INFO("NPS type : %d\n", nps_info->nps_type);
	AMDGV_INFO("NUMA count : %d\n", nps_info->count);
	if (nps_info->count > AMDGV_MAX_NUMA_NODES) {
		AMDGV_WARN("Invalid NUMA count %u!\n", nps_info->count);
		return 0;
	}
	adapt->mcp.numa_count = nps_info->count;
	for (i = 0; i < adapt->mcp.numa_count; i++) {
		AMDGV_INFO("NUMA%d base=0x%016llx limit=0x%016llx\n",
			i,
			nps_info->instance_info[i].base_address,
			nps_info->instance_info[i].limit_address);
		adapt->mcp.numa_range[i].start = nps_info->instance_info[i].base_address;
		adapt->mcp.numa_range[i].end = nps_info->instance_info[i].limit_address;
	}

	return 0;
}

static int mi300_parse_harvest_table(struct amdgv_adapter *adapt)
{
	uint8_t i;

	for (i = 0; i < HARVEST_TABLE_LEN; i++) {
		if (adapt->ip_discovery.pf_copy.htbl->list[i].hw_id == 0)
			break;

		switch (adapt->ip_discovery.pf_copy.htbl->list[i].hw_id) {
		case GC_HWID:
			adapt->mcp.gfx.num_xcc--;
			adapt->mcp.gfx.xcc_mask &= ~(1U << adapt->ip_discovery.pf_copy.htbl->list[i].number_instance);
			break;
		case SDMA0_HWID:
			adapt->sdma.num_instances--;
			adapt->mcp.gfx.sdma_mask &= ~(1U << adapt->ip_discovery.pf_copy.htbl->list[i].number_instance);
			break;
		case UMC_HWID:
			adapt->umc.num_umc--;
			adapt->umc.active_mask &= ~(1U << adapt->ip_discovery.pf_copy.htbl->list[i].number_instance);
			break;
		case MMHUB_HWID:
			adapt->mmhub.num_instances--;
			adapt->mmhub.active_mask &= ~(1U << adapt->ip_discovery.pf_copy.htbl->list[i].number_instance);
			break;
		default:
			break;
		}
	}

	return 0;
}

static void mi300_hw_ip_count(struct amdgv_adapter *adapt, struct amdgv_ip_v4 *ip)
{
	uint32_t hw_ip;

	for (hw_ip = 0; hw_ip < MAX_HWIP; hw_ip++) {
		if (hw_id_map[hw_ip] == ip->hw_id && hw_id_map[hw_ip] != 0) {
			switch (ip->hw_id) {
			case GC_HWID:
				adapt->config.gfx.major = ip->major;
				adapt->config.gfx.minor = ip->minor;
				adapt->mcp.gfx.num_xcc++;
				adapt->mcp.gfx.xcc_mask |= (1U << ip->instance_number);
				break;
			case SDMA0_HWID:
			case SDMA1_HWID:
			case SDMA2_HWID:
			case SDMA3_HWID:
				adapt->sdma.num_instances++;
				adapt->mcp.gfx.sdma_mask |= (1U << ip->instance_number);
				break;
			case VCE_HWID:
				adapt->config.mm.count[AMDGV_VCE_ENGINE]++;
				break;
			case VCN_HWID:
				adapt->config.mm.count[AMDGV_VCN_ENGINE]++;
				break;
			case UMC_HWID:
				adapt->umc.num_umc++;
				adapt->umc.node_inst_num++;
				adapt->umc.active_mask |= (1ULL << ip->instance_number);
				break;
			case MMHUB_HWID:
				adapt->mmhub.num_instances++;
				adapt->mmhub.active_mask |= (1U << ip->instance_number);
				break;
			default:
				break;
			}
		}
	}
}

static void mi300_hw_ip_baddr(struct amdgv_adapter *adapt, struct amdgv_ip_v4 *ip, struct amdgv_ip_discovery_header *ihdr)
{
	uint32_t hw_ip, ipn;
	int k;

	if (ihdr->base_addr_64_bit) { /* IP List contains 64-bit addresses */
		/* If the data contains 64-bit addresses, convert them in-place
		 * (within the buffer) into 32-bit addresses. This allows
		 * reading adapt->reg_offset[][][x] and getting a (non-zero)
		 * 32-bit address */
		for (k = 0; k < ip->num_base_address; k++) {
			ip->base_address[k] = (uint32_t)ip->base_address_64[k];
		}
	}

	for (hw_ip = 0; hw_ip < MAX_HWIP; hw_ip++) {
		if (hw_id_map[hw_ip] == ip->hw_id && hw_id_map[hw_ip] != 0) {

			adapt->ip_versions[hw_ip][ip->instance_number] =
				IP_VERSION_FULL(ip->major,
						ip->minor,
						ip->revision,
						ip->variant,
						ip->sub_revision);
			ipn = ip->instance_number;
			adapt->reg_offset[hw_ip][ipn] = ip->base_address;
			for (k = 0; k < ip->num_base_address; k++) {
				AMDGV_DEBUG("\t0x%08x\n", ip->base_address[k]);
			}
		}
	}
}

static int mi300_parse_ip_baddr(struct amdgv_adapter *adapt)
{
	struct amdgv_ip_discovery_info *pf_copy;
	struct amdgv_die_header *dhdr;
	int i, j;
	struct amdgv_ip_v4 *ip = NULL;
	uint8_t *start;

	pf_copy = &adapt->ip_discovery.pf_copy;

	start = (uint8_t *)pf_copy->data;

	AMDGV_DEBUG("PF Parsed IPs:\n");
	forEachDie(i, dhdr, pf_copy->ihdr, start) {
		AMDGV_DEBUG("Die: %d ID: 0x%x num_ips: %d\n", i, dhdr->die_id, dhdr->num_ips);
		forEachIPv4(j, ip, dhdr, pf_copy->ihdr) {
			AMDGV_DEBUG("ip %s [hwid=%d, inst=%d, nba=%d]"
				"\tv%d.%d rev(%d)\t@%016x\n",
				hw_id_names[ip->hw_id], ip->hw_id, ip->instance_number, ip->num_base_address,
				ip->major, ip->minor, ip->revision, ip->base_address_64[0]);
			mi300_hw_ip_baddr(adapt, ip, pf_copy->ihdr);
		}
	}
	return 0;
}

static int mi300_parse_ip_discovery(struct amdgv_adapter *adapt)
{
	struct amdgv_ip_discovery_info *pf_copy;
	struct amdgv_die_header *dhdr;
	int i, j;
	struct amdgv_ip_v4 *ip = NULL;
	uint8_t *start;

	pf_copy = &adapt->ip_discovery.pf_copy;

	mi300_ip_discovery_map(adapt, pf_copy);

	if (pf_copy->bhdr->binary_signature != BINARY_SIGNATURE) {
		AMDGV_ERROR("ip_discovery_binary_signature MISMATCH"
			    " expected=0x%08x readback=0x%08x\n",
			    BINARY_SIGNATURE, pf_copy->bhdr->binary_signature);
		return AMDGV_FAILURE;
	}

	if (pf_copy->ihdr->signature != DISCOVERY_TABLE_SIGNATURE) {
		AMDGV_ERROR("ip_discovery_table_signature MISMATCH"
			    " expected=0x%08x readback=0x%08x\n",
			    DISCOVERY_TABLE_SIGNATURE, pf_copy->ihdr->signature);
		return AMDGV_FAILURE;
	}

	if (mi300_ip_discovery_binary_checksum(adapt, pf_copy, VERIFY))
		return AMDGV_FAILURE;

	if (mi300_ip_discovery_table_checksum(adapt, pf_copy, VERIFY))
		return AMDGV_FAILURE;

	/* Parse IP discovery table */

	AMDGV_INFO("IP discovery version: 0x%x\n", pf_copy->ihdr->version);
	AMDGV_INFO("IP discovery table size: %u bytes\n", pf_copy->ihdr->size);
	AMDGV_INFO("IP discovery id: 0x%x\n", pf_copy->ihdr->id);
	AMDGV_INFO("IP discovery num dies: %d\n", pf_copy->ihdr->num_dies);
	if (pf_copy->ihdr->base_addr_64_bit)
		AMDGV_INFO("IP discovery 64bit base address size\n");
	else
		AMDGV_INFO("IP discovery 32bit base address size\n");

	start = (uint8_t *)pf_copy->data;

//	adapt->mcp.num_aid = pf_copy->ihdr->num_dies; //defer until IP Discovery Data provides this information correctly
	switch (adapt->dev_id) {
	case (0x74A1): /* MI300 EP */
	case (0x74A5): /* MI325 */
	case (0x74A2): /* MI308X */
	case (0x74A8): /* MI308X */
	case (0x74A9): /* MI300X HC */
		adapt->mcp.num_aid = 4;
		break;
	default:
		AMDGV_ERROR("not getting proper num_aid setting.\n");
		break;
	}

	/* initialize to zero before they are discovered and harvested */
	adapt->mcp.gfx.xcc_mask = 0;
	adapt->mcp.gfx.num_xcc = 0;
	adapt->mcp.gfx.sdma_mask = 0;
	adapt->sdma.num_instances = 0;
	adapt->umc.num_umc = 0;
	adapt->umc.node_inst_num = 0;
	adapt->umc.active_mask = 0;
	adapt->mmhub.num_instances = 0;
	adapt->mmhub.active_mask = 0;

	/* count IPs (XCCs, SDMAs, VCNs) */
	forEachDie(i, dhdr, pf_copy->ihdr, start) {
		forEachIPv4(j, ip, dhdr, pf_copy->ihdr) {
			mi300_hw_ip_count(adapt, ip);
		}
	}

	return 0;
}

static int mi300_ip_discovery_patch_vf_copy(struct amdgv_adapter *adapt,
					     struct amdgv_ip_discovery_info *pf_copy,
					     struct amdgv_ip_discovery_info *vf_copy)
{
	struct amdgv_die_header *dhdr;
	struct amdgv_die_header *dst_dhdr;
	struct amdgv_ip_v4 *ip = NULL;
	struct amdgv_ip_v4 *ip_first = NULL;
	uint8_t *start;
	uint32_t *src_ptr; /* copy IP List from here */
	uint32_t *dst_ptr; /* trimmed IP List will be copied here */
	int i, j, k, l, m, n, ip_size;
	/* the number of instances for this IP (used for trimming) */
	int num_ip_instances = 0;
	/* remove harvested IPs from VF copy */
	int num_ip_harvested = 0;
	/* total number of all IP instances for this Die (after trimming) */
	int num_ip_total = 0;
	/* number of IPs of this type to copy to VF */
	int num_ip_to_copy = 0;
	uint16_t hw_id_new = 0;
	uint64_t ip_harvest_mask;
	char log_buf[256];
	int log_len;

	if (adapt->mcp.gfx.num_xcc % adapt->num_vf != 0) {
		AMDGV_ERROR("Invalid num_vf=%d for VF IP Discovery Data generation\n",
				adapt->num_vf);
		log_len = oss_vsnprintf(log_buf, sizeof(log_buf),
			"For %u XCCs only num_vf=1", adapt->mcp.gfx.num_xcc);
		for (i = 2; i <= adapt->max_num_vf; i++) {
			if (adapt->mcp.gfx.num_xcc % i == 0)
				log_len += oss_vsnprintf(log_buf + log_len, sizeof(log_buf) - log_len, ", %u", i);
		}
		log_len += oss_vsnprintf(log_buf + log_len, sizeof(log_buf) - log_len, " are valid\n");
		log_buf[log_len] = '\0';
		AMDGV_ERROR("%s", log_buf);
		oss_clear_conf_file(adapt->dev);
		return AMDGV_FAILURE;
	}
	AMDGV_DEBUG("Preparing VF IP Discovery Data based on num_vf=%d\n",
		adapt->num_vf);

	start = (uint8_t *)pf_copy->data;
	forEachDie(i, dhdr, pf_copy->ihdr, start) {
		/* location of num_ips for this Die */
		dst_dhdr = (struct amdgv_die_header *)((uint8_t *)vf_copy->data + vf_copy->ihdr->die_info[i].die_offset);
		/* location of the IP List for this Die */
		dst_ptr = (uint32_t *)(vf_copy->data + (vf_copy->ihdr->die_info[i].die_offset + 4)/4);
		num_ip_instances = 0;
		num_ip_harvested = 0;
		num_ip_total = 0;
		ip_first = (struct amdgv_ip_v4 *)((uint8_t *)(dhdr) + sizeof(struct amdgv_die_header));
		hw_id_new = ip_first->hw_id;
		forEachIPv4(j, ip, dhdr, pf_copy->ihdr) {
			/* check if same IP as before AND not last IP */
			if ((hw_id_new == ip->hw_id) && (j != (dhdr)->num_ips-1)) {
				/* increment number of this IP's instances until a new IP is found */
				num_ip_instances++;
				continue;
			}
			ip_harvest_mask = 0;

			/* new IP found or last IP - now trim the number of instances of this IP to
			 * send to the VF based on spatial paritioning mode (number of VFs) */

			src_ptr = (uint32_t *)ip_first; /* go back to the 1st instance of this IP */

			/* take into account the original Harvest Table to harvest out IPs */
			for (l = 0; l < HARVEST_TABLE_LEN; l++) {
				if (adapt->ip_discovery.pf_copy.htbl->list[l].hw_id == 0)
					break;

				/* if this IP is in the Harevest Table, then the number of instances from
				 * this table need to be trimmed and not sent to the VF */
				if (adapt->ip_discovery.pf_copy.htbl->list[l].hw_id == ip_first->hw_id) {
					AMDGV_DEBUG("Harvesting IP %s hwid=%d inst=%d\n",
						hw_id_names[ip_first->hw_id], ip_first->hw_id,
						adapt->ip_discovery.pf_copy.htbl->list[l].number_instance);
					num_ip_harvested++;
					/* harvested instance should be trimmed from vf ip discovery
					 * save the harvest ips to the ip_harvest_mask */
					ip_harvest_mask |= 1ULL << adapt->ip_discovery.pf_copy.htbl->list[l].number_instance;
				}
			}

			/* copy number of IPs based on number of VFs and harvested IPs */
			if (adapt->num_vf == 8) { /* CPX mode */
				if ((ip_first->hw_id == GC_HWID) ||
					(ip_first->hw_id == SDMA0_HWID) ||
					(ip_first->hw_id == UMC_HWID))
					num_ip_to_copy = (num_ip_instances - num_ip_harvested)/adapt->num_vf;
				else /* use the same number of IPs for CPX mode as QPX mode */
					num_ip_to_copy = MAX((num_ip_instances - num_ip_harvested)/4, 1);
			} else { /* not CPX mode */
				num_ip_to_copy = MAX((num_ip_instances - num_ip_harvested)/adapt->num_vf, 1);
			}

			n = 0;
			for (k = 0; k < num_ip_instances; k++) {
				if (n >= num_ip_to_copy)
					break;
				if (pf_copy->ihdr->base_addr_64_bit) {
					ip_size = 2 + ((*src_ptr & 0xFF000000) >> 24) * 2;
				} else {
					ip_size = 2 + ((*src_ptr & 0xFF000000) >> 24);
				}
				/* if harvested, skip the copy */
				if (ip_harvest_mask & (1ULL << k)) {
					src_ptr += ip_size;
					continue;
				}
				n++;

				for (m = 0; m < ip_size; m++) {
					*dst_ptr = *src_ptr; dst_ptr++; src_ptr++;
				}
			}

			num_ip_total += num_ip_to_copy;

			/* when a new hw_id is found later, keep track of the location of
			 * the first instance so that you can come back to it and
			 * start copying data from there */
			ip_first = ip;
			num_ip_instances = 1;
			num_ip_harvested = 0;
			hw_id_new = ip->hw_id;
		}

		/* update the number of IPs for this Die
		 * based on the total number of trimmed IPs */
		dst_dhdr->num_ips = num_ip_total;
	}

	AMDGV_DEBUG("VF Parsed IPs:\n");
	start = (uint8_t *)vf_copy->data;
	forEachDie(i, dhdr, vf_copy->ihdr, start) {
		AMDGV_DEBUG("Die: %d ID: 0x%x num_ips: %d\n", i, dhdr->die_id, dhdr->num_ips);
		forEachIPv4(j, ip, dhdr, vf_copy->ihdr) {
			AMDGV_DEBUG("ip %s [hwid=%d, inst=%d, nba=%d]"
				"\tv%d.%d rev(%d)\t@%016x\n",
				hw_id_names[ip->hw_id], ip->hw_id, ip->instance_number, ip->num_base_address,
				ip->major, ip->minor, ip->revision, ip->base_address_64[0]);
		}
	}

	return 0;
}

static void mi300_ip_discovery_patch_vf_copy_by_index(struct amdgv_adapter *adapt,
					     struct amdgv_ip_discovery_info *copy, uint32_t idx_vf)
{
	struct amdgv_nps_info_v1_0 *nps_info;
	struct amdgv_vf_device *entry;
	uint64_t vf_fb_offset, vf_fb_size, vf_base, vf_limit, pf_lfb_adjusted;
	int i, j;

	nps_info = (struct amdgv_nps_info_v1_0 *)((uint8_t *)copy->data + copy->bhdr->table_list[NPS_INFO].offset);

	entry = &adapt->array_vf[idx_vf];
	vf_fb_offset = MBYTES_TO_BYTES(entry->fb_offset);
	vf_fb_size = MBYTES_TO_BYTES(entry->real_fb_size);
	pf_lfb_adjusted = (adapt->memmgr_pf).mc_base - adapt->mc_fb_loc_addr;

	AMDGV_DEBUG("version = %d, size = 0x%x, nps_type = %d, count = %d\n",
		nps_info->header.version_major, nps_info->header.size_bytes, nps_info->nps_type, nps_info->count);
	if (adapt->num_vf < nps_info->count) {
		nps_info->count = nps_info->count / adapt->num_vf;

		for (j = 0; j < nps_info->count; j++) {
			i = j + idx_vf * nps_info->count;

			AMDGV_DEBUG("Range %d : [base = 0x%llx - limit = 0x%llx]\n", i,
				nps_info->instance_info[i].base_address, nps_info->instance_info[i].limit_address);
			AMDGV_DEBUG("PF global base = 0x%llx | PF local base = 0x%llx | VF offset = 0x%llx\n",
					adapt->mc_fb_loc_addr, (adapt->memmgr_pf).mc_base, vf_fb_offset);

			// convert base to VF GPA
			// difference between (base - PF LFB) - VF FB offset
			if (nps_info->instance_info[i].base_address - pf_lfb_adjusted > vf_fb_offset)
				vf_base = nps_info->instance_info[i].base_address - pf_lfb_adjusted - vf_fb_offset;
			else
				vf_base = 0;

			if (nps_info->instance_info[i].limit_address - pf_lfb_adjusted > vf_fb_size + vf_fb_offset)
				vf_limit = vf_fb_size - 1;
			else
				vf_limit = nps_info->instance_info[i].limit_address - pf_lfb_adjusted - vf_fb_offset;

			AMDGV_DEBUG("VF offsets %d : [VF base = 0x%llx - VF limit = 0x%llx]\n", j, vf_base, vf_limit);
			if (adapt->xgmi.phy_nodes_num > 1) {
				vf_base += vf_fb_size * adapt->xgmi.phy_node_id;
				vf_limit += vf_fb_size * adapt->xgmi.phy_node_id;
			}

			AMDGV_INFO("VF range %d : [VF base = 0x%llx - VF limit = 0x%llx]\n", j, vf_base, vf_limit);
			AMDGV_DEBUG("PF range %d size = %lld | VF range %d size = %lld\n", i,
				nps_info->instance_info[i].limit_address - nps_info->instance_info[i].base_address + 1, j, vf_limit - vf_base + 1);
			nps_info->instance_info[j].base_address = vf_base;
			nps_info->instance_info[j].limit_address = vf_limit;

		}
	} else {
		// For multi VF, the FB offset and size should already be adjusted to fit in a single numa range
		if (vf_fb_size > nps_info->instance_info[0].limit_address - nps_info->instance_info[0].base_address) {
			//TODO: compare smallest range after reservations and print some warning
			AMDGV_WARN("%s FB size is greater than Numa size. Performance may be impacted\n", amdgv_idx_to_str(idx_vf));
		}

		nps_info->count = 1;
		vf_base = 0;
		vf_limit = vf_fb_size - 1;

		AMDGV_DEBUG("%s offset : [VF base = 0x%llx - VF limit = 0x%llx]\n", amdgv_idx_to_str(idx_vf), vf_base, vf_limit);
		if (adapt->xgmi.phy_nodes_num > 1) {
			vf_base += vf_fb_size * adapt->xgmi.phy_node_id;
			vf_limit += vf_fb_size * adapt->xgmi.phy_node_id;
		}

		AMDGV_INFO("%s range : [VF base = 0x%llx - VF limit = 0x%llx]\n",  amdgv_idx_to_str(idx_vf), vf_base, vf_limit);
		AMDGV_DEBUG("PF range size = %lld | VF range size = %lld\n",
				nps_info->instance_info[0].limit_address - nps_info->instance_info[0].base_address + 1, vf_limit - vf_base + 1);

		oss_memset(nps_info->instance_info, 0, nps_info->header.size_bytes);
		nps_info->instance_info[0].base_address = vf_base;
		nps_info->instance_info[0].limit_address = vf_limit;
	}
}

static int mi300_prepare_ip_discovery_vf(struct amdgv_adapter *adapt)
{
	struct amdgv_ip_discovery_info *pf_copy;
	struct amdgv_ip_discovery_info *vf_copy;

	pf_copy = &adapt->ip_discovery.pf_copy;
	vf_copy = &adapt->ip_discovery.vf_copy;

	/* From the original PF copy of the IP Discovery Data (which has not
	 * been modified yet), copy the Binary Header, IP Discovery Table
	 * (upto the IP List which will be trimmed), GC and VCN Tables to
	 * the VF copy.  The Harvest Table will already be made zero in
	 * the VF copy due to zalloc */

	/* copy from the Binary Header up to the IP List */
	oss_memcpy((void *)vf_copy->data, (void *)pf_copy->data, IP_LIST_OFFSET);

	mi300_ip_discovery_map(adapt, pf_copy);
	mi300_ip_discovery_map(adapt, vf_copy);

	/* copy the GC Table (and up to the Harvest Table signature) */
	oss_memcpy((uint8_t *)vf_copy->data + vf_copy->bhdr->table_list[GC].offset,
		(uint8_t *)pf_copy->data + pf_copy->bhdr->table_list[GC].offset,
		pf_copy->gchdr->size_bytes + 4);
		/* add 4 to include Havest Table signature only */

	/* copy the VCN Table */
	oss_memcpy((uint8_t *)vf_copy->data + vf_copy->bhdr->table_list[VCN_INFO].offset,
		(uint8_t *)pf_copy->data + pf_copy->bhdr->table_list[VCN_INFO].offset,
		pf_copy->bhdr->table_list[VCN_INFO].size);

	/* copy the MALL Table */
	oss_memcpy((uint8_t *)vf_copy->data + vf_copy->bhdr->table_list[MALL_INFO].offset,
		(uint8_t *)pf_copy->data + pf_copy->bhdr->table_list[MALL_INFO].offset,
		pf_copy->bhdr->table_list[MALL_INFO].size);

	/* copy the NPS Table */
	oss_memcpy((uint8_t *)vf_copy->data + vf_copy->bhdr->table_list[NPS_INFO].offset,
		(uint8_t *)pf_copy->data + pf_copy->bhdr->table_list[NPS_INFO].offset,
		pf_copy->bhdr->table_list[NPS_INFO].size);

	/* patch VF copy according to Spatial Partitioning Mode and
	 * original Harvesting Table contents.  IPs will be trimmed from
	 * the original IP List, and all VFs will get the same data */
	if (mi300_ip_discovery_patch_vf_copy(adapt, pf_copy, vf_copy))
		return AMDGV_FAILURE;

	/* recalulate the checksums after trimming the IP List and
	 * removing the Harvest Table */
	if (mi300_ip_discovery_table_checksum(adapt, vf_copy, UPDATE))
		return AMDGV_FAILURE;

	if (mi300_ip_discovery_binary_checksum(adapt, vf_copy, UPDATE))
		return AMDGV_FAILURE;

	return 0;
}

static int8_t mi300_logical_to_dev_inst(struct amdgv_adapter *adapt,
					enum amdgv_hw_ip_block_type block, int8_t inst)
{
	int8_t dev_inst;

	switch (block) {
	case GC_HWIP:
	case SDMA0_HWIP:
		dev_inst = adapt->ip_map.dev_inst[block][inst];
		break;
	default:
		/* For rest of the IPs, no look up required.
		 * Assume 'logical instance == physical instance' for all configs. */
		dev_inst = inst;
		break;
	}

	AMDGV_DEBUG4("hw_block=%x, inst=0x%x, dev_inst=0x%x", block, inst, dev_inst);
	return dev_inst;
}

static uint32_t mi300_logical_to_dev_mask(struct amdgv_adapter *adev,
					 enum amdgv_hw_ip_block_type block,
					 uint32_t mask)
{
	uint32_t dev_mask = 0;
	int8_t log_inst, dev_inst;

	while (mask) {
		log_inst = amdgv_ffs(mask) - 1;
		dev_inst = mi300_logical_to_dev_inst(adev, block, log_inst);
		dev_mask |= (1 << dev_inst);
		mask &= ~(1 << log_inst);
	}

	return dev_mask;
}

static void mi300_ip_map_init(struct amdgv_adapter *adapt)
{
	int xcc_mask, sdma_mask;
	int i = 0, j = 0;

	xcc_mask = adapt->mcp.gfx.xcc_mask;
	sdma_mask = adapt->mcp.gfx.sdma_mask;

	/* Map GC instances */
	while (xcc_mask) {
		if (xcc_mask & (1 << i)) {
			adapt->ip_map.dev_inst[GC_HWIP][j++] = i;
			xcc_mask &= ~(1 << i);
		}
		i++;
	}
	for (; j < HWIP_MAX_INSTANCE; j++)
		adapt->ip_map.dev_inst[GC_HWIP][j] = -1;

	/* Map SDMA instances */
	i = 0;
	j = 0;
	while (sdma_mask) {
		if (sdma_mask & (1 << i)) {
			adapt->ip_map.dev_inst[SDMA0_HWIP][j++] = i;
			sdma_mask &= ~(1 << i);
		}
		i++;
	}
	for (; j < HWIP_MAX_INSTANCE; j++)
		adapt->ip_map.dev_inst[SDMA0_HWIP][j] = -1;

	adapt->ip_map.logical_to_dev_inst = mi300_logical_to_dev_inst;
	adapt->ip_map.logical_to_dev_mask = mi300_logical_to_dev_mask;
}

static void mi300_get_total_active_cu_count(struct amdgv_adapter *adapt)
{
	int i = 0;

	adapt->config.gfx.active_cu_count = 0;
	for (i = 0; i < adapt->mcp.gfx.num_xcc; i++) {
		adapt->config.gfx.active_cu_count += mi300_gfx_get_xcc_cu_count(adapt, i);
	}
	AMDGV_INFO("+gc_num_active_cu   : %d\n", adapt->config.gfx.active_cu_count);
}

int mi300_discover_ip(struct amdgv_adapter *adapt)
{
	/* save ASIC name and supported flags and restore it */
	char asic_name[AMDGV_SMI_ASIC_NAME];
	uint32_t supported_flags = adapt->config.caps.supported_fields_flags;

	/* skip during live update import, it is already called during sw_init() */
	if (adapt->status == AMDGV_STATUS_SW_INIT && adapt->opt.skip_hw_init)
		return 0;

	oss_memcpy(asic_name, adapt->config.name, AMDGV_SMI_ASIC_NAME);

	/* clear IP discovery parsing on init */
	oss_memset(&adapt->config, 0, sizeof(adapt->config));

	oss_memcpy(adapt->config.name, asic_name, AMDGV_SMI_ASIC_NAME);
	adapt->config.caps.supported_fields_flags = supported_flags;

	/* read the IP Discovery Data from the Frame Buffer */
	if (mi300_read_ip_discovery(adapt, 0, AMDGV_IP_DISCOVERY_SIZE))
		return AMDGV_FAILURE;

	/* count IPs (XCCs, SDMAs, VCNs) and perform checksums */
	if (mi300_parse_ip_discovery(adapt))
		return AMDGV_FAILURE;

	/* harvest IPs (XCCs, SDMAs) */
	if (mi300_parse_harvest_table(adapt))
		return AMDGV_FAILURE;

	/* create VF copy of IP Discovery Data from the original PF Data
	 * (before any base address updates to the PF Data)
	 * All VFs will get the exact same data. */
	if (mi300_prepare_ip_discovery_vf(adapt))
		return AMDGV_FAILURE;

	/* now (after VF has updated its copy of IP Discovery Data from the PF copy),
	 * update the base addresses in the PF copy */
	mi300_parse_ip_baddr(adapt);

	if (mi300_parse_gc_table(adapt))
		return AMDGV_FAILURE;

	if (mi300_parse_nps_table(adapt))
		return AMDGV_FAILURE;

	/* Map the GC and SDMA instances after they have been parsed and harvested from the IP Discovery Data */
	mi300_ip_map_init(adapt);

	mi300_get_total_active_cu_count(adapt);

	AMDGV_INFO("\n[IP Config]\n"
			   "Num AID:   0x%x\n" "Num XCC:   0x%x\n" "Num SDMA:  0x%x\n"
			   "XCC Mask:  0x%x\n" "SDMA Mask: 0x%x\n",
			   adapt->mcp.num_aid, adapt->mcp.gfx.num_xcc, adapt->sdma.num_instances,
			   adapt->mcp.gfx.xcc_mask, adapt->mcp.gfx.sdma_mask);

	return 0;
}

int mi300_discover_ip_nps_table(struct amdgv_adapter *adapt)
{
	struct amdgv_ip_discovery_info *pf_copy;
	struct amdgv_ip_discovery_info *vf_copy;

	pf_copy = &adapt->ip_discovery.pf_copy;
	vf_copy = &adapt->ip_discovery.vf_copy;

	/* read the IP Discovery Data for NPS table from the Frame Buffer */
	if (mi300_read_ip_discovery(adapt,
			pf_copy->bhdr->table_list[NPS_INFO].offset,
			pf_copy->bhdr->table_list[NPS_INFO].size))
		return AMDGV_FAILURE;

	if (mi300_parse_nps_table(adapt))
		return AMDGV_FAILURE;

	/* copy the NPS Table to VF */
	oss_memcpy((uint8_t *)vf_copy->data + vf_copy->bhdr->table_list[NPS_INFO].offset,
		(uint8_t *)pf_copy->data + pf_copy->bhdr->table_list[NPS_INFO].offset,
		pf_copy->bhdr->table_list[NPS_INFO].size);

	return 0;
}

int mi300_copy_ip_data_to_vf(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint64_t offset;
	struct amdgv_vf_device *vf;
	struct amdgv_ip_discovery_info vf_copy = {0};
	int ret;

	/* duplicated common VF copy IP discovery data */
	vf_copy.data = (uint32_t *)oss_alloc_memory(AMDGV_IP_DISCOVERY_SIZE);
	if (vf_copy.data == NULL) {
		amdgv_put_error(
			AMDGV_PF_IDX,
			AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
			AMDGV_IP_DISCOVERY_SIZE
		);
		return AMDGV_FAILURE;
	}
	oss_memcpy(vf_copy.data, adapt->ip_discovery.vf_copy.data, AMDGV_IP_DISCOVERY_SIZE);

	/* update pointers in this duplicated vf_copy */
	mi300_ip_discovery_map(adapt, &vf_copy);

	/* patch VF copy according to individual VF index */
	mi300_ip_discovery_patch_vf_copy_by_index(adapt, &vf_copy, idx_vf);
	mi300_ip_discovery_table_checksum(adapt, &vf_copy, UPDATE);
	mi300_ip_discovery_binary_checksum(adapt, &vf_copy, UPDATE);

	vf = &adapt->array_vf[idx_vf];

	offset = ((uint64_t)vf->real_fb_size << 20) - AMDGV_IP_DISCOVERY_OFFSET;
	ret = amdgv_vfmgr_copy_to_vf_fb(adapt, idx_vf, offset, vf_copy.data, AMDGV_IP_DISCOVERY_SIZE);

	oss_free_memory(vf_copy.data);

	return ret;
}

static void mi300_setup_common_timeout(struct amdgv_adapter *adapt)
{
	if (adapt->flags & AMDGV_FLAG_SIM_MODE) {
		/* PSP */
		AMDGV_TIMEOUT(TIMEOUT_PSP_REG) = 1000 * 1000 * 5 * 60 * 3;
		AMDGV_TIMEOUT(TIMEOUT_PSP_MEM) = 1000 * 1000 * 5 * 60 * 3;
		/* READ VBIOS */
		AMDGV_TIMEOUT(TIMEOUT_READ_VBIOS) = 5 * 1000 * 1000 * 60 * 3;
	} else if (adapt->flags & AMDGV_FLAG_EMU_MODE) {
		/* PSP */
		AMDGV_TIMEOUT(TIMEOUT_PSP_REG) = 1000 * 1000 * 900;
		AMDGV_TIMEOUT(TIMEOUT_PSP_MEM) = 1000 * 1000 * 900;
		/* READ VBIOS */
		AMDGV_TIMEOUT(TIMEOUT_READ_VBIOS) = 9 * 1000 * 1000 * 100;
	} else {
		/* PSP */
		AMDGV_TIMEOUT(TIMEOUT_PSP_REG) = 1000 * 1000;
		AMDGV_TIMEOUT(TIMEOUT_PSP_MEM) = 1000 * 1000 * 2;
		/* READ VBIOS */
		AMDGV_TIMEOUT(TIMEOUT_READ_VBIOS) = 5 * 1000 * 1000;
	}
	/* SMU */
	AMDGV_TIMEOUT(TIMEOUT_SMU_REG) = 200 * 1000;
	AMDGV_TIMEOUT(TIMEOUT_SMU_IND_REG) = 200 * 1000;
	/* RESET */
	AMDGV_TIMEOUT(TIMEOUT_RESET) = 100 * 1000;
	/* STATUS */
	AMDGV_TIMEOUT(TIMEOUT_STATUS_REG) = 50 * 1000;
	AMDGV_TIMEOUT(TIMEOUT_GRBM_STATUS) = 100 * 1000;
	/* COMMAND */
	AMDGV_TIMEOUT(TIMEOUT_CMD_RESP) = 200 * 1000;
	/* CP_DMA */
	AMDGV_TIMEOUT(TIMEOUT_CP_DMA) = 20 * 1000;
	/* MANUAL SWITCH */
	AMDGV_TIMEOUT(TIMEOUT_MANUAL_SWITCH) = 500 * 1000;
	/* AUTO SWITCH (MMSCH) */
	AMDGV_TIMEOUT(TIMEOUT_AUTO_SWITCH_MM) = 50 * 1000;
	/* GUEST IDH */
	AMDGV_TIMEOUT(TIMEOUT_GUEST_IDH_RESP) = 10 * 1000;
	AMDGV_TIMEOUT(TIMEOUT_GUEST_IDH_RESP_GPU_RESET) = 500 * 1000;
	/* PCI PENDING TRANSACTION */
	AMDGV_TIMEOUT(TIMEOUT_PCI_TRANS) = 400;
	/* BACO Hardware recovery */
	AMDGV_TIMEOUT(TIMEOUT_BACO_HW) = 2 * 1000 * 1000;
	/* RAS boot polling timeout */
	AMDGV_TIMEOUT(TIMEOUT_RAS_BOOT_STATUS) = AMDGV_RAS_BOOT_STATUS_POLLING_LIMIT * 1000 * 2;
}

static int mi300_ip_discovery_sw_init(struct amdgv_adapter *adapt)
{
	mi300_setup_common_timeout(adapt);

	adapt->ip_discovery.copy_to_vf = mi300_copy_ip_data_to_vf;
	adapt->ip_discovery.discover_ip = mi300_discover_ip;

	adapt->ip_discovery.pf_copy.data =
		(uint32_t *)oss_zalloc(AMDGV_IP_DISCOVERY_SIZE);
	adapt->ip_discovery.vf_copy.data =
		(uint32_t *)oss_zalloc(AMDGV_IP_DISCOVERY_SIZE);

	if (adapt->ip_discovery.pf_copy.data == NULL ||
	    adapt->ip_discovery.vf_copy.data == NULL)
		return AMDGV_FAILURE;

	if (mi300_discover_ip(adapt))
		return AMDGV_FAILURE;

	return 0;
}

static int mi300_ip_discovery_sw_fini(struct amdgv_adapter *adapt)
{
	if (adapt->ip_discovery.pf_copy.data) {
		oss_free(adapt->ip_discovery.pf_copy.data);
		adapt->ip_discovery.pf_copy.data = NULL;
	}
	if (adapt->ip_discovery.vf_copy.data) {
		oss_free(adapt->ip_discovery.vf_copy.data);
		adapt->ip_discovery.vf_copy.data = NULL;
	}

	return 0;
}

static int mi300_ip_discovery_hw_init(struct amdgv_adapter *adapt)
{
	return 0;
}

static int mi300_ip_discovery_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

const struct amdgv_init_func mi300_ip_discovery_func = {
	.name = "mi300_ip_discovery_func",
	.sw_init = mi300_ip_discovery_sw_init,
	.sw_fini = mi300_ip_discovery_sw_fini,
	.hw_init = mi300_ip_discovery_hw_init,
	.hw_fini = mi300_ip_discovery_hw_fini,
};
