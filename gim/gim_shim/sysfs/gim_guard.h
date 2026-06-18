/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef GIM_GUARD_H
#define GIM_GUARD_H
#include "gim.h"

int gim_guard_init_dev_sys(struct pci_dev *pdev);

int gim_guard_remove_dev_sys(struct pci_dev *pdev);


int gim_guard_init_drv_sys(struct device_driver *drv);
int gim_guard_remove_drv_sys(struct device_driver *drv);
#endif
