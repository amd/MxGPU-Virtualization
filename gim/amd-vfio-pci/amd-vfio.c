/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE
 */

#include <linux/version.h>

#include <linux/init.h>
#include <linux/module.h>
#if defined(SUPPORT_LIVE_MIGRATION)
#include <linux/pci.h>
#include <linux/iommu.h>
#include <linux/vfio_pci_core.h>
#include "gim_vfio_pci.h"

#define get_mig_info_symbol "gim_get_mig_info"

typedef int (*get_mig_info_t)(struct device *dev, struct gim_mig_info *mig_info);

static int amd_vfio_pci_get_mig_info(struct device *dev, struct gim_mig_info *mig_info)
{
	get_mig_info_t get_mig_info;
	int ret;

	get_mig_info = __symbol_get(get_mig_info_symbol);

	if (get_mig_info == NULL) {
		pr_err("Failed to get symbol: %s!", get_mig_info_symbol);
		return -EINVAL;
	}

	ret = get_mig_info(dev, mig_info);
	if (ret) {
		__symbol_put(get_mig_info_symbol);
		return ret;
	}

	if (!mig_info->mig_ops || !mig_info->mig_ops->migration_get_data_size ||
	    !mig_info->mig_ops->migration_get_state ||
	    !mig_info->mig_ops->migration_set_state) {
		pr_err("Incomplete mig_ops detected\n");
		mig_info->flags = 0;
		mig_info->mig_ops = NULL;
		ret = -EINVAL;
		__symbol_put(get_mig_info_symbol);
	}

	return ret;
};

static struct file *amd_vfio_pci_migration_set_state(struct vfio_device *device,
						     enum vfio_device_mig_state new_state)
{
	int ret;
	struct file *filp = NULL;
	struct gim_mig_info mig_info;

	ret = amd_vfio_pci_get_mig_info(device->dev, &mig_info);
	if (ret)
		return ERR_PTR(ret);

	filp = mig_info.mig_ops->migration_set_state(device, new_state);
	__symbol_put(get_mig_info_symbol);
	return filp;
}

static int amd_vfio_pci_migration_get_state(struct vfio_device *device,
					    enum vfio_device_mig_state *curr_state)
{
	struct gim_mig_info mig_info;
	int ret;

	ret = amd_vfio_pci_get_mig_info(device->dev, &mig_info);
	if (ret)
		return ret;

	ret = mig_info.mig_ops->migration_get_state(device, curr_state);
	__symbol_put(get_mig_info_symbol);
	return ret;
}

static int amd_vfio_pci_migration_get_data_size(struct vfio_device *device,
						unsigned long *stop_copy_length)
{
	struct gim_mig_info mig_info;
	int ret;

	ret = amd_vfio_pci_get_mig_info(device->dev, &mig_info);
	if (ret)
		return ret;

	ret = mig_info.mig_ops->migration_get_data_size(device, stop_copy_length);
	__symbol_put(get_mig_info_symbol);
	return ret;
}

static struct vfio_migration_ops amd_vfio_migration_ops = {
	.migration_get_state = amd_vfio_pci_migration_get_state,
	.migration_set_state = amd_vfio_pci_migration_set_state,
	.migration_get_data_size = amd_vfio_pci_migration_get_data_size,
};

static int amd_vfio_pci_open_device(struct vfio_device *vdev)
{
	int ret;
	struct gim_mig_info mig_info;
	struct vfio_pci_core_device *core_dev =
		container_of(vdev, struct vfio_pci_core_device, vdev);
	struct pci_dev *pdev = to_pci_dev(vdev->dev);

	if (device_iommu_capable(&pdev->dev, IOMMU_CAP_DIRTY_TRACKING)) {
		ret = amd_vfio_pci_get_mig_info(vdev->dev, &mig_info);
		if (ret) {
			dev_err(&pdev->dev,
				 "Failed to get migration info, live migration disabled\n");
		} else {
			if (mig_info.mig_ops->migration_set_state(vdev, VFIO_DEVICE_STATE_RUNNING)) {
				dev_err(&pdev->dev,
					"Failed to set migration state to RUNNING, live migration disabled\n");
			} else {
				vdev->migration_flags = mig_info.flags;
				vdev->mig_ops = &amd_vfio_migration_ops;
				dev_info(&pdev->dev, "Live migration enabled\n");
			}

			__symbol_put(get_mig_info_symbol);
		}
	} else {
		dev_info(&pdev->dev, "IOMMU dirty page tracking is disabled\n");
	}

	ret = vfio_pci_core_enable(core_dev);
	if (ret)
		return ret;

	vfio_pci_core_finish_enable(core_dev);

	return 0;
}

static const struct pci_device_id amd_vfio_pci_table[] = { { 0x1002, 0x74B6, PCI_ANY_ID,
							     PCI_ANY_ID, 0, 0, 11 },
							   {} };

static const struct vfio_device_ops amd_vfio_pci_ops = {
	.name = "amd-vfio-pci",
	.init = vfio_pci_core_init_dev,
	.release = vfio_pci_core_release_dev,
	.open_device = amd_vfio_pci_open_device,
	.close_device = vfio_pci_core_close_device,
	.ioctl = vfio_pci_core_ioctl,
	.device_feature = vfio_pci_core_ioctl_feature,
	.read = vfio_pci_core_read,
	.write = vfio_pci_core_write,
	.mmap = vfio_pci_core_mmap,
	.request = vfio_pci_core_request,
	.match = vfio_pci_core_match,
	.bind_iommufd = vfio_iommufd_physical_bind,
	.unbind_iommufd = vfio_iommufd_physical_unbind,
	.attach_ioas = vfio_iommufd_physical_attach_ioas,
	.detach_ioas = vfio_iommufd_physical_detach_ioas,
};

static int amd_vfio_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct vfio_pci_core_device *core_dev;
	int ret;

	core_dev =
		vfio_alloc_device(vfio_pci_core_device, vdev, &pdev->dev, &amd_vfio_pci_ops);
	if (IS_ERR(core_dev)) {
		return PTR_ERR(core_dev);
	}
	dev_set_drvdata(&pdev->dev, core_dev);

	ret = vfio_pci_core_register_device(core_dev);
	if (ret) {
		vfio_put_device(&core_dev->vdev);
		return ret;
	}

	return ret;
}

static void amd_vfio_pci_remove(struct pci_dev *pdev)
{
	struct vfio_pci_core_device *core_dev = dev_get_drvdata(&pdev->dev);

	vfio_pci_core_unregister_device(core_dev);
	vfio_put_device(&core_dev->vdev);
}

static struct pci_driver amd_vfio_pci_driver = {
	.name = "amd-vfio-pci",
	.id_table = amd_vfio_pci_table,
	.probe = amd_vfio_pci_probe,
	.remove = amd_vfio_pci_remove,
	.err_handler = &vfio_pci_core_err_handlers,
	.driver_managed_dma = true,
};
#endif

static int __init amd_vfio_pci_init(void)
{
#if defined(SUPPORT_LIVE_MIGRATION)
	int ret;

	/* Register and scan for devices */
	ret = pci_register_driver(&amd_vfio_pci_driver);
	if (ret)
		return ret;
	pr_info("AMD VFIO PCI driver initialized successfully.\n");
	return 0;

#endif
	pr_warn("Kernel version too old for AMD VFIO PCI live migration support.Please upgrade to kernel 6.8 or later.\n");
	return -ENODEV;
};

static void __exit amd_vfio_pci_cleanup(void)
{
#if defined(SUPPORT_LIVE_MIGRATION)
	pci_unregister_driver(&amd_vfio_pci_driver);
	pr_info("AMD VFIO PCI driver cleaned up successfully.\n");
#endif
}

module_init(amd_vfio_pci_init);
module_exit(amd_vfio_pci_cleanup);

MODULE_AUTHOR("Advanced Micro Devices, Inc.");
MODULE_DESCRIPTION("AMD VFIO PCI MODULE");
MODULE_LICENSE("Dual MIT/GPL");