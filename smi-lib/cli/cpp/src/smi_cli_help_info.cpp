/* * Copyright (C) 2024 Advanced Micro Devices. All rights reserved.
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

#include "smi_cli_platform.h"
#include "smi_cli_help_info.h"
#include "smi_cli_helpers.h"
#include "smi_cli_exception.h"

std::string copyright_message =
	"Copyright 2023-2024 Advanced Micro Devices, Inc. All rights reserved.\n\n";
std::string help_common =
	"usage: amd-smi help \n\n"
	"AMD System Management Interface | %s\n\n"
	"AMD-SMI Commands:\n"
	"                      Descriptions:\n"
	"    version           Display version information\n"
	"    list              List GPU information\n"
	"    static            Gets static information about the specified GPU\n"
	"    metric            Gets metric information about the specified GPU\n"
	"    monitor           Monitor metrics for target devices\n";
std::string help_command_windows_host =
	"    bad-pages         Gets bad page information about the specified GPU\n"
	"    event             Displays event information for the given GPU\n"
	"    firmware          Gets firmware information about the specified GPU\n"
	"    profile           Displays information about all profiles and current profile\n";
std::string help_command_linux_host =
	"    bad-pages         Gets bad page information about the specified GPU\n"
	"    event             Displays event information for the given GPU\n"
	"    firmware          Gets firmware information about the specified GPU\n";
std::string help_command_bm =
	"    firmware          Gets firmware information about the specified GPU\n"
	"    process           Lists general process information running on the specified GPU\n"
	"    set               Set options for devices\n"
	"    reset             Reset options for devices\n";
std::string help_command_guest =
	"    process           Lists general process information running on the specified GPU\n"
	"    set               Set options for devices\n"
	"    reset             Reset options for devices\n";
std::string help_command_mi30x_host =
	"    set               Set options for devices\n"
	"    reset             Reset options for devices\n"
	"    xgmi              Displays xgmi information of the devices\n"
	"    topology          Displays topology information of the devices\n"
	"    partition         Displays partition information of the devices\n"
	"    ras               Displays ras information of the devices\n";
std::string help_command_mi200_host =
	"    set               Set options for devices\n"
	"    reset             Reset options for devices\n"
	"    xgmi              Displays xgmi information of the devices\n"
	"    topology          Displays topology information of the devices\n";
std::string version_common =
	"usage: amd-smi version [-h | --help] [--json | --csv] [--file FILE]\n\n"
	"Display information about current version of the tool\n\n"
	"Version arguments:\n"
	"                      Description:\n"
	"    -h, --help        show this help message and exit\n\n";
std::string list_common =
	"usage: amd-smi list [-h | --help] [--json | --csv] [--file FILE] [-g | --gpu [GPU ...]]\n\n"
	"List all GPUs and VFs on the system and their most basic general information.\n"
	"If no GPU is specified, returns metric information for all GPUs on the system.\n\n"
	"List arguments:\n"
	"                          Description:\n"
	"    -h, --help            show this help message and exit\n"
	"    -g, --gpu [GPU ...]   Select a GPU ID, BDF or UUID, if not selected it will return for all GPUs\n\n";
std::string usage_static_common =
	"usage: amd-smi static [-h | --help] [-g | --gpu [GPU ...]] [--json | --csv] [--file FILE]\n"
	"                      [-a | --asic] [-b | --bus] [-V | --vbios] [-d | --driver]\n";
std::string usage_static_hyperv =
	"                      [-p | --partition]\n";
std::string usage_static_host =
	"                      [-l | --limit] [-B | --board] [-r | --ras] [-D | --dfc-ucode] [-f | --fb-info]\n"
	"                      [-n | --num-vf] [-v | --vram] [-c | --cache]\n";
std::string usage_static_host_linux =
	"                      [-l | --limit] [-B | --board] [-r | --ras] [-f | --fb-info]\n"
	"                      [-n | --num-vf] [-v | --vram] [-c | --cache]\n";
std::string usage_static_host_linux_mi200 =
	"                      [-l | --limit] [-B | --board] [-f | --fb-info]\n"
	"                      [-n | --num-vf] [-v | --vram]\n";
std::string usage_vf_static_host =
	"                      [--vf=<gpu_index:vf_index, vf_bdf, vf_uuid>]\n\n";
std::string usage_static_bm =
	"                      [-l | --limit] [-R | --process-isolation]\n\n";
std::string static_usage_message =
	"Gest static informations about specific GPU\n"
	"If no GPU is provided, returns information for all GPUs on the system\n"
	"If no static information argument is provided all static information will be displayed\n\n";
std::string static_common =
	"Static arguments:\n"
	"                                                  Description:\n"
	"    -h, --help                                    show this help message and exit\n"
	"    -g, --gpu [GPU ...]                           Select a GPU ID, BDF or UUID, if not selected it will return for all GPUs\n"
	"    -a, --asic                                    All asic inforamtion\n"
	"    -b, --bus                                     All bus information\n"
	"    -V, --vbios                                   All video bios information\n"
	"    -d, --driver                                  Displays driver version\n";
std::string static_host_windows =
	"    -B, --board                                   All board information\n"
	"    -l, --limit                                   All limit metric values (i.e. power and thermal limits)\n"
	"    -r, --ras                                     Displays ras features information\n"
	"    -D, --dfc-ucode                               All dfc ucode table information\n"
	"    -f, --fb-info                                 All fb information\n"
	"    -n, --num-vf                                  Displays number of supported and enabled VFs\n"
	"    -v, --vram                                    All vram information\n"
	"    -c, --cache                                   All cache info\n";
std::string static_host_vf =
	"    --vf=<gpu_index:vf_index, vf_bdf, vf_uuid>    Gets general information about the specified VF (e.g. timeslice, fb info)\n\n";
std::string static_host_mi30x =
	"    -p, --partition                               Gets current memory and accelerator partition information\n";
std::string static_bm =
	"    --limit                                       All limit metric values (i.e. power and thermal limits)\n"
	"    --process-isolation                           The process isolation status\n\n";
std::string static_host_linux =
	"    -B, --board                                   All board information\n"
	"    -l, --limit                                   All limit metric values (i.e. power and thermal limits)\n"
	"    -r, --ras                                     Displays ras features information\n"
	"    -f, --fb-info                                 All fb information\n"
	"    -n, --num-vf                                  Displays number of supported and enabled VFs\n"
	"    -v, --vram                                    All vram information\n"
	"    -c, --cache                                   All cache info\n";
std::string static_host_linux_mi200 =
	"    -B, --board                                   All board information\n"
	"    -l, --limit                                   All limit metric values (i.e. power and thermal limits)\n"
	"    -f, --fb-info                                 All fb information\n"
	"    -n, --num-vf                                  Displays number of supported and enabled VFs\n"
	"    -v, --vram                                    All vram information\n";
std::string metric_message =
	"Gets metric information about the specified GPU\n"
	"If no argument is provided, returns information for all GPUs on the system\n"
	"If no metric information argument is provided all metric information will be displayed\n\n";
std::string metric_common =
	"Metric arguments:\n"
	"                                                              Description:\n"
	"    -h, --help                                                show this help message and exit\n"
	"    -g, --gpu [GPU ...]                                       Select a GPU ID, BDF or UUID, if not selected it will return for all GPUs\n"
	"    -w, --watch INTERVAL                                      Reprint the command in a loop of INTERVAL seconds\n"
	"                                                              Looping stops by entering 'CTRL' + 'C'\n"
	"                                                              JSON and CSV formats cannot be printed in stdout\n"
	"    -W, --watch_time TIME                                     The total TIME to watch the given command\n"
	"                                                              Looping stops by entering 'CTRL' + 'C'\n"
	"                                                              If not specified the program will run indefinitely\n"
	"    -i, --iterations ITERATIONS                               Total number of ITERATIONS to loop on the given command\n"
	"                                                              Looping stops by entering 'CTRL' + 'C'\n"
	"                                                              If not specified the program will run indefinitely\n"
	"    -u, --usage                                               All usage information\n";
std::string metric_usage_common =
	"usage amd-smi metric [-h | --help] [-g | --gpu [GPU ...]] [--json | --csv] [--file FILE]\n"
	"                     [-w | --watch INTERVAL] [-W | --watch_time TIME] [-i | --iterations ITERATIONS] [-u | --usage]\n";
std::string metric_host =
	"    -p, --power                                               All power readings information\n"
	"    -c, --clock                                               All frequency sensor readings\n"
	"    -t, --temperature                                         All thermal sensor readings\n"
	"    -e, --ecc                                                 All ecc information\n"
	"    -k, --ecc-block                                           Number of ECC errors per block\n"
	"    -P, --pcie                                                Current pcie information\n"
	"    -E, --energy                                              Amount of energy consumed\n"
	"    --vf=<gpu_index:vf_index from list, vf_bdf, vf_uuid>      Gets metric information about the specified VF\n"
	"                                                              If no metric information argument is provided all metric information will be displayed\n"
	"        --schedule                                            All scheduling info\n"
	"        --guard                                               All guard information\n"
	"        --guest-data                                          All guest data information\n\n";
std::string metric_host_mi200 =
	"    -p, --power                                               All power readings information\n"
	"    -c, --clock                                               All frequency sensor readings\n"
	"    -t, --temperature                                         All thermal sensor readings\n"
	"    -P, --pcie                                                Current pcie information\n"
	"    --vf=<gpu_index:vf_index from list, vf_bdf, vf_uuid>      Gets metric information about the specified VF\n"
	"                                                              If no metric information argument is provided all metric information will be displayed\n"
	"        --schedule                                            All scheduling info\n"
	"        --guard                                               All guard information\n"
	"        --guest-data                                          All guest data information\n\n";
std::string metric_usage_host =
	"                     [-p | --power] [-c | --clock] [-t | --temperature] [-e | --ecc] [-P | --pcie] [-E | --energy] [--vf [VF]]\n\n";
std::string metric_usage_host_mi200 =
	"                     [-p | --power] [-c | --clock] [-t | --temperature] [-P | --pcie] [--vf [VF]]\n\n";
std::string metric_guest =
	"    -fb, --fb-usage                                           Total and used framebuffer\n\n";
std::string metric_usage_guest =
	"                     [-fb | --fb-usage]\n\n";
std::string metric_bm =
	"    -fb, --fb-usage                                           Total and used framebuffer\n"
	"    -p, --power                                               All power readings information\n"
	"    -c, --clock                                               All frequency sensor readings\n"
	"    -t, --temperature                                         All thermal sensor readings\n"
	"    -e, --ecc                                                 All ecc information\n"
	"    -P, --pcie                                                Current pcie information\n\n";
std::string metric_usage_bm =
	"                     [-fb | --fb-usage] [-p | --power] [-c | --clock] [-t | --temperature] [-e | --ecc] [-P | --pcie]\n\n";
std::string bad_pages_common = "";
std::string bad_pages_message =
	"usage: amd-smi bad-pages [-h | --help] [--json | --csv] [--file FILE] [-g | --gpu [GPU ...]]\n\n"
	"Gets bad page information about the specified GPU.\n"
	"If no GPU is specified, returns bad page information for all GPUs on the system.\n"
	"If no argument is provided, returns information for all GPUs on the system.\n\n";
std::string bad_pages_usage_host = "";
std::string bad_pages_host =
	"Bad-pages arguments:\n"
	"                                Description:\n"
	"    -h, --help                  show this help message and exit\n"
	"    -g, --gpu [GPU ...]         Select a GPU ID, BDF or UUID, if not selected it will return for all GPUs\n\n";
std::string firmware_common =
	"Firmware arguments:\n"
	"                                                                Description:\n"
	"    -h, --help                                                  show this help message and exit\n"
	"    -g, --gpu [GPU ...]                                         Select a GPU ID, BDF or UUID, if not selected it will return for all GPUs\n";

std::string firmware_message =
	"Gets firmware information about the specified GPU\n"
	"If no argument is provided, returns information for all GPUs on the system\n"
	"If no GPU is specified, returns firmware information for all GPUs on the system.\n\n";
std::string firmware_usage_common =
	"usage: amd-smi firmware [-h | --help] [--json | --csv] [--file FILE] [-g | --gpu [GPU ...]]\n";
std::string firmware_usage_host =
	"                        [--fw-list] [--error-records]\n\n";
std::string firmware_usage_bm =
	"                        [--fw-list]\n\n";
std::string firmware_host =
	"    --fw-list                                                   All firmware list information\n"
	"    --error-records                                             All error records information\n"
	"    --vf=<gpu_index:vf_index from list, vf_bdf, vf_uuid>        Gets firmware information about the specified VF\n"
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
	"                                   Description:\n"
	"    -h, --help                     show this help message and exit\n"
	"    -g, --gpu [GPU ...]            Select a GPU ID, BDF or UUID, if not selected it will return for all GPUs\n"
	"    -w, --watch INTERVAL           Reprint the command in a loop of INTERVAL seconds\n"
	"                                   Looping stops by entering 'CTRL' + 'C'\n"
	"                                   JSON and CSV formats cannot be printed in stdout\n"
	"    -W, --watch_time TIME          The total TIME to watch the given command\n"
	"                                   Looping stops by entering 'CTRL' + 'C'\n"
	"                                   If not specified the program will run indefinitely\n"
	"    -i, --iterations ITERATIONS    Total number of ITERATIONS to loop on the given command\n"
	"                                   Looping stops by entering 'CTRL' + 'C'\n"
	"                                   If not specified the program will run indefinitely\n"
	"    --general                      pid, process name, memory usage\n"
	"    --engine                       All engine usages\n"
	"    --pid                          Gets all process information about the specified process based on Process ID\n"
	"                                   Multiple pid can be specified and tool will return information for all of them.\n"
	"                                   Example amd-smi process --pid=<pid1> --pid=<pid2>\n"
	"    --name                         Gets all process information about the specified process based on Process Name\n"
	"                                   If multiple processes have the same name information is returned for all of them\n"
	"                                   Multiple name can be specified and tool will return information for all of them\n"
	"                                   Example amd-smi process --name=<name1> --name=<name2>\n\n";
std::string process_usage_bm =
	"usage: amd-smi process [-h | --help] [--json | --csv] [--file FILE] [-g | --gpu [GPU ...]]\n"
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
	"                              Description:\n"
	"    -h, --help                show this help message and exit\n"
	"    -g, --gpu [GPU ...]       Select a GPU ID, BDF or UUID, if not selected it will return for all GPUs\n";
std::string profile_usage_host_windows =
	"usage: amd-smi profile [-h | --help] [--json | --csv] [--file FILE] [-g | --gpu [GPU ...]]\n\n";
std::string event_common = "";
std::string event_host =
	"Event arguments:\n"
	"                                Description:\n"
	"    -h, --help                  show this help message and exit\n"
	"    -g, --gpu [GPU ...]         Select a GPU ID, BDF or UUID, if not selected it will return for all GPUs\n\n";
std::string event_usage_common = "";
std::string event_usage_host =
	"usage: amd-smi event [-h | --help] [--json | --csv] [--file FILE] [-g | --gpu [GPU ...]]\n\n"
	"Displays event information for GPU\n"
	"If no argument is provided, returns event informations for all GPUs on the system\n\n";
std::string xgmi_common = "";
std::string xgmi_usage_common = "";
std::string xgmi_message =
	"Displays XGMI capabilities, framebuffer sharing and metric information\n"
	"If no argument is provided, returns information for all GPUs on the system\n\n";
std::string xgmi_usage_host =
	"usage: amd-smi xgmi [-h | --help] [--json] [--file FILE] [-g | --gpu [GPU ...]]\n"
	"                    [--caps] [--fb-sharing] [--metric]\n\n";
std::string xgmi_usage_host_mi200 =
	"usage: amd-smi xgmi [-h | --help] [--json] [--file FILE] [-g | --gpu [GPU ...]]\n"
	"                    [--caps] [--fb-sharing]\n\n";
std::string xgmi_host =
	"Xgmi arguments:\n"
	"                                Description:\n"
	"    -h, --help                  show this help message and exit\n"
	"    -g, --gpu [GPU ...]         Select a GPU ID, BDF or UUID, if not selected it will return for all GPUs\n"
	"    --caps                      XGMI capabilities\n"
	"    --fb-sharing                Framebuffer sharing for each mode\n"
	"    --metric                    Metric XGMI information\n\n";
std::string xgmi_host_mi200 =
	"Xgmi arguments:\n"
	"                                Description:\n"
	"    -h, --help                  show this help message and exit\n"
	"    -g, --gpu [GPU ...]         Select a GPU ID, BDF or UUID, if not selected it will return for all GPUs\n"
	"    --caps                      XGMI capabilities\n"
	"    --fb-sharing                Framebuffer sharing for each mode\n";
std::string topology_common = "";
std::string topology_usage_common = "";
std::string topology_usage_host =
	"usage: amd-smi topology [-h | --help] [--json] [--file FILE] [-g | --gpu [GPU ...]]\n"
	"                        [--weight] [--hops] [--fb-sharing] [--link-type] [--link-status]\n\n";
std::string topology_message =
	"Displays link topology information\n"
	"If no argument is provided, returns information for all GPUs on the system\n\n";
std::string topology_host =
	"Topology arguments:\n"
	"                                Description:\n"
	"    -h, --help                  show this help message and exit\n"
	"    -g, --gpu [GPU ...]         Select a GPU ID, BDF or UUID, if not selected it will return for all GPUs\n"
	"    --weight                    Current weight information\n"
	"    --hops                      Current hops information\n"
	"    --fb-sharing                Current framebuffer sharing information\n"
	"    --link-type                 Link type information\n"
	"    --link-status               Link status information\n\n";
std::string set_common = "";
std::string set_usage_common = "";
std::string set_message = "";
std::string set_usage_host =
	"usage: amd-smi set [-h | --help] [--file FILE] [-xgmi ----fb-sharing-mode=[MODE] --group[<GPUx, GPUy>]]\n"
	"                   [--memory-partition [PARTITION_MODE]] [ --accelerator-partition [PROFILE_INDEX]] [ --power-cap [POWER_CAP_VALUE]]\n\n";
std::string set_usage_host_mi200 =
	"usage: amd-smi set [-h | --help] [--file FILE] [-xgmi ----fb-sharing-mode=[MODE] --group[<GPUx, GPUy>]]\n";
std::string set_usage_bm =
	"usage: amd-smi set [-h | --help] [--json | --csv] [--file FILE]\n\n";
std::string set_host =
	"Set arguments:\n"
	"                                                                                           Description:\n"
	"    -h, --help                                                                             show this help message and exit\n"
	"    --xgmi --fb-sharing-mode=<AmdSmiXgmiFbSharingMode> --group=\"<gpu_id1-gpu_id2>\"         Sets framebuffer sharing mode from group [\"MODE_1\", \"MODE_2\", \"MODE_4\", \"MODE_8\", \"CUSTOM\"]\n"
	"                                                                                           Where, MODE_X represents that X GPUs will be in the same group, linked together:\n"
	"                                                                                           MODE_1 (one GPU in a group), MODE_2 (two GPUs in a group), MODE_4 (four GPUs in a group), MODE_8 (eight GPUs in a group).\n"
	"                                                                                           Note: This command will only work if there's no guest VM running.\n"
	"                                                                                           All possible configurations can be seen by running the .\\amd-smi.exe xgmi command.\n\n"
	"    --memory-partition=<AmdSmiMemoryPartitionSetting>                                      Sets memory partition setting from group ['NPS1', 'NPS2', 'NPS4', 'NPS8']\n"
	"                                                                                           Note: This command will only work if there's no guest VM running.\n"
	"                                                                                           All possible configurations can be seen by running the .\\amd-smi.exe partition command.\n\n"
	"    --accelerator-partition=<profile_index>                                                Sets accelerator partition setting to a mode based on profile_index from partition command\n"
	"                                                                                           Note: This command will only work if there's no guest VM running.\n"
	"                                                                                           All possible configurations can be seen by running the .\\amd-smi.exe partition command.\n\n"
	"    --power-cap=<power_cap_value>                                                          Sets power cap to the provided power cap value.\n"
	"                                                                                           Note: Cap value must be between the minimum (min_power_cap) and maximum (max_power_cap) power cap values.\n"
	"                                                                                           Range of the cap value can be seen by running the .\\amd-smi.exe static command.\n\n";
std::string set_host_mi200 =
	"Set arguments:\n"
	"                                                                                           Description:\n"
	"    -h, --help                                                                             show this help message and exit\n"
	"    --xgmi --fb-sharing-mode=<AmdSmiXgmiFbSharingMode> --group=\"<gpu_id1-gpu_id2>\"         Sets framebuffer sharing mode from group [\"MODE_1\", \"MODE_2\", \"MODE_4\", \"MODE_8\", \"CUSTOM\"]\n"
	"                                                                                           Where, MODE_X represents that X GPUs will be in the same group, linked together:\n"
	"                                                                                           MODE_1 (one GPU in a group), MODE_2 (two GPUs in a group), MODE_4 (four GPUs in a group), MODE_8 (eight GPUs in a group).\n"
	"                                                                                           Note: This command will only work if there's no guest VM running.\n"
	"                                                                                           All possible configurations can be seen by running the .\\amd-smi.exe xgmi command.\n\n";
std::string set_bm =
	"Set arguments:\n"
	"                                                                                           Description:\n"
	"    -h, --help                                                                             show this help message and exit\n"
	"    --process-isolation=<0 or 1>                                                           Enable or disable the GPU process isolation: 0 for disable and 1 for enable\n\n"
	"    --power-cap=<power_cap_value>                                                          Sets power cap to the provided power cap value.\n"
	"                                                                                           Note: Cap value must be between the minimum (min_power_cap) and maximum (max_power_cap) power cap values.\n"
	"                                                                                           Range of the cap value can be seen by running the .\\amd-smi.exe static command.\n\n";
std::string reset_common = "";
std::string reset_usage_common = "";
std::string reset_usage_linux =
	"usage: amd-smi reset [-h | --help] [--file FILE] [--vf=<VF> <--vf-fb>]\n\n";
std::string reset_usage_bm =
	"usage: amd-smi reset [-h | --help] [--json | --csv] [--file FILE]\n\n";
std::string reset_message ="";
std::string reset_host_linux =
	"Reset arguments:\n"
	"                                                                 Description:\n"
	"    --vf=<gpu_index:vf_index from list, vf_bdf, vf_uuid>         Cleanup VF FB for the specified VF\n"
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
	"                                 Description:\n"
	"    -h, --help                   show this help message and exit\n"
	"    -g, --gpu [GPU ...]          Select a GPU ID, BDF or UUID, if not selected it will return for all GPUs\n"
	"    -w, --watch INTERVAL         Reprint the command in a loop of INTERVAL seconds\n"
	"                                 Looping stops by entering 'CTRL' + 'C'\n"
	"                                 JSON and CSV formats cannot be printed in stdout\n"
	"    -W, --watch_time TIME        The total TIME to watch the given command\n"
	"                                 Looping stops by entering 'CTRL' + 'C'\n"
	"                                 If not specified the program will run indefinitely\n"
	"    -i, --iterations ITERATIONS  Total number of ITERATIONS to loop on the given command\n"
	"                                 Looping stops by entering 'CTRL' + 'C'\n"
	"                                 If not specified the program will run indefinitely\n"
	"    -u, --gfx                    Monitor graphics utilization (%) and clock (MHz)\n"
	"    -m, --mem                    Monitor memory utilization (%) and clock (MHz)\n"
	"    -n, --encoder                Monitor encoder utilization (%) and clock (MHz)\n"
	"    -e, --ecc                    Monitor ECC single bit, ECC double bit\n"
	"    -r, --pcie                   Monitor PCIe bandwidth in Mb/s and PCIe replay error count\n";
std::string monitor_host =
	"    -p, --power-usage            Monitor power usage in Watts\n"
	"    -t, --temperature            Monitor temperature in Celsius\n"
	"    -d, --decoder                Monitor decoder utilization (%) and clock (MHz)\n\n";
std::string monitor_guest =
	"    -v, --vram-usage             Monitor memory usage in MB\n"
	"    -q, --process                Include process output underneath monitor output\n\n";
std::string monitor_bm =
	"    -p, --power-usage            Monitor power usage in Watts\n"
	"    -t, --temperature            Monitor temperature in Celsius\n"
	"    -q, --process                Include process output underneath monitor output\n\n";
std::string partition_common = "";
std::string partition_host =
	"Partition arguments:\n"
	"                                 Description:\n"
	"    -h, --help                   Show this help message and exit\n"
	"    -g, --gpu [GPU ...]          Select a GPU ID, BDF or UUID, if not selected it will return for all GPUs\n"
	"    -c, --current                Displays current memory and accelerator partition mode\n"
	"    -m, --memory                 Displays caps and current memory partition setting\n"
	"    -a, --accelerator            Displays caps and current accelerator partition setting.\n\n";
std::string partition_usage_common = "";
std::string partition_usage_host =
	"usage: amd-smi partition [-h | --help] [--file FILE] [-g | --gpu [GPU ...]]\n"
	"                         [-c | --current] [-m | --memory] [-a | --accelerator]\n\n";
std::string partition_message =
	"Displays partition information about specific GPU.\n"
	"If no GPU is provided, returns information for all GPUs on the system\n"
	"If no partition information argument is provided all partition information will be displayed\n\n";
std::string command_modifiers =
	"Command Modifiers:\n"
	"                      Description:\n"
	"--json                Displays output in JSON format (humman readable by default).\n"
	"--csv                 Displays output in CSV format (humman readable by default).\n"
	"--file FILE           Saves output into a file on the provided path (stdout by default).\n";
std::string xgmi_topology_modifiers =
	"Command Modifiers:\n"
	"                      Description:\n"
	"--json                Displays output in JSON format (humman readable by default).\n"
	"--file FILE           Saves output into a file on the provided path (stdout by default).\n";
std::string metric_modifiers =
	"Command Modifiers:\n"
	"                      Description:\n"
	"--json                Displays output in JSON format (humman readable by default).\n"
	"--csv                 Displays output in CSV format (humman readable by default).\n"
	"                      It can be used only with one argument and cannot be used without or with more than one argument, in that case, the call will fail\n"
	"--file FILE           Saves output into a file on the provided path (stdout by default).\n";
std::string partition_modifiers =
	"Command Modifiers:\n"
	"                      Description:\n"
	"--file FILE           Saves output into a file on the provided path (stdout by default).\n";

std::string ras_usage_message =
	"\nGet ras informations \n"
	"If no GPU is provided, returns information for all GPUs on the system\n"
	"If no ras information argument is provided all ras information will be displayed\n\n";

std::string usage_ras_host = "usage: amd-smi ras [-h | --help] [--cper] [--severity=[fatal, nonfatal-uncorrected, nonfatal-corrected, all]] [--folder=[FOLDER]] "
							 "[--file_limit=[NUMBER_OF_FILES]] [--follow] \n";

std::string ras_host = "Ras arguments:\n"
	"                                Description:\n"
	"    -h, --help                  show this help message and exit\n"
	"    --cper --severity=<fatal, nonfatal-uncorrected, nonfatal-corrected, all> --folder=[FOLDER]      Get ras cper errors and saved in file based on severity. \n"
	"           --file_limit=<number_of_files> --follow                        If the --folder option is not provided, no files will be dumped. \n"
	"                                                                       By default, it will dump the cper report currently cached in the driver. \n"
	"                                                                       If user specify the --file_limit=<number_of_files> option, the CLI will only keep max <number_of_files> files. \n"
	"                                                                       If the --follow option is provided, the cli will continuous monitoring and \n"
	"                                                                       dump the report until the ctrl-c is pressed.\n";

std::string usage_ras_common = "";
std::string ras_common = "";



AmdSmiHelpInfo::AmdSmiHelpInfo()
{
	version_specific = "";
	list_specific = "";
	usage_list_specific = "";
	usage_bad_pages_specific = "";
	if (AmdSmiPlatform::getInstance().is_windows()) {
		if (AmdSmiPlatform::getInstance().is_host()) {
			if (AmdSmiPlatform::getInstance().is_mi300()) {
				help_specific = help_command_windows_host + help_command_mi30x_host;
				static_specific = static_host_windows + static_host_mi30x + static_host_vf;
				usage_static_specific = usage_static_host + usage_vf_static_host;
				xgmi_specific = xgmi_host;
				usage_xgmi_specific = xgmi_usage_host;
				topology_specific = topology_host;
				usage_topology_specific = topology_usage_host;
				set_specific = set_host;
				usage_set_specific = set_usage_host;
				reset_specific = "";
				usage_reset_specific = "";
				partition_specific = partition_host;
				usage_partition_specific = partition_usage_host;
				ras_specific = ras_host;
				usage_ras_specific = usage_ras_host;
			} else {
				help_specific = help_command_windows_host;
				static_specific = static_host_windows + static_host_vf;
				usage_static_specific = usage_static_host + usage_vf_static_host;
				xgmi_specific = "";
				usage_xgmi_specific = "";
				topology_specific = "";
				usage_topology_specific = "";
				set_specific = "";
				reset_specific = "";
				usage_set_specific = "";
				usage_reset_specific = "";
				usage_partition_specific = "";
				ras_specific = "";
				usage_ras_specific = "";
			}

			bad_pages_specific = bad_pages_host;
			firmware_specific = firmware_host;
			usage_firmware_specific = firmware_usage_host;
			metric_specific = metric_host;
			usage_metric_specific = metric_usage_host;
			process_specific = "";
			usage_process_specific = "";
			profile_specific = profile_host_windows;
			usage_profile_specific = profile_usage_host_windows;
			event_specific = event_host;
			usage_event_specific = event_usage_host;
			monitor_specific = monitor_host;
			usage_monitor_specific = monitor_usage_host;
		} else if (AmdSmiPlatform::getInstance().is_baremetal()) {
			help_specific = help_command_bm;
			static_specific = static_bm;
			usage_static_specific = usage_static_bm;
			bad_pages_specific = "";
			usage_bad_pages_specific = "";
			firmware_specific = firmware_bm;
			usage_firmware_specific = firmware_usage_bm;
			metric_specific = metric_bm;
			usage_metric_specific = metric_usage_bm;
			process_specific = process_bm;
			usage_process_specific = process_usage_bm;
			profile_specific = "";
			usage_profile_specific = "";
			event_specific = "";
			xgmi_specific = "";
			usage_xgmi_specific = "";
			topology_specific = "";
			usage_topology_specific = "";
			set_specific = set_bm;
			usage_set_specific = set_usage_bm;
			reset_specific = reset_bm;
			usage_reset_specific = reset_usage_bm;
			monitor_specific = monitor_bm;
			usage_monitor_specific = monitor_usage_bm;
		} else {
			help_specific = help_command_guest;
			static_specific = static_bm;
			usage_static_specific = usage_static_bm;
			bad_pages_specific = "";
			firmware_specific = "";
			usage_firmware_specific = "";
			metric_specific = metric_guest;
			usage_metric_specific = metric_usage_guest;
			process_specific = process_bm;
			usage_process_specific = process_usage_bm;
			profile_specific = "";
			usage_profile_specific = "";
			event_specific = "";
			usage_event_specific = "";
			xgmi_specific = "";
			usage_xgmi_specific = "";
			topology_specific = "";
			usage_topology_specific = "";
			set_specific = set_bm;
			usage_set_specific = set_usage_bm;
			reset_specific = reset_bm;
			usage_reset_specific = reset_usage_bm;
			monitor_specific = monitor_guest;
			usage_monitor_specific = monitor_usage_guest;
		}
	} else {
		if (AmdSmiPlatform::getInstance().is_host()) {
			if (AmdSmiPlatform::getInstance().is_mi300()) {
				help_specific = help_command_linux_host + help_command_mi30x_host;
				static_specific = static_host_linux + static_host_mi30x + static_host_vf;
				usage_static_specific = usage_static_host_linux + usage_static_hyperv + usage_vf_static_host;
				metric_specific = metric_host;
				usage_metric_specific = metric_usage_host;
				xgmi_specific = xgmi_host;
				usage_xgmi_specific = xgmi_usage_host;
				topology_specific = topology_host;
				usage_topology_specific = topology_usage_host;
				set_specific = set_host;
				usage_set_specific = set_usage_host;
				reset_specific = reset_host_linux;
				usage_reset_specific = reset_usage_linux;
				partition_specific = partition_host;
				usage_partition_specific = partition_usage_host;
				ras_specific = ras_host;
				usage_ras_specific = usage_ras_host;
			} else if (AmdSmiPlatform::getInstance().is_mi200()) {
				help_specific = help_command_linux_host + help_command_mi200_host;
				static_specific = static_host_linux_mi200 + static_host_vf;
				usage_static_specific = usage_static_host_linux_mi200 + usage_vf_static_host;
				metric_specific = metric_host_mi200;
				usage_metric_specific = metric_usage_host_mi200;
				xgmi_specific = xgmi_host_mi200;
				usage_xgmi_specific = xgmi_usage_host_mi200;
				topology_specific = topology_host;
				usage_topology_specific = topology_usage_host;
				set_specific = set_host_mi200;
				usage_set_specific = set_usage_host_mi200;
				reset_specific = reset_host_linux;
				usage_reset_specific = reset_usage_linux;
				partition_specific = "";
				usage_partition_specific = "";
			} else {
				help_specific = help_command_linux_host;
				usage_static_specific = usage_static_host_linux + usage_vf_static_host;
				static_specific = static_host_linux + static_host_vf;
				metric_specific = metric_host;
				usage_metric_specific = metric_usage_host;
				xgmi_specific = "";
				usage_xgmi_specific = "";
				topology_specific = "";
				usage_topology_specific = "";
				set_specific = "";
				reset_specific = "";
				usage_set_specific = "";
				usage_reset_specific = "";
				partition_specific = "";
				usage_partition_specific = "";
				ras_specific = "";
				usage_ras_specific = "";
			}
			bad_pages_specific = bad_pages_host;
			firmware_specific = firmware_host;
			usage_firmware_specific = firmware_usage_host;
			process_specific = "";
			usage_process_specific = "";
			profile_specific = "";
			usage_profile_specific = "";
			event_specific = event_host;
			usage_event_specific = event_usage_host;
			monitor_specific = monitor_host;
			usage_monitor_specific = monitor_usage_host;
		}
	}
}
bool AmdSmiHelpInfo::is_command_supported(std::string command, bool modifiers, std::string common,
		std::string specific)
{
	if (modifiers && common.size() == 0 && specific.size() == 0) {
		throw SmiToolCommandNotSupportedException(command);
	}
	return true;
}
std::string AmdSmiHelpInfo::get_list_help_message(bool modifiers = false)
{
	is_command_supported("list",modifiers,list_common,list_specific);
	return copyright_message + list_common + usage_list_specific + list_specific + command_modifiers;
}
std::string AmdSmiHelpInfo::get_static_help_message(bool modifiers = false)
{
	is_command_supported("static",modifiers,static_common,static_specific);
	return  copyright_message + usage_static_common + usage_static_specific + static_usage_message +
			static_common + static_specific +  command_modifiers;
}

std::string AmdSmiHelpInfo::get_bad_page_help_message(bool modifiers = false)
{
	is_command_supported("bad_paged",modifiers,bad_pages_common,bad_pages_specific);
	return copyright_message + usage_bad_pages_specific + bad_pages_message + bad_pages_common +
		   bad_pages_specific + command_modifiers;
}
std::string AmdSmiHelpInfo::get_firmware_help_message(bool modifiers = false)
{
	is_command_supported("firmware",modifiers,firmware_common,firmware_specific);
	return copyright_message + firmware_usage_common + usage_firmware_specific + firmware_message +
		   firmware_common + firmware_specific + command_modifiers;
}
std::string AmdSmiHelpInfo::get_metric_help_message(bool modifiers = false)
{
	is_command_supported("metric",modifiers,metric_common,metric_specific);
	return copyright_message + metric_usage_common + usage_metric_specific + metric_message +
		   metric_common + metric_specific + metric_modifiers;
}
std::string AmdSmiHelpInfo::get_process_help_message(bool modifiers = false)
{
	is_command_supported("process",modifiers,process_common,process_specific);
	return copyright_message + process_usage_common + usage_process_specific + process_message +
		   process_common + process_specific + command_modifiers;
}
std::string AmdSmiHelpInfo::get_profile_help_message(bool modifiers = false)
{
	is_command_supported("profile",modifiers,profile_common,profile_specific);
	return copyright_message + profile_usage_common + usage_profile_specific + profile_message +
		   profile_common + profile_specific + command_modifiers;
}
std::string AmdSmiHelpInfo::get_version_help_message(bool modifiers = false)
{
	is_command_supported("version",modifiers,version_common,version_specific);
	return copyright_message + version_common + version_specific + command_modifiers;
}
std::string AmdSmiHelpInfo::get_event_help_message(bool modifiers = false)
{
	is_command_supported("event",modifiers,event_common,event_specific);
	return copyright_message + usage_event_specific + event_common + event_specific + command_modifiers;
}
std::string AmdSmiHelpInfo::get_xgmi_help_message(bool modifiers = false)
{
	is_command_supported("xgmi",modifiers,xgmi_common,xgmi_specific);
	return copyright_message + xgmi_usage_common + usage_xgmi_specific + xgmi_message + xgmi_common +
		   xgmi_specific + xgmi_topology_modifiers;
}
std::string AmdSmiHelpInfo::get_topology_help_message(bool modifiers = false)
{
	is_command_supported("topology",modifiers,topology_common,topology_specific);
	return copyright_message + topology_usage_common + usage_topology_specific + topology_message +
		   topology_common + topology_specific + xgmi_topology_modifiers;
}
std::string AmdSmiHelpInfo::get_partition_help_message(bool modifiers = false)
{
	is_command_supported("partition",modifiers,partition_common,partition_specific);
	return copyright_message + partition_usage_common + usage_partition_specific + partition_message +
		   partition_common + partition_specific + partition_modifiers;
}
std::string AmdSmiHelpInfo::get_set_help_message(bool modifiers = false)
{
	is_command_supported("set",modifiers,set_common,set_specific);
	return copyright_message + set_usage_common + usage_set_specific + set_message + set_common +
		   set_specific;
}
std::string AmdSmiHelpInfo::get_reset_help_message(bool modifiers = false)
{
	is_command_supported("reset",modifiers,reset_common,reset_specific);
	return copyright_message + reset_usage_common + usage_reset_specific + reset_message + reset_common
		   + reset_specific + command_modifiers;
}
std::string AmdSmiHelpInfo::get_monitor_help_message(bool modifiers = false)
{
	is_command_supported("monitor",modifiers, monitor_common,monitor_specific);
	return copyright_message + monitor_usage_common + usage_monitor_specific + monitor_message
		   + monitor_common + monitor_specific + command_modifiers;
}
std::string AmdSmiHelpInfo::get_help_message()
{
	std::string version{};
	version = string_format("%s version %s", AMDSMI_TOOL_NAME, AMDSMI_TOOL_VERSION_STRING);
	std::string message{string_format(help_common, version.c_str())};
	return copyright_message + message + help_specific + "\n";
}

std::string AmdSmiHelpInfo::get_ras_help_message(bool modifiers = false)
{
	is_command_supported("ras", modifiers, ras_common, ras_specific);
	return  copyright_message + usage_ras_common + usage_ras_specific + ras_usage_message +
			ras_common + ras_specific +  command_modifiers;
}
