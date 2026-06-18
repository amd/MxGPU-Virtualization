/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_NOTIFY_H
#define AMDGV_NOTIFY_H

#include "amdgv_basetypes.h"
#include "amdgv_api.h"

/* Global registered callbacks */
extern amdgv_notification_handler notification_handler;

void amdgv_notify_shim(oss_dev_t dev, enum amdgv_notification api_event, const char *fmt, ...);

#endif // AMDGV_NOTIFY_H
