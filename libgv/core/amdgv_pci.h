/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_PCI_H
#define AMDGV_PCI_H

#include "amdgv_oss.h"

/*
 * True only if every device from dev up to the root port has all devcap2_mask
 * bits set in DEVCAP2. Climbs via oss_pci_upstream_bridge. Fails closed: a hop
 * that cannot be resolved or read, one with no DEVCAP2, or a missing bit
 * anywhere makes the path false.
 */
bool amdgv_pci_devcap2_supported_on_path(oss_dev_t dev, uint32_t devcap2_mask);

#endif
