/* * Copyright (C) 2023-2024 Advanced Micro Devices. All rights reserved.
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
#include <iostream>
#include <string>
#include <vector>
#include <stdlib.h>
#include <cctype>
#include <algorithm>

#include "smi_cli_platform.h"

const std::vector<std::string> dev_id_list_mi300 = {"74A0", "74A1", "74A2", "74B6", "74A9", "74BD", "74A5", "74B9"};
const std::vector<std::string> dev_id_list_mi2plus = {"7410"};
const std::vector<std::string> dev_id_list_nv3plus = {"73C4", "73C5", "73C8", "7460", "7461"};

bool check_if_mi300(std::string output)
{
	std::string::size_type n;
	bool is_mi300{false};
	for (auto x : dev_id_list_mi300) {
		n = output.find(x);
		if (std::string::npos != n) {
			is_mi300 = true;
		}
	}
	return is_mi300;
}

bool check_if_nv32(std::string output)
{
	std::string::size_type n;
	bool is_nv32{false};
	for (auto x : dev_id_list_nv3plus) {
		n = output.find(x);
		if (std::string::npos != n) {
			is_nv32 = true;
		}
	}
	return is_nv32;
}

bool check_if_mi200(std::string output)
{
	std::string::size_type n;
	bool is_mi200{false};
	for (auto x : dev_id_list_mi2plus) {
		n = output.find(x);
		if (std::string::npos != n) {
			is_mi200 = true;
		}
	}
	return is_mi200;
}

AmdSmiPlatform::AmdSmiPlatform()
{
#if defined(__linux__)
	operating_system = "linux";
	is_linux_ = true;
#elif _WIN64
	operating_system = "windows";
	is_windows_ = true;
#else
	operating_system = "unknown";
#endif
	if(is_windows_) {
		std::string diskpart_out = exec("diskpart /?");
		if (diskpart_out.find("MININT") != std::string::npos) {
			is_baremetal_ = true;
			is_nv32_ = true;
			return;
		}

		std::string wmic_output = exec("WMIC PATH Win32_VideoController GET PNPDeviceID /VALUE");
		std::string powershell_output;
		is_mi300_ = check_if_mi300(wmic_output);
		is_nv32_ = check_if_nv32(wmic_output);
		is_mi200_ = check_if_mi200(wmic_output);
		wmic_output = exec("wmic PATH Win32_Process GET Caption /VALUE");
		if ((wmic_output.find("not found") != std::string::npos) ||
				(wmic_output.find("not recognized") != std::string::npos)) {
			powershell_output = exec(
									"powershell -Command (Get-WindowsOptionalFeature -Online -FeatureName Microsoft-Hyper-V).State");
			if (powershell_output.find("enabled") != std::string::npos) {
				is_host_ = true;
			}
		} else {
			if (wmic_output.find("vmcompute") != std::string::npos) {
				is_host_ = true;
			}
		}
		if(!is_host_) {
			wmic_output = exec("wmic PATH Win32_ComputerSystem GET HypervisorPresent /VALUE");
			if ((wmic_output.find("not found") != std::string::npos) ||
					(wmic_output.find("not recognized") != std::string::npos)) {
				powershell_output = exec(
										"powershell -Command (Get-CimInstance Win32_ComputerSystem).HypervisorPresent");
				if ((powershell_output.find("true") != std::string::npos) ||
						(powershell_output.find("TRUE") != std::string::npos)) {
					is_guest_os_ = true;
				} else if ((powershell_output.find("false") != std::string::npos) ||
						   (powershell_output.find("FALSE") != std::string::npos)) {
					is_baremetal_ = true;
				} else {
					powershell_output = exec("powershell -Command diskpart /?");
					if (powershell_output.find("MININT") != std::string::npos) {
						is_baremetal_ = true;
					} else {
						unknown_platform = true;
					}
				}
			} else {
				if ((wmic_output.find("true") != std::string::npos) ||
						(wmic_output.find("TRUE") != std::string::npos)) {
					is_guest_os_ = true;
				} else if ((wmic_output.find("false") != std::string::npos) ||
						   (wmic_output.find("FALSE") != std::string::npos)) {
					is_baremetal_ = true;
				} else {
					wmic_output = exec("diskpart /?");
					if (wmic_output.find("MININT") != std::string::npos) {
						is_baremetal_ = true;
					} else {
						unknown_platform = true;
					}
				}
			}
		}
	}
	if(is_linux_) {
		std::string hypervisor_str = "hypervisor";
		std::string linux_output = exec("lscpu");
		std::string gpu_id_list = exec("lspci -nn | awk -F'[][]' "
									   "'/Display/ {print $6} "
									   "/Processing/ {print $6}"
									   "/Processing/ {print $8}' | "
									   "awk -F':' '{print $2}'");
		std::string linux_output_gim_loaded = exec("lsmod | grep gim");
		std::string linux_output_amdgpu_loaded = exec("lsmod | grep amdgpu");
		std::string linux_output_gim_user_mode = exec("pgrep gim_user_mode");

		transform(gpu_id_list.begin(), gpu_id_list.end(), gpu_id_list.begin(),
				  ::toupper);

		is_nv32_ = check_if_nv32(gpu_id_list);
		is_mi300_ = check_if_mi300(gpu_id_list);
		is_mi200_ = check_if_mi200(gpu_id_list);

		if (linux_output_gim_loaded.empty() && linux_output_amdgpu_loaded.empty()
			&& linux_output_gim_user_mode.empty()) {
			unknown_platform = true;
			std::cout << "Error: Driver is not installed!" << std::endl;
		} else if (!linux_output_gim_loaded.empty() || !linux_output_gim_user_mode.empty()) {
			is_host_ = true;
		} else if (!linux_output.empty() && linux_output.find(hypervisor_str) == std::string::npos) {
			is_baremetal_ = true;
		} else {
			is_guest_os_ = true;
		}
	}
}

std::string AmdSmiPlatform::exec(const char *cmd)
{
	char buffer[128];
	std::string result = "";
#ifdef _WIN64
	FILE *pipe = _popen(cmd, "r");
#elif __linux__
	FILE *pipe = popen(cmd, "r");
#endif

	if (!pipe)
		throw std::runtime_error("popen() failed!");
	try {
		while (fgets(buffer, sizeof buffer, pipe) != NULL) {
			result += buffer;
		}
	} catch (...) {
#ifdef _WIN64
		_pclose(pipe);
#elif __linux__
		pclose(pipe);
#endif
		throw;
	}

#ifdef _WIN64
	_pclose(pipe);
#elif __linux__
	pclose(pipe);
#endif
	return result;
}

bool AmdSmiPlatform::is_host()
{
	return is_host_;
}
bool AmdSmiPlatform::is_guest()
{
	return is_guest_os_;
}
bool AmdSmiPlatform::is_baremetal()
{
	return is_baremetal_;
}
bool AmdSmiPlatform::is_windows()
{
	return is_windows_;
}
bool AmdSmiPlatform::is_linux()
{
	return is_linux_;
}
bool AmdSmiPlatform::is_mi300()
{
	return is_mi300_;
}
bool AmdSmiPlatform::is_nv32()
{
	return is_nv32_;
}
bool AmdSmiPlatform::is_mi200()
{
	return is_mi200_;
}