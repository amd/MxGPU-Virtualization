/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SMI_DRV_H__
#define __SMI_DRV_H__

#include <smi_drv_oss.h>

#include "smi_drv_types.h"
#include "smi_cmd_ioctl.h"

struct oss_interface;

int smi_init(struct oss_interface *oss_interface,
		struct smi_shim_interface *shim_interface);
void smi_lock_devices(void);
void smi_unlock_devices(void);
void smi_cleanup(void);

/* ESXi only. */
void smi_drain_in_flight_ioctls(int timeout_ms);

/* ESXi only. Drain libgv notifiers for all open SMI fds during unload. */
#if defined(SMI_ESXI_BUILD) || defined(ESX)
void smi_release_all_notifiers(void);
#endif

int smi_open(smi_process_handle file, bool is_privileged);
int smi_release(smi_process_handle file);
long smi_ioctl_handler(smi_process_handle file, unsigned int cmd, void *arg);

#endif // __SMI_DRV_H__
