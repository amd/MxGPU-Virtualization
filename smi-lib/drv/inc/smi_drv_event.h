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

/* ESXi only. */
void smi_esxi_release_event_notifiers(struct smi_ctx *ctx);

#endif // __SMI_DRV_EVENT_H__
