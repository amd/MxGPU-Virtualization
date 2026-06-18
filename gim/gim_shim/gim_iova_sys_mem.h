/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef GIM_IOVA_MEM_SYS_H
#define GIM_IOVA_MEM_SYS_H
#include <linux/pci.h>


int gim_iova_mem_allocate(struct pci_dev *pdev, uint64_t size, uint64_t align,
			  enum oss_dma_mem_type type,
			  struct oss_dma_mem_info *dma_info);
void gim_iova_mem_free(void *handle);
uint64_t gim_iova_sg_dma_address(void *handle, uint32_t page);
#endif
