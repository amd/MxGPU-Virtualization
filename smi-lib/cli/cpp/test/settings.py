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

from subprocess import run, PIPE, STDOUT
from platform import system as pl_system
from helpers import *
import win32com.client

globalSettings = None
tool_name = "amd-smi"

dev_id_list_mi300 = ["DEV_74A0", "DEV_74A1", "DEV_74A9", "DEV_74BD"]
dev_id_list_nv3plus = ["DEV_73C4", "DEV_73C5", "DEV_73C8", "DEV_7460"]

def check_if_mi300(output):
    is_mi300 = False
    for x in dev_id_list_mi300:
        n = output.find(x)
        if n != -1:
            is_mi300 = True
    return is_mi300

def check_if_nv32(output):
    is_nv32 = False
    for x in dev_id_list_nv3plus:
        n = output.find(x)
        if n != -1:
            is_nv32 = True
    return is_nv32

def get_device_ids_wmi(query):
    locator = win32com.client.Dispatch("WbemScripting.SWbemLocator")
    services = locator.ConnectServer(".", "ROOT\\CIMV2")
    enumerator = services.ExecQuery("SELECT * FROM {}".format(query))

    device_ids = ""
    for item in enumerator:
        pnp_device_id = item.PNPDeviceID
        device_ids += "PNPDeviceID: {}\n".format(pnp_device_id)

    return device_ids

def is_vm_compute_running_wmi():
    locator = win32com.client.Dispatch("WbemScripting.SWbemLocator")
    services = locator.ConnectServer(".", "ROOT\\CIMV2")
    enumerator = services.ExecQuery("SELECT * FROM Win32_Process")

    for item in enumerator:
        if item.Caption.lower() == "vmcompute":
            return True
    return False

def is_virtualization_host_wmi():
    locator = win32com.client.Dispatch("WbemScripting.SWbemLocator")
    services = locator.ConnectServer(".", "ROOT\\CIMV2")
    enumerator = services.ExecQuery("SELECT * FROM Win32_ComputerSystem")

    for item in enumerator:
        if item.HypervisorPresent:
            return "TRUE"
        else:
            return "FALSE"
    return "UNKNOWN"

class Settings:
    def __init__(self):
        self.platformType = PlatformType.UNKNOWN
        self.platformName = PlatformName.UNKNOWN
        self.tool_binary = ""
        self.isAmdsmiSupported = False
        self.is_mi300_ = False
        self.is_nv32_ = False

        platformInfo = self._get_platform_desc()
        self.platformType = platformInfo["platform_type"]
        self.platformName = platformInfo["platform_name"]
        self.tool_binary = platformInfo["tool_binary"]

    def init_asic_detection(self):
        if self.platformName == PlatformName.WINDOWS:
            output = get_device_ids_wmi("Win32_VideoController")
            self.is_mi300_ = check_if_mi300(output)
            self.is_nv32_ = check_if_nv32(output)

            dev_id_list_nv2_nv3plus = ["DEV_73A1", "DEV_73AE", "DEV_73C4", "DEV_73C5", "DEV_73C8",
                                       "DEV_7460", "DEV_7461", "DEV_74A0", "DEV_74A1"]
            winCmdOutput = output.lower().split("\n")
            for dev_id in winCmdOutput:
                if "ven_1002" in dev_id and any(d in dev_id for d in dev_id_list_nv2_nv3plus):
                    self.isAmdsmiSupported = True
                    break
                else:
                    self.isAmdsmiSupported = False
        elif self.platformName == PlatformName.LINUX:
            output = run("lspci -n | grep 1002", shell=True, stdout=PIPE, stderr=STDOUT, encoding="UTF-8").stdout
            outputList = output.split("\n")[:-1]
            dev_mi300_id_list = ["74A0", "74A1","74b5"]
            dev_nv_id_list = ["73c4", "73c5", "73c8", "7460", "7461", "74a0", "74a1", "73a1", "73ae"]
            for output in outputList:
                if any(dev_id in output for dev_id in dev_mi300_id_list):
                    self.is_mi300_  = True
                    self.isAmdsmiSupported = True
                elif any(dev_id in output for dev_id in dev_nv_id_list):
                    self.is_nv32_  = True
                    self.isAmdsmiSupported = True
                else:
                    self.isAmdsmiSupported = False
    def _get_platform_desc(self):
        """ Returns current platform information """
        platformType = None
        platformName = None
        tool_binary = ""
        systemDesc = pl_system()
        if "Windows" in systemDesc:
            platformName = PlatformName.WINDOWS
            root_path = "C:\\Windows\\System32\\{}.exe".format(tool_name)
            current_folder_path = str(Path.cwd() / "{}.exe".format(tool_name))
            tool_binary = root_path if os.path.isfile(root_path) else current_folder_path if os.path.isfile(current_folder_path) else None

            if not tool_binary:
                raise Exception("Binary was not found in either location {} or {}".format(root_path, current_folder_path))

            if is_vm_compute_running_wmi():
                platformType = PlatformType.HOST
            else:
                status = is_virtualization_host_wmi()
                if status == "TRUE":
                    platformType = PlatformType.GUEST
                elif status == "FALSE":
                    diskpart_out = run(["diskpart", "/?"], shell=True, stdout=PIPE, stderr=STDOUT, encoding="UTF-8").stdout
                    if "MININT" in diskpart_out:
                        platformType = PlatformType.BAREMETAL
                    else:
                        platformType = PlatformType.UNKNOWN
                else:
                    platformType = PlatformType.UNKNOWN
        elif "Linux" in systemDesc:
            platformName = PlatformName.LINUX
            root_path = "/usr/bin/{}".format(tool_name)
            current_folder_path = str(Path.cwd()) + "/" + tool_name
            if os.path.isfile(root_path):
                tool_binary = root_path
            elif os.path.isfile(current_folder_path):
                tool_binary = current_folder_path
            else:
                raise Exception("Binary was not found in either location {} or {}".format(root_path,current_folder_path))
            output = run(["lscpu"], stdout=PIPE, stderr=STDOUT, encoding="UTF-8").stdout
            if "hypervisor" not in output:
                output = run("lsmod | grep gim", shell=True, stdout=PIPE, stderr=STDOUT, encoding="UTF-8").stdout
                if not output:
                    output = run("lsmod | grep amdgpu", shell=True, stdout=PIPE, stderr=STDOUT, encoding="UTF-8").stdout
                    if not output:
                        print("Error: Driver is not installed!\n")
                        platformType = PlatformType.UNKNOWN
                    else:
                        platformType = PlatformType.BAREMETAL
                else:
                    platformType = PlatformType.HOST
            else:
                platformType = PlatformType.GUEST
        else:
            platformName = PlatformName.UNKNOWN
            platformType = PlatformType.UNKNOWN

        return {
            'platform_name': platformName,
            'platform_type': platformType,
            'tool_binary': tool_binary
        }

    def is_amdsmi_supported(self):
        return self.isAmdsmiSupported

    def is_baremetal_machine(self):
        return True if self.platformType == PlatformType.BAREMETAL else False

    def is_linux_baremetal(self):
        return True if self.platformName == PlatformName.LINUX and self.is_baremetal_machine() else False

    def is_guest(self):
        return True if self.platformType == PlatformType.GUEST else False

    def is_windows(self):
        return True if self.platformName == PlatformName.WINDOWS else False

    def is_host_machine(self):
        return True if self.platformType == PlatformType.HOST else False

    def get_platform_type(self):
        return self.platformType

    def get_platform_name(self):
        return self.platformName

    def get_tool_binary(self):
        return self.tool_binary

    def is_mi300(self):
        return self.is_mi300_

    def is_nv32(self):
        return self.is_nv32_


def initGlobalSettings():
    global globalSettings
    if globalSettings is None:
        globalSettings = Settings()
