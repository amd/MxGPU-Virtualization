/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _GIM_BACO_POWER_SAVING_H
#define _GIM_BACO_POWER_SAVING_H

#include <linux/pci.h>
#include <linux/kernel.h>

int gim_enter_baco(amdgv_dev_t dev);
int gim_exit_baco(amdgv_dev_t dev);
int gim_get_baco_status(amdgv_dev_t dev, uint32_t *status);
int gim_get_power_saving_mode(void);

#endif
