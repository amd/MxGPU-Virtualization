/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include "gim_debug.h"
#include "gim_hbm_dax.h"

#define HBM_DAX_NAME_LEN 32

struct gim_hbm_dax_t {
	char dev_name[HBM_DAX_NAME_LEN];
	uint64_t phy_addr;
	uint64_t phy_size;
	dev_t dev_num;
	struct cdev cdev;
	struct class *class;
	struct device *dev;
};

/* Open function (for the character device) */
static int gim_hbm_dax_open(struct inode *inode, struct file *filp) {
	struct cdev *cdev = inode->i_cdev;
	struct gim_hbm_dax_t *gim_hbm_dax = container_of(cdev, struct gim_hbm_dax_t, cdev);

	gim_info("%s device opened\n", gim_hbm_dax->dev_name);

	filp->private_data = gim_hbm_dax;
	return 0;
}

/* Release function (for the character device) */
static int gim_hbm_dax_release(struct inode *inode, struct file *filp) {
	struct cdev *cdev = inode->i_cdev;
	struct gim_hbm_dax_t *gim_hbm_dax = container_of(cdev, struct gim_hbm_dax_t, cdev);

	gim_info("%s device released\n", gim_hbm_dax->dev_name);

	return 0;
}

/* mmap function (for mapping memory to user space) */
static int gim_hbm_dax_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long size = vma->vm_end - vma->vm_start;
	struct gim_hbm_dax_t *gim_hbm_dax = filp->private_data;

	// Check if the requested size is valid
	if (size > gim_hbm_dax->phy_size){
		return -EINVAL;
	}

	gim_info("%s device mapped size=0x%lx\n", gim_hbm_dax->dev_name, size);

	// Map the physical memory range to user space
	return remap_pfn_range(vma, vma->vm_start,
			gim_hbm_dax->phy_addr >> PAGE_SHIFT, size, vma->vm_page_prot);
}

/* File operations */
static struct file_operations gim_hbm_dax_fops = {
	.owner = THIS_MODULE,
	.open = gim_hbm_dax_open,
	.release = gim_hbm_dax_release,
	.mmap = gim_hbm_dax_mmap,
};

static ssize_t size_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct gim_hbm_dax_t *gim_hbm_dax = dev_get_drvdata(dev);

	return sprintf(buf, "%llu\n", gim_hbm_dax->phy_size);
}

static DEVICE_ATTR_RO(size);

void *gim_hbm_dax_init(const char *dev_name,
					uint64_t phy_addr, uint64_t phy_size)
{
	int ret;
	struct gim_hbm_dax_t *gim_hbm_dax;

	/* Allocate gim dax context */
	gim_hbm_dax = kzalloc(sizeof(struct gim_hbm_dax_t), GFP_KERNEL);
	if (gim_hbm_dax == NULL) {
		gim_warn("failed to allocate memory for gim dax\n");
		return NULL;
	}

	/* Allocate device number */
	strncpy(gim_hbm_dax->dev_name, dev_name, sizeof(gim_hbm_dax->dev_name));
	gim_hbm_dax->phy_addr = phy_addr;
	gim_hbm_dax->phy_size = phy_size;
	ret = alloc_chrdev_region(&gim_hbm_dax->dev_num, 0, 1, gim_hbm_dax->dev_name);
	if (ret < 0) {
		gim_warn("failed to allocate cdev for %s\n",
			gim_hbm_dax->dev_name);
		goto err_free;
	}

	/* Allocate class */
#if !defined(HAVE_CLASS_CREATE_1_ARG)
	gim_hbm_dax->class = class_create(THIS_MODULE, gim_hbm_dax->dev_name);
#else
	gim_hbm_dax->class = class_create(gim_hbm_dax->dev_name);
#endif
	if (IS_ERR(gim_hbm_dax->class)) {
		gim_warn("failed to create dax class for %s\n",
			gim_hbm_dax->dev_name);
		goto err_unregister;
	}

	/* Allocate device */
	gim_hbm_dax->dev = device_create(gim_hbm_dax->class, NULL,
					MKDEV(MAJOR(gim_hbm_dax->dev_num), 0), NULL,
					gim_hbm_dax->dev_name);
	if (IS_ERR(gim_hbm_dax->dev)) {
		gim_warn("failed to create dax device for %s\n",
			gim_hbm_dax->dev_name);
		goto err_class;
	}
	dev_set_drvdata(gim_hbm_dax->dev, gim_hbm_dax);

	/* Create size attribute */
	ret = device_create_file(gim_hbm_dax->dev, &dev_attr_size);
	if (ret) {
		gim_warn("failed to create size attr %s. ret=%d\n",
		gim_hbm_dax->dev_name, ret);
		goto err_dev;
	}

	/* Initialize the character device */
	cdev_init(&gim_hbm_dax->cdev, &gim_hbm_dax_fops);
	ret = cdev_add(&gim_hbm_dax->cdev, gim_hbm_dax->dev_num, 1);
	if (ret < 0) {
		gim_warn("failed to add cdev for %s. ret=%d\n",
			gim_hbm_dax->dev_name, ret);
		goto err_cdev;
	}
	kobject_uevent(&gim_hbm_dax->cdev.kobj, KOBJ_ADD);

	gim_info("%s device loaded. base address: %llx length: %llx\n",
		gim_hbm_dax->dev_name, gim_hbm_dax->phy_addr, gim_hbm_dax->phy_size);
	return (void *)gim_hbm_dax;

err_cdev:
	device_remove_file(gim_hbm_dax->dev, &dev_attr_size);
err_dev:
	device_destroy(gim_hbm_dax->class, gim_hbm_dax->dev->devt);
err_class:
	class_destroy(gim_hbm_dax->class);
err_unregister:
	unregister_chrdev_region(gim_hbm_dax->dev_num, 1);
err_free:
	kfree(gim_hbm_dax);
	return NULL;
}

void gim_hbm_dax_fini(void *hbm_dax) {
	struct gim_hbm_dax_t *gim_hbm_dax = (struct gim_hbm_dax_t *)hbm_dax;
	if (gim_hbm_dax != NULL) {
		kobject_uevent(&gim_hbm_dax->cdev.kobj, KOBJ_REMOVE);
		cdev_del(&gim_hbm_dax->cdev);
		device_remove_file(gim_hbm_dax->dev, &dev_attr_size);
		device_destroy(gim_hbm_dax->class, gim_hbm_dax->dev->devt);
		class_destroy(gim_hbm_dax->class);
		unregister_chrdev_region(gim_hbm_dax->dev_num, 1);
		gim_info("%s device unloaded\n", gim_hbm_dax->dev_name);
		kfree(gim_hbm_dax);
	}
}
