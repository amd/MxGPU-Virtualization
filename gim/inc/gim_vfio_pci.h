/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef GIM_VFIO_PCI_H
#define GIM_VFIO_PCI_H
#if defined(SUPPORT_LIVE_MIGRATION)
#include <linux/vfio.h>

#define VFIO_DEVICE_STATE_DEFAULT_GIM (VFIO_DEVICE_STATE_NR)

struct gim_mig_info {
	unsigned int flags;
	const struct vfio_migration_ops *mig_ops;
};
#endif
#endif