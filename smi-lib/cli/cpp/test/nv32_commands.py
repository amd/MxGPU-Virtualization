#
# Copyright (C) 2024 Advanced Micro Devices. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
# host windows

host_windows_list_options = []

host_windows_static_options_gpu = ["--asic", "--bus", "--vbios", "--board", "--limit", "--driver", "--ras",
                           "--dfc-ucode", "--fb-info", "--num-vf",  "--vram"]

host_windows_static_options_vf = []

host_windows_ucode_options_gpu = ["--ucode-list", "--error-records"]

host_windows_ucode_options_vf = ["--fw-list"]

host_windows_bad_pages_options = []

host_windows_metric_options_gpu = ["--usage", "--power",
                           "--clock", "--temperature", "--ecc", "--pcie"]

host_windows_metric_options_gpu_fail = ["--fb-usage"]

host_windows_metric_options_vf = ["--schedule", "--guard", "--guest-data"]

host_windows_process_options_gpu_fail = ["", "--general", "--engine"]

host_windows_profile_options = []

host_windows_options = [host_windows_list_options,host_windows_list_options, host_windows_static_options_gpu, host_windows_bad_pages_options,
                host_windows_ucode_options_gpu, host_windows_ucode_options_gpu,
                host_windows_metric_options_gpu, host_windows_profile_options]

host_windows_commands = ["list","discovery", "static", "bad-pages", "ucode", "firmware", "metric", "profile"]

host_windows_options_csv = []

host_windows_commands_csv = []

host_windows_options_fail = [host_windows_metric_options_gpu_fail,
                     host_windows_process_options_gpu_fail]

host_windows_commands_fail = ["metric", "process"]

host_windows_options_vf = [host_windows_static_options_vf,
                   host_windows_ucode_options_vf,  host_windows_ucode_options_vf, host_windows_metric_options_vf]

host_windows_commands_vf = ["static", "ucode", "firmware", "metric"]

# host linux

host_linux_list_options = []

host_linux_static_options_gpu = ["--asic", "--bus", "--vbios", "--board", "--limit", "--driver", "--ras",
                           "--fb-info", "--num-vf",  "--vram"]

host_linux_static_options_vf = []

host_linux_ucode_options_gpu = ["--ucode-list", "--error-records"]

host_linux_ucode_options_vf = ["--fw-list"]

host_linux_bad_pages_options = []

host_linux_metric_options_gpu = ["--usage", "--power",
                           "--clock", "--temperature", "--ecc", "--pcie"]

host_linux_metric_options_gpu_fail = ["--fb-usage"]

host_linux_metric_options_vf = ["--schedule", "--guard", "--guest-data"]

host_linux_process_options_gpu_fail = ["", "--general", "--engine"]

host_linux_options = [host_linux_list_options,host_linux_list_options, host_linux_static_options_gpu, host_linux_bad_pages_options,
                host_linux_ucode_options_gpu, host_linux_ucode_options_gpu,
                host_linux_metric_options_gpu]

host_linux_commands = ["list","discovery", "static", "bad-pages", "ucode", "firmware", "metric"]

host_linux_options_csv = []

host_linux_commands_csv = []

host_linux_options_fail = [host_linux_metric_options_gpu_fail,
                     host_linux_process_options_gpu_fail]

host_linux_commands_fail = ["metric", "process"]

host_linux_options_vf = [host_linux_static_options_vf,
                   host_linux_ucode_options_vf,  host_linux_ucode_options_vf, host_linux_metric_options_vf]

host_linux_commands_vf = ["static", "ucode", "firmware", "metric"]

# bm windows

baremetal_windows_list_options = []

baremetal_windows_static_options_gpu = [
    "--asic", "--bus", "--vbios", "--limit", "--driver"]

baremetal_windows_static_options_gpu_fail = ["--board", "--dfc-ucode", "--fb-info", "--num-vf"]

baremetal_windows_ucode_options_gpu = ["--ucode-list"]

baremetal_windows_bad_pages_options = [""]

baremetal_windows_metric_options_gpu = ["--usage", "--fb-usage",
                                        "--power", "--clock", "--temperature", "--ecc", "--pcie"]

baremetal_windows_process_options_gpu = ["--general", "--engine"]

baremetal_windows_profile_options_fail = [""]

baremetal_windows_options = [baremetal_windows_list_options,baremetal_windows_list_options, baremetal_windows_ucode_options_gpu,
                                baremetal_windows_ucode_options_gpu, baremetal_windows_static_options_gpu, baremetal_windows_metric_options_gpu, baremetal_windows_process_options_gpu]

baremetal_windows_commands = ["list","discovery", "ucode", "firmware", "static", "metric", "process"]

baremetal_windows_options_fail = [baremetal_windows_static_options_gpu_fail, baremetal_windows_profile_options_fail,
                                  baremetal_windows_bad_pages_options]

baremetal_windows_commands_fail = ["static", "profile", "bad-pages"]

# bm linux

baremetal_linux_list_options = []

baremetal_linux_static_options_gpu = [
    "--asic", "--bus", "--vbios", "--board", "--driver", "--ras"]

baremetal_linux_static_options_gpu_fail = ["--dfc-ucode", "--fb-info", "--num-vf"]

baremetal_linux_ucode_options_gpu = ["--ucode-list"]

baremetal_linux_bad_pages_options = []

baremetal_linux_metric_options_gpu = ["--usage", "--fb-usage",
                                      "--power", "--clock", "--temperature", "--ecc", "--pcie"]

baremetal_linux_process_options_gpu = ["--general", "--engine"]

baremetal_linux_profile_options_fail = [""]

baremetal_linux_options = [baremetal_linux_list_options,baremetal_linux_list_options, baremetal_linux_static_options_gpu,
                           baremetal_linux_ucode_options_gpu, baremetal_linux_ucode_options_gpu, baremetal_linux_bad_pages_options, baremetal_linux_metric_options_gpu,
                           baremetal_linux_process_options_gpu]

baremetal_linux_commands = ["list","discovery", "static",
                             "ucode", "firmware", "bad-pages" "metric", "process"]

baremetal_linux_options_fail = [
    baremetal_linux_static_options_gpu_fail, baremetal_linux_profile_options_fail]

baremetal_linux_commands_fail = ["static", "profile"]

# guest windows

guest_windows_list_options = []

guest_static_options = ["--asic", "--bus", "--vbios", "--driver"]

guest_static_fail = ["--board", "--limit", "--ras",
                     "--dfc-ucode", "--fb-info", "--num-vf"]

guest_ucode_fail = ["", "--ucode-list", "--error-records"]

guest_bad_pages_fail = [""]

guest_metric = ["--usage", "--fb-usage"]

guest_metric_fail = ["--power", "--clock", "--temperature", "--ecc", "--pcie"]

guest_process = ["--general", "--engine"]

guest_profile_fail = [""]

guest_event_fail = [""]

guest_options = [guest_windows_list_options,guest_windows_list_options, guest_static_options,
                 guest_metric, guest_process]

guest_commands = ["list","discovery", "static", "metric", "process"]

guest_options_fail = [guest_static_fail, guest_ucode_fail, guest_bad_pages_fail, guest_metric_fail, guest_profile_fail,
                      guest_event_fail]

guest_commands_fail = ["static", "ucode",
                       "bad-pages", "metric", "profile", "event"]

# guest linux

guest_linux_list_options = ["--help"]

guest_linux_static_options_gpu = [
    "--help"]

guest_linux_static_options_gpu_fail = ["--dfc-ucode", "--fb-info", "--num-vf"]

guest_linux_ucode_options_gpu = ["--help", "--ucode-list"]

guest_linux_bad_pages_options = ["--help"]

guest_linux_metric_options_gpu = ["--help"]

guest_linux_process_options_gpu = ["--help"]

guest_linux_profile_options_fail = [""]

guest_linux_options_csv = []

guest_linux_commands_csv = []

guest_linux_options = [guest_linux_list_options,guest_linux_list_options, guest_linux_static_options_gpu, guest_linux_bad_pages_options,
                           guest_linux_ucode_options_gpu, guest_linux_metric_options_gpu,
                           guest_linux_process_options_gpu]

guest_linux_commands = ["list","discovery", "static",
                            "firmware", "metric", "process"]

guest_linux_options_fail = [
    guest_linux_static_options_gpu_fail, guest_linux_profile_options_fail]

guest_linux_commands_fail = ["static", "profile"]