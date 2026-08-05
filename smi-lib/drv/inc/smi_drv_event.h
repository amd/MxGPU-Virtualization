/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SMI_DRV_EVENT_H__
#define __SMI_DRV_EVENT_H__
#include "smi_handle.h"
int smi_create_event(struct smi_ctx *smi, amdgv_dev_t *adev, struct smi_event_set_config *config);
int smi_read_event(struct smi_ctx *ctx, amdgv_dev_t *adev, uint64_t dev_id, struct smi_event_entry *event, int64_t timeout_usec);
int smi_destroy_event(struct smi_ctx *ctx, amdgv_dev_t *adev, uint64_t dev_id);

/* Linux only. Revoke every event fd bound to adev before the device is torn
 * down (PCI unbind/remove/shutdown), so held fds cannot dereference freed
 * adapter/notifier state. Also marks adev as tearing down so an in-flight
 * CREATE_EVENT refuses to publish a new fd onto it. Must run before
 * amdgv_device_fini_ex(). */
void smi_revoke_device_events(amdgv_dev_t *adev);

/* Linux only. Drop the adev-teardown marker set by smi_revoke_device_events()
 * once the adapter has been freed, so a later probe reusing the same address
 * is not refused. Must run after amdgv_device_fini_ex(). */
void smi_clear_device_teardown(amdgv_dev_t *adev);

/* ESXi only. */
void smi_esxi_release_event_notifiers(struct smi_ctx *ctx);

#endif // __SMI_DRV_EVENT_H__
