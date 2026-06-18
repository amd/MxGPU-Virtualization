/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef GIM_MONITOR_H
#define GIM_MONITOR_H

#include "gim.h"

int gim_mon_create_dev_sys(struct gim_dev_data *data);
void gim_mon_remove_dev_sys(struct gim_dev_data *data);

int gim_mon_create_drv_sys(struct device_driver *drv);
void gim_mon_remove_drv_sys(struct device_driver *drv);

int gim_cmd_handler_init(void);
void gim_cmd_handler_fini(void);

#endif
