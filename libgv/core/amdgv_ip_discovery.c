/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include <amdgv_ip_discovery.h>
#include <amdgv_init_table.h>
#include <amdgv.h>
#include <amdgv_vfmgr.h>
#include "hwip/asic_reg/soc15_hw_ip.h"

#define HARVEST_TABLE_LEN 32 //Check if this is right.
#define MAX(a, b)	((a) > (b) ? (a) : (b))
#define HW_ID_MAX 300
/* Location of first IP offset */
#define IP_LIST_OFFSET(bhdr) \
		(DISCOVERY_DATA_OFFSET(BINARY_HEADER_TABLE_LIST(bhdr), IP_DISCOVERY) + \
		sizeof(struct amdgv_ip_discovery_header) + 0x4)



static const uint32_t this_block = AMDGV_SECURITY_BLOCK;

static int amdgv_init_funcs_table(struct amdgv_adapter *adapt)
{
	uint32_t gc_version = adapt->ip_versions[GC_HWIP][0];

	switch (gc_version) {
		case IP_VERSION(12, 1, 0):
			adapt->init_funcs = gc_12_1_init_table;
			adapt->num_funcs = amdgv_count_array(adapt->init_funcs);

			adapt->miti_table = gc_12_1_mitigation_table;
			adapt->num_miti = amdgv_count_array(adapt->miti_table);

			adapt->live_info_funcs = NULL;

			adapt->enable_mes_kiq = true;

			AMDGV_DEBUG("Init funcs table for GC version: 0x%x\n", gc_version);
			break;
		default:
			AMDGV_ERROR("Unsupported GC version: 0x%x\n", gc_version);
			return AMDGV_FAILURE;
	}
	return 0;
}

static const char *hw_id_names[HW_ID_MAX] = {
	[MP1_HWID]		= "MP1",
	[MP2_HWID]		= "MP2",
	[MP5_HWID]		= "MP5",
	[THM_HWID]		= "THM",
	[SMUIO_HWID]	= "SMUIO",
	[FUSE_HWID]		= "FUSE",
	[CLKA_HWID]		= "CLKA",
	[PWR_HWID]		= "PWR",
	[GC_HWID]		= "GC",
	[UVD_HWID]		= "UVD",
	[AUDIO_AZ_HWID]	= "AUDIO_AZ",
	[ACP_HWID]		= "ACP",
	[DCI_HWID]		= "DCI",
	[DMU_HWID]		= "DMU",
	[DCO_HWID]		= "DCO",
	[DIO_HWID]		= "DIO",
	[XDMA_HWID]		= "XDMA",
	[DCEAZ_HWID]	= "DCEAZ",
	[DAZ_HWID]		= "DAZ",
	[SDPMUX_HWID]	= "SDPMUX",
	[NTB_HWID]		= "NTB",
	[IOHC_HWID]		= "IOHC",
	[L2IMU_HWID]	= "L2IMU",
	[VCE_HWID]		= "VCE",
	[MMHUB_HWID]	= "MMHUB",
	[ATHUB_HWID]	= "ATHUB",
	[DBGU_NBIO_HWID]	= "DBGU_NBIO",
	[DFX_HWID]		= "DFX",
	[DBGU0_HWID]	= "DBGU0",
	[DBGU1_HWID]	= "DBGU1",
	[OSSSYS_HWID]	= "OSSSYS",
	[HDP_HWID]		= "HDP",
	[SDMA0_HWID]	= "SDMA0",
	[SDMA1_HWID]	= "SDMA1",
	[SDMA2_HWID]	= "SDMA2",
	[SDMA3_HWID]	= "SDMA3",
	[LSDMA_HWID]	= "LSDMA",
	[ISP_HWID]		= "ISP",
	[DBGU_IO_HWID]	= "DBGU_IO",
	[DF_HWID]		= "DF",
	[CLKB_HWID]		= "CLKB",
	[FCH_HWID]		= "FCH",
	[DFX_DAP_HWID]	= "DFX_DAP",
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
	[IOAGR_HWID]	= "IOAGR",
	[NBIF_HWID]		= "NBIF",
	[IOAPIC_HWID]		= "IOAPIC",
	[SYSTEMHUB_HWID]	= "SYSTEMHUB",
	[NTBCCP_HWID]		= "NTBCCP",
	[UMC_HWID]		= "UMC",
	[SATA_HWID]		= "SATA",
	[USB_HWID]		= "USB",
	[CCXSEC_HWID]	= "CCXSEC",
	[XGMI_HWID]		= "XGMI",
	[XGBE_HWID]		= "XGBE",
	[MP0_HWID]		= "MP0",
	[VPE_HWID]		= "VPE",
	[LSDMA_HWID]	= "LSDMA",
	[ATU_HWID]		= "ATU",
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
	[GC_HWIP]		= GC_HWID,
	[HDP_HWIP]		= HDP_HWID,
	[SDMA0_HWIP]	= SDMA0_HWID,
	[SDMA1_HWIP]	= SDMA1_HWID,
	[SDMA2_HWIP]	= SDMA2_HWID,
	[SDMA3_HWIP]	= SDMA3_HWID,
	[MMHUB_HWIP]	= MMHUB_HWID,
	[ATHUB_HWIP]	= ATHUB_HWID,
	[NBIO_HWIP]		= NBIF_HWID,
	[MP0_HWIP]		= MP0_HWID,
	[MP1_HWIP]		= MP1_HWID,
	[MP5_HWIP]		= MP5_HWID,
	[VCN_HWIP]		= VCN_HWID,
	[VCE_HWIP]		= VCE_HWID,
	[DF_HWIP]		= DF_HWID,
	[DCE_HWIP]		= DCEAZ_HWID,
	[OSSSYS_HWIP]	= OSSSYS_HWID,
	[SMUIO_HWIP]	= SMUIO_HWID,
	[PWR_HWIP]		= PWR_HWID,
	[NBIF_HWIP]		= NBIF_HWID,
	[THM_HWIP]		= THM_HWID,
	[CLK_HWIP]		= CLKA_HWID,
	[UMC_HWIP]		= UMC_HWID,
	[LSDMA_HWIP]	= LSDMA_HWID,
	[ATU_HWIP]		= ATU_HWID,
};

static void amdgv_ip_discovery_get_table_list(struct amdgv_binary_header *bhdr,
					struct amdgv_ip_table_info **table_list, uint16_t *table_count) {

	if (IP_BINARY_HEADER_MAJOR_VERSION(bhdr) >= 2) {
		*table_list = bhdr->v2.table_list;
		if (table_count)
			*table_count = bhdr->v2.table_count;
	}
	else {
		*table_list = bhdr->v1.table_list;
		if (table_count)
			*table_count = TOTAL_TABLES_V1;
	}
}

static void amdgv_ip_get_harvest_table_list(union amdgv_harvest_table *harvest_table,
									struct amdgv_harvest_info **list, uint32_t *count) {

	switch(harvest_table->v1.header.version) {
		case 2:
			*list = harvest_table->v2.list;
			*count = harvest_table->v2.count;
			break;
		default:
			*list = harvest_table->v1.list;
			*count = HARVEST_TABLE_LEN;
			break;
	}
}

static void amdgv_ip_discovery_map_v1(struct amdgv_adapter *adapt,
	struct amdgv_ip_discovery_info *copy) {
	uint8_t *addr;
	uint8_t *base;

	base = (uint8_t *)copy->data;

	addr = base + copy->bhdr->v1.table_list[IP_DISCOVERY].offset;
	copy->ihdr = (struct amdgv_ip_discovery_header *)addr;
	adapt->ip_discovery.header_index[IP_DISCOVERY] = IP_DISCOVERY;

	addr = base + copy->bhdr->v1.table_list[GC_INFO].offset;
	copy->gchdr = (struct amdgv_gpu_info_header *)addr;
	adapt->ip_discovery.header_index[GC_INFO] = GC_INFO;

	adapt->ip_discovery.header_index[VCN_INFO] = VCN_INFO;

	addr = base + copy->bhdr->v1.table_list[HARVEST_INFO].offset;
	copy->htbl = (union amdgv_harvest_table *)addr;
	adapt->ip_discovery.header_index[HARVEST_INFO] = HARVEST_INFO;

	addr = base + copy->bhdr->v1.table_list[MALL_INFO].offset;
	copy->mhdr = (struct amdgv_mall_info_header *)addr;
	adapt->ip_discovery.header_index[MALL_INFO] = MALL_INFO;

	addr = base + copy->bhdr->v1.table_list[NPS_INFO].offset;
	copy->nhdr = (struct amdgv_nps_info_header *)addr;
	adapt->ip_discovery.header_index[NPS_INFO] = NPS_INFO;
}

static void amdgv_ip_discovery_map_v2(struct amdgv_adapter *adapt,
	struct amdgv_ip_discovery_info *copy)
{
	uint8_t *addr;
	uint8_t *base;
	uint16_t i, table_count;
	struct amdgv_ip_table_info *table_list;

	addr = base = (uint8_t *)copy->data;

	table_list = copy->bhdr->v2.table_list;
	table_count = copy->bhdr->v2.table_count;

	for (i = 0; i < table_count; i++) {

		addr = base + table_list[i].offset;

		switch (table_list[i].table_id) {
			case IP_DISCOVERY:
				adapt->ip_discovery.header_index[IP_DISCOVERY] = i;
				copy->ihdr = (struct amdgv_ip_discovery_header *)addr;
				break;
			case GC_INFO:
				adapt->ip_discovery.header_index[GC_INFO] = i;
				copy->gchdr = (struct amdgv_gpu_info_header *)addr;
				break;
			case VCN_INFO:
				adapt->ip_discovery.header_index[VCN_INFO] = i;
				break;
			case HARVEST_INFO:
				adapt->ip_discovery.header_index[HARVEST_INFO] = i;
				copy->htbl = (union amdgv_harvest_table *)addr;
				break;
			case MALL_INFO:
				adapt->ip_discovery.header_index[MALL_INFO] = i;
				copy->mhdr = (struct amdgv_mall_info_header *)addr;
				break;
			case NPS_INFO:
				adapt->ip_discovery.header_index[NPS_INFO] = i;
				copy->nhdr = (struct amdgv_nps_info_header *)addr;
				break;
			case MEM_RSV_INFO:
				adapt->ip_discovery.header_index[MEM_RSV_INFO] = i;
				copy->mrsvhdr = (struct amdgv_mem_rsv_info_header *)addr;
				break;
			case ATOMBIOS_INFO:
				adapt->ip_discovery.header_index[ATOMBIOS_INFO] = i;
				copy->abhdr = (struct amdgv_atombios_info_header *)addr;
				break;
			default:
				AMDGV_WARN("Unknown IP Discovery Table ID: %d\n", table_list[i].table_id);
				break;
		}
	}
}

static void amdgv_ip_discovery_map(struct amdgv_adapter *adapt,
	struct amdgv_ip_discovery_info *copy)
{

	copy->bhdr = (struct amdgv_binary_header *)copy->data;

	if (IP_BINARY_HEADER_MAJOR_VERSION(copy->bhdr) >= 2)
		amdgv_ip_discovery_map_v2(adapt, copy);
	else
		amdgv_ip_discovery_map_v1(adapt, copy);
}


static uint16_t amdgv_ip_discovery_get_checksum(uint8_t *data, uint32_t size)
{
	uint16_t checksum = 0;
	int i;

	for (i = 0; i < size; i++)
		checksum += data[i];

	return checksum;
}

static int amdgv_ip_discovery_table_checksum(struct amdgv_adapter *adapt,
					     struct amdgv_ip_discovery_info *copy,
					     enum amdgv_checksum_cmd opt)
{
	int i;
	uint8_t *data;
	uint16_t checksum, table_count;
	uint32_t size;
	struct amdgv_ip_table_info *table_list;

	amdgv_ip_discovery_get_table_list(copy->bhdr, &table_list, &table_count);

	for (i = 0; i < table_count; i++) {
		data = (uint8_t *)copy->data + table_list[i].offset;
		size = table_list[i].size;

		checksum = amdgv_ip_discovery_get_checksum(data, size);
		if (opt == UPDATE) {
			table_list[i].checksum = checksum;
		} else if (table_list[i].checksum != checksum) {
			amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_VBIOS_IP_DISCOVERY_TABLE_CHECKSUM_FAIL,
				AMDGV_LOG_DATA_16_16_32(i, checksum, table_list[i].checksum));
			return AMDGV_FAILURE;
		}
	}
	return 0;
}

static int amdgv_ip_discovery_binary_checksum(struct amdgv_adapter *adapt,
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
	checksum = amdgv_ip_discovery_get_checksum((uint8_t *)copy->data + offset,
						   copy->bhdr->binary_size - offset);
	if (opt == UPDATE) {
		copy->bhdr->binary_checksum = checksum;
	} else if (copy->bhdr->binary_checksum != checksum) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_VBIOS_IP_DISCOVERY_BINARY_CHECKSUM_FAIL,
			AMDGV_LOG_DATA_32_32(checksum, copy->bhdr->binary_checksum));
		return AMDGV_FAILURE;
	}
	return 0;
}

static int amdgv_ip_discovery_patch_vf_copy(struct amdgv_adapter *adapt,
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
	uint32_t list_count;
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
	struct amdgv_harvest_info *harvest_list;

	if (adapt->mcp.gfx.num_xcc % adapt->num_vf != 0) {
		AMDGV_DEBUG("Invalid num_vf=%d for VF IP Discovery Data generation\n",
				adapt->num_vf);
		return AMDGV_FAILURE;
	}
	AMDGV_DEBUG("Preparing VF IP Discovery Data based on num_vf=%d\n",
		adapt->num_vf);

	start = (uint8_t *)pf_copy->data;
	amdgv_ip_get_harvest_table_list(adapt->ip_discovery.pf_copy.htbl, &harvest_list, &list_count);
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
		for (j = 0; j <= dhdr->num_ips; j++) {
			/* do not move ip if on last loop because there is no
			 * next ip. still exectute the final loop to copy the
			 * last ip */
			if (j < dhdr->num_ips) {
				ip = (struct amdgv_ip_v4 *)(j == 0 ?
					(uint8_t *)dhdr + sizeof(struct amdgv_die_header) :
					(uint8_t *)ip + sizeof(struct amdgv_ip_v4)
					+ (pf_copy->ihdr->base_addr_64_bit ? 8 : 4)
					* ip->num_base_address);
			}

			if (!ip)
				break;

			/* check if same IP as before AND not last IP */
			if ((hw_id_new == ip->hw_id) && (j != (dhdr)->num_ips)) {
				/* increment number of this IP's instances until a new IP is found */
				num_ip_instances++;
				continue;
			}
			ip_harvest_mask = 0;

			/* new IP found or last IP - now trim the number of instances of this IP to
			 * send to the VF based on spatial paritioning mode (number of VFs) */

			src_ptr = (uint32_t *)ip_first; /* go back to the 1st instance of this IP */

			/* take into account the original Harvest Table to harvest out IPs */
			for (l = 0; l < list_count; l++) {
				if (harvest_list[l].hw_id == 0)
					break;

				/* if this IP is in the Harevest Table, then the number of instances from
				 * this table need to be trimmed and not sent to the VF */
				if (harvest_list[l].hw_id == ip_first->hw_id) {
					AMDGV_DEBUG("Harvesting IP %s hwid=%d inst=%d\n",
						hwid_name(ip_first->hw_id), ip_first->hw_id,
						harvest_list[l].number_instance);
					num_ip_harvested++;
					/* harvested instance should be trimmed from vf ip discovery
					 * save the harvest ips to the ip_harvest_mask */
					ip_harvest_mask |= 1ULL << harvest_list[l].number_instance;
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
			AMDGV_DEBUG("ip %s [hwid=%d, inst=%d, nba=%d]\tv%d.%d rev(%d)",
				    hwid_name(ip->hw_id), ip->hw_id, ip->instance_number,
				    ip->num_base_address, ip->major, ip->minor, ip->revision);
			for (k = 0; k < ip->num_base_address; k++) {
				if (pf_copy->ihdr->base_addr_64_bit)
					AMDGV_DEBUG("\t@%016x", ((uint64_t *)ip->base_address)[k]);
				else
					AMDGV_DEBUG("\t@%08x", ip->base_address[k]);
			}
			AMDGV_DEBUG("\n");
		}
	}

	return 0;
}

static int amdgv_ip_discovery_patch_vf_copy_by_index(struct amdgv_adapter *adapt,
					     struct amdgv_ip_discovery_info *copy, uint32_t idx_vf)
{
	struct amdgv_nps_info_v1_0 *nps_info;
	struct amdgv_vf_device *entry;
	uint64_t vf_fb_offset, vf_fb_size, vf_base, vf_limit, pf_lfb_adjusted;
	int i;
	struct amdgv_ip_table_info *table_list;

	amdgv_ip_discovery_get_table_list(copy->bhdr, &table_list, NULL);
	nps_info = (struct amdgv_nps_info_v1_0 *)(DISCOVERY_DATA_PTR(copy->data, table_list, NPS_INFO));
	if (nps_info->header.version_major != 1 || nps_info->header.version_minor != 0) {
		AMDGV_ERROR("Unsupported NPS Info version: %d.%d\n",
			nps_info->header.version_major, nps_info->header.version_minor);
		return AMDGV_FAILURE;
	}

	entry = &adapt->array_vf[idx_vf];
	vf_fb_offset = MBYTES_TO_BYTES(entry->fb_offset);
	vf_fb_size = MBYTES_TO_BYTES(entry->real_fb_size);
	pf_lfb_adjusted = (adapt->memmgr_pf).mc_base - adapt->mc_fb_loc_addr;

	AMDGV_DEBUG("version = %d, size = 0x%x, nps_type = %d, count = %d\n",
		nps_info->header.version_major, nps_info->header.size_bytes, nps_info->nps_type, nps_info->count);
	if (adapt->num_vf == 1) {
		for (i = 0; i < nps_info->count; i++) {
			AMDGV_DEBUG("Range i = %d : [base = 0x%llx - limit = 0x%llx]\n", i,
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

			AMDGV_DEBUG("VF offsets i = %d : [VF base = 0x%llx - VF limit = 0x%llx]\n", i, vf_base, vf_limit);
			if (adapt->xgmi.phy_nodes_num > 1) {
				vf_base += vf_fb_size * adapt->xgmi.phy_node_id;
				vf_limit += vf_fb_size * adapt->xgmi.phy_node_id;
			}

			AMDGV_DEBUG("VF range i = %d : [VF base = 0x%llx - VF limit = 0x%llx]\n",  i, vf_base, vf_limit);
			AMDGV_DEBUG("PF range %d size = %lld | VF range %d size = %lld\n", i,
				nps_info->instance_info[i].limit_address - nps_info->instance_info[i].base_address, i, vf_limit - vf_base);
			nps_info->instance_info[i].base_address = vf_base;
			nps_info->instance_info[i].limit_address = vf_limit;

		}
	} else if (adapt->num_vf >= nps_info->count) {
		// For multi VF, the FB offset and size should already be adjusted to fit in a single numa range
		if (vf_fb_size > nps_info->instance_info[0].limit_address - nps_info->instance_info[0].base_address) {
			//TODO: compare smallest range after reservations and print some warning
			AMDGV_WARN("%s FB size is greater than Numa size. Performance may be impacted\n", amdgv_idx_to_str(idx_vf));
		}

		nps_info->count = 1;
		oss_memset(nps_info->instance_info, 0, nps_info->header.size_bytes);
		vf_base = 0;
		vf_limit = vf_fb_size - 1;

		AMDGV_DEBUG("%s offset : [VF base = 0x%llx - VF limit = 0x%llx]\n", amdgv_idx_to_str(idx_vf), vf_base, vf_limit);
		if (adapt->xgmi.phy_nodes_num > 1) {
			vf_base += vf_fb_size * adapt->xgmi.phy_node_id;
			vf_limit += vf_fb_size * adapt->xgmi.phy_node_id;
		}

		AMDGV_DEBUG("%s range : [VF base = 0x%llx - VF limit = 0x%llx]\n",  amdgv_idx_to_str(idx_vf), vf_base, vf_limit);
		nps_info->instance_info[0].base_address = vf_base;
		nps_info->instance_info[0].limit_address = vf_limit;
	}
	else
	{
		AMDGV_ERROR("Invalid configuration - numvf < nps count");
		return AMDGV_FAILURE;
	}
	return 0;
}

static int amdgv_prepare_ip_discovery_vf(struct amdgv_adapter *adapt)
{
	struct amdgv_ip_discovery_info *pf_copy;
	struct amdgv_ip_discovery_info *vf_copy;
	struct amdgv_ip_table_info *pf_table_list;
	struct amdgv_ip_table_info *vf_table_list;

	pf_copy = &adapt->ip_discovery.pf_copy;
	vf_copy = &adapt->ip_discovery.vf_copy;

	/* From the original PF copy of the IP Discovery Data (which has not
	 * been modified yet), copy the Binary Header, IP Discovery Table
	 * (upto the IP List which will be trimmed), GC and VCN Tables to
	 * the VF copy.  The Harvest Table will already be made zero in
	 * the VF copy due to zalloc */

	/* copy from the Binary Header up to the IP List */
	oss_memcpy((void *)vf_copy->data, (void *)pf_copy->data, IP_LIST_OFFSET(pf_copy->bhdr));

	amdgv_ip_discovery_map(adapt, pf_copy);
	amdgv_ip_discovery_map(adapt, vf_copy);

	amdgv_ip_discovery_get_table_list(pf_copy->bhdr, &pf_table_list, NULL);
	amdgv_ip_discovery_get_table_list(vf_copy->bhdr, &vf_table_list, NULL);

	/* copy the GC Table (and up to the Harvest Table signature) */
	oss_memcpy(DISCOVERY_DATA_PTR(vf_copy->data, vf_table_list, GC_INFO),
		DISCOVERY_DATA_PTR(pf_copy->data, pf_table_list, GC_INFO),
		pf_copy->gchdr->size_bytes);

	/* copy the Harvest table signature only */
	oss_memcpy(DISCOVERY_DATA_PTR(vf_copy->data, vf_table_list, HARVEST_INFO),
		DISCOVERY_DATA_PTR(pf_copy->data, pf_table_list, HARVEST_INFO),
		4);

	/* copy the VCN Table */
	oss_memcpy(DISCOVERY_DATA_PTR(vf_copy->data, vf_table_list, VCN_INFO),
		DISCOVERY_DATA_PTR(pf_copy->data, pf_table_list, VCN_INFO),
		DISCOVERY_DATA_SIZE(pf_table_list, VCN_INFO));

	/* copy the MALL Table */
	oss_memcpy(DISCOVERY_DATA_PTR(vf_copy->data, vf_table_list, MALL_INFO),
		DISCOVERY_DATA_PTR(pf_copy->data, pf_table_list, MALL_INFO),
		DISCOVERY_DATA_SIZE(pf_table_list, MALL_INFO));

	/* copy the NPS Table */
	oss_memcpy(DISCOVERY_DATA_PTR(vf_copy->data, vf_table_list, NPS_INFO),
		DISCOVERY_DATA_PTR(pf_copy->data, pf_table_list, NPS_INFO),
		DISCOVERY_DATA_SIZE(pf_table_list, NPS_INFO));

	/* patch VF copy according to Spatial Partitioning Mode and
	 * original Harvesting Table contents.  IPs will be trimmed from
	 * the original IP List, and all VFs will get the same data */
	if (amdgv_ip_discovery_patch_vf_copy(adapt, pf_copy, vf_copy))
		return AMDGV_FAILURE;

	/* recalulate the checksums after trimming the IP List and
	 * removing the Harvest Table */
	if (amdgv_ip_discovery_table_checksum(adapt, vf_copy, UPDATE))
		return AMDGV_FAILURE;

	if (amdgv_ip_discovery_binary_checksum(adapt, vf_copy, UPDATE))
		return AMDGV_FAILURE;

	return 0;
}

static int amdgv_parse_harvest_table(struct amdgv_adapter *adapt)
{
	uint8_t i;
	uint32_t table_len;
	struct amdgv_harvest_info *list;

	amdgv_ip_get_harvest_table_list(adapt->ip_discovery.pf_copy.htbl, &list, &table_len);

	for (i = 0; i < table_len; i++) {
		if (list[i].hw_id == 0)
			break;

		switch (list[i].hw_id) {
		case GC_HWID:
			adapt->mcp.gfx.num_xcc--;
			adapt->mcp.gfx.xcc_mask &= ~(1U << list[i].number_instance);
			break;
		case SDMA0_HWID:
			adapt->sdma.num_instances--;
			adapt->mcp.gfx.sdma_mask &= ~(1U << list[i].number_instance);
			break;
		case UMC_HWID:
			adapt->umc.num_umc--;
			adapt->umc.active_mask &= ~(1ULL << list[i].number_instance);
			break;
		case MMHUB_HWID:
			adapt->mmhub.num_instances--;
			adapt->mmhub.active_mask &= ~(1U << list[i].number_instance);
			break;
		default:
			break;
		}
	}

	return 0;
}

static void amdgv_hw_ip_count(struct amdgv_adapter *adapt, struct amdgv_ip_v4 *ip)
{
	uint32_t hw_ip;

	for (hw_ip = 0; hw_ip < MAX_HWIP; hw_ip++) {
		if (hw_id_map[hw_ip] == ip->hw_id && hw_id_map[hw_ip] != 0) {
			switch (ip->hw_id) {
			case GC_HWID:
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

static int amdgv_parse_ip_discovery(struct amdgv_adapter *adapt)
{
	struct amdgv_ip_discovery_info *pf_copy;
	struct amdgv_die_header *dhdr;
	int i, j;
	struct amdgv_ip_v4 *ip;
	uint8_t *start;

	pf_copy = &adapt->ip_discovery.pf_copy;

	amdgv_ip_discovery_map(adapt, pf_copy);

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

	if (amdgv_ip_discovery_binary_checksum(adapt, pf_copy, VERIFY)) {
		AMDGV_ERROR("ip_discovery_binary_checksum verification FAILED\n");
		return AMDGV_FAILURE;
	}

	if (amdgv_ip_discovery_table_checksum(adapt, pf_copy, VERIFY))
		return AMDGV_FAILURE;

	/* Parse IP discovery table */

	AMDGV_INFO("IP discovery version: 0x%x\n", pf_copy->ihdr->version);
	AMDGV_DEBUG("IP discovery table size: %u bytes\n", pf_copy->ihdr->size);
	AMDGV_DEBUG("IP discovery id: 0x%x\n", pf_copy->ihdr->id);
	AMDGV_DEBUG("IP discovery num dies: %d\n", pf_copy->ihdr->num_dies);
	if (pf_copy->ihdr->base_addr_64_bit)
		AMDGV_DEBUG("IP discovery 64bit base address size\n");
	else
		AMDGV_DEBUG("IP discovery 32bit base address size\n");

	start = (uint8_t *)pf_copy->data;

	adapt->mcp.num_aid = pf_copy->ihdr->num_dies; //defer until IP Discovery Data provides this information correctly

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
			amdgv_hw_ip_count(adapt, ip);
		}
	}
	return 0;
}

static void amdgv_hw_ip_baddr(struct amdgv_adapter *adapt, struct amdgv_ip_v4 *ip, struct amdgv_ip_discovery_header *ihdr)
{
	uint32_t hw_ip, ipn;
	int k;

	AMDGV_DEBUG("ip->num_base_address: %d\n", ip->num_base_address);
	if (ihdr->base_addr_64_bit) { /* IP List contains 64-bit addresses */
		/* If the data contains 64-bit addresses, convert them in-place
		 * (within the buffer) into 32-bit addresses. This allows
		 * reading adapt->reg_offset[][][x] and getting a (non-zero)
		 * 32-bit address */
		for (k = 0; k < ip->num_base_address; k++) {
			ip->base_address[k] = ((uint64_t *)ip->base_address)[k];
		}
	}

	for (hw_ip = 0; hw_ip < MAX_HWIP; hw_ip++) {
		if (hw_id_map[hw_ip] == ip->hw_id && hw_id_map[hw_ip] != 0) {
			ipn = ip->instance_number;
			if (ipn >= HWIP_MAX_INSTANCE) {
				AMDGV_WARN("IP instance %u for hw_ip %u exceeds HWIP_MAX_INSTANCE (%d), skipping\n",
					   ipn, hw_ip, HWIP_MAX_INSTANCE);
				continue;
			}

			adapt->ip_versions[hw_ip][ipn] =
				IP_VERSION_FULL(ip->major,
						ip->minor,
						ip->revision,
						ip->variant,
						ip->sub_revision);
			adapt->reg_offset[hw_ip][ipn] = ip->base_address;
			adapt->reg_num_segs[hw_ip][ipn] = ip->num_base_address;
			for (k = 0; k < ip->num_base_address; k++) {
				AMDGV_DEBUG("\t0x%08x\n", ip->base_address[k]);
			}
		}
	}
}

static int amdgv_parse_ip_baddr(struct amdgv_adapter *adapt)
{
	struct amdgv_ip_discovery_info *pf_copy;
	struct amdgv_die_header *dhdr;
	int i, j, k;
	struct amdgv_ip_v4 *ip;
	uint8_t *start;

	pf_copy = &adapt->ip_discovery.pf_copy;

	start = (uint8_t *)pf_copy->data;

	AMDGV_DEBUG("PF Parsed IPs:\n");
	forEachDie(i, dhdr, pf_copy->ihdr, start) {
		AMDGV_DEBUG("Die: %d ID: 0x%x num_ips: %d\n", i, dhdr->die_id, dhdr->num_ips);
		forEachIPv4(j, ip, dhdr, pf_copy->ihdr) {
			AMDGV_DEBUG("ip %s [hwid=%d, inst=%d, nba=%d]\tv%d.%d rev(%d)",
				    hwid_name(ip->hw_id), ip->hw_id, ip->instance_number,
				    ip->num_base_address, ip->major, ip->minor, ip->revision);
			for (k = 0; k < ip->num_base_address; k++) {
				if (pf_copy->ihdr->base_addr_64_bit)
					AMDGV_DEBUG("\t@%016x", ((uint64_t *)ip->base_address)[k]);
				else
					AMDGV_DEBUG("\t@%08x", ip->base_address[k]);
			}
			AMDGV_DEBUG("\n");
			amdgv_hw_ip_baddr(adapt, ip, pf_copy->ihdr);
		}
	}
	return 0;
}

static int amdgv_parse_gc_table(struct amdgv_adapter *adapt)
{
	struct amdgv_ip_discovery_info *pf_copy;
	union gc_info *gc_info;
	//uint32_t i;

	pf_copy = &adapt->ip_discovery.pf_copy;

	AMDGV_DEBUG("GC TABLE id: %d\n", pf_copy->gchdr->table_id);
	AMDGV_DEBUG("GC TABLE major: %d\n", pf_copy->gchdr->version_major);
	AMDGV_DEBUG("GC TABLE minor: %d\n", pf_copy->gchdr->version_minor);
	AMDGV_DEBUG("GC TABLE size: %d bytes\n", pf_copy->gchdr->size_bytes);

	gc_info = (union gc_info *)pf_copy->gchdr;

	switch (pf_copy->gchdr->version_major) {
		case 1:
			/* For major version 1, libgv currently only use members from v1_0
			 * that are common across v1_0 to v1_3. If in the future need
			 * to use new members, then add cases for v1_1, v1_2, v1_3 accordingly.
			 */
			adapt->config.gfx.max_shader_engines = gc_info->v1_0.gc_num_se;
			adapt->config.gfx.max_cu_per_sh =
				2 * (gc_info->v1_0.gc_num_wgp0_per_sa + gc_info->v1_0.gc_num_wgp1_per_sa);
			adapt->config.gfx.max_sh_per_se = gc_info->v1_0.gc_num_sa_per_se;
			adapt->config.gfx.max_waves_per_simd = gc_info->v1_0.gc_max_waves_per_simd;
			adapt->config.gfx.wave_size = gc_info->v1_0.gc_wave_size;

			AMDGV_INFO("+gc_num_se          : %d\n", gc_info->v1_0.gc_num_se);
			AMDGV_INFO("+gc_num_wgp0_per_sa : %d\n", gc_info->v1_0.gc_num_wgp0_per_sa);
			AMDGV_INFO("+gc_num_wgp1_per_sa : %d\n", gc_info->v1_0.gc_num_wgp1_per_sa);
			AMDGV_INFO("+gc_num_sa_per_se   : %d\n", gc_info->v1_0.gc_num_sa_per_se);
			break;
		default:
			/* The legacy asics have their own callbacks to parse gc infos.
			 * Currently only v1 is used. Port the v2 parsing if needed in future.
			 */
			AMDGV_ERROR("Unsupported GC version %u.%u!\n",
				pf_copy->gchdr->version_major, pf_copy->gchdr->version_minor);
			return AMDGV_FAILURE;
	}

	adapt->config.gfx.active_cu_count = 0;

	// TODO: check active cu when we have the gc reg def for gc12_1
	// for (i = 0; i < adapt->mcp.gfx.num_xcc; i++) {
	// 	adapt->config.gfx.active_cu_count += gfx_v12_1_get_xcc_cu_count(adapt, i);
	// }
	adapt->config.gfx.active_cu_count = adapt->mcp.gfx.num_xcc;

	AMDGV_INFO("+gc_num_active_cu   : %d\n", adapt->config.gfx.active_cu_count);

	return 0;
}

static int amdgv_parse_nps_table(struct amdgv_adapter *adapt)
{
	struct amdgv_ip_discovery_info *pf_copy;
	struct amdgv_nps_info_v1_0 *nps_info;
	uint32_t i;

	pf_copy = &adapt->ip_discovery.pf_copy;

	AMDGV_INFO("NPS TABLE id: %d\n", pf_copy->nhdr->table_id);
	AMDGV_INFO("NPS TABLE major: %d\n", pf_copy->nhdr->version_major);
	AMDGV_INFO("NPS TABLE minor: %d\n", pf_copy->nhdr->version_minor);
	AMDGV_DEBUG("NPS TABLE size: %d bytes\n", pf_copy->nhdr->size_bytes);

	if (pf_copy->nhdr->version_major != 1 || pf_copy->nhdr->version_minor != 0) {
		AMDGV_WARN("Unsupported NPS version %u.%u!\n",
			pf_copy->nhdr->version_major, pf_copy->nhdr->version_minor);
		return 0;
	}

	nps_info = GET_NPS_INFO_V1_0(pf_copy->nhdr);
	AMDGV_DEBUG("NPS type : %d\n", nps_info->nps_type);
	AMDGV_DEBUG("NUMA count : %d\n", nps_info->count);
	if (nps_info->count > AMDGV_MAX_NUMA_NODES) {
		AMDGV_WARN("Invalid NUMA count %u!\n", nps_info->count);
		return 0;
	}
	adapt->mcp.numa_count = nps_info->count;
	for (i = 0; i < adapt->mcp.numa_count; i++) {
		AMDGV_DEBUG("NUMA%d base=0x%016llx limit=0x%016llx\n",
			i,
			nps_info->instance_info[i].base_address,
			nps_info->instance_info[i].limit_address);
		adapt->mcp.numa_range[i].start = nps_info->instance_info[i].base_address;
		adapt->mcp.numa_range[i].end = nps_info->instance_info[i].limit_address;
	}

	return 0;
}

static int amdgv_parse_mem_rsv_table(struct amdgv_adapter *adapt)
{
	struct amdgv_ip_discovery_info *pf_copy;
	struct amdgv_mem_rsv_info_v1_0 *mem_rsv_info;
	uint32_t i, region_id;

	pf_copy = &adapt->ip_discovery.pf_copy;

	if (!pf_copy->mrsvhdr) {
		AMDGV_DEBUG("MEM_RSV TABLE not present in IP discovery\n");
		return 0;
	}

	AMDGV_INFO("MEM_RSV TABLE id: %d\n", pf_copy->mrsvhdr->table_id);
	AMDGV_INFO("MEM_RSV TABLE major: %d\n", pf_copy->mrsvhdr->version_major);
	AMDGV_INFO("MEM_RSV TABLE minor: %d\n", pf_copy->mrsvhdr->version_minor);
	AMDGV_DEBUG("MEM_RSV TABLE size: %d bytes\n", pf_copy->mrsvhdr->size_bytes);

	if (pf_copy->mrsvhdr->version_major != 1 || pf_copy->mrsvhdr->version_minor != 0) {
		AMDGV_WARN("Unsupported MEM_RSV version %u.%u!\n",
			pf_copy->mrsvhdr->version_major, pf_copy->mrsvhdr->version_minor);
		return 0;
	}

	mem_rsv_info = GET_MEM_RSV_INFO_V1_0(pf_copy->mrsvhdr);
	adapt->mem_rsv_info.count = mem_rsv_info->mem_rsv_count;

	for (i = 0; i < adapt->mem_rsv_info.count; i++) {
		region_id = mem_rsv_info->instance_info[i].region_id;

		AMDGV_DEBUG("id=%u size=0x%016llx start_addr=0x%016llx\n",
			region_id,
			mem_rsv_info->instance_info[i].region_size,
			mem_rsv_info->instance_info[i].region_start_addr);

		/* Store region info in entries array indexed by region_id */
		adapt->mem_rsv_info.entries[region_id].size = mem_rsv_info->instance_info[i].region_size;
		adapt->mem_rsv_info.entries[region_id].start_addr = mem_rsv_info->instance_info[i].region_start_addr;
	}

	return 0;
}

static int amdgv_parse_atombios_table(struct amdgv_adapter *adapt)
{
	struct amdgv_ip_discovery_info *pf_copy;
	struct amdgv_atombios_info *atombios_info;
	uint8_t *vbios_image;

	pf_copy = &adapt->ip_discovery.pf_copy;

	/* Old IP discovery binary doesn't have ATOMBIOS table */
	if (!pf_copy->abhdr) {
		AMDGV_DEBUG("ATOMBIOS_INFO TABLE not present in IP discovery\n");
		return 0;
	}

	AMDGV_INFO("ATOMBIOS_INFO TABLE id: 0x%x\n", pf_copy->abhdr->table_id);
	AMDGV_INFO("ATOMBIOS_INFO TABLE version: 0x%x\n", pf_copy->abhdr->version);
	AMDGV_DEBUG("ATOMBIOS_INFO TABLE size: %d bytes\n", pf_copy->abhdr->size_bytes);

	if (pf_copy->abhdr->version != 1) {
		AMDGV_WARN("Unsupported ATOMBIOS version %u!\n", pf_copy->abhdr->version);
		return 0;
	}

	atombios_info = (struct amdgv_atombios_info *)pf_copy->abhdr;

	/* Calculate VBIOS image address from IP discovery base + dest_offset */
	vbios_image = (uint8_t *)pf_copy->data + atombios_info->dest_offset;

	/* Store VBIOS info for later use */
	adapt->vbios.ip_discovery_image = vbios_image;
	adapt->vbios.ip_discovery_image_size = atombios_info->data_size;

	AMDGV_DEBUG("ATOMBIOS: VBIOS image found at offset 0x%x, size 0x%x\n",
		   atombios_info->dest_offset, atombios_info->data_size);

	return 0;
}

static int8_t amdgv_logical_to_dev_inst(struct amdgv_adapter *adapt,
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

static uint32_t amdgv_logical_to_dev_mask(struct amdgv_adapter *adapt,
					 enum amdgv_hw_ip_block_type block,
					 uint32_t mask)
{
	uint32_t dev_mask = 0;
	int8_t log_inst, dev_inst;

	while (mask) {
		log_inst = amdgv_ffs(mask) - 1;
		dev_inst = amdgv_logical_to_dev_inst(adapt, block, log_inst);
		dev_mask |= (1 << dev_inst);
		mask &= ~(1 << log_inst);
	}

	return dev_mask;
}

static void amdgv_ip_map_init(struct amdgv_adapter *adapt)
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

	adapt->ip_map.logical_to_dev_inst = amdgv_logical_to_dev_inst;
	adapt->ip_map.logical_to_dev_mask = amdgv_logical_to_dev_mask;
}

static int amdgv_ip_discovery_read_from_mem(struct amdgv_adapter *adapt)
{
	unsigned int i;
	uint32_t rcc_config;
	uint32_t size;
	uint64_t fb_offset;

	AMDGV_DEBUG("Reading IP Discovery Data from Memory...\n");

	rcc_config = RREG32(regRCC_CONFIG_RESERVED);
	if (rcc_config >> 24) {

		fb_offset = RREG32(regDRIVER_SCRATCH_0);
		fb_offset |= ((uint64_t)RREG32(regDRIVER_SCRATCH_1) << 32);
		size = RREG32(regDRIVER_SCRATCH_2);
	} else {
		// read from FB TOP - 64KB
		fb_offset = RREG32(regRCC_CONFIG_MEMSIZE) << 20;
		fb_offset -= AMDGV_IP_DISCOVERY_OFFSET;
		size = AMDGV_IP_DISCOVERY_SIZE;
	}

	if (adapt->mapped_fb_size >= fb_offset + size)
		oss_memcpy(adapt->ip_discovery.pf_copy.data, (uint8_t *)adapt->fb + fb_offset, size);
	else {
		for (i = 0; i < size >> 2; i++, fb_offset += 4)
			adapt->ip_discovery.pf_copy.data[i] = READ_FB32(fb_offset);
	}

	return 0;
}

static int amdgv_ip_discovery_read_from_psp(struct amdgv_adapter *adapt)
{
	AMDGV_DEBUG("Reading IP Discovery Data from PSP...\n");
	return amdgv_psp_read_ip_discovery(adapt,
			adapt->ip_discovery.ip_discovery_mem);
}

int amdgv_discover_ip(struct amdgv_adapter *adapt)
{
	/* save ASIC name and supported flags and restore it */
	char asic_name[AMDGV_SMI_ASIC_NAME];
	uint32_t supported_flags = adapt->config.caps.supported_fields_flags;

	oss_memcpy(asic_name, adapt->config.name, AMDGV_SMI_ASIC_NAME);

	/* clear IP discovery parsing on init */
	oss_memset(&adapt->config, 0, sizeof(adapt->config));

	oss_memcpy(adapt->config.name, asic_name, AMDGV_SMI_ASIC_NAME);
	adapt->config.caps.supported_fields_flags = supported_flags;

	if (amdgv_ip_discovery_read_from_psp(adapt)) {
		AMDGV_WARN("Failed to read IP Discovery Data from PSP, fallback to read from memory\n");
		amdgv_ip_discovery_read_from_mem(adapt);
	}

	/* count IPs (XCCs, SDMAs, VCNs) and perform checksums */
	if (amdgv_parse_ip_discovery(adapt))
		return AMDGV_FAILURE;

	/* Capture the full (pre-harvest) XCC count before harvesting removes XCCs.
	 */
	adapt->mcp.gfx.max_xcc = adapt->mcp.gfx.num_xcc;

	/* harvest IPs (XCCs, SDMAs) */
	if (amdgv_parse_harvest_table(adapt))
		return AMDGV_FAILURE;


	/* create VF copy of IP Discovery Data from the original PF Data
	 * (before any base address updates to the PF Data)
	 * All VFs will get the exact same data. */
	if (amdgv_prepare_ip_discovery_vf(adapt))
		return AMDGV_FAILURE;

	/* now (after VF has updated its copy of IP Discovery Data from the PF copy),
	 * update the base addresses in the PF copy */
	amdgv_parse_ip_baddr(adapt);

	if (amdgv_parse_gc_table(adapt))
		return AMDGV_FAILURE;
	if (amdgv_init_funcs_table(adapt))
		return AMDGV_FAILURE;
	if (amdgv_parse_nps_table(adapt))
		return AMDGV_FAILURE;
	if (amdgv_parse_mem_rsv_table(adapt))
		return AMDGV_FAILURE;
	if (amdgv_parse_atombios_table(adapt))
		return AMDGV_FAILURE;

	/* Map the GC and SDMA instances after they have been parsed and harvested from the IP Discovery Data */
	amdgv_ip_map_init(adapt);

	AMDGV_INFO("\n[IP Config]\n"
			   "Num AID:   0x%x\n" "Num XCC:   0x%x\n" "Num SDMA:  0x%x\n" "Num UMC:   0x%x\n"
			   "XCC Mask:  0x%x\n" "SDMA Mask: 0x%x\n" "UMC Mask: 0x%x\n",
			   adapt->mcp.num_aid, adapt->mcp.gfx.num_xcc, adapt->sdma.num_instances, adapt->umc.num_umc,
			   adapt->mcp.gfx.xcc_mask, adapt->mcp.gfx.sdma_mask, adapt->umc.active_mask);

	return 0;
}

int amdgv_copy_ip_data_to_vf(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
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
	amdgv_ip_discovery_map(adapt, &vf_copy);

	/* patch VF copy according to individual VF index */
	amdgv_ip_discovery_patch_vf_copy_by_index(adapt, &vf_copy, idx_vf);
	amdgv_ip_discovery_table_checksum(adapt, &vf_copy, UPDATE);
	amdgv_ip_discovery_binary_checksum(adapt, &vf_copy, UPDATE);

	vf = &adapt->array_vf[idx_vf];

	ret = amdgv_vfmgr_copy_to_vf_xchg_table(adapt, idx_vf, AMD_SRIOV_MSG_IPD_TABLE_ID, 0,
						vf_copy.data, AMDGV_IP_DISCOVERY_SIZE);

	oss_free_memory(vf_copy.data);

	return ret;
}

static void amdgv_setup_common_timeout(struct amdgv_adapter *adapt)
{
	/* PSP */
	AMDGV_TIMEOUT(TIMEOUT_PSP_REG) = 1000 * 1000;
	AMDGV_TIMEOUT(TIMEOUT_PSP_MEM) = 1000 * 1000 * 2;
	/* READ VBIOS */
	AMDGV_TIMEOUT(TIMEOUT_READ_VBIOS) = 5 * 1000 * 1000;
	/* STATUS */
	AMDGV_TIMEOUT(TIMEOUT_STATUS_REG) = 50 * 1000;
	AMDGV_TIMEOUT(TIMEOUT_GRBM_STATUS) = 100 * 1000;
	/* SMU */
	AMDGV_TIMEOUT(TIMEOUT_SMU_REG) = 500 * 1000;

	AMDGV_TIMEOUT(TIMEOUT_SMU_IND_REG) = 200 * 1000;
	/* RESET */
	AMDGV_TIMEOUT(TIMEOUT_RESET) = 100 * 1000;
	/* COMMAND */
	AMDGV_TIMEOUT(TIMEOUT_CMD_RESP) = 200 * 1000;
	/* CP_DMA */
	AMDGV_TIMEOUT(TIMEOUT_CP_DMA) = 20 * 1000;
	/* LSDMA_PIO */
	AMDGV_TIMEOUT(TIMEOUT_LSDMA) = 20 * 1000;
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

int amdgv_ip_discovery_init(struct amdgv_adapter *adapt)
{

	adapt->ip_discovery.copy_to_vf = amdgv_copy_ip_data_to_vf;

	adapt->ip_discovery.pf_copy.data =
		(uint32_t *)oss_zalloc(AMDGV_IP_DISCOVERY_SIZE);
	adapt->ip_discovery.vf_copy.data =
		(uint32_t *)oss_zalloc(AMDGV_IP_DISCOVERY_SIZE);

	amdgv_setup_common_timeout(adapt);

	if (adapt->ip_discovery.pf_copy.data == NULL ||
	    adapt->ip_discovery.vf_copy.data == NULL)
		goto fail;

	if (amdgv_memmgr_pf_init(adapt))
		goto fail;

	adapt->ip_discovery.ip_discovery_mem =
			amdgv_memmgr_alloc_align(&adapt->memmgr_pf,
					AMDGV_IP_DISCOVERY_SIZE,
					KBYTES_TO_BYTES(16), MEM_IP_DISCOVERY);

	if (!adapt->ip_discovery.ip_discovery_mem)
		goto fail;

	if (adapt->asic_type == CHIP_IP_DISCOVERY) {
		if (amdgv_discover_ip(adapt))
			goto fail;
	}

	return 0;

fail:
	amdgv_ip_discovery_fini(adapt);
	return AMDGV_FAILURE;
}

int amdgv_ip_discovery_fini(struct amdgv_adapter *adapt)
{
	if (adapt->ip_discovery.pf_copy.data) {
		oss_free(adapt->ip_discovery.pf_copy.data);
		adapt->ip_discovery.pf_copy.data = NULL;
	}
	if (adapt->ip_discovery.vf_copy.data) {
		oss_free(adapt->ip_discovery.vf_copy.data);
		adapt->ip_discovery.vf_copy.data = NULL;
	}
	if (adapt->ip_discovery.ip_discovery_mem) {
		amdgv_memmgr_free(adapt->ip_discovery.ip_discovery_mem);
		adapt->ip_discovery.ip_discovery_mem = NULL;
	}

	amdgv_memmgr_pf_fini(adapt);
	return 0;
}
