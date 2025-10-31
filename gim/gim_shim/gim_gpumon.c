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

#include <linux/string.h>
#include <linux/rtc.h>
#include <linux/pci_regs.h>
#include <linux/version.h>

#include "gim.h"
#include "gim_gpumon.h"
#include "amdgv_gpumon.h"
#include "gim_debug.h"

#define GIM_GPUMON_HOUR_IN_SEC		(3600)
#define GIM_GPUMON_MINUTE_IN_SEC	(60)

#define GIM_GPUMON_LAST_INIT_START_STR	"\t\tVF Last Initialization Start: "
#define GIM_GPUMON_LAST_INIT_FIN_STR	"\t\tVF Last Initialization Finish: "
#define GIM_GPUMON_LAST_SHUT_START_STR	"\t\tVF Last Shutdown Start: "
#define GIM_GPUMON_LAST_SHUT_FIN_STR	"\t\tVF Last Shutdown Finish: "
#define GIM_GPUMON_LAST_RESET_STR	"\t\tVF Last Reset At: "
#define GIM_GPUMON_STR_FORMAT		"%04d-%02d-%02d %02d:%02d:%02d\n"

#define gim_gpumon_set_info(fmt, s...)	sprintf(result, fmt, ##s)
#define gim_gpumon_get_hour(sec)	((sec) / GIM_GPUMON_HOUR_IN_SEC)
#define gim_gpumon_get_minute(sec)	(((sec) % GIM_GPUMON_HOUR_IN_SEC) \
						/ (GIM_GPUMON_MINUTE_IN_SEC))
#define gim_gpumon_get_second(sec)	((sec) % GIM_GPUMON_MINUTE_IN_SEC)

static int gim_gpumon_get_pcie_link(struct gim_dev_data *dev_data, int *speed,
				int *width)
{
/*
In case of older kernel without higher speed decode support,
use hard-coded speed cap definition for decoding.

pci link caps definition

PCI_EXP_LNKCAP		0x0c	Link Capabilities
 PCI_EXP_LNKCAP_SLS	0x0000000f Supported Link Speeds
 PCI_EXP_LNKCAP_SLS_2_5GB 0x00000001 LNKCAP2 SLS Vector bit 0
 PCI_EXP_LNKCAP_SLS_5_0GB 0x00000002 LNKCAP2 SLS Vector bit 1
 PCI_EXP_LNKCAP_SLS_8_0GB 0x00000003 LNKCAP2 SLS Vector bit 2
 PCI_EXP_LNKCAP_SLS_16_0GB 0x00000004 LNKCAP2 SLS Vector bit 3
 PCI_EXP_LNKCAP_SLS_32_0GB 0x00000005 LNKCAP2 SLS Vector bit 4
 PCI_EXP_LNKCAP_SLS_64_0GB 0x00000006 LNKCAP2 SLS Vector bit 5

PCI_EXP_LNKCAP2		0x2c	Link Capabilities 2
 PCI_EXP_LNKCAP2_SLS_2_5GB	0x00000002 Supported Speed 2.5GT/s
 PCI_EXP_LNKCAP2_SLS_5_0GB	0x00000004 Supported Speed 5GT/s
 PCI_EXP_LNKCAP2_SLS_8_0GB	0x00000008 Supported Speed 8GT/s
 PCI_EXP_LNKCAP2_SLS_16_0GB	0x00000010 Supported Speed 16GT/s
 PCI_EXP_LNKCAP2_SLS_32_0GB	0x00000020 Supported Speed 32GT/s
 PCI_EXP_LNKCAP2_SLS_64_0GB	0x00000040 Supported Speed 64GT/s
 PCI_EXP_LNKCAP2_CROSSLINK	0x00000100 Crosslink supported
 */
	uint32_t lnkcap, lnkcap2;

	*speed = 0;
	*width = 0;

	pcie_capability_read_dword(dev_data->pdev, PCI_EXP_LNKCAP2, &lnkcap2);
	if (lnkcap2) {
		/* check PCI_EXP_LNKCAP2 for link speed cap*/
		/* match PCI_EXP_LNKCAP2_SLS_* */
		if (lnkcap2 & 0x00000040)
			*speed = 64000;
		else if (lnkcap2 & 0x00000020)
			*speed = 32000;
		else if (lnkcap2 & 0x00000010)
			*speed = 16000;
		else if (lnkcap2 & 0x00000008)
			*speed =  8000;
		else if (lnkcap2 & 0x00000004)
			*speed = 5000;
		else if (lnkcap2 & 0x00000002)
			*speed = 2500;
	}
	pcie_capability_read_dword(dev_data->pdev, PCI_EXP_LNKCAP, &lnkcap);
	if ((*speed == 0) && lnkcap) {
		/* fallback to PCI_EXP_LNKCAP if PCI_EXP_LNKCAP2 not available */
		/* match PCI_EXP_LNKCAP_SLS */
		if (lnkcap & 0x00000006)
			*speed = 64000;
		else if (lnkcap & 0x00000005)
			*speed = 32000;
		else if (lnkcap & 0x00000004)
			*speed = 16000;
		else if (lnkcap & 0x00000003)
			*speed = 8000;
		else if (lnkcap & 0x00000002)
			*speed = 5000;
		else if (lnkcap & 0x00000001)
			*speed = 2500;
	}

	pcie_capability_read_dword(dev_data->pdev, PCI_EXP_LNKCAP, &lnkcap);
	if (lnkcap)
		*width = (lnkcap & PCI_EXP_LNKCAP_MLW) >> 4;

	return GIM_GPUMON_ERR_SUCCESS;
}

int gim_gpumon_get_pcie_confs(struct gim_dev_data *dev_data, int *speed, int *width, int *max_vf)
{
	int ret;

	ret = gim_gpumon_get_pcie_link(dev_data, speed, width);
	if (ret)
		return ret;

	*max_vf = pci_sriov_get_totalvfs(dev_data->pdev);
	return 0;
}

/* return micro second in utc time */
uint64_t gim_gpumon_ktime_to_utc(uint64_t ktime)
{
	uint64_t ktime_now;
	uint64_t utc_now;
	uint64_t utc;

	/* Timestamp for kernel time to UTC time conversion */
	ktime_now = (uint64_t) ktime_to_us(ktime_get());
	utc_now = (uint64_t) ktime_to_us(ktime_get_real());

	utc = utc_now + ktime - ktime_now;

	return utc;
}
