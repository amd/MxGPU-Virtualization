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
#include "smi_cli_exception.h"

#ifdef _WIN64
#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")
#endif

const std::vector<std::string> dev_id_list_mi30x = {"74A0", "74A1", "74A2", "74B6", "74A9", "74BD", "74A5", "74B9", "74A8", "74BC", "75A0", "75A1", "75A3", "75B0", "75B1", "75B3"};
const std::vector<std::string> dev_id_list_mi2plus = {"7410"};
const std::vector<std::string> dev_id_list_nv3plus = {"73C4", "73C5", "73C8", "7460", "7461"};

bool check_if_mi30x(std::string output)
{
	std::string::size_type n;
	bool is_mi300{false};
	for (auto x : dev_id_list_mi30x) {
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

#ifdef _WIN64
IWbemServices* connect_to_wmi()
{
	HRESULT hres;
	//initialize COM interface
	CoInitializeEx(0, COINIT_MULTITHREADED);
	//set COM security levels
	CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE,
						 NULL, EOAC_NONE, NULL);

	//connect to a WMI namespace
	IWbemLocator *locator = 0;
	CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator,
					 (LPVOID *)&locator); //find service
	IWbemServices *services = 0;
	BSTR networkResource = SysAllocString(L"ROOT\\CIMV2");  //Common Information Model (CIM)
	hres = locator->ConnectServer(networkResource, NULL, NULL, 0, NULL, 0, 0,
								  &services);  //connect to it
	if (FAILED(hres)) {
		locator->Release();
		CoUninitialize();
		throw std::runtime_error("Failed to connect to WMI service."); // Program has failed.
	}
	SysFreeString(networkResource);

	//release the locator and return the services
	locator->Release();
	return services;
}

//function to execute a WMI query and return the results
IEnumWbemClassObject* execute_wmi_query(IWbemServices* services, const wchar_t* query)
{
	HRESULT hres;
	//execute WMI query
	IEnumWbemClassObject* enumerator;
	BSTR language = SysAllocString(L"WQL"); //wmi query language
	BSTR bstrQuery = SysAllocString(query);
	hres = services->ExecQuery(language, bstrQuery,
							   WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &enumerator);
	if (FAILED(hres)) {
		services->Release();
		throw std::runtime_error("Failed to execute WMI query.");  // Program has failed.
	}
	SysFreeString(language);
	SysFreeString(bstrQuery);

	return enumerator;
}

std::string get_device_ids()
{
	//connect to WMI and execute query
	IWbemServices* services = connect_to_wmi();
	if (services == NULL) {
		CoUninitialize();
		throw std::runtime_error("Failed to connect to WMI service.");
	}
	IEnumWbemClassObject* enumerator = execute_wmi_query(services,
									   L"SELECT * FROM Win32_VideoController");
	if (enumerator == NULL) {
		services->Release();
		CoUninitialize();
		throw std::runtime_error("Failed to execute WMI query.");;
	}

	//get the results
	IWbemClassObject *result = NULL;
	ULONG returnedCount = 0;
	std::string all_device_ids;
	while (enumerator->Next(WBEM_INFINITE, 1, &result, &returnedCount) == WBEM_S_NO_ERROR) {
		//get the PNPDeviceID property
		VARIANT pnp_device_id;
		result->Get(L"PNPDeviceID", 0, &pnp_device_id, 0, 0);

		//convert the PNPDeviceID to a std::string
		int length = WideCharToMultiByte(CP_UTF8, 0, pnp_device_id.bstrVal, -1, NULL, 0, NULL, NULL);
		std::string output;
		output.resize(length - 1);  //subtract 1 to exclude the null terminator
		WideCharToMultiByte(CP_UTF8, 0, pnp_device_id.bstrVal, -1, &output[0], length, NULL, NULL);

		all_device_ids += "PNPDeviceID: " + output + "\n";

		//clean up
		VariantClear(&pnp_device_id);
		result->Release();
	}

	//clean up
	enumerator->Release();
	services->Release();
	CoUninitialize();

	return all_device_ids;
}

//function to check if the vmcompute process is running
bool is_vm_compute_running()
{
	//connect to WMI and execute query
	IWbemServices* services = connect_to_wmi();
	if (services == NULL) {
		CoUninitialize();
		throw std::runtime_error("Failed to connect to WMI service.");
	}
	IEnumWbemClassObject* enumerator = execute_wmi_query(services, L"SELECT * FROM Win32_Process");
	if (enumerator == NULL) {
		services->Release();
		CoUninitialize();
		throw std::runtime_error("Failed to execute WMI query.");
	}

	//get the results
	IWbemClassObject *result = NULL;
	ULONG returned_count = 0;
	bool is_running = false;
	while (enumerator->Next(WBEM_INFINITE, 1, &result, &returned_count) == WBEM_S_NO_ERROR) {
		//get the caption property
		VARIANT caption;
		result->Get(L"Caption", 0, &caption, 0, 0);

		//check if the caption is "vmcompute"
		if (wcsncmp(caption.bstrVal, L"vmcompute", 9) == 0) {
			is_running = true;
			VariantClear(&caption);
			break;
		}

		//clean up
		VariantClear(&caption);
		result->Release();
	}

	//clean up
	enumerator->Release();
	services->Release();
	CoUninitialize();

	return is_running;
}

std::string is_virtualization_host()
{
	//connect to WMI and execute query
	IWbemServices* services = connect_to_wmi();
	if (services == NULL) {
		CoUninitialize();
		throw std::runtime_error("Failed to connect to WMI service.");
	}
	IEnumWbemClassObject* enumerator = execute_wmi_query(services,
									   L"SELECT * FROM Win32_ComputerSystem");
	if (enumerator == NULL) {
		services->Release();
		CoUninitialize();
		throw std::runtime_error("Failed to execute WMI query.");
	}

	//get the result
	IWbemClassObject *result = NULL;
	ULONG returned_count = 0;
	enumerator->Next(WBEM_INFINITE, 1, &result, &returned_count);

	std::string status = "UNKNOWN";
	if (returned_count > 0) {
		VARIANT virtualization_present;
		result->Get(L"HypervisorPresent", 0, &virtualization_present, 0, 0);
		if (virtualization_present.boolVal == VARIANT_TRUE) {
			status = "TRUE";
		} else {
			status = "FALSE";
		}
		VariantClear(&virtualization_present);
	}

	// Clean up
	result->Release();
	enumerator->Release();
	services->Release();
	CoUninitialize();

	return status;
}
#endif

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
#ifdef _WIN64
		std::string output = get_device_ids();
		is_mi300_ = check_if_mi30x(output);
		is_nv32_ = check_if_nv32(output);
		is_mi200_ = check_if_mi200(output);

		if (is_vm_compute_running()) {
			is_host_ = true;
		}

		if(!is_host_) {
			std::string status = is_virtualization_host();
			if (status == "TRUE") {
				is_guest_os_ = true;
			} else if (status == "FALSE") {
				is_baremetal_ = true;
			} else {
				std::string diskpart_out = exec("diskpart /?");
				if (diskpart_out.find("MININT") != std::string::npos) {
					is_baremetal_ = true;
				} else {
					unknown_platform = true;
				}
			}
		}
#endif
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
		is_mi300_ = check_if_mi30x(gpu_id_list);
		is_mi200_ = check_if_mi200(gpu_id_list);

		if (linux_output_gim_loaded.empty() && linux_output_amdgpu_loaded.empty()
				&& linux_output_gim_user_mode.empty()) {
			throw SmiToolSMILIBErrorException(34);
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
