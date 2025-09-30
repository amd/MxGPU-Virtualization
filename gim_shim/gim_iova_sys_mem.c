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
#include <linux/mutex.h>
#include <linux/iommu.h>
#include <linux/dma-direct.h>
#include <linux/version.h>

#include "gim.h"
#include "amdgv_oss.h"
#include "gim_iova_sys_mem.h"
#include <asm/set_memory.h>

struct gim_iova_mem_info {
	struct pci_dev   *pdev;
	uint64_t bus_addr;
	void *va_ptr;
	int	nr_pages, sg_cnt;
	struct scatterlist *sg;
	bool alloc_new;
	enum oss_dma_mem_type type;
};

static int gim_iova_mem_alloc_direct(struct gim_iova_mem_info *iova_info)
{
	if (iova_info->nr_pages > MAX_ORDER_NR_PAGES) {
#if defined(HAVE_MAX_PAGE_ORDER)
		pr_err("Please enlarge MAX_PAGE_ORDER from %d to %d to alloc %dMB pysical contiguous system memory\n",
					MAX_PAGE_ORDER,
					order_base_2(iova_info->nr_pages),
					iova_info->nr_pages >> 8);
#else
		pr_err("Please enlarge MAX_ORDER from %d to %d to alloc %dMB pysical contiguous system memory\n",
					MAX_ORDER,
					order_base_2(iova_info->nr_pages) - order_base_2(MAX_ORDER_NR_PAGES) + MAX_ORDER,
					iova_info->nr_pages >> 8);
#endif
		return -EINVAL;
	}

	iova_info->va_ptr =
#if !defined(HAVE_LINUX_PCI_DMA_COMPAT_H)
		dma_alloc_coherent(&iova_info->pdev->dev, iova_info->nr_pages << PAGE_SHIFT, &iova_info->bus_addr, GFP_KERNEL);
#else
		pci_alloc_consistent(iova_info->pdev, iova_info->nr_pages << PAGE_SHIFT, &iova_info->bus_addr);
#endif
	if (!iova_info->va_ptr)
		return -ENOMEM;

	return 0;
}

static void gim_iova_mem_free_direct(struct gim_iova_mem_info *iova_info)
{
#if !defined(HAVE_LINUX_PCI_DMA_COMPAT_H)
	dma_free_coherent(&iova_info->pdev->dev, iova_info->nr_pages << PAGE_SHIFT,
			iova_info->va_ptr, iova_info->bus_addr);
#else
	pci_free_consistent(iova_info->pdev, iova_info->nr_pages << PAGE_SHIFT,
			iova_info->va_ptr, iova_info->bus_addr);
#endif
}

static struct scatterlist *gim_iova_alloc_sg_pages(struct gim_iova_mem_info *iova_info)
{
	struct page *pg;
	struct scatterlist *sg = NULL;
	int i = 0;

	if (!iova_info->va_ptr) {
		iova_info->va_ptr = gim_vmalloc(iova_info->nr_pages << PAGE_SHIFT);
		if (!iova_info->va_ptr)
			return NULL;
		iova_info->alloc_new = true;
	}
	if (set_memory_wc((unsigned long)iova_info->va_ptr, iova_info->nr_pages)) {
		pr_err("Set memory wc fail\n");
		goto error_out;
	}

	sg = gim_vzalloc(iova_info->nr_pages * sizeof(*sg));
	if (!sg) {
		pr_err("Alloc sg fail\n");
		goto error_out;
	}

	sg_init_table(sg, iova_info->nr_pages);
	for (i = 0; i < iova_info->nr_pages; i++) {
		if (is_vmalloc_addr(iova_info->va_ptr))
			pg = vmalloc_to_page(iova_info->va_ptr + i * PAGE_SIZE);
		else
			pg = virt_to_page(iova_info->va_ptr + i * PAGE_SIZE);
		sg_set_page(&sg[i], pg, PAGE_SIZE, 0);
	}

	return sg;
error_out:
	if (iova_info->alloc_new && iova_info->va_ptr)
		gim_vfree(iova_info->va_ptr);
	if (sg)
		gim_vfree(sg);
	return NULL;
}

static void gim_iova_free_sg_pages(struct gim_iova_mem_info *iova_info)
{
	if (iova_info->alloc_new)
		gim_vfree(iova_info->va_ptr);
	gim_vfree(iova_info->sg);
}

static int gim_iova_mem_alloc_dma(struct gim_iova_mem_info *iova_info, uint64_t size, uint64_t align,
			  enum oss_dma_mem_type type)
{
	struct scatterlist *sg;
	int i = 0;
	int j = 0;

	iova_info->sg = gim_iova_alloc_sg_pages(iova_info);
	if (!iova_info->sg) {
		pr_err("Alloc satterlist failed\n");
		return -ENOMEM;
	}

	iova_info->sg_cnt = dma_map_sg(&iova_info->pdev->dev, iova_info->sg,
			iova_info->nr_pages, DMA_BIDIRECTIONAL);

	if (!iova_info->sg_cnt) {
		pr_err("DMA map failed\n");
		goto error_map;
	}

	for_each_sg(iova_info->sg, sg, iova_info->sg_cnt, i) {
		j += sg_dma_len(sg);
	}

	if ((j >> PAGE_SHIFT) != iova_info->nr_pages) {
		pr_err("DMA map failed, actual:%d, wanted:%d\n", j, iova_info->nr_pages);
		goto error_num;
	}

	return 0;
error_num:
	dma_unmap_sg(&iova_info->pdev->dev, iova_info->sg,
			iova_info->sg_cnt, DMA_BIDIRECTIONAL);
error_map:
	gim_iova_free_sg_pages(iova_info);
	pr_err("iova memory allocation fail\n");
	return -ENOMEM;
}

static void gim_iova_mem_free_dma(struct gim_iova_mem_info *iova_info)
{
	dma_unmap_sg(&iova_info->pdev->dev, iova_info->sg,
			iova_info->sg_cnt, DMA_BIDIRECTIONAL);
	gim_iova_free_sg_pages(iova_info);
}

int gim_iova_mem_allocate(struct pci_dev *pdev, uint64_t size, uint64_t align,
			  enum oss_dma_mem_type type,
			  struct oss_dma_mem_info *dma_info)
{
	struct gim_iova_mem_info *iova_info;
	uint32_t size_align;
	int r;

	if (size == 0 || align == 0 || dma_info == NULL)
		return -EINVAL;

	iova_info = gim_kzalloc(sizeof(struct gim_iova_mem_info), GFP_KERNEL);
	if (!iova_info)
		return -ENOMEM;

	align = ALIGN(align, PAGE_SIZE);
	size_align = ALIGN(size, align);
	iova_info->pdev = pdev;
	iova_info->nr_pages = size_align / PAGE_SIZE;
	iova_info->bus_addr = dma_info->bus_addr;
	iova_info->va_ptr = dma_info->va_ptr;
	iova_info->type = type;
	if (type == OSS_DMA_MEM_CACHEABLE || type == OSS_DMA_PA_CONTIGUOUS)
		r =  gim_iova_mem_alloc_direct(iova_info);
	else if (type == OSS_DMA_ALLOW_DMA_NOT_CONTIGUOUS)
		r = gim_iova_mem_alloc_dma(iova_info, size_align, align, type);

	if (r) {
		gim_kfree(iova_info);
		return r;
	}

	dma_info->bus_addr = iova_info->bus_addr;
	dma_info->va_ptr = iova_info->va_ptr;
	dma_info->phys_addr = dma_to_phys(&pdev->dev, dma_info->bus_addr);
	dma_info->handle = iova_info;

	return 0;
}

void gim_iova_mem_free(void *handle)
{
	struct gim_iova_mem_info *iova_info = (struct gim_iova_mem_info *)handle;

	if (iova_info == NULL)
		return;

	if (iova_info->type == OSS_DMA_MEM_CACHEABLE || iova_info->type == OSS_DMA_PA_CONTIGUOUS)
		gim_iova_mem_free_direct(iova_info);
	else
		gim_iova_mem_free_dma(iova_info);

	gim_kfree(iova_info);
}

uint64_t gim_iova_sg_dma_address(void *handle, uint32_t page)
{
	struct gim_iova_mem_info *iova_info = (struct gim_iova_mem_info *)handle;

	if (!iova_info)
		return 0;

	if (iova_info->type == OSS_DMA_MEM_CACHEABLE || iova_info->type == OSS_DMA_PA_CONTIGUOUS) {
		return iova_info->bus_addr + page * PAGE_SIZE;
	} else if (iova_info->type == OSS_DMA_ALLOW_DMA_NOT_CONTIGUOUS) {
		struct scatterlist *sg;
		int i = 0;
		int j = 0, nr_page;

		for_each_sg(iova_info->sg, sg, iova_info->sg_cnt, i) {
			nr_page = sg_dma_len(sg) >> PAGE_SHIFT;
			j += nr_page;
			if (page < j)
				return sg_dma_address(sg) + ((page - (j - nr_page)) << PAGE_SHIFT);
		}
	}

	return 0;
}
