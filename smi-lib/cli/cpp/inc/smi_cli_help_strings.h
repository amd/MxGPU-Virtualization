/* * Copyright (C) 2025 Advanced Micro Devices. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <string>
#include "smi_cli_argument.h"

std::string copyright_message =
	"Copyright 2025 Advanced Micro Devices, Inc. All rights reserved.\n\n";
std::string help_common =
	"usage: amd-smi help \n\n"
	"AMD System Management Interface | %s\n\n"
	"AMD-SMI Commands:\n"
	"                          Descriptions\n";
static const std::map<std::string, std::map<std::string, std::vector<std::string>>>
help_supported_command_map = {
	{
		"gpu_nic_common", {
			{"common", {"list", "static", "metric"}}
		}
	},
	{
		"gpu", {
			{"common", {"version", "monitor"}},
			{"windows_host", {"bad-pages", "event", "firmware", "profile"}},
			{"linux_host", {"bad-pages", "event", "firmware"}},
			{"bm", {"firmware", "process", "set", "reset"}},
			{"guest", {"process", "set", "reset"}},
			{"host_mi3xx", {"set", "reset", "xgmi", "topology", "partition", "ras"}},
			{"host_mi350", {"node"}},
			{"host_mi200", {"set", "reset", "xgmi", "topology"}},
			{"host_spec", {"set", "reset"}}
		}
	}
};
std::string version_common =
	"usage: amd-smi version [-h | --help] [--json | --csv] [--file FILE]\n\n"
	"Display information about current version of the tool\n\n"
	"Version arguments:\n"
	"                      Description:\n"
	"    -h, --help        show this help message and exit\n\n";
std::string usage_list_common =
	"usage: amd-smi list";
std::string list_usage_message =
	"List all devices and VFs on the system and their most basic general information.\n"
	"If no device is specified, returns basic information for all devices on the system.\n\n";
std::string list_common =
	"List arguments:" +
	SmiCliArgument::get_description_continuation_indent() + "Description\n";
static const std::map<std::string, std::map<std::string, std::vector<std::string>>>
list_argument_vectors_map = {
	{
		"gpu", {
			{"common", {"vf"}}
		}
	}
};
std::string common_gpu =
	"\nGPU arguments:" +
	SmiCliArgument::get_description_continuation_indent() + "Description\n";
std::string common_nic =
	"\nNIC arguments:" +
	SmiCliArgument::get_description_continuation_indent() + "Description\n";
std::string usage_static_common =
	"usage: amd-smi static";
std::string static_usage_message =
	"\nGets static information about specific device\n"
	"If no argument is provided, returns information for all devices on the system\n"
	"If no static information argument is provided all static information will be displayed\n\n";
std::string static_common =
	"Static arguments:" +
	SmiCliArgument::get_description_continuation_indent() + "Description\n";
static const std::map<std::string, std::map<std::string, std::vector<std::string>>>
static_argument_vectors_map = {
	{
		"gpu_nic_common", {
			{"common", {"asic", "bus", "driver"}},
			{"host_linux", {"numa"}}
		}
	},
	{
		"gpu", {
			{"common", {"ifwi"}},
			{"host_windows", {"board", "limit", "ras", "dfc-ucode", "fb-info", "num-vf", "vram", "cache", "virtualization-mode"}},
			{"host_mi3xx", {"partition", "xgmi-plpd", "soc-pstate"}},
			{"bm", {"limit", "process-isolation"}},
			{"host_linux", {"board", "limit", "fb-info", "num-vf", "vram"}},
			{"host_vf", { "vf"}},
			{"host_linux_spec", {"ras", "cache"}}
		}
	},
	{
		"nic", {
			{"host_linux", {"port", "rdma-devices"}}
		}
	}
};
std::string metric_message =
	"\nGets metric information about the specified devices\n"
	"If no argument is provided, returns information for all devices on the system\n"
	"If no metric information argument is provided all metric information will be displayed\n\n";
std::string usage_metric_common =
	"usage: amd-smi metric";
std::string metric_common =
	"Metric arguments:" +
	SmiCliArgument::get_description_continuation_indent() + "Description\n";
static const std::map<std::string, std::map<std::string, std::vector<std::string>>>
metric_argument_vectors_map = {
	{
		"gpu", {
			{"common", {"watch_time", "iterations", "usage"}},
			{"host", {"power", "clock", "temperature", "pcie"}},
			{"host_vf", {"vf"}},
			{"host_linux_spec", {"ecc", "ecc-block", "energy"}},
			{"guest", {"fb-usage"}},
			{"bm", {"fb-usage", "power", "clock", "temperature", "ecc", "pcie"}}
		}
	},
	{
		"nic", {
			{"host_linux", {"port", "rdma-devices"}}
		}
	},
	{
		"vf", {
			{"host_vf", {"schedule", "guard", "guest-data"}},
			{"host_linux_mi3xx_vf", {"per-partition"}}
		}
	}
};

std::string bad_pages_common = "";
std::string bad_pages_message =
	"usage: amd-smi bad-pages [-h | --help] [--json | --csv] [--file FILE] [-g | --gpu <gpu_index | gpu_bdf | gpu_uuid>]\n\n"
	"Gets bad page information about the specified GPU.\n"
	"If no GPU is specified, returns bad page information for all GPUs on the system.\n"
	"If no argument is provided, returns information for all GPUs on the system.\n\n";
std::string bad_pages_usage_host = "";
std::string bad_pages_host =
	"Bad-pages arguments:\n"
	"                                                       Description:\n"
	"    -h, --help                                         show this help message and exit\n"
	"    -g, --gpu=<gpu_index | gpu_bdf | gpu_uuid>         Select a GPU ID, BDF or UUID, if not selected it will return for all GPUs\n\n";
std::string firmware_common =
	"Firmware arguments:\n"
	"                                                                Description:\n"
	"    -h, --help                                                  show this help message and exit\n"
	"    -g, --gpu=<gpu_index | gpu_bdf | gpu_uuid>                  Select a GPU ID, BDF or UUID, if not selected it will return for all GPUs\n";

std::string firmware_message =
	"Gets firmware information about the specified GPU\n"
	"If no argument is provided, returns information for all GPUs on the system\n"
	"If no GPU is specified, returns firmware information for all GPUs on the system.\n\n";
std::string firmware_usage_common =
	"usage: amd-smi firmware [-h | --help] [--json | --csv] [--file FILE] [-g | --gpu <gpu_index | gpu_bdf | gpu_uuid>]\n";
std::string firmware_usage_host =
	"                        [--fw-list] [--error-records]\n\n";
std::string firmware_usage_bm =
	"                        [--fw-list]\n\n";
std::string firmware_host =
	"    --fw-list                                                   All firmware list information\n"
	"    --error-records                                             All error records information\n"
	"    --vf=<gpu_index:vf_index | vf_bdf | vf_uuid>                Gets firmware information about the specified VF\n"
	"    vf arguments:\n"
	"        --fw-list                                               All firmware list information\n\n";
std::string firmware_bm =
	"    --fw-list                                                   All firmware list information\n";
std::string process_common = "";
std::string process_usage_common = "";
std::string process_message =
	"Lists general process information running on the specified GPU\n"
	"If no argument is provided, returns information for all GPUs on the system\n"
	"If no argument is provided all process information will be displayed\n\n";
std::string process_bm =
	"Process arguments:\n"
	"                                                          Description:\n"
	"    -h, --help                                            show this help message and exit\n"
	"    -g, --gpu=<gpu_index | gpu_bdf | gpu_uuid>            Select a GPU ID, BDF or UUID, if not selected it will return for all GPUs\n"
	"    -w, --watch INTERVAL                                  Reprint the command in a loop of INTERVAL seconds\n"
	"                                                          Looping stops by entering 'CTRL' + 'C'\n"
	"                                                          JSON and CSV formats cannot be printed in stdout\n"
	"    -W, --watch_time TIME                                 The total TIME to watch the given command\n"
	"                                                          Looping stops by entering 'CTRL' + 'C'\n"
	"                                                          If not specified the program will run indefinitely\n"
	"    -i, --iterations ITERATIONS                           Total number of ITERATIONS to loop on the given command\n"
	"                                                          Looping stops by entering 'CTRL' + 'C'\n"
	"                                                          If not specified the program will run indefinitely\n"
	"    --general                                             pid, process name, memory usage\n"
	"    --engine                                              All engine usages\n"
	"    --pid                                                 Gets all process information about the specified process based on Process ID\n"
	"                                                          Multiple pid can be specified and tool will return information for all of them.\n"
	"                                                          Example amd-smi process --pid=<pid1> --pid=<pid2>\n"
	"    --name                                                Gets all process information about the specified process based on Process Name\n"
	"                                                          If multiple processes have the same name information is returned for all of them\n"
	"                                                          Multiple name can be specified and tool will return information for all of them\n"
	"                                                          Example amd-smi process --name=<name1> --name=<name2>\n\n";
std::string process_usage_bm =
	"usage: amd-smi process [-h | --help] [--json | --csv] [--file FILE] [-g | --gpu <gpu_index | gpu_bdf | gpu_uuid>]\n"
	"                       [-w | --watch INTERVAL] [-W | --watch_time TIME] [-i | --iterations ITERATIONS]\n"
	"                       [-G | --general] [-e | --engine]\n"
	"                       [--pid PID] [--name NAME]\n\n";
std::string profile_common = "";
std::string profile_usage_common = "";
std::string profile_message =
	"Displays information about all profiles and current profile\n"
	"If no argument is provided, returns information for all GPUs on the system\n\n";
std::string profile_host_windows =
	"Profile arguments:\n"
	"                                                     Description:\n"
	"    -h, --help                                       show this help message and exit\n"
	"    -g, --gpu=<gpu_index | gpu_bdf | gpu_uuid>       Select a GPU ID, BDF or UUID, if not selected it will return for all GPUs\n";
std::string profile_usage_host_windows =
	"usage: amd-smi profile [-h | --help] [--json | --csv] [--file FILE] [-g | --gpu <gpu_index | gpu_bdf | gpu_uuid>]\n\n";
std::string event_common = "";
std::string event_host =
	"Event arguments:\n"
	"                                                       Description:\n"
	"    -h, --help                                         show this help message and exit\n"
	"    -g, --gpu=<gpu_index | gpu_bdf | gpu_uuid>         Select a GPU ID, BDF or UUID, if not selected it will return for all GPUs\n\n";
std::string event_usage_common = "";
std::string event_usage_host =
	"usage: amd-smi event [-h | --help] [--json | --csv] [--file FILE] [-g | --gpu <gpu_index | gpu_bdf | gpu_uuid>]\n\n"
	"Displays event information for GPU\n"
	"If no argument is provided, returns event informations for all GPUs on the system\n\n";
std::string xgmi_common = "";
std::string xgmi_usage_common = "";
std::string xgmi_message =
	"Displays XGMI capabilities, framebuffer sharing and metric information\n"
	"If no argument is provided, returns information for all GPUs on the system\n\n";
std::string xgmi_usage_host =
	"usage: amd-smi xgmi [-h | --help] [--json] [--file FILE] [-g | --gpu <gpu_index | gpu_bdf | gpu_uuid>]\n"
	"                    [--caps] [--fb-sharing] [--metric]\n\n";
std::string xgmi_usage_host_mi200 =
	"usage: amd-smi xgmi [-h | --help] [--json] [--file FILE] [-g | --gpu <gpu_index | gpu_bdf | gpu_uuid>]\n"
	"                    [--caps] [--fb-sharing]\n\n";
std::string xgmi_host =
	"Xgmi arguments:\n"
	"                                                       Description:\n"
	"    -h, --help                                         show this help message and exit\n"
	"    -g, --gpu=<gpu_index | gpu_bdf | gpu_uuid>         Select a GPU ID, BDF or UUID, if not selected it will return for all GPUs\n"
	"    --caps                                             XGMI capabilities\n"
	"    --fb-sharing                                       Framebuffer sharing for each mode\n"
	"    --metric                                           Metric XGMI information\n"
	"    --source-status                                    Source GPU status information\n"
	"    --link-status                                      XGMI link status between two GPUs in the xgmi command \n\n";
std::string xgmi_host_mi200 =
	"Xgmi arguments:\n"
	"                                                       Description:\n"
	"    -h, --help                                         show this help message and exit\n"
	"    -g, --gpu=<gpu_index | gpu_bdf | gpu_uuid>         Select a GPU ID, BDF or UUID, if not selected it will return for all GPUs\n"
	"    --caps                                             XGMI capabilities\n"
	"    --fb-sharing                                       Framebuffer sharing for each mode\n";
std::string topology_common = "";
std::string topology_usage_common = "";
std::string topology_usage_host =
	"usage: amd-smi topology [-h | --help] [--json] [--file FILE] [-g | --gpu <gpu_index | gpu_bdf | gpu_uuid>]\n"
	"                        [--weight] [--hops] [--fb-sharing] [--link-type]\n"
	"                        [--coherent] [--atomics] [--bi-dir] [--dma]\n\n";
std::string topology_message =
	"Displays link topology information\n"
	"If no argument is provided, returns information for all GPUs on the system\n\n";
std::string topology_host =
	"Topology arguments:\n"
	"                                                       Description:\n"
	"    -h, --help                                         show this help message and exit\n"
	"    -g, --gpu=<gpu_index | gpu_bdf | gpu_uuid>         Select a GPU ID, BDF or UUID, if not selected it will return for all GPUs\n"
	"    --weight                                           Current weight information\n"
	"    --hops                                             Current hops information\n"
	"    --fb-sharing                                       Current framebuffer sharing information\n"
	"    --link-type                                        Link type information\n"
	"    --coherent                                         Cache coherent information\n"
	"    --atomics                                          32 and 64-bit atomic link capability information\n"
	"    --bi-dir                                           bi-directional link capability information\n"
	"    --dma                                              dma link capability information\n";

std::string set_common = "";
std::string set_usage_common = "";
std::string set_message = "";
std::string set_usage_host =
	"usage: amd-smi set [-h | --help] --num-vf=<NUM_VF> [-g=<GPU> | --gpu=<GPU>]\n\n";
std::string set_usage_host_mi300 =
	"usage: amd-smi set [-h | --help] [-xgmi --fb-sharing-mode=[MODE] --group[<GPUx, GPUy>]]\n"
	"                   [--memory-partition [PARTITION_MODE]] [ --accelerator-partition [PROFILE_INDEX]] [ --power-cap [POWER_CAP_VALUE]]\n"
	"                   [--xgmi-plpd [XGMI_PLPD_VALUE]] [--num-vf=<NUM_VF> [-g=<GPU> | --gpu=<GPU>]] [ --soc-pstate [SOC_PSTATE_VALUE]]\n\n";
std::string set_usage_host_mi200 =
	"usage: amd-smi set [-h | --help] [-xgmi --fb-sharing-mode=[MODE] --group[<GPUx, GPUy>]]\n";
std::string set_usage_bm =
	"usage: amd-smi set [-h | --help]\n\n";
std::string set_host =
	"Set arguments:\n"
	"                                                                                           Description:\n"
	"    -h, --help                                                                             show this help message and exit\n"
	"    --num-vf=<num_vf>                                                                      Sets number of VFs\n"
	"    -g=<gpu_id>, --gpu=<gpu_id>                                                            Select a GPU ID, BDF or UUID, if not selected it will set given num of VFs for all GPUs\n";
std::string set_host_mi300 =
	"Set arguments:\n"
	"                                                                                           Description:\n"
	"    -h, --help                                                                             show this help message and exit\n"
	"    --xgmi --fb-sharing-mode=<AmdSmiXgmiFbSharingMode> --group=\"<gpu_id1-gpu_id2>\"       Sets framebuffer sharing mode from group [\"MODE_1\", \"MODE_2\", \"MODE_4\", \"MODE_8\", \"CUSTOM\"]\n"
	"                                                                                           Where, MODE_X represents that X GPUs will be in the same group, linked together:\n"
	"                                                                                           MODE_1 (one GPU in a group), MODE_2 (two GPUs in a group), MODE_4 (four GPUs in a group), MODE_8 (eight GPUs in a group).\n"
	"                                                                                           Note: This command will only work if there's no guest VM running.\n"
	"                                                                                           All possible configurations can be seen by running the amd-smi xgmi command.\n\n"
	"    --memory-partition=<AmdSmiMemoryPartitionSetting>                                      Sets memory partition setting\n"
	"                                                                                           Note: This command will only work if there's no guest VM running.\n"
	"                                                                                           Run 'amd-smi partition' to list memory-partition modes supported on current platform.\n\n"
	"    --accelerator-partition=<profile_index>                                                Sets accelerator partition setting to a mode based on profile_index from partition command\n"
	"                                                                                           Note: This command will only work if there's no guest VM running.\n"
	"                                                                                           All possible configurations can be seen by running the amd-smi partition command.\n\n"
	"    --power-cap=<power_cap_value>                                                          Sets power cap to the provided power cap value.\n"
	"                                                                                           Note: Cap value must be between the minimum (min_power_cap) and maximum (max_power_cap) power cap values.\n"
	"                                                                                           Range of the cap value can be seen by running the amd-smi static command.\n\n"
	"    --num-vf=<num_vf>                                                                      Sets number of VFs\n"
	"    --xgmi-plpd=<xgmi-plpd_value>                                                          Sets xgmi plpd setting to the provided xgmi plpd value.\n"
	"    --soc-pstate=<soc-pstate_value>                                                        Sets soc pstate setting to the provided soc pstate value.\n"
	"    -g=<gpu_id>, --gpu=<gpu_id>                                                            Select a GPU ID, BDF or UUID, if not selected it will set given num of VFs for all GPUs\n\n";
std::string set_host_mi200 =
	"Set arguments:\n"
	"                                                                                           Description:\n"
	"    -h, --help                                                                             show this help message and exit\n"
	"    --xgmi --fb-sharing-mode=<AmdSmiXgmiFbSharingMode> --group=\"<gpu_id1-gpu_id2>\"       Sets framebuffer sharing mode from group [\"MODE_1\", \"MODE_2\", \"MODE_4\", \"MODE_8\", \"CUSTOM\"]\n"
	"                                                                                           Where, MODE_X represents that X GPUs will be in the same group, linked together:\n"
	"                                                                                           MODE_1 (one GPU in a group), MODE_2 (two GPUs in a group), MODE_4 (four GPUs in a group), MODE_8 (eight GPUs in a group).\n"
	"                                                                                           Note: This command will only work if there's no guest VM running.\n"
	"                                                                                           All possible configurations can be seen by running the amd-smi xgmi command.\n\n";
std::string set_bm =
	"Set arguments:\n"
	"                                                                                           Description:\n"
	"    -h, --help                                                                             show this help message and exit\n"
	"    --process-isolation=<0 or 1>                                                           Enable or disable the GPU process isolation: 0 for disable and 1 for enable\n\n"
	"    --power-cap=<power_cap_value>                                                          Sets power cap to the provided power cap value.\n"
	"                                                                                           Note: Cap value must be between the minimum (min_power_cap) and maximum (max_power_cap) power cap values.\n"
	"                                                                                           Range of the cap value can be seen by running the amd-smi static command.\n\n";
std::string reset_common = "";
std::string reset_usage_common = "";
std::string reset_usage_linux =
	"usage: amd-smi reset [-h | --help] [-g | --gpu <gpu_index | gpu_bdf | gpu_uuid> <-G | --gpureset >] [--vf=<VF> <--vf-fb>]\n\n";
std::string reset_usage_bm =
	"usage: amd-smi reset [-h | --help]\n\n";
std::string reset_message ="";
std::string reset_host_linux =
	"Reset arguments:\n"
	"                                                                 Description:\n"
	"    -g, --gpu=<gpu_index | gpu_bdf | gpu_uuid>                   Select a GPU ID, BDF or UUID.\n"
	"                                                                 if not selected it will return for all GPUs\n"
	"    --gpu arguments:\n"
	"        -G, --gpureset                                           Reset all GPUs\n"
	"    --vf=<gpu_index:vf_index | vf_bdf | vf_uuid>                 Cleanup VF FB for the specified VF\n"
	"                                                                 If no argument is provided, returns tool exception\n"
	"    vf arguments:\n"
	"        --vf-fb                                                  Cleanup VF FB for the specified VF\n\n";
std::string reset_bm =
	"Reset arguments:\n"
	"                                                                 Description:\n"
	"    --clean-local-data                                           Clean up data in LDS/GPRs\n\n";

std::string monitor_message =
	"Monitor a target device for the specified arguments.\n"
	"If no arguments are provided, all arguments will be enabled.\n"
	"Use the watch arguments to run continuously\n\n";
std::string monitor_usage_common =
	"usage: amd-smi monitor [-h | --help] [--json | --csv] [--file FILE]\n"
	"                       [-w | --watch INTERVAL] [-W | --watch_time TIME] [-i | --iterations ITERATIONS]\n"
	"                       [-u | --gfx] [-m | mem] [-n | --encode] [-e | --ecc] [-r | --pcie]\n";
std::string monitor_usage_host =
	"                       [-p | --power-usage] [-t | --temperature] [-d | --decoder]\n";
std::string monitor_usage_guest =
	"                       [-u | --vram-usage] [-q | --process]\n";
std::string monitor_usage_bm =
	"                       [-p | --power-usage] [-t | --temperature] [-q | --process]\n";
std::string monitor_common =
	"Monitor arguments:\n"
	"                                                        Description:\n"
	"    -h, --help                                          show this help message and exit\n"
	"    -g, --gpu=<gpu_index | gpu_bdf | gpu_uuid>          Select a GPU ID, BDF or UUID, if not selected it will return for all GPUs\n"
	"    -w, --watch INTERVAL                                Reprint the command in a loop of INTERVAL seconds\n"
	"                                                        Looping stops by entering 'CTRL' + 'C'\n"
	"                                                        JSON and CSV formats cannot be printed in stdout\n"
	"    -W, --watch_time TIME                               The total TIME to watch the given command\n"
	"                                                        Looping stops by entering 'CTRL' + 'C'\n"
	"                                                        If not specified the program will run indefinitely\n"
	"    -i, --iterations ITERATIONS                         Total number of ITERATIONS to loop on the given command\n"
	"                                                        Looping stops by entering 'CTRL' + 'C'\n"
	"                                                        If not specified the program will run indefinitely\n"
	"    -u, --gfx                                           Monitor graphics utilization (%) and clock (MHz)\n"
	"    -m, --mem                                           Monitor memory utilization (%) and clock (MHz)\n"
	"    -n, --encoder                                       Monitor encoder utilization (%) and clock (MHz)\n"
	"    -e, --ecc                                           Monitor ECC single bit, ECC double bit\n"
	"    -r, --pcie                                          Monitor PCIe bandwidth in Mb/s and PCIe replay error count\n";
std::string monitor_host =
	"    -p, --power-usage                                   Monitor power usage in Watts\n"
	"    -t, --temperature                                   Monitor temperature in Celsius\n"
	"    -d, --decoder                                       Monitor decoder utilization (%) and clock (MHz)\n\n";
std::string monitor_guest =
	"    -v, --vram-usage                                    Monitor memory usage in MB\n"
	"    -q, --process                                       Include process output underneath monitor output\n\n";
std::string monitor_bm =
	"    -p, --power-usage                                   Monitor power usage in Watts\n"
	"    -t, --temperature                                   Monitor temperature in Celsius\n"
	"    -q, --process                                       Include process output underneath monitor output\n\n";
std::string partition_common = "";
std::string partition_host =
	"Partition arguments:\n"
	"                                                        Description:\n"
	"    -h, --help                                          Show this help message and exit\n"
	"    -g, --gpu=<gpu_index | gpu_bdf | gpu_uuid>          Select a GPU ID, BDF or UUID, if not selected it will return for all GPUs\n"
	"    -c, --current                                       Displays current memory and accelerator partition mode\n"
	"    -m, --memory                                        Displays caps and current memory partition setting\n"
	"    -a, --accelerator                                   Displays caps and current accelerator partition setting.\n"
	"    -gl, --global                                       Displays global partitioning setting.\n\n";
std::string partition_usage_common = "";
std::string partition_usage_host =
	"usage: amd-smi partition [-h | --help] [--file FILE] [-g | --gpu <gpu_index | gpu_bdf | gpu_uuid>]\n"
	"                         [-c | --current] [-m | --memory] [-a | --accelerator] [-gl | --global]\n\n";
std::string partition_message =
	"Displays partition information about specific GPU.\n"
	"If no GPU is provided, returns information for all GPUs on the system\n"
	"If no partition information argument is provided all partition information will be displayed\n\n";
std::string command_modifiers =
	"Command Modifiers:\n"
	"                      Description:\n"
	"--json                Displays output in JSON format (human readable by default).\n"
	"--csv                 Displays output in CSV format (human readable by default, GPU only).\n"
	"--file FILE           Saves output into a file on the provided path (stdout by default).\n";
std::string xgmi_topology_modifiers =
	"Command Modifiers:\n"
	"                      Description:\n"
	"--json                Displays output in JSON format (human readable by default).\n"
	"--file FILE           Saves output into a file on the provided path (stdout by default).\n";
std::string metric_modifiers =
	"Command Modifiers:\n"
	"                      Description:\n"
	"--json                Displays output in JSON format (human readable by default).\n"
	"--csv                 Displays output in CSV format (human readable by default).\n"
	"                      It can be used only with one argument and cannot be used without or with more than one argument, in that case, the call will fail (GPU only)\n"
	"--file FILE           Saves output into a file on the provided path (stdout by default).\n";
std::string partition_modifiers =
	"Command Modifiers:\n"
	"                      Description:\n"
	"--file FILE           Saves output into a file on the provided path (stdout by default).\n";

std::string ras_usage_message =
	"\nGets ras information. \n"
	"For --cper operations: If no GPU is provided, returns information for all GPUs on the system\n"
	"For --afid operations: GPU filtering is not supported (operates on CPER files)\n"
	"The use of command modifiers is not supported for afid and cper\n\n";

std::string usage_ras_host =
	"usage: amd-smi ras [-h | --help] [--cper] [--severity=[fatal, nonfatal-uncorrected, nonfatal-corrected, all]] [--folder=[FOLDER]] "
	"[--file-limit=[NUMBER_OF_FILES]] [--follow] [-g | --gpu <gpu_index | gpu_bdf | gpu_uuid>] \n"
	"       amd-smi ras [-h | --help] [--afid] [--cper-file=[FILE]] \n"
	"       amd-smi ras [-h | --help] [--policy] \n";

std::string ras_host = "Ras arguments:\n"
	"                                                                                                    Description:\n"
	"    -h, --help                                                                                      show this help message and exit\n"
	"    -g, --gpu=<gpu_index | gpu_bdf | gpu_uuid>                                                      Select a GPU ID, BDF or UUID (only valid with --cper)\n"
	"    --cper --severity=<fatal, nonfatal-uncorrected, nonfatal-corrected, all> --folder=[FOLDER]      Get ras cper errors and saved in file based on severity. \n"
	"           --file-limit=<number_of_files> --follow                                                  Supports GPU filtering. If --folder not provided, no files dumped. \n"
	"                                                                                                    By default, dumps cper report currently cached in driver. \n"
	"                                                                                                    If --file-limit=<number> specified, CLI keeps max <number> files. \n"
	"                                                                                                    If --follow specified, continuous monitoring until ctrl+c pressed.\n"
	"    --afid --cper-file=[FILE]                                                                       Get AFID list from existing CPER file (GPU filtering not supported)\n"
	"    --policy                                                                                        Ras policy information(GPU filtering is supported)\n";

std::string usage_ras_common = "";
std::string ras_common = "";
std::string node_common = "";
std::string node_mi350 =
	"Node arguments:\n"
	"                                 Description:\n"
	"    -h, --help                   Show this help message and exit\n"
	"    -b, --baseboard              Show baseboard information\n"
	"    -p, --power-management       Show power management information\n\n";
std::string usage_node = "";
std::string usage_node_mi350 =
	"usage: amd-smi node [-h | --help] [--json | --csv] [--file FILE]\n"
	"                    [--b] [--baseboard]  [-p] [--power-management]\n\n";
