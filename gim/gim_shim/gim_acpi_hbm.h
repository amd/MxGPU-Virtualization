/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef GIM_ACPI_HBM_H
#define GIM_ACPI_HBM_H

#include <linux/types.h>

struct pci_dev;

/*
 * Match the PF BDF to an SRAT generic initiator and, when HMAT identifies
 * one, fill the GPU HBM system physical range.
 * Returns 0 when the PF proximity domain is found. *hbm_size is 0 when no
 * HBM range was found. Returns -ENOENT when the PF BDF is not in SRAT.
 */
int gim_acpi_discover_hbm(struct pci_dev *pdev, int *pf_pd,
			  uint64_t *hbm_pa, uint64_t *hbm_size);

/*
 * Find the SRAT generic-initiator proximity domain for one PCI handle.
 * The handle keeps the PF segment and bus. Its devfn is the PF devfn plus
 * the VF index plus one, not the SR-IOV routing ID.
 * Returns 0 when the handle is found. Returns -ENOENT otherwise.
 */
int gim_acpi_lookup_pd(uint32_t seg, uint8_t bus, uint8_t devfn, int *pd);

#endif
