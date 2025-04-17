/*
 * Copyright (c) 2018-2021 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef AMDGV_IP_DISCOVERY_H
#define AMDGV_IP_DISCOVERY_H

#define AMDGV_IP_DISCOVERY_SIZE	  adapt->ip_discovery.size
#define MI300_IP_DISCOVERY_SIZE   0x2800
#define DEFAULT_IP_DISCOVERY_SIZE 2048
#define AMDGV_IP_DISCOVERY_OFFSET 0x10000
#define BINARY_SIGNATURE	  0x28211407
#define DISCOVERY_TABLE_SIGNATURE 0x53445049
#define GPU_INFO_GC_INFO_TABLE_ID 0x00004347
#define MALL_INFO_TABLE_ID	  0x4D414C4C

#define forEachDie(i, dhdr, ihdr, start)                                                      \
	for (i = 0;                                                                           \
	     (i < (ihdr)->num_dies) &&                                                        \
	     ((dhdr = (struct amdgv_die_header *)((start) + (ihdr)->die_info[i].die_offset)) != 0);  \
	     i++)

#define forEachIP(i, ip, dhdr)                                                                \
	for (i = 0;                                                                           \
	     i < (dhdr)->num_ips &&                                                           \
	     (((ip) = (struct amdgv_ip *)(i == 0 ?                                             \
		(uint8_t *)(dhdr) + sizeof(struct amdgv_die_header) :                         \
		(uint8_t *)(ip) + sizeof(struct amdgv_ip) + 4 * ((ip)->num_base_address - 1))) != 0); \
	     i++)

#define forEachIPv4(i, ip, dhdr, ihdr)                                                        \
	for (i = 0;                                                                           \
	     i < (dhdr)->num_ips &&                                                           \
	     (((ip) = (struct amdgv_ip_v4 *)(i == 0 ?                                          \
		(uint8_t *)(dhdr) + sizeof(struct amdgv_die_header) :                         \
		ihdr->base_addr_64_bit == 0 ?                                                 \
		(uint8_t *)(ip) + (sizeof(struct amdgv_ip_v4) - 4) + 4 * ((ip)->num_base_address - 1) : \
		(uint8_t *)(ip) + sizeof(struct amdgv_ip_v4) + 8 * ((ip)->num_base_address - 1))) != 0); \
	     i++)

#define IP_VERSION_FULL(mj, mn, rv, var, srev) \
	(((mj) << 24) | ((mn) << 16) | ((rv) << 8) | ((var) << 4) | (srev))
#define IP_VERSION(mj, mn, rv)		IP_VERSION_FULL(mj, mn, rv, 0, 0)
#define IP_VERSION_MAJ(ver)		((ver) >> 24)
#define IP_VERSION_MIN(ver)		(((ver) >> 16) & 0xFF)
#define IP_VERSION_REV(ver)		(((ver) >> 8) & 0xFF)
#define IP_VERSION_VARIANT(ver)		(((ver) >> 4) & 0xF)
#define IP_VERSION_SUBREV(ver)		((ver) & 0xF)
#define IP_VERSION_MAJ_MIN_REV(ver)	((ver) >> 8)

#define GET_GC_TABLE_V1_0(gchdr) \
	((struct amdgv_gc_info_v1_0 *) gchdr)
#define GET_GC_TABLE_V1_1(gchdr) \
	((struct amdgv_gc_info_v1_1 *) gchdr)
#define GET_GC_TABLE_V1_2(gchdr) \
	((struct amdgv_gc_info_v1_2 *) gchdr)
#define GET_GC_TABLE_V2_1(gchdr) \
	((struct amdgv_gc_info_v2_1 *) gchdr)

#define GET_MALL_INFO_V2_0(mhdr) \
	((struct amdgv_mall_info_v2_0 *) mhdr)

#define GET_NPS_INFO_V1_0(nhdr) \
	((struct amdgv_nps_info_v1_0 *) nhdr)

enum amdgv_checksum_cmd { VERIFY = 0, UPDATE };

enum amdgv_ip_discovery_table {
	IP_DISCOVERY = 0,
	GC,
	HARVEST_INFO,
	VCN_INFO,
	MALL_INFO,
	NPS_INFO,
	TOTAL_TABLES = 6
};

#pragma pack(push, 1)

struct amdgv_gpu_info_header {
	uint32_t table_id;
	uint16_t version_major;
	uint16_t version_minor;
	uint32_t size_bytes;
};

struct amdgv_gc_info_v1_0 {
	struct amdgv_gpu_info_header gpu_info_header;
	uint32_t gc_num_se;
	uint32_t gc_num_wgp0_per_sa;
	uint32_t gc_num_wgp1_per_sa;
	uint32_t gc_num_rb_per_se;
	uint32_t gc_num_gl2c;
	uint32_t gc_num_gprs;
	uint32_t gc_num_max_gs_thds;
	uint32_t gc_gs_table_depth;
	uint32_t gc_gsprim_buff_depth;
	uint32_t gc_parameter_cache_depth;
	uint32_t gc_double_offchip_lds_buffer;
	uint32_t gc_wave_size;
	uint32_t gc_max_waves_per_simd;
	uint32_t gc_max_scratch_slots_per_cu;
	uint32_t gc_lds_size;
	uint32_t gc_num_sc_per_se;
	uint32_t gc_num_sa_per_se;
	uint32_t gc_num_packer_per_sc;
	uint32_t gc_num_gl2a;
};

struct amdgv_gc_info_v1_1 {
    struct amdgv_gpu_info_header gpu_info_header;
    uint32_t gc_num_se;
    uint32_t gc_num_wgp0_per_sa;
    uint32_t gc_num_wgp1_per_sa;
    uint32_t gc_num_rb_per_se;
    uint32_t gc_num_gl2c;
    uint32_t gc_num_gprs;
    uint32_t gc_num_max_gs_thds;
    uint32_t gc_gs_table_depth;
    uint32_t gc_gsprim_buff_depth;
    uint32_t gc_parameter_cache_depth;
    uint32_t gc_double_offchip_lds_buffer;
    uint32_t gc_wave_size;
    uint32_t gc_max_waves_per_simd;
    uint32_t gc_max_scratch_slots_per_cu;
    uint32_t gc_lds_size;
    uint32_t gc_num_sc_per_se;
    uint32_t gc_num_sa_per_se;
    uint32_t gc_num_packer_per_sc;
    uint32_t gc_num_gl2a;
    uint32_t gc_num_tcp_per_sa;
    uint32_t gc_num_sdp_interface;
    uint32_t gc_num_tcps;
};

struct amdgv_gc_info_v1_2 {
    struct amdgv_gpu_info_header gpu_info_header;
    uint32_t gc_num_se;
    uint32_t gc_num_wgp0_per_sa;
    uint32_t gc_num_wgp1_per_sa;
    uint32_t gc_num_rb_per_se;
    uint32_t gc_num_gl2c;
    uint32_t gc_num_gprs;
    uint32_t gc_num_max_gs_thds;
    uint32_t gc_gs_table_depth;
    uint32_t gc_gsprim_buff_depth;
    uint32_t gc_parameter_cache_depth;
    uint32_t gc_double_offchip_lds_buffer;
    uint32_t gc_wave_size;
    uint32_t gc_max_waves_per_simd;
    uint32_t gc_max_scratch_slots_per_cu;
    uint32_t gc_lds_size;
    uint32_t gc_num_sc_per_se;
    uint32_t gc_num_sa_per_se;
    uint32_t gc_num_packer_per_sc;
    uint32_t gc_num_gl2a;
    uint32_t gc_num_tcp_per_sa;
    uint32_t gc_num_sdp_interface;
    uint32_t gc_num_tcps;
    uint32_t gc_num_tcp_per_wpg;
    uint32_t gc_tcp_l1_size;
    uint32_t gc_num_sqc_per_wgp;
    uint32_t gc_l1_instruction_cache_size_per_sqc;
    uint32_t gc_l1_data_cache_size_per_sqc;
    uint32_t gc_gl1c_per_sa;
    uint32_t gc_gl1c_size_per_instance;
    uint32_t gc_gl2c_per_gpu;
};


struct amdgv_gc_info_v2_1 {
    struct amdgv_gpu_info_header gpu_info_header;
    /* carried over from v2.0 */
    uint32_t gc_num_se;
    uint32_t gc_num_cu_per_sh;
    uint32_t gc_num_sh_per_se;
    uint32_t gc_num_rb_per_se;
    uint32_t gc_num_tccs;
    uint32_t gc_num_gprs;
    uint32_t gc_num_max_gs_thds;
    uint32_t gc_gs_table_depth;
    uint32_t gc_gsprim_buff_depth;
    uint32_t gc_parameter_cache_depth;
    uint32_t gc_double_offchip_lds_buffer;
    uint32_t gc_wave_size;
    uint32_t gc_max_waves_per_simd;
    uint32_t gc_max_scratch_slots_per_cu;
    uint32_t gc_lds_size;
    uint32_t gc_num_sc_per_se;
    uint32_t gc_num_packer_per_sc;
    /* new for v2.1 */
    uint32_t gc_num_tcp_per_sh;
    uint32_t gc_tcp_size_per_cu;
    uint32_t gc_num_sdp_interface;
    uint32_t gc_num_cu_per_sqc;
    uint32_t gc_instruction_cache_size_per_sqc;
    uint32_t gc_scalar_data_cache_size_per_sqc;
    uint32_t gc_tcc_size;
};

struct amdgv_mall_info_header {
	uint32_t table_id; /* table ID */
	uint16_t version_major; /* table version */
	uint16_t version_minor; /* table version */
	uint32_t size_bytes; /* size of the entire header+data in bytes */
};

struct amdgv_mall_info_v2_0 {
    struct amdgv_mall_info_header header;
    uint32_t mall_size_per_umc ;
    uint32_t reserved[8];
};

struct amdgv_harvest_info_header {
	uint32_t signature;
	uint32_t version;
};

struct amdgv_harvest_info {
	uint16_t hw_id;
	uint8_t number_instance;
	uint8_t reserved;
};

struct amdgv_harvest_table {
	struct amdgv_harvest_info_header header;
	struct amdgv_harvest_info list[32];
};

struct amdgv_table_info {
	uint16_t offset;
	uint16_t checksum;
	uint16_t size;
	uint16_t padding;
};

struct amdgv_binary_header {
	uint32_t binary_signature;
	uint16_t major;
	uint16_t minor;
	uint16_t binary_checksum;
	uint16_t binary_size;
	struct amdgv_table_info table_list[TOTAL_TABLES];
};

struct amdgv_die_info {
	uint16_t die_id;
	uint16_t die_offset;
};

struct amdgv_ip_discovery_header {
	uint32_t signature;
	uint16_t version;
	uint16_t size;
	uint32_t id;
	uint16_t num_dies;
	struct amdgv_die_info die_info[16];
	union {
		uint16_t padding[1];	/* version <=3 */
		struct {		/* version >=4 */
			uint8_t base_addr_64_bit : 1;
			uint8_t reserved : 7;
			uint8_t reserved2;
		};
	};
};

struct amdgv_ip {
	uint16_t hw_id;
	uint8_t instance_number;
	uint8_t num_base_address;
	uint8_t major;
	uint8_t minor;
	uint8_t revision;
	uint8_t harvest	 : 4;
	uint8_t reserved : 4;
	uint32_t base_address[1];
};

struct amdgv_ip_v4 {
	uint16_t hw_id;
	uint8_t instance_number;
	uint8_t num_base_address;
	uint8_t major;
	uint8_t minor;
	uint8_t revision;
	uint8_t sub_revision : 4; /* HCID Sub-Revision */
	uint8_t variant : 4; /* HW variant */
	union {
		uint32_t base_address[1];
		uint64_t base_address_64[1];
	};
};

struct amdgv_die_header {
	uint16_t die_id;
	uint16_t num_ips;
};

struct amdgv_ip_structure {
	struct amdgv_ip_discovery_header *header;
	struct amdgv_die {
		struct amdgv_die_header *die_header;
		struct amdgv_ip *ip_list;
	} die;
};

#define NPS_INFO_TABLE_MAX_NUM_INSTANCES 12

struct amdgv_nps_info_header {
	uint32_t table_id; /* table ID */
	uint16_t version_major; /* table version */
	uint16_t version_minor; /* table version */
	uint32_t size_bytes; /* size of the entire header+data in bytes = 0x000000D4 (212) */
};

struct amdgv_nps_instance_info_v1_0 {
	uint64_t base_address;
	uint64_t limit_address;
};

struct amdgv_nps_info_v1_0 {
	struct amdgv_nps_info_header header;
	uint32_t nps_type;
	uint32_t count;
	struct amdgv_nps_instance_info_v1_0
		instance_info[NPS_INFO_TABLE_MAX_NUM_INSTANCES];
};
#pragma pack(pop)

struct amdgv_ip_discovery_info {
	uint32_t *data;
	struct amdgv_binary_header *bhdr;
	struct amdgv_ip_discovery_header *ihdr;
	struct amdgv_gpu_info_header *gchdr;
	struct amdgv_harvest_table *htbl;
	struct amdgv_mall_info_header *mhdr;
	struct amdgv_nps_info_header *nhdr;
};

struct amdgv_ip_discovery {
	struct amdgv_ip_discovery_info pf_copy;
	struct amdgv_ip_discovery_info vf_copy;
	int (*copy_to_vf)(struct amdgv_adapter *adapt, uint32_t idx_vf);
	int (*discover_ip)(struct amdgv_adapter *adapt);
	int (*parse_gc_table)(struct amdgv_adapter *adapt);
	uint16_t size;
};

#endif
