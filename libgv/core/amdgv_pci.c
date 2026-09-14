/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>

#include "amdgv_oss_wrapper.h"
#include "amdgv_pci_def.h"
#include "amdgv_pci.h"

/* Upper bound on an upstream walk: the whole PCI bus-number space */
#define AMDGV_PCIE_MAX_PATH_DEPTH 256

bool amdgv_pci_devcap2_supported_on_path(oss_dev_t dev, uint32_t devcap2_mask)
{
	oss_dev_t cur = dev;
	int hop;

	for (hop = 0; hop < AMDGV_PCIE_MAX_PATH_DEPTH; ++hop) {
		uint16_t flags = 0;
		uint32_t devcap2 = 0;
		int pos;

		pos = oss_pci_find_capability(cur, PCI_CAP_ID__PCIE);
		if (!pos)
			return false;

		if (oss_pci_read_config_word(cur, pos + PCIE_CAP_FLAGS, &flags))
			return false;

		if ((flags & PCIE_CAP_FLAGS__VERSION) < PCIE_CAP_VERSION_MIN_DEVCAP2)
			return false;

		if (oss_pci_read_config_dword(cur, pos + PCIE_DEVICE_CAP2, &devcap2))
			return false;

		if ((devcap2 & devcap2_mask) != devcap2_mask)
			return false;

		if (((flags & PCIE_CAP_FLAGS__DEV_TYPE) >>
		     PCIE_CAP_FLAGS__DEV_TYPE_SHIFT) == PCIE_DEV_TYPE__ROOT_PORT)
			return true;

		cur = oss_pci_upstream_bridge(cur);
		if (cur == NULL)
			return false;
	}

	return false;
}
