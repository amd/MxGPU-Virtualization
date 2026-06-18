/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv_notify.h"
#include "amdgv_oss_wrapper.h"
#include "amdgv_list.h"

/* Global callback to store external notification handler */
amdgv_notification_handler notification_handler;

void amdgv_notify_shim(oss_dev_t dev, enum amdgv_notification api_event, const char *fmt, ...)
{
	if (!notification_handler)
		return;

	/* filter out non-exposed events */
	if (api_event < AMDGV_NOTIFICATION_MAX) {
		va_list args;

		va_start(args, fmt);

		notification_handler(dev, api_event, fmt, args);

		va_end(args);
	}
}
