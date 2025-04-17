/*
 * Copyright (c) 2017-2019 Advanced Micro Devices, Inc. All rights reserved.
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

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/signal.h>
#include <linux/interrupt.h>
#include <linux/utsname.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/random.h>
#include <linux/dma-mapping.h>
#include <linux/iommu.h>
#include <linux/scatterlist.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/completion.h>
#include <linux/eventfd.h>
#include <linux/poll.h>
#include <linux/firmware.h>
#include <linux/namei.h>
#include <linux/cpumask.h>
#include <linux/workqueue.h>

#if defined(HAVE_LINUX_DMA_DIRECT_H)
#include <linux/dma-direct.h>
#else
#include <asm/dma-mapping.h>
#endif

#include <linux/crypto.h>
#include <linux/scatterlist.h>
#include <crypto/hash.h>

#include <linux/ftrace.h>
#include "gim_ftrace.h"

#include "dcore_drv.h"

#include "amdgv_oss.h"
#include "gim_debug.h"
#include "gim_config.h"
#include "gim.h"

extern struct gim_error_ring_buffer *gim_error_rb;

#define GIM_GFX_DUMP_DATA_PATH "/var/log/%s"
#define GIM_DUMP_FILE_PATH "/var/log/gim_dump_%04x_%04x_%04x"
#ifdef WS_RECORD
#define GIM_WS_RECORD_FILE_PATH "/var/log/gim_ws_record_%04x_%04x_%04x"
#define GIM_AUTO_WS_RECORD_FILE_PATH "/var/log/gim_auto_ws_record_%04x_%04x_%04x"
#endif
#define GIM_RLCV_TIMESTAMP_FILE_PATH "/var/log/rlcv_timestamp_%04x_%04x_%04x"
#define FW_NAME_SIZE_MAX 40
#define FW_PATH_SIZE_MAX 60

#define AMD_XCC_DSM_GET_NUM_FUNCS 	0
#define AMD_XCC_DSM_GET_SUPP_MODE 	1
#define AMD_XCC_DSM_GET_XCP_MODE 	2
#define AMD_XCC_DSM_GET_VF_XCC_MAPPING 	4
#define AMD_XCC_DSM_GET_TMR_INFO 	5
#define AMD_XCC_DSM_NUM_FUNCS 		5

#define PCIE_SRIOV_VF_BASE_ADDR_0	0x0354
#define PCIE_SRIOV_VF_BASE_ADDR_1	0x0358
#define PCIE_SRIOV_VF_BASE_ADDR_2	0x035c
#define PCIE_SRIOV_VF_BASE_ADDR_3	0x0360
#define PCIE_SRIOV_VF_BASE_ADDR_4	0x0364
#define PCIE_SRIOV_VF_BASE_ADDR_5	0x0368

static oss_dev_t gim_get_vf_dev_from_bdf(uint32_t bdf)
{
	unsigned int domain = bdf >> 16;
	unsigned int bus = (bdf >> 8) & 0xff;
	unsigned int devfn = bdf & 0xff;

	return pci_get_domain_bus_and_slot(domain, bus, devfn);
}

static void gim_put_vf_dev(oss_dev_t dev)
{
	pci_dev_put((struct pci_dev *)dev);
}

static int gim_map_vf_dev_res(oss_dev_t dev, struct oss_dev_res *res)
{
	unsigned int flag = 0, i;
	struct pci_dev *pdev = (struct pci_dev *)dev;

	/* Find the MMIO BAR.
	 * The MMIO has the attributes of MEMORY and non-prefetch.
	 */
	for (i = 0; i < DEVICE_COUNT_RESOURCE; i++) {
		flag = pci_resource_flags(pdev, i);
		if ((flag & IORESOURCE_MEM) && !(flag & IORESOURCE_PREFETCH))
			break;
	}

	if (i == DEVICE_COUNT_RESOURCE) {
		gim_put_error(AMDGV_ERROR_DRIVER_MMIO_FAIL, 0);
		return -1;
	}

	/* map MMIO BAR */
	res->mmio = ioremap(pci_resource_start(pdev, i),
				   pci_resource_len(pdev, i));
	if (res->mmio == NULL) {
		gim_put_error(AMDGV_ERROR_DRIVER_MMIO_FAIL, 0);
		gim_info("Failed to map mmio region 0x%llx for length %lld\n",
			pci_resource_start(pdev, i),
			pci_resource_len(pdev, i));
		return -1;
	}

	res->mmio_size = pci_resource_len(pdev, i);

	/* map framebuffer BAR */
	gim_info("Map region 0x%llx for length %lld\n",
		pci_resource_start(pdev, 0),
		 pci_resource_len(pdev, 0));

	res->fb = ioremap(pci_resource_start(pdev, 0),
				pci_resource_len(pdev, 0));
	if (res->fb == NULL) {
		gim_info("Failed to map fb region 0x%llx for length %lld\n",
			pci_resource_start(pdev, 0),
			pci_resource_len(pdev, 0));
		gim_put_error(AMDGV_ERROR_DRIVER_MMIO_FAIL, 0);
		goto failed;
	}

	res->fb_size = pci_resource_len(pdev, 0);
	return 0;

failed:
	iounmap(res->mmio);
	return -1;
}

static void gim_unmap_vf_dev_res(oss_dev_t dev, struct oss_dev_res *res)
{
	if (res->fb)
		iounmap(res->fb);

	if (res->mmio)
		iounmap(res->mmio);
}

static uint8_t gim_mm_readb(void *addr)
{
	return readb((void __iomem *)addr);
}

static uint16_t gim_mm_readw(void *addr)
{
	return readw((void __iomem *)addr);
}

static uint32_t gim_mm_readl(void *addr)
{
	return readl((void __iomem *)addr);
}

static void gim_mm_writeb(void *addr, uint8_t val)
{
	writeb(val, (void __iomem *)addr);
}

static void gim_mm_writew(void *addr, uint16_t val)
{
	writew(val, (void __iomem *)addr);
}

static void gim_mm_writel(void *addr, uint32_t val)
{
	writel(val, (void __iomem *)addr);
}

static void gim_mm_writeq(void *addr, uint64_t val)
{
	writeq(val, (void __iomem *)addr);
}

static uint8_t gim_io_readb(void *addr)
{
	return ioread8((void __iomem *)addr);
}

static uint16_t gim_io_readw(void *addr)
{
	return ioread16((void __iomem *)addr);
}

static uint32_t gim_io_readl(void *addr)
{
	return ioread32((void __iomem *)addr);
}

static void gim_io_writeb(void *addr, uint8_t val)
{
	iowrite8(val, (void __iomem *)addr);
}

static void gim_io_writew(void *addr, uint16_t val)
{
	iowrite16(val, (void __iomem *)addr);
}

static void gim_io_writel(void *addr, uint32_t val)
{
	iowrite32(val, (void __iomem *)addr);
}

static int gim_pci_read_config_byte(oss_dev_t dev, int where, uint8_t *val_ptr)
{
	int ret;

	ret = pci_read_config_byte((struct pci_dev *)dev, where, val_ptr);
	if (ret)
		gim_put_error(AMDGV_ERROR_DRIVER_NO_ACCESS_PCI_REGION, where);
	return ret;
}

static int gim_pci_read_config_word(oss_dev_t dev, int where, uint16_t *val_ptr)
{
	int ret;

	ret = pci_read_config_word((struct pci_dev *)dev, where, val_ptr);
	if (ret)
		gim_put_error(AMDGV_ERROR_DRIVER_NO_ACCESS_PCI_REGION, where);
	return ret;
}

static int gim_pci_read_config_dword(oss_dev_t dev, int where,
				     uint32_t *val_ptr)
{
	int ret;

	ret = pci_read_config_dword((struct pci_dev *)dev, where, val_ptr);
	if (ret)
		gim_put_error(AMDGV_ERROR_DRIVER_NO_ACCESS_PCI_REGION, where);
	return ret;
}

static int gim_pci_write_config_byte(oss_dev_t dev, int where, uint8_t val)
{
	int ret;

	ret = pci_write_config_byte((struct pci_dev *)dev, where, val);
	if (ret)
		gim_put_error(AMDGV_ERROR_DRIVER_NO_ACCESS_PCI_REGION, where);
	return ret;
}

static int gim_pci_write_config_word(oss_dev_t dev, int where, uint16_t val)
{
	int ret;

	ret = pci_write_config_word((struct pci_dev *)dev, where, val);
	if (ret)
		gim_put_error(AMDGV_ERROR_DRIVER_NO_ACCESS_PCI_REGION, where);
	return ret;
}

static int gim_pci_write_config_dword(oss_dev_t dev, int where, uint32_t val)
{
	int ret;

	ret = pci_write_config_dword((struct pci_dev *)dev, where, val);
	if (ret)
		gim_put_error(AMDGV_ERROR_DRIVER_NO_ACCESS_PCI_REGION, where);
	return ret;
}

#define PCI_EXT_CAP_ID_VF_REBAR	0x24

static int gim_pci_resize_vf_bar(oss_dev_t dev, int bar_idx, uint32_t num_vf)
{
	struct pci_dev *pdev = dev;
	struct resource *res = pdev->resource + bar_idx + PCI_IOV_RESOURCES;
	uint16_t cmd, vf_cmd, offset, total_vf;
	uint32_t save_lo, save_hi;
	int pos, iov_pos;
	uint64_t size;
	int bar_base;
	uint32_t tmp;
	int id;
	int ret;

	iov_pos = pci_find_ext_capability(pdev, PCI_EXT_CAP_ID_SRIOV);
	if (iov_pos == 0)
		return -ENOTSUPP;

	if (bar_idx < 0 || bar_idx >= PCI_SRIOV_NUM_BARS)
		return -EINVAL;

	/* Resize the VF BAR:
	 * 1. resource_size(res) returns (default BAR size) * (total_VF)
	 * 2. size per VF is then rounded down to the nearest power of 2 for alignment
	 * 3. BAR size is then offset 20 bits as per the resizable BAR specification
	 *
	 * If num_vf == 0, it means restoring the BAR size to the default value,
	 * so total_vf is used instead of num_vf.
	 */
	if (num_vf == 0) {
		pci_read_config_word(pdev, iov_pos + PCI_SRIOV_TOTAL_VF, &total_vf);
		pci_read_config_word(pdev, iov_pos + PCI_SRIOV_NUM_VF, (uint16_t *)&num_vf);
		size = rounddown_pow_of_two(resource_size(res) / total_vf);
	} else
		size = rounddown_pow_of_two(resource_size(res) / num_vf);

	/* Disable memory decoding while we change the BAR addresses and size */
	pci_read_config_word(pdev, PCI_COMMAND, &cmd);
	pci_write_config_word(pdev, PCI_COMMAND,
				cmd & ~PCI_COMMAND_MEMORY);

	/* Disable VF Memory Space Enable (MSE) before changing VF BAR */
	pci_read_config_word(pdev, iov_pos + PCI_SRIOV_CTRL, &vf_cmd);
	pci_write_config_word(pdev, iov_pos + PCI_SRIOV_CTRL,
				vf_cmd & ~PCI_SRIOV_CTRL_MSE);

	pci_read_config_word(pdev, iov_pos + PCI_SRIOV_VF_OFFSET, &offset);

	/* Save the base of the BAR, this is cleared by the size operation */
	bar_base = iov_pos + PCI_SRIOV_BAR + 4 * bar_idx;
	pci_read_config_dword(pdev, bar_base, &save_lo);
	pci_read_config_dword(pdev, bar_base + 4, &save_hi);

	if (!(save_lo & ~0xc) && !save_hi) {
		/* bar_base might be lost (value read is 0x0000000c) at this moment.
		 * a possible case is that amdgpu has been loaded prior to gim,
		 * during which a mode1 reset is triggered. BAR0 did not get restored
		 * when amdgpu is unloaded, so gim needs to handle such case.
		 */
		ret = pci_assign_resource(pdev, bar_idx + PCI_IOV_RESOURCES);
		if (ret)
			return ret;

		pci_read_config_dword(pdev, bar_base, &save_lo);
		pci_read_config_dword(pdev, bar_base + 4, &save_hi);
		gim_warn("SR-IOV BAR%d base is 0, reassigned to 0x%08x%08x\n",
					bar_idx, save_hi, (save_lo & ~0xc));
	}

	/* Check if VF resizable BAR is supported */
	pos = pci_find_ext_capability(pdev, PCI_EXT_CAP_ID_VF_REBAR);
	if (pos == 0)
		goto end;

	/* Resize the BAR */
	pci_read_config_dword(pdev, pos + bar_idx * 8 + 8, &tmp);
	tmp &= ~0x3F00;
	tmp |= (max(ilog2(size), 20) - 20) << 8;
	pci_write_config_dword(pdev, pos + bar_idx * 8 + 8, tmp);

	/* Hack up the resources of the virtfn devices */
	for (id = 0; id < num_vf; ++id) {
		struct pci_dev *virtfn;
		uint32_t pf_bdf, vf_bdf;

		pf_bdf = PCI_DEVID(pdev->bus->number, pdev->devfn) | (pci_domain_nr(pdev->bus) << 16);
		vf_bdf = pf_bdf + offset + id;
		virtfn = gim_get_vf_dev_from_bdf(vf_bdf);
		if (!virtfn) {
			gim_put_error(AMDGV_ERROR_DRIVER_INVALID_VALUE, vf_bdf);
			return -ENODEV;
		}

		virtfn->resource[bar_idx].start = res->start + size * id;
		virtfn->resource[bar_idx].end = res->start + (size * (id + 1)) - 1;

		gim_put_vf_dev(virtfn);
	}

	/* Writeback the saved BAR base */
	pci_write_config_dword(pdev, bar_base, save_lo);
	pci_write_config_dword(pdev, bar_base + 4, save_hi);

end:
	/* Enable VF Memory Space Enable (MSE) after changing VF BAR */
	pci_write_config_word(pdev, iov_pos + PCI_SRIOV_CTRL,
				vf_cmd | PCI_SRIOV_CTRL_MSE);

	/* And finally allow decoding memory requests again */
	pci_write_config_word(pdev, PCI_COMMAND, cmd);
	return 0;
}

static int gim_pci_restore_vf_rebar(oss_dev_t dev, int bar_idx)
{
	int ret = 0;
	struct pci_dev *pdev = dev;
	uint32_t num_vf;
	int pos;

	pos = pci_find_ext_capability(pdev, PCI_EXT_CAP_ID_SRIOV);
	ret = pci_read_config_dword(pdev, pos + PCI_SRIOV_NUM_VF, &num_vf);
	if (ret) {
		gim_put_error(AMDGV_ERROR_DRIVER_NO_ACCESS_PCI_REGION, pos + PCI_SRIOV_NUM_VF);
		return ret;
	}

	ret = gim_pci_resize_vf_bar(dev, bar_idx, num_vf);
	if (ret) {
		gim_put_error(AMDGV_ERROR_DRIVER_VF_RESIZE_BAR_FAIL, 0);
		gim_info("Restore VF BAR to default size and disable SR-IOV\n");
		gim_pci_resize_vf_bar(dev, 0, 0);
		pci_disable_sriov((struct pci_dev *)dev);
	}
	return ret;
}

static int gim_pci_enable_sriov(oss_dev_t dev, uint32_t num_vf)
{
	int ret;

	ret = pci_enable_sriov((struct pci_dev *)dev, num_vf);
	if (ret)
		return ret;

	ret = gim_pci_resize_vf_bar(dev, 0, num_vf);
	if (ret) {
		gim_put_error(AMDGV_ERROR_DRIVER_VF_RESIZE_BAR_FAIL, 0);
		gim_info("Restore VF BAR to default size and disable SR-IOV\n");
		gim_pci_resize_vf_bar(dev, 0, 0);
		pci_disable_sriov((struct pci_dev *)dev);
	}
	return ret;
}

static int gim_pci_disable_sriov(oss_dev_t dev)
{
	int ret;

	/* restore vf bar to default size */
	ret = gim_pci_resize_vf_bar(dev, 0, 0);

	pci_disable_sriov((struct pci_dev *)dev);

	return ret;
}

static int gim_pci_bus_reset(oss_dev_t dev)
{
	struct pci_dev *pdev = dev;

#if !defined(HAVE_BRIDGE_SECONDARY_BUS_RESET)
	pci_reset_bridge_secondary_bus(pdev->bus->self);
#else
	pci_bridge_secondary_bus_reset(pdev->bus->self);
#endif

	return 0;
}

static int gim_pci_find_ext_cap(oss_dev_t dev, int cap)
{
	return pci_find_ext_capability(dev, cap);
}

static int gim_pci_find_next_ext_cap(oss_dev_t dev, int start_pos, int cap)
{
	return pci_find_next_ext_capability(dev, start_pos, cap);
}

static int gim_pci_find_cap(oss_dev_t dev, int cap)
{
	return pci_find_capability(dev, cap);
}

static bool gim_read_bios_from_rom_bar(struct pci_dev *pdev, unsigned char *dest, unsigned long *bytes_copied, unsigned long max_size)
{
	bool ret = false;

	size_t size;
	unsigned char *bios_io_ptr;
	unsigned char *bios_ptr;

	*bytes_copied = 0;

	bios_io_ptr = pci_map_rom(pdev, &size);
	if (!bios_io_ptr) {
		gim_put_error(AMDGV_ERROR_DRIVER_NO_ACCESS_PCI_REGION, 0);
		goto end;
	}

	bios_ptr = (unsigned char *)kzalloc(size, GFP_KERNEL);
	if (!bios_ptr) {
		gim_put_error(AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL, size);
		goto unmap;
	}

	memcpy_fromio(bios_ptr, bios_io_ptr, size);

	*bytes_copied = min(size, max_size);
	memcpy(dest, bios_ptr, *bytes_copied);
	ret = true;

	kfree(bios_ptr);
unmap:
	pci_unmap_rom(pdev, bios_io_ptr);
end:
	return ret;
}

static bool gim_read_bios_from_platform(struct pci_dev *pdev, unsigned char *dest, unsigned long *bytes_copied, unsigned long max_size)
{
	bool ret = false;

	unsigned char *bios_io_ptr;
	unsigned char *bios_ptr;

	if (!pdev->rom || pdev->romlen == 0)
		goto end;

	bios_io_ptr = ioremap(pdev->rom, pdev->romlen);
	if (!bios_io_ptr) {
		gim_put_error(AMDGV_ERROR_DRIVER_NO_ACCESS_PCI_REGION, 0);
		goto end;
	}

	bios_ptr = kzalloc(pdev->romlen, GFP_KERNEL);
	if (!bios_ptr) {
		gim_put_error(AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL, pdev->romlen);
		goto unmap;
	}

	memcpy_fromio(bios_ptr, bios_io_ptr, pdev->romlen);

	*bytes_copied = min(pdev->romlen, max_size);
	memcpy(dest, bios_ptr, *bytes_copied);
	ret = true;

	kfree(bios_ptr);
unmap:
	iounmap(bios_io_ptr);
end:
	return ret;

}

static bool gim_pci_read_rom(oss_dev_t dev, unsigned char *dest, unsigned long *bytes_copied, unsigned long max_size)
{
	struct pci_dev *pdev = (struct pci_dev *)dev;

	/* ROM reading is platform dependent and need to support them all */
	if (gim_read_bios_from_platform(pdev, dest, bytes_copied, max_size)) {
		gim_info("Read VBIOS from platform\n");
		goto success;
	}

	if (gim_read_bios_from_rom_bar(pdev, dest, bytes_copied, max_size)) {
		gim_info("Read VBIOS from ROM BAR\n");
		goto success;
	}

	gim_put_error(AMDGV_ERROR_DRIVER_ROM_MAP_FAIL, 0);
	return false;

success:
	return true;
}

/* deprecated */
static void *gim_pci_map_rom(oss_dev_t dev, unsigned long *size)
{
	return pci_map_rom(dev, size);
}

/* deprecated */
static void gim_pci_unmap_rom(oss_dev_t dev, void *rom)
{
	return pci_unmap_rom(dev, rom);
}

static irqreturn_t gim_isr_wrapper(int irq, void *priv)
{
	int ret;
	struct oss_intr_regrt_entry *intr_entry;

	intr_entry = (struct oss_intr_regrt_entry *)priv;

	ret = intr_entry->int_cb.interrupt_cb(intr_entry->context);

	if (ret == OSS_IRQ_NONE)
		return IRQ_NONE;

	return IRQ_HANDLED;
}

static irqreturn_t gim_isr_wrapper2(int irq, void *priv)
{
	int ret;
	struct oss_intr_regrt_entry *intr_entry;
	struct oss_interrupt_cb_info *cb_info;

	intr_entry = (struct oss_intr_regrt_entry *)priv;

	/*todo: fill in cb_info*/
	cb_info = NULL;

	ret = intr_entry->int_cb.interrupt_cb2(intr_entry->context, cb_info);

	if (ret == OSS_IRQ_NONE)
		return IRQ_NONE;

	return IRQ_HANDLED;
}

static int gim_register_interrupt(oss_dev_t dev,
				  struct oss_intr_regrt_info *intr_regrt_info)
{
	unsigned int num_msi_vectors, idx_msi_vector, irq;
	struct oss_intr_regrt_entry *intr_entry;
	struct msix_entry *entries = NULL;
	enum oss_intr_type intr_type;
	int rc, i, vectors;
	struct pci_dev *pdev = (struct pci_dev *)dev;

	if (!intr_regrt_info) {
		gim_put_error(AMDGV_ERROR_DRIVER_INVALID_VALUE, intr_regrt_info);
		return -1;
	}

	intr_type = intr_regrt_info->intr_type;

	if (intr_type == OSS_INTR_TYPE_MSI) {
		num_msi_vectors = intr_regrt_info->num_msi_vectors;

		/* enable MSI capability */
#if defined(HAVE_PCI_ALLOC_IRQ_VECTORS)
		vectors = pci_alloc_irq_vectors(pdev, num_msi_vectors,
						num_msi_vectors, PCI_IRQ_MSI);
#else
		vectors = pci_enable_msi_exact(pdev, num_msi_vectors);
#endif
		if (vectors != num_msi_vectors) {
			gim_put_error(AMDGV_ERROR_DRIVER_INTERRUPT_INIT_FAIL, vectors);
			goto disable_pci_intr;
		}
	} else if (intr_type == OSS_INTR_TYPE_MSIX) {
		num_msi_vectors = intr_regrt_info->num_msi_vectors;
		entries = kmalloc(sizeof(struct msix_entry) * num_msi_vectors,
				  GFP_KERNEL);

		if (entries == NULL) {
			gim_put_error(AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct msix_entry) * num_msi_vectors);
			return -1;
		}

		for (i = 0; i < num_msi_vectors; i++) {
			intr_entry = &intr_regrt_info->intr_entries[i];
			idx_msi_vector = intr_entry->idx_msi_vector;
			entries[i].entry = idx_msi_vector;
		}

		/* enable MSI-x capability */
		vectors = pci_enable_msix_range(pdev, entries, num_msi_vectors,
						num_msi_vectors);
		if (vectors != num_msi_vectors) {
			gim_put_error(AMDGV_ERROR_DRIVER_INTERRUPT_INIT_FAIL, vectors);
			goto disable_pci_intr;
		}
	} else {
		gim_put_error(AMDGV_ERROR_DRIVER_INVALID_VALUE, intr_type);
		return -1;
	}

	for (i = 0; i < intr_regrt_info->num_intr_entry; i++) {
		intr_entry = &intr_regrt_info->intr_entries[i];
		idx_msi_vector = intr_entry->idx_msi_vector;

		if (intr_type == OSS_INTR_TYPE_MSI)
			irq = pdev->irq + idx_msi_vector;
		else
			irq = entries[i].vector;

		if (intr_entry->intr_cb_type == OSS_INTR_CB_REGULAR)
		/* bind each MSI vector irq to interrupt callback */
			rc = request_irq(irq, gim_isr_wrapper, 0,
				"gim intr", intr_entry);
		else
			rc = request_irq(irq, gim_isr_wrapper2, 0,
				"gim intr", intr_entry);

		if (rc) {
			gim_put_error(AMDGV_ERROR_DRIVER_INTERRUPT_INIT_FAIL, irq);
			goto fail_to_request_irq;
		}
	}

	intr_regrt_info->priv = (void *)entries;

	return 0;

fail_to_request_irq:

	while (--i >= 0) {
		intr_entry = &intr_regrt_info->intr_entries[i];
		idx_msi_vector = intr_entry->idx_msi_vector;

		if (intr_type == OSS_INTR_TYPE_MSI)
			irq = pdev->irq + idx_msi_vector;
		else
			irq = entries[i].vector;

		free_irq(irq, intr_entry);
	}

disable_pci_intr:

	/* disable MSI/MSI-x capability */
	if (intr_type == OSS_INTR_TYPE_MSI) {
		pci_disable_msi(pdev);
	} else {
		pci_disable_msix(pdev);
		kfree(entries);
	}

	return -1;
}

static int gim_unregister_interrupt(oss_dev_t dev,
				    struct oss_intr_regrt_info *intr_regrt_info)
{
	int i;
	unsigned int irq;
	unsigned int num_msi_vectors;
	unsigned int idx_msi_vector;
	struct oss_intr_regrt_entry *intr_entry;
	struct pci_dev *pdev = (struct pci_dev *)dev;
	struct msix_entry *entries = NULL;
	enum oss_intr_type intr_type;

	intr_type = intr_regrt_info->intr_type;
	num_msi_vectors = intr_regrt_info->num_msi_vectors;

	entries = (struct msix_entry *)intr_regrt_info->priv;

	if ((entries == NULL)
		&& (intr_type != OSS_INTR_TYPE_MSI))
		return -1;

	for (i = 0; i < num_msi_vectors; i++) {
		intr_entry = &intr_regrt_info->intr_entries[i];
		idx_msi_vector = intr_entry->idx_msi_vector;

		if (intr_type == OSS_INTR_TYPE_MSI)
			irq = pdev->irq + idx_msi_vector;
		else
			irq = entries[i].vector;

		free_irq(irq, intr_entry);
	}

	if (intr_type == OSS_INTR_TYPE_MSI) {
		pci_disable_msi(pdev);
	} else {
		pci_disable_msix(pdev);
		kfree(entries);
	}

	return 0;
}

static void *gim_alloc_small_memory(uint32_t size)
{
	return kmalloc(size, GFP_KERNEL);
}

static void *gim_alloc_small_memory_atomic(uint32_t size)
{
	return kmalloc(size, GFP_ATOMIC);
}

static void *gim_alloc_small_zero_memory(uint32_t size)
{
	return kzalloc(size, GFP_KERNEL);
}

static void gim_free_small_memory(void *ptr)
{
	kfree(ptr);
}

static void *gim_get_physical_addr(void *addr)
{
	return (void *)virt_to_phys(addr);
}

static void *gim_alloc_memory(uint32_t size)
{
	return vmalloc(size);
}

static void gim_free_memory(void *ptr)
{
	vfree(ptr);
}

static void *gim_memremap(uint64_t offset, uint32_t size, enum oss_memremap_type type)
{
	return memremap(offset, size, type);
}

static void gim_memunmap(void *addr)
{
	return memunmap(addr);
}

static void *gim_memset(void *src, int c, uint64_t n)
{
	return memset(src, c, n);
}

static void *gim_memcpy(void *dest, const void *src, uint64_t n)
{
	return memcpy(dest, src, n);
}

static int gim_memcmp(const void *s1, const void *s2, uint64_t n)
{
	return memcmp(s1, s2, n);
}

int gim_strncmp(const char *s1, const char *s2, uint64_t n)
{
	return strncmp(s1, s2, n);
}

uint32_t gim_strlen(const char *s)
{
	return strlen(s);
}

uint32_t gim_strnlen(const char *s, uint32_t len)
{
	return strnlen(s, len);
}

uint32_t gim_do_div(uint64_t *n, uint32_t base)
{
	return do_div(*n, base);
}

struct gim_dma_mem_info {
	struct pci_dev   *dev;
	dma_addr_t   bus_addr;
	void             *va_ptr;
	unsigned int     size;
	struct scatterlist *sg;
	int	sg_cnt;
};

static void gim_free_dma_mem_iova(struct gim_dma_mem_info *mem_info)
{
	dma_unmap_sg(&mem_info->dev->dev, mem_info->sg, mem_info->sg_cnt,
		DMA_BIDIRECTIONAL);
	vfree(mem_info->sg);
	vfree(mem_info->va_ptr);
}

static void gim_free_dma_mem_system(struct gim_dma_mem_info *mem_info)
{
#if !defined(HAVE_LINUX_PCI_DMA_COMPAT_H)
	dma_free_coherent(&mem_info->dev->dev, mem_info->size,
			mem_info->va_ptr, mem_info->bus_addr);
#else
	pci_free_consistent(mem_info->dev, mem_info->size,
			mem_info->va_ptr, mem_info->bus_addr);
#endif
}

static int gim_alloc_dma_mem_iova(oss_dev_t dev,
			     struct gim_dma_mem_info *mem_info)
{
	int nr_pages;
	unsigned long dma_addr, dma_addr_next;
	unsigned long len;
	int i;
	struct page *pg;

	nr_pages = mem_info->size/PAGE_SIZE;

	mem_info->va_ptr = vzalloc(mem_info->size);
	if (!mem_info->va_ptr)
		goto error_out;

	mem_info->sg = vzalloc(nr_pages * sizeof(*(mem_info->sg)));
	if (!mem_info->sg)
		goto error_kzfree;

	sg_init_table(mem_info->sg, nr_pages);
	for (i = 0; i < nr_pages; i++) {
		pg = vmalloc_to_page(mem_info->va_ptr + i * PAGE_SIZE);
		sg_set_page(&mem_info->sg[i], pg, PAGE_SIZE, 0);
	}
	mem_info->sg_cnt = nr_pages;

	if (!dma_map_sg(&mem_info->dev->dev, mem_info->sg,
			mem_info->sg_cnt, DMA_BIDIRECTIONAL))
		goto error_vfree;

	for (i = 0; i < mem_info->sg_cnt - 1; i++) {
		dma_addr = sg_dma_address(&mem_info->sg[i]);
		dma_addr_next  = sg_dma_address(&mem_info->sg[i+1]);
		len = mem_info->sg[i].length;

		gim_dbg("DMA offset[%d] %llx->%llx\n",
				i, sg_dma_address(&mem_info->sg[i]),
				sg_phys(&mem_info->sg[i]));

		if ((dma_addr_next - dma_addr) != len) {
			gim_info("DMA buffers not contiguous\n");
			goto error_unmap;
		}
	}

	mem_info->bus_addr = sg_dma_address(mem_info->sg);
	return 0;
error_unmap:
	dma_unmap_sg(&mem_info->dev->dev, mem_info->sg, mem_info->sg_cnt,
		DMA_BIDIRECTIONAL);
error_vfree:
	vfree(mem_info->sg);
error_kzfree:
	vfree(mem_info->va_ptr);
error_out:
	return -1;
}

static int gim_alloc_dma_mem_system(oss_dev_t dev,
			     struct gim_dma_mem_info *mem_info)
{
	int nr_pages;

	nr_pages = mem_info->size/PAGE_SIZE;

	if (nr_pages > MAX_ORDER_NR_PAGES) {
#ifdef MAX_PAGE_ORDER
		gim_info("Please enlarge MAX_PAGE_ORDER from %d to %d to alloc %dMB pysical contiguous system memory\n",
					MAX_PAGE_ORDER,
					order_base_2(nr_pages),
					nr_pages >> 8);
#else
		gim_info("Please enlarge MAX_ORDER from %d to %d to alloc %dMB pysical contiguous system memory\n",
					MAX_ORDER,
					order_base_2(nr_pages) - order_base_2(MAX_ORDER_NR_PAGES) + MAX_ORDER,
					nr_pages >> 8);
#endif
		return -1;
	}

	mem_info->va_ptr =
#if !defined(HAVE_LINUX_PCI_DMA_COMPAT_H)
		dma_alloc_coherent(&((struct pci_dev *)dev)->dev, mem_info->size,
				     &mem_info->bus_addr, GFP_KERNEL);
#else
		pci_alloc_consistent((struct pci_dev *)dev, mem_info->size,
				     &mem_info->bus_addr);
#endif
	if (!mem_info->va_ptr)
		return -1;

	return 0;
}

static int gim_alloc_dma_mem_iova_align(oss_dev_t dev,
			     struct gim_dma_mem_info *mem_info)
{

	if (gim_alloc_dma_mem_iova(dev, mem_info))
		return -1;
	else if (mem_info->bus_addr & (mem_info->size - 1)) {
		gim_free_dma_mem_iova(mem_info);
		mem_info->size = mem_info->size << 1;
		if (gim_alloc_dma_mem_iova(dev, mem_info))
			return -1;
	}
	return 0;
}

static int gim_alloc_dma_mem_system_align(oss_dev_t dev,
			     struct gim_dma_mem_info *mem_info)
{
	if (gim_alloc_dma_mem_system(dev, mem_info))
		return -1;
	else if (mem_info->bus_addr & (mem_info->size - 1)) {
		gim_free_dma_mem_system(mem_info);
		mem_info->size = mem_info->size << 1;
		if (gim_alloc_dma_mem_system(dev, mem_info))
			return -1;
	}
	return 0;
}

/**
It will follow logic to allocate 16MB continues dma memory:
 * First try to use iova method to allocate 16MB
 * Check whether it aligns to 16MB, if no, then double size and choose
 * the bus_addr with aligning to 16MB
 * If double size, still fail, then fallback to pci_alloc_consistent
 * For pci_alloc_consistent, first try 16MB
 * Check whether it aligns to 16MB, if no, then
 * double size to allocate, and find the align to 16MB address
 * If pci_alloc_consistent also fail, then fallback to framebuffer
 */
static int gim_alloc_dma_mem(oss_dev_t dev, uint32_t size,
			     enum oss_dma_mem_type type,
			     struct oss_dma_mem_info *dma_mem_info)
{
	struct gim_dma_mem_info *mem_info;
	dma_mem_info->va_ptr = NULL;
	dma_mem_info->handle = (void *)NULL;

	if ((type != OSS_DMA_MEM_CACHEABLE) && (type != OSS_DMA_PA_CONTIGUOUS)) {
		gim_put_error(AMDGV_ERROR_DRIVER_INVALID_VALUE, type);
		return -1;
	}

	mem_info = kzalloc(sizeof(struct gim_dma_mem_info), GFP_KERNEL);
	if (!mem_info)
		return -1;
	mem_info->dev = (struct pci_dev *)dev;

	if (type == OSS_DMA_MEM_CACHEABLE) {
		int size_align = ALIGN(size, PAGE_SIZE);

		mem_info->size = size_align;
		if (gim_alloc_dma_mem_iova_align(dev, mem_info)) {
			gim_put_error(AMDGV_ERROR_DRIVER_ALLOC_IOVA_ALIGN_FAIL, 0);
			mem_info->sg_cnt = 0;
			if (gim_alloc_dma_mem_system_align(dev, mem_info)) {
				gim_put_error(AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL, size_align);
				kfree(mem_info);
				return -1;
			}
		}
		dma_mem_info->bus_addr = ALIGN(mem_info->bus_addr, size_align);
		dma_mem_info->va_ptr = mem_info->va_ptr + (dma_mem_info->bus_addr - mem_info->bus_addr);
		if (mem_info->sg_cnt != 0)
			dma_mem_info->phys_addr = sg_phys(mem_info->sg);
		else
			dma_mem_info->phys_addr = dma_to_phys(&mem_info->dev->dev, mem_info->bus_addr);

		dma_mem_info->handle = (void *)mem_info;
	} else if (type == OSS_DMA_PA_CONTIGUOUS) {
		mem_info->size = size;
		if (gim_alloc_dma_mem_system(dev, mem_info)) {
			gim_put_error(AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL, size);
			kfree(mem_info);
			return -1;
		}
		dma_mem_info->bus_addr = mem_info->bus_addr;
		dma_mem_info->va_ptr = mem_info->va_ptr;
		dma_mem_info->phys_addr = dma_to_phys(&mem_info->dev->dev, mem_info->bus_addr);
		dma_mem_info->handle = (void *)mem_info;
	}

	return 0;
}

static void gim_free_dma_mem(void *handle)
{
	struct gim_dma_mem_info *mem_info = (struct gim_dma_mem_info *)handle;

	if (mem_info) {
		if (mem_info->sg_cnt)
			gim_free_dma_mem_iova(mem_info);
		else
			gim_free_dma_mem_system(mem_info);
		kfree(mem_info);
	}
}

struct gim_spin_lock {
	int rank;
	unsigned long flags;
	spinlock_t lock;
};

static void *gim_spin_lock_init(int rank)
{
	struct gim_spin_lock *p;

	p = kmalloc(sizeof(struct gim_spin_lock), GFP_KERNEL);
	if (p) {
		p->rank = rank;
		spin_lock_init(&p->lock);
	} else{
		gim_put_error(AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
			sizeof(struct gim_spin_lock));
	}
	return p;
}

static void gim_spin_lock(void *p)
{
	struct gim_spin_lock *sl;

	sl = (struct gim_spin_lock *)p;

	spin_lock(&sl->lock);
}

static void gim_spin_unlock(void *p)
{
	struct gim_spin_lock *sl;

	sl = (struct gim_spin_lock *)p;

	spin_unlock(&sl->lock);
}

static void gim_spin_lock_irq(void *p)
{
	struct gim_spin_lock *sl;

	sl = (struct gim_spin_lock *)p;

	spin_lock_irqsave(&sl->lock, sl->flags);
}

static void gim_spin_unlock_irq(void *p)
{
	struct gim_spin_lock *sl;

	sl = (struct gim_spin_lock *)p;

	spin_unlock_irqrestore(&sl->lock, sl->flags);
}

static void gim_spin_lock_fini(void *p)
{
	kfree(p);
}

static void *gim_mutex_init(void)
{
	struct mutex *p = kmalloc(sizeof(struct mutex), GFP_KERNEL);

	if (p)
		mutex_init(p);

	return p;
}

static void gim_mutex_lock(void *p)
{
	mutex_lock((struct mutex *)p);
}

static void gim_mutex_unlock(void *p)
{
	mutex_unlock((struct mutex *)p);
}

static void gim_mutex_fini(void *p)
{
	mutex_destroy((struct mutex *)p);
	kfree(p);
}
static void *gim_rwlock_init(void)
{
	rwlock_t *p =
		kmalloc(sizeof(rwlock_t), GFP_KERNEL);

	if (p) {
		rwlock_t lock = *p;
		rwlock_init(&lock);
	}
	return p;
}

static void gim_rwlock_read_lock(void *p)
{
	rwlock_t *lock = (rwlock_t *)p;
	BUG_ON(!lock);
	read_lock(lock);
}

static int gim_rwlock_read_trylock(void *p)
{
	rwlock_t *lock = (rwlock_t *)p;
	BUG_ON(!lock);
	return read_trylock(lock);
}

static void gim_rwlock_read_unlock(void *p)
{
	rwlock_t *lock = (rwlock_t *)p;
	BUG_ON(!lock);
	read_unlock(lock);
}

static void gim_rwlock_write_lock(void *p)
{
	rwlock_t *lock = (rwlock_t *)p;
	BUG_ON(!lock);
	write_lock(lock);
}

static int gim_rwlock_write_trylock(void *p)
{
	rwlock_t *lock = (rwlock_t *)p;
	BUG_ON(!lock);
	return write_trylock(lock);
}

static void gim_rwlock_write_unlock(void *p)
{
	rwlock_t *lock = (rwlock_t *)p;
	BUG_ON(!lock);
	write_unlock(lock);
}

static void gim_rwlock_fini(void *p)
{
	kfree(p);
}

static void *gim_rwsema_init(void)
{
	struct rw_semaphore *p =
		kmalloc(sizeof(*p), GFP_KERNEL);

	if (p)
		init_rwsem(p);

	return p;
}

static void gim_rwsema_read_lock(void *p)
{
	struct rw_semaphore *lock = (struct rw_semaphore *)p;
	BUG_ON(!lock);

retry:
	down_read(lock);

	/* lock grabbed with writer waiting */
	if (rwsem_is_contended(lock)) {
		up_read(lock);

		schedule();
		goto retry;
	}

}

static int gim_rwsema_read_trylock(void *p)
{
	struct rw_semaphore *lock = (struct rw_semaphore *)p;
	int ret;

	BUG_ON(!lock);
	ret = down_read_trylock(lock);

	/* lock grabbed with writer waiting */
	if ((ret == 1) && rwsem_is_contended(lock)) {
		up_read(lock);
		return 0;
	}

	return ret;
}

static void gim_rwsema_read_unlock(void *p)
{
	struct rw_semaphore *lock = (struct rw_semaphore *)p;

	BUG_ON(!lock);
	up_read(lock);
}

static void gim_rwsema_write_lock(void *p)
{
	struct rw_semaphore *lock = (struct rw_semaphore *)p;

	BUG_ON(!lock);

	down_write(lock);
}

static int gim_rwsema_write_trylock(void *p)
{
	struct rw_semaphore *lock = (struct rw_semaphore *)p;

	BUG_ON(!lock);

	return down_write_trylock(lock);

}

static void gim_rwsema_write_unlock(void *p)
{
	struct rw_semaphore *lock = (struct rw_semaphore *)p;

	BUG_ON(!lock);
	up_write(lock);
}

static void gim_rwsema_fini(void *p)
{
	kfree(p);
}

static void *gim_event_init(void)
{
	struct gim_event *ge = kzalloc(sizeof(struct gim_event), GFP_KERNEL);

	if (ge == NULL)
		return NULL;

	init_completion(&ge->cp);
	return (void *)&ge->cp;
}

static void gim_signal_event(void *event)
{
	struct gim_event *ge = container_of(event, struct gim_event, cp);

	complete(&ge->cp);
}

static void gim_signal_event_forever(void *event)
{
	struct gim_event *ge = container_of(event, struct gim_event, cp);

	complete_all(&ge->cp);
}

static void gim_signal_event_with_flag(void *event, uint64_t flag)
{
	struct gim_event *ge = container_of(event, struct gim_event, cp);

	ge->flags |= flag;

	complete(&ge->cp);
}

static void gim_signal_event_forever_with_flag(void *event, uint64_t flag)
{
	struct gim_event *ge = container_of(event, struct gim_event, cp);

	ge->flags |= flag;

	complete_all(&ge->cp);
}

/* timeout is in micro seconds */
static enum oss_event_state gim_wait_event(void *event, uint32_t timeout)
{
	int ret = -1;
	int tmp_ret;

	struct gim_event *ge = container_of(event, struct gim_event, cp);

	if (timeout > 0) {
		tmp_ret = wait_for_completion_interruptible_timeout(&ge->cp,
					usecs_to_jiffies(timeout));
		if (tmp_ret > 0)
			ret = OSS_EVENT_STATE_WAKE_UP;
		else if (tmp_ret == 0)
			return OSS_EVENT_STATE_TIMEOUT;
		else if (tmp_ret == -ERESTARTSYS)
			ret = OSS_EVENT_STATE_INTERRUPTED;

	} else {
		tmp_ret = wait_for_completion_interruptible(&ge->cp);
		if (tmp_ret == 0)
			ret = OSS_EVENT_STATE_WAKE_UP;
		else if (tmp_ret == -ERESTARTSYS)
			ret = OSS_EVENT_STATE_INTERRUPTED;
	}

	if (ge->flags & EVENT_FLAGS_SKIPPED)
		ret = OSS_EVENT_FAKE_SIGNAL;

	WARN_ON(ret == -1);

	return ret;
}

static void gim_event_fini(void *event)
{
	struct gim_event *ge = container_of(event, struct gim_event, cp);
	kfree(ge);
}


static void *gim_atomic_init(void)
{
	return kzalloc(sizeof(atomic64_t), GFP_KERNEL);
}

static uint64_t gim_atomic_read(void *p)
{
	return atomic64_read((atomic64_t *)p);
}

static void gim_atomic_set(void *p, uint64_t val)
{
	atomic64_set((atomic64_t *)p, val);
}

static void gim_atomic_inc(void *p)
{
	atomic64_inc((atomic64_t *)p);
}

static void gim_atomic_dec(void *p)
{
	atomic64_dec((atomic64_t *)p);
}

static uint64_t gim_atomic_inc_return(void *p)
{
	return atomic64_inc_return((atomic64_t *)p);
}

static uint64_t gim_atomic_dec_return(void *p)
{
	return atomic64_dec_return((atomic64_t *)p);
}

static int64_t gim_atomic_cmpxchg(void *p, int64_t comperand, int64_t exchange)
{
	return atomic64_cmpxchg((atomic64_t *)p, comperand, exchange);
}

static void gim_atomic_fini(void *p)
{
	kfree(p);
}

static void *gim_create_thread(oss_callback_t threadfn, void *context,
				const char *name)
{
	struct task_struct *k;

	k = kthread_run((void *)threadfn, context, name);
	if (IS_ERR(k))
		return NULL;

	return k;
}

static void *gim_get_current_thread(void)
{
	return get_current();
}

static bool gim_is_current_running_thread(void *thread)
{
	struct task_struct *given_thread = (struct task_struct *)thread;
	struct task_struct *current_thread = get_current();

	return (given_thread == current_thread);
}

static void gim_close_thread(void *thread)
{
	struct task_struct *k = (struct task_struct *)thread;

	kthread_stop(k);
}

static bool gim_thread_should_stop(void *thread)
{
	return kthread_should_stop();
}

struct gim_timer {
	struct hrtimer timer;
	oss_callback_t timer_cb;
	void *context;
	uint64_t interval_us;
	enum oss_timer_type type;
};

static enum hrtimer_restart gim_timer_callback_wrapper(struct hrtimer *timer)
{
	int ret;
	struct gim_timer *t;

	t = container_of(timer, struct gim_timer, timer);

	ret = t->timer_cb(t->context);

	if (t->type == OSS_TIMER_TYPE_PERIODIC
		&& ret == OSS_TIMER_RETURN_RESTART)
		return HRTIMER_RESTART;

	return HRTIMER_NORESTART;
}

static void *gim_timer_init(oss_callback_t timer_cb, void *context)
{
	struct gim_timer *t = (struct gim_timer *)
		kzalloc(sizeof(struct gim_timer), GFP_KERNEL);

	if (t == NULL)
		return NULL;

	t->timer_cb = timer_cb;
	t->context = context;

	hrtimer_init(&t->timer, CLOCK_MONOTONIC,
			HRTIMER_MODE_REL);
	t->timer.function = gim_timer_callback_wrapper;

	return (void *)t;
}

static void *gim_timer_init_ex(oss_callback_t timer_cb, void *context,
				oss_dev_t dev)
{
	struct gim_timer *t = (struct gim_timer *)
		kzalloc(sizeof(struct gim_timer), GFP_KERNEL);

	if (t == NULL)
		return NULL;

	t->timer_cb = timer_cb;
	t->context = context;

	hrtimer_init(&t->timer, CLOCK_MONOTONIC,
			HRTIMER_MODE_REL);
	t->timer.function = gim_timer_callback_wrapper;

	return (void *)t;
}

static int gim_start_timer(void *timer, uint64_t interval_us,
			enum oss_timer_type type)
{
	s64 secs;
	unsigned long nsecs;
	struct gim_timer *t = (struct gim_timer *) timer;

	t->interval_us = interval_us;
	t->type = type;

	secs = interval_us / USEC_PER_SEC;
	nsecs = (interval_us % USEC_PER_SEC) * 1000;

	hrtimer_start(&t->timer, ktime_set(secs, nsecs),
		      HRTIMER_MODE_REL);

	return 0;
}

static int gim_pause_timer(void *timer)
{
	int ret = 0;

	struct gim_timer *t = (struct gim_timer *)timer;

	ret = hrtimer_cancel(&t->timer);

	return ret;
}

static void gim_close_timer(void *timer)
{
	struct gim_timer *t = (struct gim_timer *)timer;

	hrtimer_cancel(&t->timer);

	kfree(t);
}

static void gim_udelay(uint32_t usecs)
{
	if (num_online_cpus() == 1)
		usleep_range(usecs, usecs);
	else
		udelay(usecs);
}

static void gim_usleep(uint32_t usecs)
{
	usleep_range(usecs, usecs);
}

static void gim_msleep(uint32_t msecs)
{
	msleep(msecs);
}

static void gim_yield(void)
{
	schedule();
}

static void gim_memory_fence(void)
{
	/* dummy functions */
	return;
}

static uint64_t gim_get_time_stamp(void)
{
	return (uint64_t)ktime_to_us(ktime_get());
}

static uint64_t gim_get_utc_time_stamp(void)
{
	ktime_t cur_time;

	cur_time = ktime_get_real();

	return (uint64_t)ktime_divns(cur_time, NSEC_PER_SEC);
}

static void gim_get_utc_time_stamp_str(char *buf, uint32_t buf_size)
{
	time64_t cur_time;
    struct tm tm_time;

    cur_time = ktime_get_real_seconds() / 60;

    time64_to_tm(cur_time * 60, 0, &tm_time);

    snprintf(buf, buf_size, "%04ld%02d%02d_%02d%02d",
			tm_time.tm_year + 1900,
			tm_time.tm_mon + 1,
			tm_time.tm_mday,
			tm_time.tm_hour,
			tm_time.tm_min);
}

static bool gim_ari_supported(void)
{
	return true;
}

static void gim_print(int level, const char *fmt, va_list args)
{
	int klevel;

	switch (level) {
	case AMDGV_DEBUG_LEVEL:
		klevel = LOGLEVEL_DEBUG;
		break;

	case AMDGV_INFO_LEVEL:
		klevel = LOGLEVEL_INFO;
		break;

	case AMDGV_WARN_LEVEL:
		klevel = LOGLEVEL_WARNING;
		break;

	case AMDGV_ERROR_LEVEL:
		klevel = LOGLEVEL_ERR;
		break;

	default:
		klevel = LOGLEVEL_DEBUG;
	}

	if (!in_interrupt() || printk_ratelimit()) {
#if !defined(HAVE_VPRINTK_EMIT_5_ARG)
		vprintk_emit(0, klevel, NULL, 0, fmt, args);
#else
		vprintk_emit(0, klevel, NULL, fmt, args);
#endif
	}

	return;
}

int gim_vsnprintf(char *buf, uint32_t size, const char *fmt, va_list args)
{
	return vsnprintf(buf, size, fmt, args);
}

/* This function should never be called because gim_shim
 * does not set the AMDGV_FLAG_USE_PF flag.
 */
static int gim_send_msg(oss_dev_t dev, uint32_t *msg_data, int msg_len,
			bool need_valid)
{
	return 0;
}

static int gim_store_dump(const char *buf, uint32_t bdf)
{
	struct file *file = NULL;
	long ret;
	char *path;
	loff_t pos = 0;

	/* use dynamic allocate to avoid frame size compiling warn */
	path = kzalloc(PATH_MAX, GFP_KERNEL);
	if (NULL == path)
		return -ENOMEM;

	sprintf(path, GIM_DUMP_FILE_PATH, bdf >> 8 & 0xff,
		bdf >> 3 & 0x1f, bdf & 0x7);
	file = filp_open(path, O_CREAT | O_TRUNC | O_SYNC | O_WRONLY, 0);

	if (IS_ERR_OR_NULL(file)) {
		kfree(path);
		ret = PTR_ERR(file);
		return (int)ret == 0 ? -EINVAL : (int)ret;
	}

	ret = gim_kernel_write(file, buf, strlen(buf), pos);
	if (ret != strlen(buf))
		return (int)ret == 0 ? -EINVAL : (int)ret;

	filp_close(file, NULL);
	kfree(path);
	return 0;
}
#ifdef WS_RECORD
static int gim_store_record(const char *buf, uint32_t bdf, bool auto_sched)
{
	struct file *file = NULL;
	long ret;
	char *path;
	const char *file_path_format;
	loff_t pos = 0;

	/* use dynamic allocate to avoid frame size compiling warn */
	path = kzalloc(PATH_MAX, GFP_KERNEL);
	if (NULL == path)
		return -ENOMEM;

	file_path_format = GIM_WS_RECORD_FILE_PATH;
	if (auto_sched)
		file_path_format = GIM_AUTO_WS_RECORD_FILE_PATH;

	sprintf(path, file_path_format, bdf >> 8 & 0xff,
		bdf >> 3 & 0x1f, bdf & 0x7);

	/* record file should be create once and then always been appended */
	file = filp_open(path, O_CREAT | O_SYNC | O_WRONLY | O_APPEND, 0);

	if (IS_ERR_OR_NULL(file)) {
		kfree(path);
		ret = PTR_ERR(file);
		return (int)ret == 0 ? -EINVAL : (int)ret;
	}

	ret = gim_kernel_write(file, buf, strlen(buf), pos);
	if (ret != strlen(buf)) {
		filp_close(file, NULL);
		kfree(path);
		return (int)ret == 0 ? -EINVAL : (int)ret;
	}

	filp_close(file, NULL);
	kfree(path);
	return 0;
}
#endif

static int gim_store_rlcv_timestamp(const char *buf, uint32_t size, uint32_t bdf)
{
	long ret;
	struct file *csa_file;
	char *path;
	loff_t pos = 0;

	/* use dynamic allocate to avoid frame size compiling warn */
	path = kzalloc(PATH_MAX, GFP_KERNEL);
	if (NULL == path)
		return -ENOMEM;

	sprintf(path, GIM_RLCV_TIMESTAMP_FILE_PATH, bdf >> 8 & 0xff,
		bdf >> 3 & 0x1f, bdf & 0x7);
	/* record file should be create once and then always been appended */
	csa_file = filp_open(path, O_CREAT | O_SYNC | O_WRONLY | O_APPEND, 0666);

	if (IS_ERR_OR_NULL(csa_file)) {
		kfree(path);
		ret = PTR_ERR(csa_file);
		return (int)ret == 0 ? -EINVAL : (int)ret;
	}

	ret = gim_kernel_write(csa_file, buf, size, pos);
	if (ret != size)
		return (int)ret == 0 ? -EINVAL : (int)ret;

	filp_close(csa_file, NULL);
	kfree(path);
	return 0;
}

static int gim_notifier_wakeup(void *notifier, uint64_t count)
{
	wait_queue_head_t *wait = (wait_queue_head_t *) notifier;

	wake_up_interruptible_all(wait);

	return 0;
}

int gim_calc_hash_ext(const char *alg_name, void *out, const void *msg, uint64_t len)
{
	int ret = 0;

#if !defined(HAVE_CRYPTO_HASH_DIGEST)
	struct crypto_shash *tfm;
	struct shash_desc *desc;

	tfm = crypto_alloc_shash(alg_name, 0, 0);
	if (IS_ERR(tfm))
		return -ENOMEM;

	desc = kmalloc(sizeof(*desc) + crypto_shash_descsize(tfm), GFP_KERNEL);

	if (!desc) {
		ret = -ENOMEM;
		goto out;
	}

	desc->tfm = tfm;
#if defined(HAVE_SHASH_DESC_FLAGS)
	desc->flags = CRYPTO_TFM_REQ_MAY_SLEEP;
#endif
	ret = crypto_shash_digest(desc, msg, len, out);

	shash_desc_zero(desc);

	kfree(desc);
out:
	crypto_free_shash(tfm);
#else
	struct crypto_hash *tfm;
	struct hash_desc desc;
	struct scatterlist sg;

	sg_init_one(&sg, msg, len);

	tfm = crypto_alloc_hash(alg_name, 0, 0);
	if (IS_ERR(tfm))
		return -ENOMEM;

	desc.tfm = tfm;
	desc.flags = CRYPTO_TFM_REQ_MAY_SLEEP;

	ret = crypto_hash_digest(&desc, &sg, len, out);

	crypto_free_hash(tfm);
#endif
	return ret;
}

static void gim_sema_down(void *sem)
{
	down((struct semaphore *)sem);
}

static void gim_sema_up(void *sem)
{
	up((struct semaphore *)sem);
}

static void *gim_sema_init(int32_t val)
{
	struct semaphore *sem;

	sem = kmalloc(sizeof(struct semaphore), GFP_KERNEL);
	if (sem)
		sema_init(sem, val);
	else {
		gim_put_error(AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
			sizeof(struct semaphore));
	}

	return sem;
}

static void gim_sema_fini(void *p)
{
	kfree(p);
}

static int gim_strnstr(const char *str, const char *substr, uint32_t max_size)
{
	int index = 0;

	uint32_t str_size;
	uint32_t substr_size;

	char *str_cp = NULL;
	char *substr_cp = NULL;
	char *ptr = NULL;

	str_size = strnlen(str, max_size);
	substr_size = strnlen(substr, max_size);

	str_cp = kmalloc(str_size+1, GFP_KERNEL);
	substr_cp = kmalloc(substr_size+1, GFP_KERNEL);

	if (str_cp == NULL || substr_cp == NULL) {
		index = -1;
		goto free;
	}

	memcpy(str_cp, str, str_size);
	memcpy(substr_cp, substr, substr_size);
	str_cp[str_size] = '\0';
	substr_cp[substr_size] = '\0';

	/* O(m+n) */
	ptr = strstr(str_cp, substr_cp);
	if (ptr == NULL)
		index = -1;
	else
		index = (int)((uint64_t)ptr - (uint64_t)str_cp);

free:
	if (str_cp != NULL)
		kfree(str_cp);
	if (substr_cp != NULL)
		kfree(substr_cp);

	return index;
}

static int gim_ucode_validate(const struct firmware *fw)
{
	const struct common_firmware_header *hdr =
		(const struct common_firmware_header *)fw->data;

	if (fw->size == le32_to_cpu(hdr->size_bytes))
		return 0;

	return -EINVAL;
}

static int gim_get_firmware_name(enum amdgv_firmware_id fw_id,
			enum amd_asic_type asic_type, char *fw_name)
{
	char *chip_name = NULL;
	char *ip_name = NULL;

	switch (asic_type) {
	case CHIP_MI300X:
		chip_name = "mi300x";
		break;
	default:
		return -1;
	}

	switch (fw_id) {
	case AMDGV_FIRMWARE_ID__DFC_FW:
		ip_name = "dfc";
		break;
	default:
		return -1;
	}

	snprintf(fw_name, FW_NAME_SIZE_MAX, "gim/%s_%s.bin", chip_name, ip_name);

	return 0;
}

static int gim_detect_firmware(oss_dev_t dev, enum amdgv_firmware_id fw_id,
			enum amd_asic_type asic_type)
{
	char fw_name[FW_NAME_SIZE_MAX];
	char fw_path[FW_PATH_SIZE_MAX] = "/lib/firmware/";
	struct path path;
	int ret;

	ret = gim_get_firmware_name(fw_id, asic_type, fw_name);
	if (ret)
		return ret;

	strcat(fw_path, fw_name);
	/* check file exist first */
	ret = kern_path(fw_path, LOOKUP_FOLLOW, &path);
	if (ret)
		return ret;
	path_put(&path);

	return 0;
}

static int gim_get_firmware(oss_dev_t dev, enum amdgv_firmware_id fw_id,
				enum amd_asic_type asic_type, unsigned char *pfw_image,
				uint32_t *pfw_size, uint32_t fw_size_max)
{
	struct pci_dev *pdev = dev;
	const struct firmware *fw_entry;
	const struct common_firmware_header *hdr;
	const __le32 *fw_data;
	uint32_t fw_size;
	char fw_name[40];
	int ret;

	ret = gim_get_firmware_name(fw_id, asic_type, fw_name);
	if (ret)
		return ret;

	ret = request_firmware(&fw_entry, fw_name, &pdev->dev);
	if (ret)
		return ret;
	if (fw_entry->size > fw_size_max)
		goto out;

	ret = gim_ucode_validate(fw_entry);
	if (ret)
		goto out;
	hdr = (const struct common_firmware_header *)fw_entry->data;
	gim_info("%s version: 0x%X\n", fw_name, le32_to_cpu(hdr->ucode_version));

	fw_data = (const __le32 *)(fw_entry->data +
		le32_to_cpu(hdr->ucode_array_offset_bytes));
	fw_size = le32_to_cpu(hdr->ucode_size_bytes);
	memcpy(pfw_image, fw_data, fw_size);
	*pfw_size = fw_size;

out:
	release_firmware(fw_entry);
	return ret;
}

static void gim_dump_stack(void)
{
	dump_stack();
}

static int gim_copy_from_user(void *to, const void *from, uint32_t size)
{
	return copy_from_user(to, from, size);
}

static int gim_copy_to_user(void *to, const void *from, uint32_t size)
{
	return copy_to_user(to, from, size);
}

static int notrace gim_copy_ftrace_buffer(void *trace_buf, uint32_t trace_buf_len)
{
	return gim_ftrace_get_buffer(trace_buf, trace_buf_len);
}

static int gim_save_fb_sharing_mode(oss_dev_t dev, uint32_t mode)
{
	int ret = 0;
	struct gim_dev_data *dev_data;

	list_for_each_entry(dev_data, &gim_device_list, list) {
		if (dev_data && dev_data->pdev != (struct pci_dev *)dev) {
			continue;
		}

		if (dev_data && mode > 0) {
			ret = gim_conf_set_opt(CONF_OPT_FB_SHARING_MODE, mode);
			if (ret)
				return ret;
		}
	}

	return ret;
}

static uint32_t gim_get_assigned_vf_count(oss_dev_t dev, bool all_gpus)
{
	struct gim_dev_data *dev_data;
	struct pci_dev *pdev_vf;
	int i;
	uint32_t enable_count = 0;

	list_for_each_entry(dev_data, &gim_device_list, list) {

		if (!all_gpus && dev_data && dev_data->pdev != (struct pci_dev *)dev)
			continue;

		for (i = 0; i < dev_data->vf_num; i++) {
			pdev_vf = dev_data->vf_map[i].pdev;
			if (atomic_read(&pdev_vf->enable_cnt) > 0)
				enable_count++;
		}
	}
	return enable_count;
}

static int gim_store_gfx_dump_data(const char *buf, uint32_t size, char *filename)
{
	long ret;
	struct file *csa_file;
	char *path;
	loff_t pos = 0;

	/* use dynamic allocate to avoid frame size compiling warn */
	path = kzalloc(PATH_MAX, GFP_KERNEL);
	if (NULL == path)
		return -ENOMEM;

	sprintf(path, GIM_GFX_DUMP_DATA_PATH, filename);
	/* record file should be create once and then always been appended */
	csa_file = filp_open(path, O_CREAT | O_SYNC | O_WRONLY | O_APPEND, 0666);

	if (IS_ERR_OR_NULL(csa_file)) {
		kfree(path);
		ret = PTR_ERR(csa_file);
		return (int)ret == 0 ? -EINVAL : (int)ret;
	}

	ret = gim_kernel_write(csa_file, buf, size, pos);
	if (ret != size)
		return (int)ret == 0 ? -EINVAL : (int)ret;

	filp_close(csa_file, NULL);
	kfree(path);
	return 0;
}

static int gim_save_accelerator_partition_mode(oss_dev_t dev, uint32_t accelerator_partition_mode)
{
	return gim_conf_set_accelerator_partition_mode_opt(accelerator_partition_mode);
}

static int gim_save_memory_partition_mode(oss_dev_t dev, uint32_t memory_partition_mode)
{
	return gim_conf_set_memory_partition_mode_opt(memory_partition_mode);
}

static int gim_clear_conf_file(oss_dev_t dev)
{
	return gim_conf_clear_conf_file();
}

struct gim_work_queue {
	struct delayed_work work;
	oss_callback_t fn;
	void *context;
};

static void gim_work_func(struct work_struct *work)
{
	struct gim_work_queue *work_ctx =
		container_of(to_delayed_work(work), struct gim_work_queue, work);

	/* Call the callback function */
	if (work_ctx->fn) {
		work_ctx->fn(work_ctx->context);
	}

	/* Free the allocated memory for the work */
	kfree(work_ctx);
}

static int gim_schedule_work(oss_dev_t dev, oss_callback_t fn, void *context)
{
	struct gim_work_queue *work_ctx;

	/* Allocate work queue */
	work_ctx = kmalloc(sizeof(struct gim_work_queue), GFP_KERNEL);
	if (!work_ctx) {
		gim_warn("Failed to allocate memory for work structure\n");
		return -ENOMEM;
	}

	/* Initialize the delayed work */
	INIT_DELAYED_WORK(&work_ctx->work, gim_work_func);
	work_ctx->fn = fn;
	work_ctx->context = context;

	/* Schedule the work immediately */
	schedule_delayed_work(&work_ctx->work, 0);

	return 0;
}

struct oss_interface gim_oss_interfaces = {
	.get_vf_dev_from_bdf = gim_get_vf_dev_from_bdf,
	.put_vf_dev = gim_put_vf_dev,
	.map_vf_dev_res = gim_map_vf_dev_res,
	.unmap_vf_dev_res = gim_unmap_vf_dev_res,
	.mm_readb = gim_mm_readb,
	.mm_readw = gim_mm_readw,
	.mm_readl = gim_mm_readl,
	.mm_writeb = gim_mm_writeb,
	.mm_writew = gim_mm_writew,
	.mm_writel = gim_mm_writel,
	.mm_writeq = gim_mm_writeq,
	.io_readb = gim_io_readb,
	.io_readw = gim_io_readw,
	.io_readl = gim_io_readl,
	.io_writeb = gim_io_writeb,
	.io_writew = gim_io_writew,
	.io_writel = gim_io_writel,
	.pci_read_config_byte = gim_pci_read_config_byte,
	.pci_read_config_word = gim_pci_read_config_word,
	.pci_read_config_dword = gim_pci_read_config_dword,
	.pci_write_config_byte = gim_pci_write_config_byte,
	.pci_write_config_word = gim_pci_write_config_word,
	.pci_write_config_dword = gim_pci_write_config_dword,
	.pci_find_ext_cap = gim_pci_find_ext_cap,
	.pci_find_next_ext_cap = gim_pci_find_next_ext_cap,
	.pci_find_cap = gim_pci_find_cap,
	.pci_map_rom = gim_pci_map_rom,
	.pci_unmap_rom = gim_pci_unmap_rom,
	.pci_read_rom = gim_pci_read_rom,
	.pci_restore_vf_rebar = gim_pci_restore_vf_rebar,
	.pci_enable_sriov = gim_pci_enable_sriov,
	.pci_disable_sriov = gim_pci_disable_sriov,
	.pci_bus_reset = gim_pci_bus_reset,
	.register_interrupt = gim_register_interrupt,
	.unregister_interrupt = gim_unregister_interrupt,
	.alloc_small_memory = gim_alloc_small_memory,
	.alloc_small_memory_atomic = gim_alloc_small_memory_atomic,
	.alloc_small_zero_memory = gim_alloc_small_zero_memory,
	.free_small_memory = gim_free_small_memory,
	.get_physical_addr = gim_get_physical_addr,
	.alloc_memory = gim_alloc_memory,
	.free_memory = gim_free_memory,
	.alloc_dma_mem = gim_alloc_dma_mem,
	.free_dma_mem = gim_free_dma_mem,
	.memremap = gim_memremap,
	.memunmap = gim_memunmap,
	.memset = gim_memset,
	.memcpy = gim_memcpy,
	.memcmp = gim_memcmp,
	.strncmp = gim_strncmp,
	.strlen = gim_strlen,
	.strnlen = gim_strnlen,
	.do_div = gim_do_div,
	.spin_lock_init = gim_spin_lock_init,
	.spin_lock = gim_spin_lock,
	.spin_unlock = gim_spin_unlock,
	.spin_lock_irq = gim_spin_lock_irq,
	.spin_unlock_irq = gim_spin_unlock_irq,
	.spin_lock_fini = gim_spin_lock_fini,
	.mutex_init = gim_mutex_init,
	.mutex_lock = gim_mutex_lock,
	.mutex_unlock = gim_mutex_unlock,
	.mutex_fini = gim_mutex_fini,
	.rwlock_init = gim_rwlock_init,
	.rwlock_read_lock = gim_rwlock_read_lock,
	.rwlock_read_unlock = gim_rwlock_read_unlock,
	.rwlock_read_trylock = gim_rwlock_read_trylock,
	.rwlock_write_lock = gim_rwlock_write_lock,
	.rwlock_write_unlock = gim_rwlock_write_unlock,
	.rwlock_write_trylock = gim_rwlock_write_trylock,
	.rwlock_fini = gim_rwlock_fini,
	.rwsema_init = gim_rwsema_init,
	.rwsema_read_lock = gim_rwsema_read_lock,
	.rwsema_read_unlock = gim_rwsema_read_unlock,
	.rwsema_read_trylock = gim_rwsema_read_trylock,
	.rwsema_write_lock = gim_rwsema_write_lock,
	.rwsema_write_unlock = gim_rwsema_write_unlock,
	.rwsema_write_trylock = gim_rwsema_write_trylock,
	.rwsema_fini = gim_rwsema_fini,
	.event_init = gim_event_init,
	.signal_event = gim_signal_event,
	.signal_event_with_flag = gim_signal_event_with_flag,
	.signal_event_forever = gim_signal_event_forever,
	.signal_event_forever_with_flag = gim_signal_event_forever_with_flag,
	.wait_event = gim_wait_event,
	.event_fini = gim_event_fini,
	.atomic_init = gim_atomic_init,
	.atomic_read = gim_atomic_read,
	.atomic_set = gim_atomic_set,
	.atomic_inc = gim_atomic_inc,
	.atomic_dec = gim_atomic_dec,
	.atomic_inc_return = gim_atomic_inc_return,
	.atomic_dec_return = gim_atomic_dec_return,
	.atomic_cmpxchg = gim_atomic_cmpxchg,
	.atomic_fini = gim_atomic_fini,
	.create_thread = gim_create_thread,
	.get_current_thread = gim_get_current_thread,
	.is_current_running_thread = gim_is_current_running_thread,
	.close_thread = gim_close_thread,
	.thread_should_stop = gim_thread_should_stop,
	.timer_init = gim_timer_init,
	.timer_init_ex = gim_timer_init_ex,
	.start_timer = gim_start_timer,
	.pause_timer = gim_pause_timer,
	.close_timer = gim_close_timer,
	.udelay = gim_udelay,
	.msleep = gim_msleep,
	.usleep = gim_usleep,
	.yield = gim_yield,
	.memory_fence = gim_memory_fence,
	.get_time_stamp = gim_get_time_stamp,
	.get_utc_time_stamp = gim_get_utc_time_stamp,
	.get_utc_time_stamp_str = gim_get_utc_time_stamp_str,
	.ari_supported = gim_ari_supported,
	.print = gim_print,
	.vsnprintf = gim_vsnprintf,
	.send_msg = gim_send_msg,
	.store_dump = gim_store_dump,
	.notifier_wakeup = gim_notifier_wakeup,
	.sema_up = gim_sema_up,
	.sema_down = gim_sema_down,
	.sema_init = gim_sema_init,
	.sema_fini = gim_sema_fini,
	.copy_from_user = gim_copy_from_user,
	.copy_to_user = gim_copy_to_user,
	.strnstr = gim_strnstr,
	.detect_fw = gim_detect_firmware,
	.get_fw = gim_get_firmware,
	.get_assigned_vf_count = gim_get_assigned_vf_count,
	.dump_stack = gim_dump_stack,
	.copy_call_trace_buffer = gim_copy_ftrace_buffer,
#ifdef WS_RECORD
	.store_record = gim_store_record,
#endif
	.store_rlcv_timestamp = gim_store_rlcv_timestamp,
	.signal_reset_happened = dcore_signal_reset_happened,
	.signal_diag_data_ready = dcore_signal_diag_data_ready,
	.diag_data_collect_disabled = dcore_diag_data_collect_disabled,
	.signal_manual_dump_happened = dcore_signal_manual_dump_happened,
	.store_gfx_dump_data = gim_store_gfx_dump_data,
	.save_fb_sharing_mode = gim_save_fb_sharing_mode,
	.save_accelerator_partition_mode = gim_save_accelerator_partition_mode,
	.save_memory_partition_mode = gim_save_memory_partition_mode,
	.clear_conf_file = gim_clear_conf_file,
	.schedule_work = gim_schedule_work,
};
