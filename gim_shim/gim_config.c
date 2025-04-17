/*
 * Copyright (c) 2014-2019 Advanced Micro Devices, Inc. All rights reserved.
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
#include <linux/ctype.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/pci.h>
#include <linux/mod_devicetable.h>
#include <linux/fs.h>
#include <linux/version.h>

#include "gim.h"
#include "gim_debug.h"
#include "gim_config.h"
#include "amdgv_api.h"

#define _STR(x) #x
#define STR(x) _STR(x)

/* Options defined in configure file */
struct gim_conf_opt {
	const char *name;

	int         value[AMDGV_MAX_GPU_NUM];
	int         repeat_val_idx;

	const int   min;
	const int   max;
	const int   def;
	const bool  array;
	bool        changed;
	bool        persistent;
};

struct gim_conf_opt conf_opts[] = {
	[CONF_OPT_VF_NUMBER] = { .name = VF_NUMBER__KEY,
				 .value = { VF_NUMBER__DEFAULT },
				 .repeat_val_idx = 1,
				 .min = VF_NUMBER__START,
				 .max = VF_NUMBER__MAX,
				 .def = VF_NUMBER__DEFAULT,
				 .persistent = true,
				 .array = true },
	[CONF_OPT_FW_LOAD_TYPE] = { .name = FW_LOAD_TYPE__KEY,
				    .value = { FW_LOAD_TYPE__DEFAULT },
				    .repeat_val_idx = 1,
				    .min = FW_LOAD_TYPE__START,
				    .max = FW_LOAD_TYPE__MAX,
				    .def = FW_LOAD_TYPE__DEFAULT,
				    .array = true },
	[CONF_OPT_LOG_LEVEL] = { .name = LOG_LEVEL__KEY,
				 .value = { LOG_LEVEL__DEFAULT },
				 .repeat_val_idx = 1,
				 .min = LOG_LEVEL__START,
				 .max = LOG_LEVEL__MAX,
				 .def = LOG_LEVEL__DEFAULT,
				 .array = true },
	[CONF_OPT_GUARD] = { .name = GUARD__KEY,
			     .value = { GUARD__DEFAULT },
			     .repeat_val_idx = 1,
			     .min = GUARD__START,
			     .max = GUARD__MAX,
			     .def = GUARD__DEFAULT,
			     .array = true },
	[CONF_OPT_SCH_POLICY] = { .name = SCH_POLICY__KEY,
				  .value = { SCH_POLICY__DEFAULT },
				  .repeat_val_idx = 1,
				  .min = SCH_POLICY__START,
				  .max = SCH_POLICY__MAX,
				  .def = SCH_POLICY__DEFAULT,
				  .array = true },
	[CONF_OPT_FULLACCESS_TIMEOUT] = { .name = FULLACCESS_TIMEOUT__KEY,
					  .value = { FULLACCESS_TIMEOUT__DEFAULT },
					  .repeat_val_idx = 1,
					  .min = FULLACCESS_TIMEOUT__START,
					  .max = FULLACCESS_TIMEOUT__MAX,
					  .def = FULLACCESS_TIMEOUT__DEFAULT,
					  .array = true },
	[CONF_OPT_PERF_MON_EN] = { .name = PERF_MON_ENABLE__KEY,
				   .value = { PERF_MON_ENABLE__DEFAULT },
				   .repeat_val_idx = 1,
				   .min = PERF_MON_ENABLE__START,
				   .max = PERF_MON_ENABLE__MAX,
				   .def = PERF_MON_ENABLE__DEFAULT,
				   .array = true },
	[CONF_OPT_SKIP_CHECK_BGPU] = { .name = SKIP_CHECK_BGPU__KEY,
				       .value = { SKIP_CHECK_BGPU__DEFAULT },
				       .repeat_val_idx = 1,
				       .min = SKIP_CHECK_BGPU__START,
				       .max = SKIP_CHECK_BGPU__MAX,
				       .def = SKIP_CHECK_BGPU__DEFAULT,
				       .array = true },
	[CONF_OPT_POWER_SAVING_MODE] = { .name = POWER_SAVING_MODE__KEY,
					 .value = { POWER_SAVING_MODE__DEFAULT },
					 .repeat_val_idx = 1,
					 .min = POWER_SAVING_MODE__START,
					 .max = POWER_SAVING_MODE__MAX,
					 .def = POWER_SAVING_MODE__DEFAULT,
					 .array = false },
	[CONF_OPT_MM_POLICY] = { .name = MM_POLICY__KEY,
				 .value = { MM_POLICY__DEFAULT },
				 .repeat_val_idx = 1,
				 .min = MM_POLICY__START,
				 .max = MM_POLICY__MAX,
				 .def = MM_POLICY__DEFAULT,
				 .array = true },
	[CONF_OPT_HANG_DUMP_TIMEOUT] = { .name = HANG_DUMP_TIMEOUT__KEY,
					 .value = { HANG_DUMP_TIMEOUT__DEFAULT },
					 .repeat_val_idx = 1,
					 .min = HANG_DUMP_TIMEOUT__START,
					 .max = HANG_DUMP__MAX,
					 .def = HANG_DUMP_TIMEOUT__DEFAULT,
					 .array = true },
	/* Only support single mode for whole hive */
	[CONF_OPT_FB_SHARING_MODE] = {	.name = FB_SHARING_MODE__KEY,
					.value  = {FB_SHARING_MODE__DEFAULT},
					.repeat_val_idx = 1,
					.min = FB_SHARING_MODE__START,
					.max = FB_SHARING_MODE__MAX,
					.def = FB_SHARING_MODE__DEFAULT,
					.array = false },
	/* Only support single mode for whole hive */
	[CONF_OPT_ACCELERATOR_PARTITION_MODE] = {	.name = ACCELERATOR_PARTITION__KEY,
					.value  = {ACCELERATOR_PARTITION__DEFAULT},
					.repeat_val_idx = 1,
					.min = ACCELERATOR_PARTITION__START,
					.max = ACCELERATOR_PARTITION__MAX,
					.def = ACCELERATOR_PARTITION__DEFAULT,
					.persistent = true,
					.array = false },
	/* Only support single mode for whole hive */
	[CONF_OPT_MEMORY_PARTITION_MODE] = {	.name = MEMORY_PARTITION_MODE__KEY,
					.value  = {MEMORY_PARTITION_MODE__DEFAULT},
					.repeat_val_idx = 1,
					.min = MEMORY_PARTITION_MODE__START,
					.max = MEMORY_PARTITION_MODE__MAX,
					.def = MEMORY_PARTITION_MODE__DEFAULT,
					.persistent = true,
					.array = false },
	/* control per partition full access*/
	[CONF_OPT_PARTITION_FULL_ACCESS_EN] = {	.name = PARTITION_FULL_ACCESS_ENABLE__KEY,
					.value = {PARTITION_FULL_ACCESS_ENABLE__DEFAULT},
					.repeat_val_idx = 1,
					.min = PARTITION_FULL_ACCESS_ENABLE__START,
					.max = PARTITION_FULL_ACCESS_ENABLE__MAX,
					.def = PARTITION_FULL_ACCESS_ENABLE__DEFAULT,
					.array = false },
	[CONF_OPT_HANG_DETECTION_MODE] = {	.name = HANG_DETECTION_MODE__KEY,
					.value  = {HANG_DETECTION_MODE__DEFAULT},
					.repeat_val_idx = 1,
					.min = HANG_DETECTION_MODE__START,
					.max = HANG_DETECTION_MODE__MAX,
					.def = HANG_DETECTION_MODE__DEFAULT,
					.array = true },
	[CONF_OPT_ASYMMETRIC_FB_MODE] = {	.name = ASYMMETRIC_FB_MODE__KEY,
					.value  = {ASYMMETRIC_FB_MODE__DEFAULT},
					.repeat_val_idx = 1,
					.min = ASYMMETRIC_FB_MODE__START,
					.max = ASYMMETRIC_FB_MODE__MAX,
					.def = ASYMMETRIC_FB_MODE__DEFAULT,
					.array = true },
	[CONF_OPT_DEBUG_DUMP_RESERVE_SIZE] = { .name = DEBUG_DUMP_RESERVE_SIZE__KEY,
			      .value = { DEBUG_DUMP_RESERVE_SIZE__DEFAULT },
			      .repeat_val_idx = 1,
			      .min = DEBUG_DUMP_RESERVE_SIZE__START,
			      .max = DEBUG_DUMP_RESERVE_SIZE__MAX,
			      .def = DEBUG_DUMP_RESERVE_SIZE__DEFAULT,
			      .array = true },
	[CONF_OPT_DEFERRED_FULL_LIVE_UPDATE] = { .name = DEFERRED_FULL_LIVE_UPDATE__KEY,
					.value = { DEFERRED_FULL_LIVE_UPDATE__DEFAULT },
					.repeat_val_idx = 1,
					.min = DEFERRED_FULL_LIVE_UPDATE__START,
					.max = DEFERRED_FULL_LIVE_UPDATE__MAX,
					.def = DEFERRED_FULL_LIVE_UPDATE__DEFAULT,
					.array = false },
	/* break point mode */
	[CONF_OPT_BP_MODE] = { .name = BP_MODE__KEY,
				 .value = { BP_MODE__DEFAULT },
				 .repeat_val_idx = 1,
				 .min = BP_MODE__START,
				 .max = BP_MODE__MAX,
				 .def = BP_MODE__DEFAULT,
				 .array = false },
	[CONF_OPT_PF_FB_SIZE] = { .name = PF_FB_SIZE__KEY,
					  .value = { PF_FB_SIZE__DEFAULT },
					  .repeat_val_idx = 1,
					  .min = PF_FB_SIZE__START,
					  .max = PF_FB_SIZE__MAX,
					  .def = PF_FB_SIZE__DEFAULT,
					  .array = true },
	[CONF_OPT_BAD_PAGE_RECORD_THRESHOLD] = {.name = BAD_PAGE_RECORD_THRESHOLD__KEY,
					.value  = {BAD_PAGE_RECORD_THRESHOLD__DEFAULT},
					.repeat_val_idx = 1,
					.min = BAD_PAGE_RECORD_THRESHOLD__START,
					.max = BAD_PAGE_RECORD_THRESHOLD__MAX,
					.def = BAD_PAGE_RECORD_THRESHOLD__DEFAULT,
					.array = true },
	[CONF_OPT_RAS_VF_TELEMETRY_POLICY] = { .name = RAS_TELEMETRY_POLICY__KEY,
					       .value = { RAS_TELEMETRY_POLICY__DEFAULT },
					       .repeat_val_idx = 1,
					       .min = RAS_TELEMETRY_POLICY__START,
					       .max = RAS_TELEMETRY_POLICY__MAX,
					       .def = RAS_TELEMETRY_POLICY__DEFAULT,
					       .array = true },
	[CONF_OPT_MAX_CPER_COUNT] = { .name = MAX_CPER_COUNT__KEY,
				      .value = { MAX_CPER_COUNT__DEFAULT },
				      .repeat_val_idx = 1,
				      .min = MAX_CPER_COUNT__START,
				      .max = MAX_CPER_COUNT__MAX,
				      .def = MAX_CPER_COUNT__DEFAULT,
				      .array = true },
};

#define MAX_OPTION (sizeof(conf_opts)/sizeof(struct gim_conf_opt))
#define MAX_CONFIG_FILE_LENGTH 1024

/* Options input from command line */
int vf_num_size;
uint vf_num[AMDGV_MAX_GPU_NUM] = {0};
module_param_array(vf_num, uint, &vf_num_size, 0444);
MODULE_PARM_DESC(vf_num, "Number of VFs to be enabled for the GPUs\n\t"
		"vf_num=[N0[,N1[...[,Nx]]]]\n\t"
		"1 <= Nx <= 31; 1 <= x <= 31\n\t"
		"Nx: enable N VFs for GPUx");

int skip_check_bgpu_size;
uint skip_check_bgpu[AMDGV_MAX_GPU_NUM] = {0};
module_param_array(skip_check_bgpu, uint, &skip_check_bgpu_size, 0444);
MODULE_PARM_DESC(skip_check_bgpu, "whether skip check bad GPU(0:disable 1:enable)\n\t"
		"skip_check_bgpu=[G0[,G1[...[,Gx]]]]\n\t"
		"0 <= Gx <= 1, 0 <= x <= 31\n\t"
		"0: check bad GPU\n\t"
		"1: skip check bad GPU and RMA\n\t"
		"2: skip check RMA only");

int fw_load_type_size;
uint fw_load_type[AMDGV_MAX_GPU_NUM] = {0};
module_param_array(fw_load_type, uint, &fw_load_type_size, 0444);
MODULE_PARM_DESC(fw_load_type, "firmware loading type\n\t"
				"fw_load_type=[L0[,L1[...[,Lx]]]]\n\t"
				"1 <= Lx <= 3; 0 <= x <= 31\n\t"
				"1: VBIOS loads host firmwares\n\t"
				"2: GIM loads host firmwares\n\t"
				"3: GIM loads both host and guest firmwares");

int guard_size;
uint guard[AMDGV_MAX_GPU_NUM] = {1};
module_param_array(guard, uint, &guard_size, 0444);
MODULE_PARM_DESC(guard, "sensitive event guard(0:disable 1:enable)\n\t"
		"guard=[G0[,G1[...[,Gx]]]]\n\t"
		"0 <= Gx <= 1, 0 <= x <= 31\n\t"
		"0: disable sensitive event guard\n\t"
		"1: enable sensitive event guard");

int time_size;
uint fullaccess_timeout[AMDGV_MAX_GPU_NUM] = {0};
module_param_array(fullaccess_timeout, uint, &time_size, 0444);
MODULE_PARM_DESC(fullaccess_timeout, "max time for VF exclusive access\n\t"
			"fullaccess_timeout=[T0[,T1[...[,Tx]]]]\n\t"
			"0 <= Tx <= 500000 (ms), 0 <= x <= 31\n\t"
			"0: use default timeout 3000ms for single VF, 1500ms for multi VF");

int sch_policy_size;
uint sch_policy[AMDGV_MAX_GPU_NUM] = {0};
module_param_array(sch_policy, uint, &sch_policy_size, 0444);
MODULE_PARM_DESC(sch_policy, "specify scheduler for GPUs\n\t"
			"sch_policy=[S0[,S1[...[,Sx]]]]\n\t"
			"1 <= Sx <= 5, 0 <= x <= 31\n\t"
			"1: HW solid mode\n\t"
			"2: HW liquid mode\n\t"
			"3: SW fairness scheduling mode\n\t"
			"4: SW round robin mode\n\t"
			"5: Hybrid liquid mode\n\t");

int log_level_size;
uint log_level[AMDGV_MAX_GPU_NUM] = {0};
module_param_array(log_level, uint, &log_level_size, 0444);
MODULE_PARM_DESC(log_level, "DEBUG LOGGING LEVEL for GPUs\n\t"
				"log_level=[D0[,D1[...[,Dx]]]]\n\t"
				"0 <= Dx <= 4; 0 <= x <= 31\n\t"
				"0: log only error\n\t"
				"1: log only error+warnings\n\t"
				"2: log general info+error+warnings\n\t"
				"3: log all, verbose\n\t"
				"4: log excessive, all+low level\n");

uint mm_policy_size;
uint mm_policy[AMDGV_MAX_GPU_NUM] = {0};
module_param_array(mm_policy, uint, &mm_policy_size, 0444);
MODULE_PARM_DESC(mm_policy, "multimedia bandwidth policy for GPUs\n\t"
				"mm_policy=[D0[,D1[...[,Dx]]]]\n\t"
				"0 <= D <= 1;\n\t"
				"0: multimedia bandwidth policy disabled\n\t"
				"1: multimedia bandwidth policy enabled\n\t");

uint power_saving_mode;
module_param(power_saving_mode, uint, 0444);
MODULE_PARM_DESC(power_saving_mode, "power saving mode for GPUs\n\t"
				"power_saving_mode=D\n\t"
				"0 <= D <= 2;\n\t"
				"0: power saving mode off\n\t"
				"1: power saving mode auto\n\t"
				"2: power saving mode manual\n\t");

uint gpu_data_addr_hi;
module_param(gpu_data_addr_hi, uint, 0444);
MODULE_PARM_DESC(gpu_data_addr_hi, "Physical address hi for transferring gpu data\n\t"
		"gpu_data_addr_hi=0xxxxxxxxx\n\t");

uint gpu_data_addr_lo;
module_param(gpu_data_addr_lo, uint, 0444);
MODULE_PARM_DESC(gpu_data_addr_lo, "Physical address low for transferring gpu data\n\t"
		"gpu_data_addr_lo=0xxxxxxxxx\n\t");

uint gpu_data_size;
module_param(gpu_data_size, uint, 0444);
MODULE_PARM_DESC(gpu_data_size, "Size of transferring gpu data memory\n\t"
		"gpu_data_size=0xxxxxxxxx\n\t");
char *gim_enabled_devices;
 MODULE_PARM_DESC(enabled_devices, "Enabled device strings (will be set like aaaa:xx:yy.z;bbbb:xx:yy.z)");
 module_param_named(enabled_devices, gim_enabled_devices, charp, 0444);

int hangdump_timeout_size;
uint hangdump_timeout[AMDGV_MAX_GPU_NUM] = {0};
module_param_array(hangdump_timeout, uint, &hangdump_timeout_size, 0444);
MODULE_PARM_DESC(hangdump_timeout, "max time for dcore to do hang dump\n\t"
			"hangdump_timeout=[T0[,T1[...[,Tx]]]]\n\t"
			"0 <= Tx <= 60000 (ms), 0 <= x <= 31\n\t"
			"0: use default timeout 10000 ms");

int fb_sharing_mode_size;
uint fb_sharing_mode[AMDGV_MAX_GPU_NUM] = {0};
module_param_array(fb_sharing_mode, uint, &fb_sharing_mode_size, 0444);
MODULE_PARM_DESC(fb_sharing_mode, "FB Sharing mode for GPUs\n\t"
				"fb_sharing_mode=D\n\t"
				"0 <= D <= 2;\n\t"
				"0: Default FB Sharing Mode\n\t"
				"1: Disabled FB Sharing\n\t"
				"2: FB Sharing mode 2\n\t"
				"4: FB Sharing Mode 4\n\t"
				"8: FB Sharing Mode 8\n\t");

int partition_full_access_enable_size;
uint partition_full_access_enable[AMDGV_MAX_GPU_NUM] = {0};
module_param_array(partition_full_access_enable, uint, &partition_full_access_enable_size, 0444);
MODULE_PARM_DESC(partition_full_access_enable, "whether enable partition full access (0:disable 1:enable)\n\t"
				"partition_fullaccess_enable=D\n\t"
				"0 <= D <=1\n\t"
				"0: disable partition full access, use exlcusive full access\n\t"
				"1(default): enable per-partition full access\n\t");

int hang_detection_mode_size;
uint hang_detection_mode[AMDGV_MAX_GPU_NUM] = {0};
module_param_array(hang_detection_mode, uint, &hang_detection_mode_size, 0444);
MODULE_PARM_DESC(hang_detection_mode, "Hang detection enablement for GPUs which support hang detection\n\t"
				"hang_detection_mode=[D0[,D1[...[,Dx]]]]\n\t"
				"0 <= D <= 1, 0 <= x <= 31\n\t"
				"1(default): enable hang detection mode, default strategy\n\t"
				"0: disable hang detection\n\t");

int debug_dump_size_array_size;
uint debug_dump_reserved_size[AMDGV_MAX_GPU_NUM] = {0};
module_param_array(debug_dump_reserved_size, uint, &debug_dump_size_array_size, 0444);
MODULE_PARM_DESC(debug_dump_reserved_size, "Auto Scheduler debug dump reserved size in MB\n\t"
				"debug_dump_reserved_size=[D0[,D1[...[,Dx]]]]\n\t"
				"0 <= D <= 256 (MB), 0 <= x <= 31\n\t");

uint deferred_full_live_update;
module_param(deferred_full_live_update, uint, 0444);
MODULE_PARM_DESC(deferred_full_live_update, "whether enable deferred full live update(0:disable 1:enable)\n\t"
				"deferred_full_live_update=D\n\t"
				"By default, it is disabled\n\t"
				"If enabled, gim will trigger a whole gpu reset when all VMs are shutdown\n\t"
				"to finish a full host driver update\n\t"
				"0: deferred full live update disabled\n\t"
				"1: deferred full live update enabled\n\t");

int asymmetric_fb_mode_size;
uint asymmetric_fb_mode[AMDGV_MAX_GPU_NUM] = {0};
module_param_array(asymmetric_fb_mode, uint, &asymmetric_fb_mode_size, 0444);
MODULE_PARM_DESC(asymmetric_fb_mode, "Asymmetric FB enablement for GPUs which support asymmetric mode\n\t"
				"asymmetric_fb_mode=[D0[,D1[...[,Dx]]]]\n\t"
				"0 <= D <= 1, 0 <= x <= 31\n\t"
				"1(default): enable asymmetric FB\n\t"
				"0: disable asymmetric FB\n\t");

uint bp_mode;
module_param(bp_mode, uint, 0444);
MODULE_PARM_DESC(bp_mode, "Whether enable BP mode, disabled by default\n\t"
				"bp_mode=D\n\t"
				"0 <= D <= 2;\n\t"
				"0: BP Mode 0, Disable BP Mode\n\t"
				"1: BP Mode 1, GFX world switch pauses at first PF Run\n\t"
				"2: BP Mode 2, Driver pauses at every world switch command and wait for user input\n\t");

int pf_fb_size_param_number;
uint pf_fb_size[AMDGV_MAX_GPU_NUM] = {0};
module_param_array(pf_fb_size, uint, &pf_fb_size_param_number, 0444);
MODULE_PARM_DESC(pf_fb_size, "PF reserved frame buffer size in MB\n\t"
				"pf_fb_size=[D0[,D1[...[,Dx]]]]\n\t"
				"256 <= Dx <= 1024 (MB), 0 <= x <= 31\n\t");

int bad_page_threshold_size;
uint bad_page_threshold[AMDGV_MAX_GPU_NUM] = {0};
module_param_array(bad_page_threshold, uint, &bad_page_threshold_size, 0444);
MODULE_PARM_DESC(bad_page_threshold, "Bad page threshold for GPUs\n\t"
				"bad_page_threshold=[D0[,D1[...[,Dx]]]]\n\t"
				"10 <= Dx <= 256;\n\t");

int ras_vf_telemetry_policy_size;
uint ras_vf_telemetry_policy[AMDGV_MAX_GPU_NUM] = {0};
module_param_array(ras_vf_telemetry_policy, uint, &ras_vf_telemetry_policy_size, 0444);
MODULE_PARM_DESC(ras_vf_telemetry_policy, "RAS VF Telemetry Policy\n\t"
		"ras_vf_telemetry_policy=[D0[,D1[...[,Dx]]]]\n\t"
		"0 <= D <= 2, 0 <= x <= 31\n\t"
		"0: Start VF logging on Guest load. Reset VF counts on guest re-load. (Default)\n\t"
		"1: Start VF logging on Host load. Do not reset VF counts on guest re-load\n\t"
		"2: Disable RAS VF Telemetry\n\t");

int max_cper_count_size;
uint max_cper_count[AMDGV_MAX_GPU_NUM] = {0};
module_param_array(max_cper_count, int, &max_cper_count_size, 0444);
MODULE_PARM_DESC(max_cper_count, "Max CPER Count\n\t"
	"max_cper_count=[D0[,D1[...[,Dx]]]]\n\t"
	"-1 <= D <= " STR(AMDGV_CPER_MAX_ALLOWED_COUNT) ", 0 <= x <= 31\n\t"
	"-1: Disable CPERs\n\t"
	"0: Driver default count: " STR(AMDGV_CPER_MAX_ALLOWED_COUNT) " CPERs (Default)\n\t"
	"1-" STR(AMDGV_CPER_MAX_ALLOWED_COUNT) ": Custom max CPER count\n\t");


static int gim_conf_search_config_key(char *key)
{
	int index;
	int ret = -1;

	for (index = 0 ; index < MAX_OPTION; ++index) {
		if (strncasecmp(conf_opts[index].name,
					key,
					strlen(conf_opts[index].name)) == 0) {
			ret = index;
			break;
		}
	}
	return ret;
}

static int gim_conf_valid_opt(int index, int value)
{
	int ret = -1;

	if (index >= CONF_OPT_START && index <  CONF_OPT_MAX) {
		if (value >= conf_opts[index].min
				&& value <= conf_opts[index].max)
			ret = 0;
	}

	return ret;
}

static void gim_conf_set_conf_opt(int index, int value)
{
	if (index >= CONF_OPT_START
			&& index <  CONF_OPT_MAX) {
		conf_opts[index].value[0] = value;

		gim_dbg("AMD GIM %s = %d\n",
				conf_opts[index].name, value);
	}
}

/* i.e. vf_num=[N0[,N1[...[,Nx]]]] */
static void gim_handle_array_token(char *token, int index)
{
	char *token_val;
	int value = conf_opts[index].def;
	int ret;
	int i = 0;

	gim_dbg("AMD GIM %s = %s\n", conf_opts[index].name, token);
	while (i < AMDGV_MAX_GPU_NUM) {
		token_val = strsep(&token, ",\0");

		if (token_val) {
			ret = kstrtoint(token_val, 10, &value);
			if ((gim_conf_valid_opt(index, value) != 0) || ret) {
				gim_warn("invalid token(%s) value: %s\n",
					conf_opts[index].name, token_val);
				conf_opts[index].value[i] =
						conf_opts[index].def;
				gim_warn("change to default value: %d\n",
					conf_opts[index].def);
				value = conf_opts[index].def;
			} else {
				conf_opts[index].value[i] = value;
			}
		} else {
			break;
		}

		i++;
	};

	conf_opts[index].repeat_val_idx = i;

	if (i < AMDGV_MAX_GPU_NUM)
		for (; i < AMDGV_MAX_GPU_NUM; i++)
			conf_opts[index].value[i] = value;
}

static void gim_conf_create_conf_file(void)
{
	struct file *config;
	config = filp_open(GIM_CONFIG_PATH, O_CREAT, 0444);
	if (IS_ERR_OR_NULL(config))
		gim_warn("failed to create config file.");

	filp_close(config, NULL);
}

static int gim_conf_trunc_conf_file(bool fill_default)
{
	struct file *config;
	int index, id;

	config = filp_open(GIM_CONFIG_PATH, O_WRONLY | O_TRUNC, 0);
	if (IS_ERR_OR_NULL(config)) {
		gim_warn("failed to truncate config file.");
		return PTR_ERR(config);
	}

	if (fill_default) {
		for (index = CONF_OPT_START; index < CONF_OPT_MAX; index++) {
			if (conf_opts[index].array) {
				for (id = 0; id < AMDGV_MAX_GPU_NUM; id++)
					conf_opts[index].value[id] = conf_opts[index].def;
			} else {
				conf_opts[index].value[0] = conf_opts[index].def;
			}
		}
	}

	filp_close(config, NULL);
	return 0;
}

static int gim_conf_parse_conf_file(char *data, unsigned long long size)
{
	const char delimiters[] = " =\n\t";
	char *token;
	char *running;
	int  empty;
	int  index;

	running = data;
	do {
		/* Get the key. */
		/* Get an non-empty token(including NULL token). */
		do {
			empty = 0;
			token = strsep(&running, delimiters);
			if (token)
				empty = (strlen(token) == 0);
		} while (empty);

		/* expecting a key. */
		if (token != NULL) {
			index = gim_conf_search_config_key(token);
			if (index < 0) {
				/* unknown token found, Error in the
				 * config file, exit.
				 */
				gim_warn("AMD GIM unknown token: %s. "
					"Config file is invalid and will be truncated.\n", token);
				return gim_conf_trunc_conf_file(true);
			}

			/* Get the value. */
			/* Get an non-empty token(including NULL token). */
			do {
				empty = 0;
				token = strsep(&running, delimiters);

				if (token)
					empty = (strlen(token) == 0);
			} while (empty);

			/* expecting a value. */
			if (token != NULL) {
				int value;
				int ret;

				if (conf_opts[index].array) {
					gim_handle_array_token(token, index);
					continue;
				}

				ret = kstrtoint(token, 10, &value);
				if (ret) {
					gim_warn("Failed to transfer str: %s\n",
								token);
				}

				if (!conf_opts[index].persistent) {
					gim_warn("AMD GIM non-persistent token: %s. "
						"Config file is outdated and will be truncated.\n",
						conf_opts[index].name);
					return gim_conf_trunc_conf_file(true);
				}

				if (gim_conf_valid_opt(index, value) == 0)
					conf_opts[index].value[0] = value;

				gim_dbg("AMD GIM %s = %d\n",
						conf_opts[index].name,
						conf_opts[index].value[0]);
			}
		}
	} while (token != NULL);

	return 0;
}

static void gim_conf_init_persist_config(void)
{
	int index, id;

	for (index = CONF_OPT_START; index < CONF_OPT_MAX; index++) {
		if (index != CONF_OPT_VF_NUMBER && conf_opts[index].persistent) {
			if (conf_opts[index].array) {
				for (id = 0; id < AMDGV_MAX_GPU_NUM; id++)
					conf_opts[index].value[id] = conf_opts[index].max;
			} else {
				conf_opts[index].value[0] = conf_opts[index].max;
			}
		}
	}
}

static void gim_conf_clear_saved_persist_config(bool config_file_created)
{
	int index, id;

	/* Persistent configs are only saved in the config file and cannot be
	 * overwritten via module params.
	 * If vf_num has changed, all saved persistent configs are considered
	 * invalid and should be reset to default values. However, if the config
	 * is just created on current driver loading, do not reset the values.
	 */
	if (config_file_created)
		conf_opts[CONF_OPT_VF_NUMBER].changed = false;

	if (conf_opts[CONF_OPT_VF_NUMBER].changed) {
		for (index = CONF_OPT_START; index < CONF_OPT_MAX; index++) {
			if (index != CONF_OPT_VF_NUMBER && conf_opts[index].persistent) {
				if (conf_opts[index].array) {
					for (id = 0; id < AMDGV_MAX_GPU_NUM; id++)
						conf_opts[index].value[id] = conf_opts[index].def;
				} else {
					conf_opts[index].value[0] = conf_opts[index].def;
				}
			}
		}
		conf_opts[CONF_OPT_VF_NUMBER].changed = false;
	}
}

static long gim_conf_read_conf_file(void)
{
	long ret, r = 0;
	unsigned long long size;
	struct file *config;
	char *content;
	loff_t pos = 0;

	config = filp_open(GIM_CONFIG_PATH, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(config))
		return PTR_ERR(config);

	size = i_size_read(file_inode(config)) + 1;
	content = kzalloc(size, GFP_KERNEL);
	if (!content)
		return -ENOMEM;

	ret = gim_kernel_read(config, content, size, pos);
	if (ret != (size - 1)) {
		r = ret == 0 ? -EINVAL : ret;
		goto exit;
	}
	gim_conf_parse_conf_file(content, size);
exit:
	kfree(content);
	filp_close(config, NULL);
	return r;
}

static long gim_conf_write_conf_file(char *content)
{
	long ret = 0;
	struct file *config;
	loff_t pos = 0;
	config = filp_open(GIM_CONFIG_PATH, O_RDWR, 0);
	if (IS_ERR_OR_NULL(config))
		return PTR_ERR(config);

	ret = gim_kernel_write(config, content, strlen(content), pos);
	if (ret != strlen(content))
		return ret == 0 ? -EINVAL : ret;
	vfs_fsync(config, 0);
	filp_close(config, NULL);

	return 0;
}

int gim_conf_save(void)
{
	char *buf;
	int  index;
	int  cursor = 0;
	int  i = 0;
	long ret;

	buf = vmalloc(MAX_CONFIG_FILE_LENGTH);
	if (!buf) {
		gim_warn("Cannot vmalloc memory\n");
		return -ENOMEM;
	}
	memset(buf, 0, MAX_CONFIG_FILE_LENGTH);
	for (index = 0; index < MAX_OPTION; ++index) {
		if (!conf_opts[index].persistent)
			continue;

		if (conf_opts[index].array) {
			cursor += snprintf(buf + cursor,
					MAX_CONFIG_FILE_LENGTH - cursor,
					"%s=%d",
					conf_opts[index].name,
					conf_opts[index].value[0]);

			for (i = 1; i < AMDGV_MAX_GPU_NUM
				&& i < conf_opts[index].repeat_val_idx; i++)
				cursor += snprintf(buf + cursor,
					MAX_CONFIG_FILE_LENGTH - cursor,
					",%d",
					conf_opts[index].value[i]);

			cursor += snprintf(buf + cursor,
					MAX_CONFIG_FILE_LENGTH - cursor,
					"\n");
		} else {
			cursor += snprintf(buf + cursor,
				MAX_CONFIG_FILE_LENGTH - cursor,
				"%s=%d\n",
				conf_opts[index].name,
				conf_opts[index].value[0]);
		}

		if (cursor >= MAX_CONFIG_FILE_LENGTH)
			break;
	}

	gim_info("configurations:\n%s", buf);

	ret = gim_conf_write_conf_file(buf);
	if (ret != 0 && ret != -ENOENT)
		gim_warn("writing to conf file failed.\n");

	vfree(buf);
	return 0;
}

static void set_array_value(uint index, uint *array_val, uint size)
{
	int i, len;
	uint value;
	char buf[512];
	struct gim_conf_opt *opt;

	value = 0;
	len = 0;
	opt = &conf_opts[index];
	for (i = 0; (i < size) && (i < AMDGV_MAX_GPU_NUM); i++) {
		if (len >= 512) {
			len = 512;
			break;
		}
		value = array_val[i];
		if (gim_conf_valid_opt(index, value)) {
			gim_warn("invalid token(%s) value: %u\n",
					opt->name, value);
			value = opt->def;
			gim_warn("change to default value: %u\n",
					value);
		}
		if (value != opt->value[i]) {
			opt->changed = true;
			gim_dbg("%s changed from %d to %u\n",
					opt->name, opt->value[i], value);
			opt->value[i] = value;
		}

		len += snprintf(buf + len, 16, "%d,", value);
	}
	if (len > 0)
		buf[len-1] = '\0';

	opt->repeat_val_idx = i;
	for (; i < AMDGV_MAX_GPU_NUM; i++)
		opt->value[i] = value;


	gim_dbg("AMD GIM %s = %s\n", opt->name, buf);
}

int gim_conf_init(void)
{
	int i, j, ret;
	struct gim_conf_opt *opt;
	bool config_file_created = false;

	/* set default value for array options */
	for (i = 0; i < MAX_OPTION; i++) {
		opt = &conf_opts[i];
		if (!opt->array)
			continue;
		for (j = 0; j < AMDGV_MAX_GPU_NUM; j++)
			opt->value[j] = opt->def;
	}

	/* load options from config file. */
	ret = gim_conf_read_conf_file();
	if (ret == -ENOENT) {
		/* distinguish the case when no saved config is found. */
		gim_conf_create_conf_file();
		gim_conf_init_persist_config();
		config_file_created = true;
	} else if (ret != 0)
		gim_warn("reading from conf file failed.\n");

	/* options of command line override options in config file. */
	if (vf_num_size > 0)
		set_array_value(CONF_OPT_VF_NUMBER, vf_num, vf_num_size);

	if (skip_check_bgpu_size > 0)
		set_array_value(CONF_OPT_SKIP_CHECK_BGPU, skip_check_bgpu, skip_check_bgpu_size);

	if (guard_size > 0)
		set_array_value(CONF_OPT_GUARD, guard, guard_size);

	if (time_size > 0)
		set_array_value(CONF_OPT_FULLACCESS_TIMEOUT,
				fullaccess_timeout, time_size);

	if (sch_policy_size > 0)
		set_array_value(CONF_OPT_SCH_POLICY,
				sch_policy, sch_policy_size);

	if (fw_load_type_size > 0)
		set_array_value(CONF_OPT_FW_LOAD_TYPE,
				fw_load_type, fw_load_type_size);

	if (log_level_size > 0)
		set_array_value(CONF_OPT_LOG_LEVEL,
				log_level, log_level_size);

	if (power_saving_mode > 0) {
		if (gim_conf_valid_opt(CONF_OPT_POWER_SAVING_MODE,
					power_saving_mode)) {
			gim_warn("invalid token (power_saving_mode) value: %d\n",
				power_saving_mode);
			power_saving_mode = POWER_SAVING_MODE__DEFAULT;
		}
		for (j = 0; j < AMDGV_MAX_GPU_NUM; j++)
			conf_opts[CONF_OPT_POWER_SAVING_MODE].value[j] = power_saving_mode;
	}

	if (mm_policy_size > 0)
		set_array_value(CONF_OPT_MM_POLICY,
				mm_policy, mm_policy_size);

	if (fb_sharing_mode_size > 0)
		set_array_value(CONF_OPT_FB_SHARING_MODE,
				fb_sharing_mode, fb_sharing_mode_size);

	if (partition_full_access_enable_size > 0)
		set_array_value(CONF_OPT_PARTITION_FULL_ACCESS_EN,
				partition_full_access_enable, partition_full_access_enable_size);

	if (hangdump_timeout_size > 0) {
		set_array_value(CONF_OPT_HANG_DUMP_TIMEOUT,
				hangdump_timeout, hangdump_timeout_size);
	}

	if (debug_dump_size_array_size > 0)
		set_array_value(CONF_OPT_DEBUG_DUMP_RESERVE_SIZE,
				debug_dump_reserved_size, debug_dump_size_array_size);

	if (hang_detection_mode_size > 0)
		set_array_value(CONF_OPT_HANG_DETECTION_MODE,
				hang_detection_mode, hang_detection_mode_size);

	if (deferred_full_live_update > 0) {
		if (gim_conf_valid_opt(CONF_OPT_DEFERRED_FULL_LIVE_UPDATE,
					deferred_full_live_update)) {
			gim_warn("invalid token (deferred_full_live_update) value: %d\n",
				deferred_full_live_update);
			deferred_full_live_update = DEFERRED_FULL_LIVE_UPDATE__DEFAULT;
		}
		for (j = 0; j < AMDGV_MAX_GPU_NUM; j++)
			conf_opts[CONF_OPT_DEFERRED_FULL_LIVE_UPDATE].value[j] = deferred_full_live_update;
	}

	if (asymmetric_fb_mode_size > 0)
		set_array_value(CONF_OPT_ASYMMETRIC_FB_MODE,
				asymmetric_fb_mode, asymmetric_fb_mode_size);

	/* only mode0 ~ mode2 supported */
	if (bp_mode > 0) {
		if (gim_conf_valid_opt(CONF_OPT_BP_MODE, bp_mode)) {
			gim_warn("invalid token (bp_mode) value: %d\n",
				bp_mode);
			bp_mode = BP_MODE__DEFAULT;
		}
		for (j = 0; j < AMDGV_MAX_GPU_NUM; j++)
			conf_opts[CONF_OPT_BP_MODE].value[j] = bp_mode;
	}

	if (pf_fb_size_param_number > 0) {
		set_array_value(CONF_OPT_PF_FB_SIZE,
				pf_fb_size, pf_fb_size_param_number);
	}

	if (bad_page_threshold_size > 0) {
		set_array_value(CONF_OPT_BAD_PAGE_RECORD_THRESHOLD,
				bad_page_threshold, bad_page_threshold_size);
	}

	if (ras_vf_telemetry_policy_size > 0) {
		set_array_value(CONF_OPT_RAS_VF_TELEMETRY_POLICY,
				ras_vf_telemetry_policy, ras_vf_telemetry_policy_size);
	}

	if (max_cper_count_size > 0) {
		set_array_value(CONF_OPT_MAX_CPER_COUNT,
				max_cper_count, max_cper_count_size);
	}

	gim_conf_clear_saved_persist_config(config_file_created);

	/* save options to config file. */
	ret = gim_conf_save();
	if (ret) {
		gim_warn("gim conf save failed.\n");
		return ret;
	}

	gim_info("INIT CONFIG\n");

	return 0;
}

uint32_t gim_conf_get_vf_num_opt(uint32_t id)
{
	if (id >= AMDGV_MAX_GPU_NUM)
		id = AMDGV_MAX_GPU_NUM - 1;

	return conf_opts[CONF_OPT_VF_NUMBER].value[id];
}

uint32_t gim_conf_get_skip_check_bgpu_opt(uint32_t id)
{
	if (id >= AMDGV_MAX_GPU_NUM)
		id = AMDGV_MAX_GPU_NUM - 1;

	return conf_opts[CONF_OPT_SKIP_CHECK_BGPU].value[id];
}

uint32_t gim_conf_get_guard_opt(uint32_t id)
{
	if (id >= AMDGV_MAX_GPU_NUM)
		id = AMDGV_MAX_GPU_NUM - 1;

	return conf_opts[CONF_OPT_GUARD].value[id];
}

uint32_t gim_conf_get_fullacces_timeout_opt(uint32_t id)
{
	if (id >= AMDGV_MAX_GPU_NUM)
		id = AMDGV_MAX_GPU_NUM - 1;

	return conf_opts[CONF_OPT_FULLACCESS_TIMEOUT].value[id];
}

uint32_t gim_conf_get_sch_policy_opt(uint32_t id)
{
	if (id >= AMDGV_MAX_GPU_NUM)
		id = AMDGV_MAX_GPU_NUM - 1;

	return conf_opts[CONF_OPT_SCH_POLICY].value[id];
}

uint32_t gim_conf_get_fw_load_type_opt(uint32_t id)
{
	if (id >= AMDGV_MAX_GPU_NUM)
		id = AMDGV_MAX_GPU_NUM - 1;

	return conf_opts[CONF_OPT_FW_LOAD_TYPE].value[id];
}

uint32_t gim_conf_get_log_level_opt(uint32_t id)
{
	if (id >= AMDGV_MAX_GPU_NUM)
		id = AMDGV_MAX_GPU_NUM - 1;

	return conf_opts[CONF_OPT_LOG_LEVEL].value[id];
}

uint32_t gim_conf_get_mm_policy_opt(uint32_t id)
{
	if (id >= AMDGV_MAX_GPU_NUM)
		id = AMDGV_MAX_GPU_NUM - 1;

	return conf_opts[CONF_OPT_MM_POLICY].value[id];
}

uint32_t gim_conf_get_perf_mon_enable_opt(uint32_t id)
{
	if (id >= AMDGV_MAX_GPU_NUM)
		id = AMDGV_MAX_GPU_NUM - 1;
	return conf_opts[CONF_OPT_PERF_MON_EN].value[id];
}

uint32_t gim_conf_get_hangdump_timeout_opt(uint32_t id)
{
	if (id >= AMDGV_MAX_GPU_NUM)
		id = AMDGV_MAX_GPU_NUM - 1;

	return conf_opts[CONF_OPT_HANG_DUMP_TIMEOUT].value[id];
}

uint32_t gim_conf_get_fb_sharing_mode_opt(uint32_t id)
{
	if (id >= AMDGV_MAX_GPU_NUM)
		id = AMDGV_MAX_GPU_NUM - 1;

	return conf_opts[CONF_OPT_FB_SHARING_MODE].value[id];
}

uint32_t gim_conf_get_accelerator_partition_mode_opt(uint32_t id)
{
	if (id >= AMDGV_MAX_GPU_NUM)
		id = AMDGV_MAX_GPU_NUM - 1;

	return conf_opts[CONF_OPT_ACCELERATOR_PARTITION_MODE].value[0];
}

uint32_t gim_conf_get_memory_partition_mode_opt(uint32_t id)
{
	if (id >= AMDGV_MAX_GPU_NUM)
		id = AMDGV_MAX_GPU_NUM - 1;

	return conf_opts[CONF_OPT_MEMORY_PARTITION_MODE].value[0];
}

uint32_t gim_conf_get_partition_full_access_enable_opt(uint32_t id)
{
	if (id >= AMDGV_MAX_GPU_NUM)
		id = AMDGV_MAX_GPU_NUM - 1;

	return conf_opts[CONF_OPT_PARTITION_FULL_ACCESS_EN].value[0];
}

uint32_t gim_conf_get_hang_detection_mode_opt(uint32_t id)
{
	if (id >= AMDGV_MAX_GPU_NUM)
		id = AMDGV_MAX_GPU_NUM - 1;

	return conf_opts[CONF_OPT_HANG_DETECTION_MODE].value[id];
}

uint32_t gim_conf_get_asymmetric_fb_mode_opt(uint32_t id)
{
	if (id >= AMDGV_MAX_GPU_NUM)
		id = AMDGV_MAX_GPU_NUM - 1;

	return conf_opts[CONF_OPT_ASYMMETRIC_FB_MODE].value[id];
}

uint32_t gim_conf_get_debug_dump_reserve_size_opt(uint32_t id)
{
	if (id >= AMDGV_MAX_GPU_NUM)
		id = AMDGV_MAX_GPU_NUM - 1;

	return conf_opts[CONF_OPT_DEBUG_DUMP_RESERVE_SIZE].value[id];
}

uint32_t gim_conf_get_deferred_full_live_update_opt(uint32_t id)
{
	if (id >= AMDGV_MAX_GPU_NUM)
		id = AMDGV_MAX_GPU_NUM - 1;

	return conf_opts[CONF_OPT_DEFERRED_FULL_LIVE_UPDATE].value[id];
}

uint32_t gim_conf_get_bp_mode_opt(uint32_t id)
{
	if (id >= AMDGV_MAX_GPU_NUM)
		id = AMDGV_MAX_GPU_NUM - 1;

	return conf_opts[CONF_OPT_BP_MODE].value[id];
}

uint32_t gim_conf_get_pf_fb_size_opt(uint32_t id)
{
	if (id >= AMDGV_MAX_GPU_NUM)
		id = AMDGV_MAX_GPU_NUM - 1;

	return conf_opts[CONF_OPT_PF_FB_SIZE].value[id];
}

uint32_t gim_conf_get_bad_page_record_threshold_opt(uint32_t id)
{
	if (id >= AMDGV_MAX_GPU_NUM)
		id = AMDGV_MAX_GPU_NUM - 1;

	return conf_opts[CONF_OPT_BAD_PAGE_RECORD_THRESHOLD].value[id];
}

uint32_t gim_conf_get_ras_vf_telemetry_policy_opt(uint32_t id)
{
	if (id >= AMDGV_MAX_GPU_NUM)
		id = AMDGV_MAX_GPU_NUM - 1;

	return conf_opts[CONF_OPT_RAS_VF_TELEMETRY_POLICY].value[id];
}

uint32_t gim_conf_get_max_cper_count_opt(uint32_t id)
{
	if (id >= AMDGV_MAX_GPU_NUM)
		id = AMDGV_MAX_GPU_NUM - 1;

	return conf_opts[CONF_OPT_MAX_CPER_COUNT].value[id];
}

uint32_t gim_conf_set_vf_num_opt(int value)
{
	return gim_conf_set_opt(CONF_OPT_VF_NUMBER, value);
}

uint32_t gim_conf_set_accelerator_partition_mode_opt(int value)
{
	return gim_conf_set_opt(CONF_OPT_ACCELERATOR_PARTITION_MODE, value);
}

uint32_t gim_conf_set_memory_partition_mode_opt(int value)
{
	return gim_conf_set_opt(CONF_OPT_MEMORY_PARTITION_MODE, value);
}

uint32_t gim_conf_clear_conf_file(void)
{
	return gim_conf_trunc_conf_file(false);
}

uint32_t gim_conf_set_opt(int index, int value)
{
	uint32_t res = -1;

	if (gim_conf_valid_opt(index, value) == 0) {
		gim_conf_set_conf_opt(index, value);

		if (!conf_opts[index].persistent)
			return 0;

		res = gim_conf_save();
		if (res) {
			gim_warn("gim conf save failed.\n");
			return res;
		}
		res = 0;
	}

	return res;
}
