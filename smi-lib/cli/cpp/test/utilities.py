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

import json
import subprocess
import re
import settings
from helpers import *

tool_name = "amd-smi"
output_folder_name = "results"

def get_py_interpreter_cmd():
        platformName = settings.globalSettings.get_platform_name()

        if platformName == PlatformName.LINUX:
            return "python3"
        elif platformName == PlatformName.WINDOWS:
            return "python"
        else:
            return ""

def get_formated_string_from_bdf(vf):
    vf_splited = re.split(r'[.:]',vf)
    vf_str = ""
    for i, vf_s in enumerate(vf_splited):
        vf_str += vf_s
        if i != (len(vf_splited) - 1):
            vf_str += "-"
    return vf_str

def get_gpu_vf_info():
    py = "" if settings.globalSettings.is_windows() else get_py_interpreter_cmd()
    tool = settings.globalSettings.get_tool_binary()

    result = subprocess.Popen("%s %s %s" % (tool, "list", "--json"),
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    outs, errs = result.communicate()
    devices_list_json = outs.decode("utf-8") if outs.decode("utf-8") != "" else None

    pf_bdf_list = []
    pf_index_list = []
    pf_uuid_list = []
    vf_bdf_list = []
    vf_index_list = []
    vf_uuid_list = []
    if devices_list_json == None:
        return pf_bdf_list, pf_index_list, pf_uuid_list, vf_bdf_list, vf_index_list, vf_uuid_list
    if "error_code" in devices_list_json:
        return pf_bdf_list, pf_index_list, pf_uuid_list, vf_bdf_list, vf_index_list, vf_uuid_list

    devices_list_json = json.loads(devices_list_json)
    for device in devices_list_json:
        pf_bdf_list.append(device['bdf'])
        pf_uuid_list.append(device['uuid'])
        pf_index_list.append(device['gpu'])
        if device.__contains__('vfs') or device.__contains__('vf'):
            vfs = device['vfs'] if device.__contains__('vfs') else device['vf']
            vf_indexed_list = []
            for bdf in vfs:
                vf_bdf_list.append(bdf['bdf'])
                vf_indexed_list.append(bdf['vf'])
                vf_uuid_list.append(bdf['uuid'])
            vf_index_list.append(vf_indexed_list)
    if len(vf_index_list) > 0:
        return pf_bdf_list, pf_index_list, pf_uuid_list, vf_bdf_list, vf_index_list, vf_uuid_list
    return pf_bdf_list, pf_index_list, pf_uuid_list

def get_process_info():
    py = "" if settings.globalSettings.is_windows() else get_py_interpreter_cmd()
    tool = settings.globalSettings.get_tool_binary()

    result = subprocess.Popen("%s %s %s %s" % (py, tool, "process", "--json"),
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    outs, errs = result.communicate()
    result = outs.decode("utf-8") if outs.decode("utf-8") != "" else None

    pids = []
    names = []

    if result == None:
        return pids, names
    if "error_code" in result:
        return pids, names

    result = json.loads(result)
    if result.__contains__('process_list'):
        for val in result["process_list"]:
            pids.append(val["process_info"]["pid"])
            names.append(val["process_info"]["name"])

        if py == "python3":
            pids.pop(len(pids) - 1)
            names.pop(len(names) - 1)
        elif py == "python" or py == "":
            pids.pop(0)
            names.pop(0)

    return pids, names
