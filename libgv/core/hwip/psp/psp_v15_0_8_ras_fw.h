/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PSP_V15_0_8_RAS_FW_H
#define PSP_V15_0_8_RAS_FW_H

#include "amdgv_basetypes.h"

struct amdgv_adapter;

/* Standard AMD firmware container header that prefixes RL/TA images shipped
 * as common-firmware blobs. The payload the PSP expects starts at
 * @ucode_array_offset_bytes and is @ucode_size_bytes long.
 */
struct psp_v15_0_8_common_firmware_header {
	uint32_t size_bytes;
	uint32_t header_size_bytes;
	uint16_t header_version_major;
	uint16_t header_version_minor;
	uint16_t ip_version_major;
	uint16_t ip_version_minor;
	uint32_t ucode_version;
	uint32_t ucode_size_bytes;
	uint32_t ucode_array_offset_bytes;
	uint32_t crc32;
};

/* Self-describing (v2) sub-binary descriptor: carries its own @fw_type. */
struct psp_v15_0_8_fw_bin_desc {
	uint32_t fw_type;
	uint32_t fw_version;
	uint32_t offset_bytes;
	uint32_t size_bytes;
};

/* v2 packaged TA: common header, a descriptor count, then a self-describing
 * descriptor table.
 */
struct psp_v15_0_8_ta_firmware_header_v2_0 {
	struct psp_v15_0_8_common_firmware_header header;
	uint32_t ta_fw_bin_count;
	struct psp_v15_0_8_fw_bin_desc ta_fw_bin[];
};

/* TA sub-binary type for RAS (matches amdgpu's TA_FW_TYPE_PSP_RAS). */
#define PSP_V15_0_8_TA_FW_TYPE_RAS 3

/* Fixed firmware-header buffer size amdgpu parses into (union
 * amdgpu_firmware_header is 0x100 bytes via its raw[] member).
 */
#define PSP_V15_0_8_FW_HEADER_BYTES 0x100u

/* Bound on the v2 descriptor count, mirroring amdgpu's UCODE_MAX_PSP_PACKAGING:
 * the number of descriptors that fit in the fixed header buffer (after the
 * common header and the ta_fw_bin_count field), doubled.
 */
#define PSP_V15_0_8_TA_MAX_BIN_COUNT \
	(((PSP_V15_0_8_FW_HEADER_BYTES - \
	   (uint32_t)sizeof(struct psp_v15_0_8_common_firmware_header) - 4u) / \
	  (uint32_t)sizeof(struct psp_v15_0_8_fw_bin_desc)) * 2u)

/*
 * Parse a flat common-firmware container (used by the RL image). On success
 * returns true and fills @payload_off / @payload_size with the embedded
 * payload location and length, and @ucode_version_out with the container's
 * ucode version. All reads are bounds-checked against @total.
 *
 * Exposed (non-static) so PhantomGV can exercise the malformed-container
 * paths the shipped blobs never reach; not part of the public PSP API.
 */
bool psp_v15_0_8_parse_embedded_container(const uint8_t *base, uint32_t total,
		uint32_t *payload_off, uint32_t *payload_size,
		uint32_t *ucode_version_out);

/*
 * Locate the RAS sub-binary inside a packaged TA container (v2 typed
 * descriptor table). On success returns true and fills @ras_off (offset from
 * @base), @ras_size and @ras_ver. All reads are bounds-checked against @total.
 *
 * Exposed (non-static) for unit testing; not part of the public PSP API.
 */
bool psp_v15_0_8_locate_ras_ta(const uint8_t *base, uint32_t total,
		uint32_t *ras_off, uint32_t *ras_size, uint32_t *ras_ver);

int amdgv_psp_v15_0_8_get_ras_ta_fw(struct amdgv_adapter *adapt,
		uint8_t **bin_addr, uint32_t *bin_size,
		uint32_t *fw_version, uint32_t *feature_version);

int amdgv_psp_v15_0_8_get_ras_rl_fw(struct amdgv_adapter *adapt,
		uint8_t **bin_addr, uint32_t *bin_size,
		uint32_t *fw_version, uint32_t *feature_version);

#endif /* PSP_V15_0_8_RAS_FW_H */
