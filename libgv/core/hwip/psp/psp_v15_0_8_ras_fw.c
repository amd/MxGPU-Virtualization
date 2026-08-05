/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv.h"
#include "amdgv_device.h"
#include "amdgv_psp_gfx_if.h"
#include "psp_v15_0_8_ras_fw.h"
#include "psp_v15_0_8_ta.h"
#include "psp_v15_0_8_rl.h"

bool psp_v15_0_8_parse_embedded_container(const uint8_t *base, uint32_t total,
		uint32_t *payload_off, uint32_t *payload_size,
		uint32_t *ucode_version_out)
{
	const struct psp_v15_0_8_common_firmware_header *hdr =
		(const struct psp_v15_0_8_common_firmware_header *)base;

	if (total < (uint32_t)sizeof(*hdr))
		return false;
	if (hdr->size_bytes != total)
		return false;
	if (hdr->header_size_bytes < (uint32_t)sizeof(*hdr) ||
	    hdr->header_size_bytes > 512u)
		return false;
	if (!hdr->ucode_size_bytes ||
	    hdr->ucode_array_offset_bytes > total)
		return false;
	if ((uint64_t)hdr->ucode_array_offset_bytes + hdr->ucode_size_bytes > total)
		return false;

	*payload_off = hdr->ucode_array_offset_bytes;
	*payload_size = hdr->ucode_size_bytes;
	*ucode_version_out = hdr->ucode_version;
	return true;
}

/*
 * Locate the RAS sub-binary inside a packaged TA container.
 *
 * The TA bin packs several tools (XGMI, RAS, HDCP, ...) behind a common
 * firmware header. The actual payload array starts at
 * @ucode_array_offset_bytes; each sub-binary lives at a further @offset_bytes
 * within that array and is @size_bytes long. The shipped 15.0.8 TA uses the
 * v2 packaging: a counted, self-describing descriptor table where the RAS
 * entry is the one whose @fw_type == PSP_V15_0_8_TA_FW_TYPE_RAS.
 *
 * On success fills @ras_off (offset from @base), @ras_size and @ras_ver with
 * the RAS sub-binary location, length and packaged version. All reads are
 * bounds-checked against @total to avoid running off the embedded array.
 */
bool psp_v15_0_8_locate_ras_ta(const uint8_t *base, uint32_t total,
		uint32_t *ras_off, uint32_t *ras_size, uint32_t *ras_ver)
{
	const struct psp_v15_0_8_common_firmware_header *hdr =
		(const struct psp_v15_0_8_common_firmware_header *)base;
	uint32_t array_off;
	uint32_t off = 0;
	uint32_t size = 0;
	uint32_t ver = 0;

	if (total < (uint32_t)sizeof(*hdr))
		return false;

	array_off = hdr->ucode_array_offset_bytes;
	if (array_off > total)
		return false;

	switch (hdr->header_version_major) {
	case 2: {
		const struct psp_v15_0_8_ta_firmware_header_v2_0 *v2 =
			(const struct psp_v15_0_8_ta_firmware_header_v2_0 *)base;
		uint32_t count;
		uint32_t i;
		uint64_t desc_end;
		bool found = false;

		if (total < (uint32_t)sizeof(*v2))
			return false;

		count = v2->ta_fw_bin_count;
		if (!count || count >= PSP_V15_0_8_TA_MAX_BIN_COUNT)
			return false;

		/* Ensure the whole descriptor table is inside the blob. */
		desc_end = (uint64_t)sizeof(*v2) +
			   (uint64_t)count * sizeof(struct psp_v15_0_8_fw_bin_desc);
		if (desc_end > total)
			return false;

		for (i = 0; i < count; i++) {
			if (v2->ta_fw_bin[i].fw_type != PSP_V15_0_8_TA_FW_TYPE_RAS)
				continue;
			off = v2->ta_fw_bin[i].offset_bytes;
			size = v2->ta_fw_bin[i].size_bytes;
			ver = v2->ta_fw_bin[i].fw_version;
			found = true;
			break;
		}

		if (!found)
			return false;
		break;
	}
	default:
		return false;
	}

	if (!size)
		return false;
	if ((uint64_t)array_off + off + size > total)
		return false;

	*ras_off = array_off + off;
	*ras_size = size;
	*ras_ver = ver;
	return true;
}

/*
 * Return the RAS TA firmware image embedded in the driver for PSP 15.0.8 and
 * parse its version information. The generated headers are required for PSP
 * 15.0.8 builds.
 *
 * The TA bin is a packaged (v2) container; the RAS sub-binary is located
 * via psp_v15_0_8_locate_ras_ta and handed to uniras. The packaged descriptor
 * version is authoritative; when it is zero we fall back to the version parsed
 * from the payload image (amdgv_psp_ras_get_ta_version).
 */
int amdgv_psp_v15_0_8_get_ras_ta_fw(struct amdgv_adapter *adapt,
		uint8_t **bin_addr, uint32_t *bin_size,
		uint32_t *fw_version, uint32_t *feature_version)
{
	const uint8_t *base = (const uint8_t *)psp_v15_0_8_ta_bin;
	const uint32_t total = (uint32_t)sizeof(psp_v15_0_8_ta_bin);
	uint32_t ras_off = 0;
	uint32_t ras_size = 0;
	uint32_t ver = 0;
	const uint8_t *img;

	if (!bin_addr || !bin_size)
		return AMDGV_FAILURE;

	if (!psp_v15_0_8_locate_ras_ta(base, total, &ras_off, &ras_size, &ver))
		return AMDGV_FAILURE;

	img = base + ras_off;

	if (!ver)
		amdgv_psp_ras_get_ta_version(adapt, (void *)img, ras_size, &ver);

	*bin_addr = (uint8_t *)img;
	*bin_size = ras_size;

	if (fw_version)
		*fw_version = ver;
	if (feature_version)
		*feature_version = ver;

	return 0;
}

/*
 * Return the RAS RL (register list) firmware embedded in the driver for PSP
 * 15.0.8.
 *
 * The RL image uses a common-firmware container; the register-list payload
 * starts at @ucode_array_offset_bytes and is @ucode_size_bytes long. The
 * container's @ucode_version is reported as both the firmware and feature
 * version.
 */
int amdgv_psp_v15_0_8_get_ras_rl_fw(struct amdgv_adapter *adapt,
		uint8_t **bin_addr, uint32_t *bin_size,
		uint32_t *fw_version, uint32_t *feature_version)
{
	const uint8_t *base = (const uint8_t *)psp_v15_0_8_rl_bin;
	const uint32_t total = (uint32_t)sizeof(psp_v15_0_8_rl_bin);
	uint32_t pay_off;
	uint32_t pay_sz;
	uint32_t c_ucode_ver;

	if (!bin_addr || !bin_size)
		return AMDGV_FAILURE;

	if (!psp_v15_0_8_parse_embedded_container(base, total, &pay_off, &pay_sz,
						 &c_ucode_ver))
		return AMDGV_FAILURE;

	(void)adapt;

	*bin_addr = (uint8_t *)(base + pay_off);
	*bin_size = pay_sz;

	if (fw_version)
		*fw_version = c_ucode_ver;
	if (feature_version)
		*feature_version = c_ucode_ver;

	return 0;
}
