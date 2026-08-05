/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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

const std::vector<std::string> dev_id_list_mi30x = {"74A0", "74A1", "74A2", "74B6", "74A9", "74BD", "74A5", "74B9", "74A8", "74BC"};
const std::vector<std::string> dev_id_list_mi308 = {"74A2", "74A8"};
const std::vector<std::string> dev_id_list_mi350 = {"75A0", "75A1", "75A3", "75B0", "75B1", "75B3"};
const std::vector<std::string> dev_id_list_mi2plus = {"7410"};
const std::vector<std::string> dev_id_list_nv = {"73A1", "73AE", "73A8", "73BF", "744B", "744C", "73C8", "73DF", "747E", "73F0", "7480", "7499", "749F", "746F", "7550", "7551", "748F", "7590", "73C4", "73C5", "7460", "7461", "7470", "7478", "7448", "7449", "744A", "745E", "7481", "7483", "7487", "7489", "748B"};
const std::vector<std::string> dev_id_list_apu = {"150E", "1586"};


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

bool check_if_mi308(std::string output)
{
	std::string::size_type n;
	bool is_mi308{false};
	for (auto x : dev_id_list_mi308) {
		n = output.find(x);
		if (std::string::npos != n) {
			is_mi308 = true;
		}
	}
	return is_mi308;
}

bool check_if_mi350(std::string output)
{
	std::string::size_type n;
	bool is_mi350{false};
	for (auto x : dev_id_list_mi350) {
		n = output.find(x);
		if (std::string::npos != n) {
			is_mi350 = true;
		}
	}
	return is_mi350;
}

bool check_if_nv(std::string output)
{
	std::string::size_type n;
	bool is_nv{false};
	for (auto x : dev_id_list_nv) {
		n = output.find(x);
		if (std::string::npos != n) {
			is_nv = true;
		}
	}
	return is_nv;
}

bool check_if_apu(std::string output)
{
	std::string::size_type n;
	bool is_apu{false};
	for (auto x : dev_id_list_apu) {
		n = output.find(x);
		if (std::string::npos != n) {
			is_apu = true;
		}
	}
	return is_apu;
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

//function to check if system has access to hypervisor virtualization namespace
//only hypervisor host has access to ROOT\virtualization\v2; guests and bare metal don't
bool can_access_hyperv_namespace()
{
	HRESULT hres;
	//initialize COM interface
	CoInitializeEx(0, COINIT_MULTITHREADED);
	//set COM security levels
	CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, 
						 RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);

	//connect to WMI locator
	IWbemLocator *locator = 0;
	hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, 
							IID_IWbemLocator, (LPVOID *)&locator);
	if (FAILED(hres)) {
		CoUninitialize();
		return false;
	}

	//attempt to connect to hypervisor virtualization namespace
	//only hosts have access to this namespace
	IWbemServices *services = 0;
	BSTR networkResource = SysAllocString(L"ROOT\\virtualization\\v2");
	hres = locator->ConnectServer(networkResource, NULL, NULL, 0, NULL, 0, 0, &services);

	SysFreeString(networkResource);
	locator->Release();

	if (FAILED(hres)) {
		//no access to virtualization namespace - not a host
		CoUninitialize();
		return false;
	}

	//successfully connected to virtualization namespace - this is a host
	services->Release();
	CoUninitialize();
	return true;
}

//function to check if vmcompute.exe process is running
bool is_vmcompute_running()
{
	IWbemServices* services = connect_to_wmi();
	if (services == NULL) {
		CoUninitialize();
		return false;
	}

	IEnumWbemClassObject* enumerator = execute_wmi_query(services,
									   L"SELECT Name FROM Win32_Process WHERE Name='vmcompute.exe'");
	if (enumerator == NULL) {
		services->Release();
		CoUninitialize();
		return false;
	}

	//check if any process was found
	IWbemClassObject *result = NULL;
	ULONG returned_count = 0;
	HRESULT hres = enumerator->Next(WBEM_INFINITE, 1, &result, &returned_count);

	bool found = (hres == WBEM_S_NO_ERROR && returned_count > 0);

	if (result) {
		result->Release();
	}
	enumerator->Release();
	services->Release();
	CoUninitialize();

	return found;
}

//function to get computer system properties (Manufacturer, Model, HypervisorPresent)
struct ComputerSystemInfo {
	std::string manufacturer;
	std::string model;
	bool hypervisor_present;
};

ComputerSystemInfo get_computer_system_info()
{
	ComputerSystemInfo info = {"", "", false};

	IWbemServices* services = connect_to_wmi();
	if (services == NULL) {
		CoUninitialize();
		return info;
	}

	IEnumWbemClassObject* enumerator = execute_wmi_query(services,
									   L"SELECT Manufacturer, Model, HypervisorPresent FROM Win32_ComputerSystem");
	if (enumerator == NULL) {
		services->Release();
		CoUninitialize();
		return info;
	}

	IWbemClassObject *result = NULL;
	ULONG returned_count = 0;
	enumerator->Next(WBEM_INFINITE, 1, &result, &returned_count);

	if (returned_count > 0) {
		// Get Manufacturer
		VARIANT manufacturer;
		if (SUCCEEDED(result->Get(L"Manufacturer", 0, &manufacturer, 0, 0))) {
			if (manufacturer.vt == VT_BSTR && manufacturer.bstrVal) {
				int length = WideCharToMultiByte(CP_UTF8, 0, manufacturer.bstrVal, -1, NULL, 0, NULL, NULL);
				info.manufacturer.resize(length - 1);
				WideCharToMultiByte(CP_UTF8, 0, manufacturer.bstrVal, -1, &info.manufacturer[0], length, NULL, NULL);
			}
			VariantClear(&manufacturer);
		}

		// Get Model
		VARIANT model;
		if (SUCCEEDED(result->Get(L"Model", 0, &model, 0, 0))) {
			if (model.vt == VT_BSTR && model.bstrVal) {
				int length = WideCharToMultiByte(CP_UTF8, 0, model.bstrVal, -1, NULL, 0, NULL, NULL);
				info.model.resize(length - 1);
				WideCharToMultiByte(CP_UTF8, 0, model.bstrVal, -1, &info.model[0], length, NULL, NULL);
			}
			VariantClear(&model);
		}

		// Get HypervisorPresent
		VARIANT hypervisor_present;
		if (SUCCEEDED(result->Get(L"HypervisorPresent", 0, &hypervisor_present, 0, 0))) {
			if (hypervisor_present.vt == VT_BOOL) {
				info.hypervisor_present = (hypervisor_present.boolVal == VARIANT_TRUE);
			}
			VariantClear(&hypervisor_present);
		}
	}

	if (result) {
		result->Release();
	}
	enumerator->Release();
	services->Release();
	CoUninitialize();

	return info;
}

//detects the MxGPU host driver by probing the device it exposes.
bool is_host_driver_present()
{
	HANDLE handle = CreateFileW(L"\\\\.\\AmdGpuvSmi",
								GENERIC_READ | GENERIC_WRITE,
								FILE_SHARE_READ | FILE_SHARE_WRITE,
								NULL, OPEN_EXISTING, 0, NULL);
	if (handle != INVALID_HANDLE_VALUE) {
		CloseHandle(handle);
		return true;
	}
	//a failure other than "not found" still means the driver is present
	DWORD err = GetLastError();
	if (err != ERROR_FILE_NOT_FOUND && err != ERROR_PATH_NOT_FOUND) {
		return true;
	}
	return false;
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
	if (result) {
		result->Release();
	}
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
#ifdef _WIN64
		char* sys_root = nullptr;
		size_t sys_root_len = 0;
		_dupenv_s(&sys_root, &sys_root_len, "SYSTEMROOT");
		if (sys_root != nullptr) {
			std::string sys_root_str(sys_root);
			free(sys_root);
			std::transform(sys_root_str.begin(), sys_root_str.end(),
						   sys_root_str.begin(), ::toupper);
			if (sys_root_str.find("MININT") != std::string::npos) {
				is_baremetal_ = true;
				is_nv_ = true;
				return;
			}
		}
		std::string output = get_device_ids();
		is_mi300_ = check_if_mi30x(output);
		is_nv_ = check_if_nv(output);
		is_apu_ = check_if_apu(output);
		is_mi200_ = check_if_mi200(output);

		bool host_driver_present = is_host_driver_present();
		ComputerSystemInfo sys_info = get_computer_system_info();

		// A "Virtual" model string is the most reliable guest signature
		std::string model_upper = sys_info.model;
		std::transform(model_upper.begin(), model_upper.end(),
					   model_upper.begin(), ::toupper);
		bool is_vm = (model_upper.find("VIRTUAL") != std::string::npos);

		// Classify by host driver first, then guest signature, else bare metal.
		// The host driver is the only reliable host indicator; generic
		// virtualization features are avoided since they are common on
		// bare-metal desktops.
		if (host_driver_present) {
			is_host_ = true;
		} else if (is_vm) {
			is_guest_os_ = true;
		} else {
			is_baremetal_ = true;
		}
#endif
	}
	if(is_linux_) {
		std::string hypervisor_str = "hypervisor";
		std::string linux_output = exec("lscpu 2>/dev/null");
		std::string gpu_id_list = exec("lspci -nn | grep -oiE '1002:?[0-9a-f]{4}' | "
									   "sed -E 's/^1002:?//I'");
		std::string linux_output_gim_loaded = exec("lsmod 2>/dev/null | grep gim");
		std::string linux_output_amdgpu_loaded = exec("lsmod 2>/dev/null | grep amdgpu");
		std::string linux_output_gim_user_mode = exec("pgrep gim_user_mode");
		std::string linux_output_amdgpuv = exec("vmkload_mod -l 2>/dev/null | grep amdgpuv | awk '{print $1}'");
		linux_output_amdgpuv.erase(std::remove(linux_output_amdgpuv.begin(), linux_output_amdgpuv.end(), '\n'),linux_output_amdgpuv.end());

		if (linux_output_amdgpuv == "amdgpuv") {
			is_esxi_ = true;
			gpu_id_list = exec("lspci -p | grep amdgpuv | grep -oi '1002:[0-9a-fA-F]*' | cut -d: -f2 | sort -u");
		}

		transform(gpu_id_list.begin(), gpu_id_list.end(), gpu_id_list.begin(),
				  ::toupper);

		is_nv_ = check_if_nv(gpu_id_list);
		is_apu_ = check_if_apu(gpu_id_list);
		is_mi300_ = check_if_mi30x(gpu_id_list);
		is_mi308_ = check_if_mi308(gpu_id_list);
		is_mi350_ = check_if_mi350(gpu_id_list);
		is_mi200_ = check_if_mi200(gpu_id_list);

		if (linux_output_gim_loaded.empty() && linux_output_amdgpu_loaded.empty()
				&& linux_output_gim_user_mode.empty() && linux_output_amdgpuv.empty()) {
			throw SmiToolSMILIBErrorException(34);
		} else if (!linux_output_gim_loaded.empty() || !linux_output_gim_user_mode.empty() || !linux_output_amdgpuv.empty()) {
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
	/* Pin $PATH to the standard system directories so popen()'s bare helper
	 * names (lsmod, grep, pgrep, lspci, ...) resolve deterministically. */
	if (setenv("PATH", "/usr/sbin:/usr/bin:/sbin:/bin", 1) != 0)
		throw std::runtime_error("setenv(PATH) failed!");
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
bool AmdSmiPlatform::is_esxi()
{
	return is_esxi_;
}
bool AmdSmiPlatform::is_mi300()
{
	return is_mi300_;
}
bool AmdSmiPlatform::is_mi308()
{
	return is_mi308_;
}
bool AmdSmiPlatform::is_mi350()
{
	return is_mi350_;
}
bool AmdSmiPlatform::is_nv()
{
	return is_nv_;
}
bool AmdSmiPlatform::is_apu()
{
	return is_apu_;
}
bool AmdSmiPlatform::is_mi200()
{
	return is_mi200_;
}
bool AmdSmiPlatform::is_mixxx()
{
	return is_mixxx_;
}
std::string AmdSmiPlatform::get_platform()
{
	std::string os = "Unknown";
	if (is_windows() == true) {
		os = "Windows";
	} else if (is_esxi() == true) {
		os = "ESXi";
	} else if (is_linux() == true) {
		os = "Linux";
	}

	std::string platform = "Unknown";
	if (is_host() == true) {
		platform = "Host";
	} else if (is_guest() == true) {
		platform = "Guest";
	} else if (is_baremetal() == true) {
		platform = "Bare Metal";
	}

	return os + " " + platform;
}
