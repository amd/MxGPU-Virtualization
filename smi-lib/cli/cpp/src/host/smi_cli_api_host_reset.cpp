/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdsmi.h"
#include "smi_cli_api_host.h"
#include "smi_cli_parser.h"
#include "smi_cli_logger_err.h"
#include "smi_cli_templates.h"
#include "smi_cli_device.h"
#include "smi_cli_platform.h"

#include <numeric>
#include <set>
#include <sstream>
#include <vector>
#ifdef _WIN64
	#include <windows.h>
	#include <sysinfoapi.h>
#endif

typedef amdsmi_status_t (*AMDSMI_GET_VF_HANDLE_FROM_BDF)(amdsmi_bdf_t, amdsmi_vf_handle_t*);
typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
								amdsmi_processor_handle*);
typedef amdsmi_status_t (*AMDSMI_CLEAR_VF_FB)(amdsmi_vf_handle_t);
typedef amdsmi_status_t (*AMDSMI_RESET_GPU)(amdsmi_processor_handle);
typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLES)(amdsmi_socket_handle, uint32_t*,
							amdsmi_processor_handle*);
typedef amdsmi_status_t (*AMDSMI_GET_LINK_TOPOLOGY)(amdsmi_processor_handle,
						    amdsmi_processor_handle,
						    amdsmi_link_topology_t*);
typedef amdsmi_status_t (*AMDSMI_GET_XGMI_FB_SHARING_CAPS)(amdsmi_processor_handle,
							   amdsmi_xgmi_fb_sharing_caps_t*);
typedef amdsmi_status_t (*AMDSMI_GET_XGMI_FB_SHARING_MODE_INFO)(amdsmi_processor_handle,
								amdsmi_processor_handle,
								amdsmi_xgmi_fb_sharing_mode_t,
								uint8_t*);

extern AMDSMI_GET_VF_HANDLE_FROM_BDF host_amdsmi_get_vf_handle_from_bdf;
extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF host_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_CLEAR_VF_FB host_amdsmi_clear_vf_fb;
extern AMDSMI_RESET_GPU host_amdsmi_reset_gpu;
extern AMDSMI_GET_PROCESSOR_HANDLES host_amdsmi_get_processor_handles;
extern AMDSMI_GET_LINK_TOPOLOGY host_amdsmi_get_link_topology;
extern AMDSMI_GET_XGMI_FB_SHARING_CAPS host_amdsmi_get_xgmi_fb_sharing_caps;
extern AMDSMI_GET_XGMI_FB_SHARING_MODE_INFO host_amdsmi_get_xgmi_fb_sharing_mode_info;

/*
 * libgv reports PCIE (not XGMI) when FB sharing is off, even for GPUs in the
 * same physical hive. Group by the largest supported FB-sharing mode instead:
 * that query uses hive membership, not the current sharing mode.
 */
static amdsmi_xgmi_fb_sharing_mode_t hive_mode_from_caps(const amdsmi_xgmi_fb_sharing_caps_t& caps)
{
	if (caps.cap.mode_8_cap)
		return AMDSMI_XGMI_FB_SHARING_MODE_8;
	if (caps.cap.mode_4_cap)
		return AMDSMI_XGMI_FB_SHARING_MODE_4;
	if (caps.cap.mode_2_cap)
		return AMDSMI_XGMI_FB_SHARING_MODE_2;
	return AMDSMI_XGMI_FB_SHARING_MODE_1;
}

int AmdSmiApiHost::amdsmi_reset_command(std::string vf_bdf_str, Arguments arg)
{
	int ret;
	amdsmi_bdf_t vf_bdf;
	amdsmi_vf_handle_t vf_handle;

	vf_bdf.bdf.domain_number   = std::stoi(vf_bdf_str.substr(0, 4), nullptr, 16);
	vf_bdf.bdf.bus_number	   = std::stoi(vf_bdf_str.substr(5, 2), nullptr, 16);
	vf_bdf.bdf.device_number   = std::stoi(vf_bdf_str.substr(8, 2), nullptr, 16);
	vf_bdf.bdf.function_number = std::stoi(vf_bdf_str.substr(11), nullptr, 16);

	ret = host_amdsmi_get_vf_handle_from_bdf(vf_bdf, &vf_handle);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_clear_vf_fb(vf_handle);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}
	return ret;
}

int AmdSmiApiHost::amdsmi_reset_gpu_command(uint64_t processor_bdf, Arguments arg)
{
	int ret;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	const bool gpu_selected = !arg.device_format[DevicesType::GPU_TYPE].empty();
	const bool is_mi =
	    AmdSmiPlatform::getInstance().is_linux() &&
	    (AmdSmiPlatform::getInstance().is_mi200() || AmdSmiPlatform::getInstance().is_mi300() ||
	     AmdSmiPlatform::getInstance().is_mi350());

	if (is_mi) {
		uint32_t gpu_count = 0;
		ret		   = host_amdsmi_get_processor_handles(NULL, &gpu_count, NULL);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__,
						  __LINE__);
			return ret;
		}

		std::vector<amdsmi_processor_handle> processors(gpu_count);
		ret = host_amdsmi_get_processor_handles(NULL, &gpu_count, processors.data());
		if (ret != AMDSMI_STATUS_SUCCESS) {
			Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__,
						  __LINE__);
			return ret;
		}

		/*
		 * Build physical XGMI hive components before issuing any reset.
		 * One reset request per hive is enough: libgv forwards it to
		 * the hive master and chain-resets the whole hive.
		 */
		std::vector<uint32_t> parent(gpu_count);
		std::iota(parent.begin(), parent.end(), 0);

		auto find_root = [&parent](uint32_t node) {
			while (parent[node] != node) {
				parent[node] = parent[parent[node]];
				node	     = parent[node];
			}
			return node;
		};

		amdsmi_xgmi_fb_sharing_caps_t caps = {};
		bool have_caps			   = false;
		for (uint32_t i = 0; i < gpu_count && !have_caps; i++) {
			ret = host_amdsmi_get_xgmi_fb_sharing_caps(processors[i], &caps);
			if (ret == AMDSMI_STATUS_NOT_SUPPORTED)
				continue;
			if (ret != AMDSMI_STATUS_SUCCESS) {
				Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__,
							  __FILE__, __LINE__);
				return ret;
			}
			have_caps = true;
		}

		const amdsmi_xgmi_fb_sharing_mode_t hive_mode = hive_mode_from_caps(caps);

		for (uint32_t i = 0; i < gpu_count; i++) {
			for (uint32_t j = i + 1; j < gpu_count; j++) {
				bool same_hive = false;

				if (have_caps) {
					uint8_t sharing = 0;
					ret		= host_amdsmi_get_xgmi_fb_sharing_mode_info(
						processors[i], processors[j], hive_mode, &sharing);
					if (ret == AMDSMI_STATUS_NOT_SUPPORTED)
						continue;
					if (ret != AMDSMI_STATUS_SUCCESS) {
						Logger::getInstance().log(LogLevel::Error, ret,
									  __FUNCTION__, __FILE__,
									  __LINE__);
						return ret;
					}
					same_hive = (sharing != 0);
				} else {
					amdsmi_link_topology_t topology = {};
					ret = host_amdsmi_get_link_topology(
					    processors[i], processors[j], &topology);
					if (ret == AMDSMI_STATUS_NOT_SUPPORTED)
						continue;
					if (ret != AMDSMI_STATUS_SUCCESS) {
						Logger::getInstance().log(LogLevel::Error, ret,
									  __FUNCTION__, __FILE__,
									  __LINE__);
						return ret;
					}
					same_hive = (topology.link_type == AMDSMI_LINK_TYPE_XGMI);
				}

				if (same_hive) {
					uint32_t root_i = find_root(i);
					uint32_t root_j = find_root(j);
					parent[root_j]	= root_i;
				}
			}
		}

		std::set<uint32_t> roots_to_reset;
		if (!gpu_selected) {
			for (uint32_t i = 0; i < gpu_count; i++) {
				if (find_root(i) == i)
					roots_to_reset.insert(i);
			}
		} else {
			for (const auto& device : arg.devices) {
				amdsmi_processor_handle selected = nullptr;
				amdsmi_bdf_t selected_bdf	 = {};
				selected_bdf.as_uint		 = device->get_bdf();
				ret = host_amdsmi_get_processor_handle_from_bdf(selected_bdf,
										&selected);
				if (ret != AMDSMI_STATUS_SUCCESS) {
					Logger::getInstance().log(LogLevel::Error, ret,
								  __FUNCTION__, __FILE__, __LINE__);
					return ret;
				}

				bool found = false;
				for (uint32_t i = 0; i < gpu_count; i++) {
					if (processors[i] == selected) {
						roots_to_reset.insert(find_root(i));
						found = true;
						break;
					}
				}
				if (!found)
					return AMDSMI_STATUS_INVAL;
			}
		}

		for (uint32_t root : roots_to_reset) {
			ret = host_amdsmi_reset_gpu(processors[root]);
			if (ret != AMDSMI_STATUS_SUCCESS) {
				Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__,
							  __FILE__, __LINE__);
				return ret;
			}
		}

		return AMDSMI_STATUS_SUCCESS;
	}

	ret = host_amdsmi_reset_gpu(processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	return ret;
}