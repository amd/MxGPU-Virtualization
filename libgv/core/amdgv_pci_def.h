/*
 * Copyright (c) 2017-2021 Advanced Micro Devices, Inc. All rights reserved.
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
 * THE SOFTWARE.
 */

#ifndef AMDGV_PCI_DEF_H
#define AMDGV_PCI_DEF_H

/* PCI CAP ID of PCIE */
#define PCI_CAP_ID__PCIE 0x10
/* PCIE Extended CAP ID*/
#define PCIE_EXT_CAP_ID__AER		 0x01
#define PCIE_EXT_CAP_ID__VENDOR_SPECIFIC 0x0B
#define PCIE_EXT_CAP_ID__ATS         0x0F
#define PCIE_EXT_CAP_ID__SRIOV		 0x10
#define PCIE_EXT_CAP_ID__REBAR		 0x24	/* Resizable BAR */

/* PCI CFG space registers */
#define PCI_COMMAND 0x04
#define PCI_STATUS  0x06

/* PCIE block registers */
#define PCIE_DEVICE_CONTROL  0x8
#define PCIE_DEVICE_STATUS   0xA
#define PCIE_DEVICE_CONTROL2 0x28
#define PCIE_DEVICE_CAP2     0x24

/* PCIE AER block registers */
#define PCIE_EXT_AER_UNCOR_STATUS   0x4
#define PCIE_EXT_AER_UNCOR_MASK	    0x8
#define PCIE_EXT_AER_COR_ERR_STATUS 0x10

#define PCIE_EXT_SRIOV_CAP_VF_10BIT_TAG     0x4  /* VF 10-Bit Tag Requester Supported */

#define PCIE_EXT_SRIOV_CTRL_VFE     0x0001  /* VF Enable */
#define PCIE_EXT_SRIOV_CTRL_MSE     0x0008  /* VF Memory Space Enable */
#define PCIE_EXT_SRIOV_CTRL_VF_10BIT_TAG  0x0020  /* VF 10-Bit Tag Requester Enable */

/* PCIE SRIOV block register */
#define PCIE_EXT_SRIOV_CAP	 0x04
#define PCIE_EXT_SRIOV_CTRL	 0x08
#define PCIE_EXT_SRIOV_INITIALVF 0x0C
#define PCIE_EXT_SRIOV_TOTALVF	 0x0E
#define PCIE_EXT_SRIOV_NUM_VF	 0x10

/* PCIE ATS block registers */
#define PCI_ATS_CTRL		0x06
#define PCI_ATS_CTRL_ENABLE	0x8000

/* PCI bit fields definitions */
#define PCI_COMMAND__BUS_MASTER_EN	   0x0004
#define PCI_COMMAND__INT_DIS		   0x0400
#define PCIE_DEVICE_CONTROL__BCR_FLR	   0x8000
#define PCIE_DEVICE_STATUS__TRANS_PEND	   0x0020
#define PCIE_DEVICE_CONTROL2__ATOMICOP_REQ 0x0040
#define PCIE_DEVICE_CAP2__ATOMIC_COMP32    0x0080
#define PCIE_DEVICE_CAP2__ATOMIC_COMP64    0x0100

/* Misc definitions  */
#define PCIE_EXT_SRIOV_SIZE 0x40

/* PCIe registers */
#define PCI_COMMAND_INTX_DISABLE 0x400
#define PCI_COMMAND_MASTER	 0x4

#define PCI_CAP_ID_EXP		     0x10
#define PCI_ERR_UNCOR_STATUS	     4
#define PCI_EXP_DEVCTL		     8
#define PCI_EXP_DEVCTL_BCR_FLR	     0x8000
#define PCI_EXP_DEVSTA		     10
#define PCI_EXP_DEVSTA_TRPND	     0x0020
#define PCI_EXP_LNKCTL		     16
#define PCI_EXP_SLTCTL		     24
#define PCI_EXP_RTCTL		     28
#define PCI_EXP_DEVCTL2		     40
#define PCI_EXP_DEVCTL2_ATOMICOP_REQ 0x0040
#define PCI_EXP_LNKCTL2		     48
#define PCI_EXP_SLTCTL2		     56
#define PCI_EXP_LNKSTA	             0x12

/* PCIe bit fields definitions */
#define PCI_EXP_LNKSTA_CLS       0x00F /* Current link speed type */
#define PCI_EXP_LNKSTA_NLW       0x3F0 /* Current link width */
#define PCI_CAP_ID_MSI 0x05

#define PCI_CAP_ID_MSIX 0x11
#define PCI_MSIX_FLAGS      2   /* Message Control */
#define PCI_MSIX_FLAGS_ENABLE  0x8000  /* MSI-X enable */

#define PCI_EXT_CAP_ID_ERR   0x01 /* AER */
#define PCI_ERR_UNCOR_STATUS 4
#define PCI_ERR_UNCOR_MASK   8 /* Uncorrectable Error Mask */
#define PCI_ERR_COR_STATUS   16

#define PCI_EXT_CAP_ID_VNDR 0x0B

#endif
