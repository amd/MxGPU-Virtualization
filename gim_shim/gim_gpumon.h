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

#ifndef __GIM_GPUMON_H__
#define __GIM_GPUMON_H__
#include <linux/pci.h>
#include <linux/cdev.h>
#include <linux/vmalloc.h>
#include "amdgv_api.h"
#include "amdgv_gpumon.h"

#define LITTLEENDIAN_CPU

#define GIM_GPUMON_CDEV_NAME			"gim_gpumon_monitor"
#define GIM_GPUMON_CLASS_NAME			"gim_gpumon_monitor_class"
#define GIM_GPUMON_DEVICE_FILE_NAME		"amdgim"

#define GIM_GPUMON_MAJOR			(10020)
#define GIM_GPUMON_MINOR_START			(0)
#define GIM_GPUMON_MINOR_LEN			(1)

#define GIM_GPUMON_STRLEN_VERYSHORT		(4)
#define GIM_GPUMON_STRLEN_SHORT			(16)
#define GIM_GPUMON_STRLEN_NORMAL		(32)
#define GIM_GPUMON_STRLEN_LONG			(64)
#define GIM_GPUMON_STRLEN_VERYLONG		(256)

#define GIM_GPUMON_BUFFER_SIZE			(4096)

#define GIM_GPUMON_GPU_ERROR_MSG_SIZE		(256)

#define GIM_GPUMON_STR_YES			"Yes"
#define GIM_GPUMON_STR_NO			"No"
#define GIM_GPUMON_STR_NONE			"None"
#define GIM_GPUMON_STR_ACTIVE			"Active"
#define GIM_GPUMON_STR_AVAILABLE		"Available"
#define GIM_GPUMON_STR_NA			"N/A"
#define GIM_GPUMON_STR_OFF			"OFF"

#define GIM_GPUMON_ERROR_STR_INVALID_CMD	"Error: invalid command."
#define GIM_GPUMON_ERROR_STR_UNEXPECTED		"Error: unexpected error."

#define GIM_GPUMON_GET_PCI_DOMAIN(x)		(((x) & 0xffff0000) >> 16)
#define GIM_GPUMON_GET_PCI_BUS(x)		(((x) & 0x0000FF00) >> 8)
#define GIM_GPUMON_GET_PCI_DEVICE(x)		(((x) & 0x000000F8) >> 3)
#define GIM_GPUMON_GET_PCI_FUNC(x)		(((x) & 0x00000007) >> 0)

#define GIM_GPUMON_FUNC_ENTRY(f)		(f)
#define GIM_GPUMON_TOSTR_TEMP(buf, tmp)		sprintf(buf, "%0.2fC", tmp)
#define GIM_GPUMON_TOSTR_WATT(buf, tmp)		sprintf(buf, "%0.2fW", tmp)
#define GIM_GPUMON_TOSTR_PERCENT(buf, tmp)	sprintf(buf, "%0.2f%%", tmp)
#define GIM_GPUMON_TOSTR_PCI_SPEED(buf, tmp)	sprintf(buf, "%dGT/s", tmp)
#define GIM_GPUMON_TOSTR_PCI_WIDTH(buf, tmp)	sprintf(buf, "x%d", tmp)
#define GIM_GPUMON_TOSTR_PCI_WIDTH(buf, tmp)	sprintf(buf, "x%d", tmp)
#define GIM_GPUMON_TOSTR_PCI_VOLT(buf, tmp)	sprintf(buf, "%0.4fV", tmp)

enum GIM_GPUMON_ERR {
	GIM_GPUMON_ERR_SUCCESS = 0,
	GIM_GPUMON_ERR_UNKNOWN,
	GIM_GPUMON_ERR_INVALID_PARAM,
	GIM_GPUMON_ERR_INVALID_MODE,
	GIM_GPUMON_ERR_CLRVFFB_VF_UNSET,
	GIM_GPUMON_ERR_CLRVFFB_VF_INUSE,

	GIM_GPUMON_ERR_CLRVFFB_VF_FULL_ACCESS,
	GIM_GPUMON_ERR_GETVF_INVALID_FB_OFFSET,
	GIM_GPUMON_ERR_GETVF_INVALID_FB_SIZE,
	GIM_GPUMON_ERR_GETVF_INVALID_GFX_PART,
	GIM_GPUMON_ERR_GETVF_NO_VF_AVAILABLE,

	GIM_GPUMON_ERR_GETVF_OSG_VF_INUSE,
	GIM_GPUMON_ERR_GETVF_OSG_IN_ZZZ,
	GIM_GPUMON_ERR_GETVF_GFX_PART_NOT_ENOUGH,
	GIM_GPUMON_ERR_GETVF_CANT_ALLOC_OFFSET_AND_SIZE,
	GIM_GPUMON_ERR_GETVF_ZZZ_IN_OSG,

	GIM_GPUMON_ERR_RELVF_VF_NOT_FOUND,
	GIM_GPUMON_ERR_RELVF_VF_INUSE,
	GIM_GPUMON_ERR_RELVF_VF_UNSET,
	GIM_GPUMON_ERR_GET_ROM_HEADER_FAIL,
	GIM_GPUMON_ERR_SKIP_ROM_OFFSET_FAIL,

	GIM_GPUMON_ERR_GET_MASTER_TABLE_FAIL,
	GIM_GPUMON_ERR_VFS_SCHEDULE_SWITCH_FAILED,
	GIM_GPUMON_ERR_NO_ENOUGH_QUOTA,
	GIM_GPUMON_ERR_GPUVS_SCLK_INVALID_DIVIDER,

	GIM_GPUMON_ERR_TEST,
	GIM_GPUMON_ERR_MAX
};

enum{
	GIM_GPUMON_ASIC_PCI_DEVICE_ID = 0,
	GIM_GPUMON_ASIC_PCI_VENDOR_ID,
	GIM_GPUMON_ASIC_PCI_REV_ID,
	GIM_GPUMON_ASIC_PCI_LEN
};

enum{
	GIM_GPUMON_ASICINFO_NAME = 0,
	GIM_GPUMON_ASICINFO_CU,
	GIM_GPUMON_ASICINFO_GFX,
	GIM_GPUMON_ASICINFO_VF_PREFIX,
	GIM_GPUMON_ASICINFO_VIDEO_ENCODER,
	GIM_GPUMON_ASICINFO_DPM_CAP,
	GIM_GPUMON_ASICINFO_POWER_CAP,
	GIM_GPUMON_ASICINFO_MAX_VF_NUM,
	GIM_GPUMON_ASICINFO_UNCORR_ECC,
	GIM_GPUMON_ASICINFO_MAX_GFX_CLK,
	GIM_GPUMON_ASICINFO_MAX_MEM_CLK,
	GIM_GPUMON_ASICINFO_LEN
};

enum{
	GIM_GPUMON_COMMAND_GPUINFO = 0,
	GIM_GPUMON_COMMAND_GPUVS,
	GIM_GPUMON_COMMAND_GPUBIOS,
	GIM_GPUMON_COMMAND_GPUVF_PF,
	GIM_GPUMON_COMMAND_GPUVF_VF,
	GIM_GPUMON_COMMAND_CLRVFFB,
	GIM_GPUMON_COMMAND_LEN
};

enum{
	GIM_GPUMON_GPUVF_TOTAL = 0,
	GIM_GPUMON_GPUVF_PF,
	GIM_GPUMON_GPUVF_VF,
	GIM_GPUMON_GPUVF_LEN
};

typedef int gim_gpumon_command_func_t(char *param, void *obj, void *result);
typedef int gim_gpumon_command_to_str_func_t(void *obj, char *str);
struct gim_gpumon_command_op {
	gim_gpumon_command_func_t *func;
};

struct gim_gpumon_command_to_str {
	gim_gpumon_command_to_str_func_t *func;
};

struct gim_gpumon_gpuinfo {
	char name[GIM_GPUMON_STRLEN_NORMAL];
	uint32_t dbdf;
	int dpm_cap;
	int power_cap;
	int fbsize;
	bool uncorr_ecc;
	int max_vf_num;
	char gfx_engine[GIM_GPUMON_STRLEN_NORMAL];
	int max_gfx_clk;
	int max_mem_clk;
	int cu_number;
	char video_encoder[GIM_GPUMON_STRLEN_LONG];
	//pcie_link_speed = pcie_speed / 2
	int pcie_speed;
	int pcie_width;
};

struct gim_gpumon_gpuinfos {
	int gpu_count;
	struct gim_gpumon_gpuinfo gpuinfo[AMDGV_MAX_GPU_NUM];
};

struct gim_gpumon_gpuvs {
	char name[GIM_GPUMON_STRLEN_NORMAL];
	uint32_t dbdf;
	uint32_t power_usage;
	uint32_t cur_volt;
	uint32_t temperature;
	uint32_t cur_dpm;
	uint32_t memory_usage;
	uint32_t available_vf;
	uint32_t gfx_clk;
	uint32_t gfx_usage;
	uint32_t vce_usage;
	uint32_t uvd_usage;
	uint32_t correctable_error;
	uint32_t uncorrectable_error;
};

struct gim_gpumon_gpuvs_group {
	int gpu_count;
	struct gim_gpumon_gpuvs gpuvs[AMDGV_MAX_GPU_NUM];
};

struct gim_gpumon_vbiosinfo {
	char name[GIM_GPUMON_STRLEN_NORMAL];
	uint32_t dbdf;
	char vbios_pn[GIM_GPUMON_STRLEN_LONG];
	uint32_t version;
	char date[GIM_GPUMON_STRLEN_NORMAL];
	char serial[GIM_GPUMON_STRLEN_VERYLONG];
	unsigned short dev_id;
	uint32_t rev_id;
	uint32_t sub_dev_id;
	uint32_t sub_ved_id;
};

struct gim_gpumon_vbiosinfos {
	int gpu_count;
	struct gim_gpumon_vbiosinfo vbiosinfos[AMDGV_MAX_GPU_NUM];
};

struct gim_gpumon_vfinfo {
	int vf_id;
	char vf_name[GIM_GPUMON_STRLEN_NORMAL];
	uint32_t dbdf;
	int vf_state;
	int fb_size;
	int gfx_engine_part;
};

struct gim_gpumon_gpu_vfinfos {
	//type=GIM_GPUMON_GPUVF_PF
	int type;
	uint32_t gpu_id;
	char name[GIM_GPUMON_STRLEN_NORMAL];
	uint32_t dbdf;
	int vf_count;
	struct gim_gpumon_vfinfo vf_info[AMDGV_MAX_VF_NUM];
};

struct gim_gpumon_total_infos {
	//type=GIM_GPUMON_GPUVF_TOTAL
	int type;
	int gpu_count;
	struct gim_gpumon_gpu_vfinfos gpu_vfinfos[AMDGV_MAX_GPU_NUM];
};

struct gim_gpumon_vf_detail {
	//type=GIM_GPUMON_GPUVF_VF
	int type;
	uint32_t gpu_dbdf;
	int gpu_active_vf;
	uint32_t vf_dbdf;
	char vf_name[GIM_GPUMON_STRLEN_NORMAL];
	char vf_type[GIM_GPUMON_STRLEN_NORMAL];
	bool vf_state;
	char guest_driver_version[GIM_GPUMON_STRLEN_LONG];
	uint32_t guest_driver_cert;
	uint64_t vf_running_section;
	uint64_t vf_active_section;
	uint64_t vf_total_active_section;
	struct amdgv_time_log time_log;
	int fbsize;
	int vce_bandwidth;
	int hevc_bandwidth;
};

struct gim_dev_data;
extern char *gim_gpumon_commands[];
extern int gim_gpumon_do_op(char *param, void *object, int index, char *result);

uint64_t gim_gpumon_ktime_to_utc(uint64_t ktime);
int gim_gpumon_get_pcie_confs(struct gim_dev_data *dev_data, int *speed, int *width, int *max_vf);
#endif
