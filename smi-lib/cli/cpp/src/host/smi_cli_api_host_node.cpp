/* * Copyright (C) 2025 Advanced Micro Devices. All rights reserved.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "amdsmi.h"
#include "smi_cli_api_host.h"
#include "smi_cli_helpers.h"
#include "smi_cli_parser.h"
#include "smi_cli_logger_err.h"
#include "smi_cli_templates.h"
#include "smi_cli_device.h"
#include "smi_cli_exception.h"
#include "smi_cli_platform.h"

#include "json/json.h"

#include <sstream>
#ifdef _WIN64
#include <windows.h>
#include <sysinfoapi.h>
#endif

#include <set>

typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLES)(amdsmi_socket_handle, uint32_t *,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_METRICS)(amdsmi_processor_handle, uint32_t *,
		amdsmi_metric_t *);
typedef amdsmi_status_t (*AMDSMI_GET_NODE_HANDLE)(amdsmi_processor_handle, amdsmi_node_handle*);
typedef amdsmi_status_t (*AMDSMI_GET_NPM_INFO)(amdsmi_node_handle, amdsmi_npm_info_t *);

extern AMDSMI_GET_PROCESSOR_HANDLES host_amdsmi_get_processor_handles;
extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF host_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_GPU_METRICS host_amdsmi_get_gpu_metrics;
extern AMDSMI_GET_NODE_HANDLE host_amdsmi_get_node_handle;
extern AMDSMI_GET_NPM_INFO host_amdsmi_get_npm_info;

std::string host_fill_node_npm_info(Arguments arg, std::string value = "N/A")
{
	std::string out{};

	if (arg.output == json) {
		nlohmann::ordered_json npm_info_json = {
			{ "limit", value },
			{ "status", value }
		};

		out = npm_info_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format("%s,%s", value.c_str(), value.c_str());
	} else {
		out = string_format(nodePowerManagementTemplate, value.c_str(), value.c_str());
	}

	return out;
}

int AmdSmiApiHost::amdsmi_get_baseboard_command(uint64_t processor_bdf, Arguments arg,
			std::string &formatted_string)
{
	int ret;
	amdsmi_processor_handle processor;
	amdsmi_metric_t *metrics;
	uint32_t metric_size = AMDSMI_MAX_NUM_METRICS;
	bool is_supported = false;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	std::vector<amdsmi_metric_t> system_temp_ubb_fpga{};
	std::vector<amdsmi_metric_t> system_temp_ubb_front{};
	std::vector<amdsmi_metric_t> system_temp_ubb_back{};
	std::vector<amdsmi_metric_t> system_temp_ubb_oam7{};
	std::vector<amdsmi_metric_t> system_temp_ubb_ibc{};
	std::vector<amdsmi_metric_t> system_temp_ubb_ufpga{};
	std::vector<amdsmi_metric_t> system_temp_ubb_oam1{};
	std::vector<amdsmi_metric_t> system_temp_oam_0_1_hsc{};
	std::vector<amdsmi_metric_t> system_temp_oam_2_3_hsc{};
	std::vector<amdsmi_metric_t> system_temp_oam_4_5_hsc{};
	std::vector<amdsmi_metric_t> system_temp_oam_6_7_hsc{};
	std::vector<amdsmi_metric_t> system_temp_ubb_fpga_0v72_vr{};
	std::vector<amdsmi_metric_t> system_temp_ubb_fpga_3v3_vr{};
	std::vector<amdsmi_metric_t> system_temp_retimer_0_1_2_3_1v2_vr{};
	std::vector<amdsmi_metric_t> system_temp_retimer_4_5_6_7_1v2_vr{};
	std::vector<amdsmi_metric_t> system_temp_retimer_0_1_0v9_vr{};
	std::vector<amdsmi_metric_t> system_temp_retimer_4_5_0v9_vr{};
	std::vector<amdsmi_metric_t> system_temp_retimer_2_3_0v9_vr{};
	std::vector<amdsmi_metric_t> system_temp_retimer_6_7_0v9_vr{};
	std::vector<amdsmi_metric_t> system_temp_oam_0_1_2_3_3v3_vr{};
	std::vector<amdsmi_metric_t> system_temp_oam_4_5_6_7_3v3_vr{};
	std::vector<amdsmi_metric_t> system_temp_ibc_hsc{};
	std::vector<amdsmi_metric_t> system_temp_ibc{};

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}

	ret = host_amdsmi_get_gpu_metrics(processor, &metric_size, NULL);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}
	metrics = (amdsmi_metric_t *)malloc(sizeof(amdsmi_metric_t)*metric_size);
	if (metrics == NULL) {
		throw SmiToolNotEnoughMemException();
	}
	ret = host_amdsmi_get_gpu_metrics(processor, &metric_size, &metrics[0]);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}

	for (uint32_t i = 0; i < metric_size; i++) {
		if (!(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC) &&
			(metrics[i].flags & AMDSMI_METRIC_TYPE_INST)) {
			switch (metrics[i].name) {
				case AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_FPGA:
					system_temp_ubb_fpga.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_FRONT:
					system_temp_ubb_front.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_BACK:
					system_temp_ubb_back.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_OAM7:
					system_temp_ubb_oam7.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_IBC:
					system_temp_ubb_ibc.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_UFPGA:
					system_temp_ubb_ufpga.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_OAM1:
					system_temp_ubb_oam1.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_0_1_HSC:
					system_temp_oam_0_1_hsc.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_2_3_HSC:
					system_temp_oam_2_3_hsc.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_4_5_HSC:
					system_temp_oam_4_5_hsc.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_6_7_HSC:
					system_temp_oam_6_7_hsc.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_FPGA_0V72_VR:
					system_temp_ubb_fpga_0v72_vr.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_FPGA_3V3_VR:
					system_temp_ubb_fpga_3v3_vr.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_0_1_2_3_1V2_VR:
					system_temp_retimer_0_1_2_3_1v2_vr.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_4_5_6_7_1V2_VR:
					system_temp_retimer_4_5_6_7_1v2_vr.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_0_1_0V9_VR:
					system_temp_retimer_0_1_0v9_vr.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_4_5_0V9_VR:
					system_temp_retimer_4_5_0v9_vr.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_2_3_0V9_VR:
					system_temp_retimer_2_3_0v9_vr.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_6_7_0V9_VR:
					system_temp_retimer_6_7_0v9_vr.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_0_1_2_3_3V3_VR:
					system_temp_oam_0_1_2_3_3v3_vr.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_4_5_6_7_3V3_VR:
					system_temp_oam_4_5_6_7_3v3_vr.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_SYSTEM_TEMP_IBC_HSC:
					system_temp_ibc_hsc.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_SYSTEM_TEMP_IBC:
					system_temp_ibc.push_back(metrics[i]);
					break;
				default:
					break;
			}
		}
	}

	free(metrics);

	if (arg.output == json) {
		nlohmann::ordered_json baseboard_json;
		if (system_temp_ubb_fpga.size() == 0) {
			baseboard_json["ubb_fpga"] = {
				{"value", "N/A"},
				{"unit", ""}
			};
		} else {
			for (uint32_t i = 0; i < system_temp_ubb_fpga.size(); i++) {
				if (system_temp_ubb_fpga[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ubb_fpga[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					baseboard_json["ubb_fpga"] = {
						{"value", system_temp_ubb_fpga[i].val},
						{"unit", system_temp_ubb_fpga[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (system_temp_ubb_front.size() == 0) {
			baseboard_json["ubb_front"] = {
				{"value", "N/A"},
				{"unit", ""}
			};
		} else {
			for (uint32_t i = 0; i < system_temp_ubb_front.size(); i++) {
				if (system_temp_ubb_front[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ubb_front[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					baseboard_json["ubb_front"] = {
						{"value", system_temp_ubb_front[i].val},
						{"unit", system_temp_ubb_front[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (system_temp_ubb_back.size() == 0) {
			baseboard_json["ubb_back"] = {
				{"value", "N/A"},
				{"unit", ""}
			};
		} else {
			for (uint32_t i = 0; i < system_temp_ubb_back.size(); i++) {
				if (system_temp_ubb_back[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ubb_back[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					baseboard_json["ubb_back"] = {
						{"value", system_temp_ubb_back[i].val},
						{"unit", system_temp_ubb_back[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (system_temp_ubb_oam7.size() == 0) {
			baseboard_json["ubb_oam7"] = {
				{"value", "N/A"},
				{"unit", ""}
			};
		} else {
			for (uint32_t i = 0; i < system_temp_ubb_oam7.size(); i++) {
				if (system_temp_ubb_oam7[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ubb_oam7[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					baseboard_json["ubb_oam7"] = {
						{"value", system_temp_ubb_oam7[i].val},
						{"unit", system_temp_ubb_oam7[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (system_temp_ubb_ibc.size() == 0) {
			baseboard_json["ubb_ibc"] = {
				{"value", "N/A"},
				{"unit", ""}
			};
		} else {
			for (uint32_t i = 0; i < system_temp_ubb_ibc.size(); i++) {
				if (system_temp_ubb_ibc[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ubb_ibc[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					baseboard_json["ubb_ibc"] = {
						{"value", system_temp_ubb_ibc[i].val},
						{"unit", system_temp_ubb_ibc[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (system_temp_ubb_ufpga.size() == 0) {
			baseboard_json["ubb_ufpga"] = {
				{"value", "N/A"},
				{"unit", ""}
			};
		} else {
			for (uint32_t i = 0; i < system_temp_ubb_ufpga.size(); i++) {
				if (system_temp_ubb_ufpga[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ubb_ufpga[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					baseboard_json["ubb_ufpga"] = {
						{"value", system_temp_ubb_ufpga[i].val},
						{"unit", system_temp_ubb_ufpga[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (system_temp_ubb_oam1.size() == 0) {
			baseboard_json["ubb_oam1"] = {
				{"value", "N/A"},
				{"unit", ""}
			};
		} else {
			for (uint32_t i = 0; i < system_temp_ubb_oam1.size(); i++) {
				if (system_temp_ubb_oam1[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ubb_oam1[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					baseboard_json["ubb_oam1"] = {
						{"value", system_temp_ubb_oam1[i].val},
						{"unit", system_temp_ubb_oam1[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (system_temp_oam_0_1_hsc.size() == 0) {
			baseboard_json["oam_0_1_hsc"] = {
				{"value", "N/A"},
				{"unit", ""}
			};
		} else {
			for (uint32_t i = 0; i < system_temp_oam_0_1_hsc.size(); i++) {
				if (system_temp_oam_0_1_hsc[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_oam_0_1_hsc[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					baseboard_json["oam_0_1_hsc"] = {
						{"value", system_temp_oam_0_1_hsc[i].val},
						{"unit", system_temp_oam_0_1_hsc[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (system_temp_oam_2_3_hsc.size() == 0) {
			baseboard_json["oam_2_3_hsc"] = {
				{"value", "N/A"},
				{"unit", ""}
			};
		} else {
			for (uint32_t i = 0; i < system_temp_oam_2_3_hsc.size(); i++) {
				if (system_temp_oam_2_3_hsc[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_oam_2_3_hsc[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					baseboard_json["oam_2_3_hsc"] = {
						{"value", system_temp_oam_2_3_hsc[i].val},
						{"unit", system_temp_oam_2_3_hsc[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (system_temp_oam_4_5_hsc.size() == 0) {
			baseboard_json["oam_4_5_hsc"] = {
				{"value", "N/A"},
				{"unit", ""}
			};
		} else {
			for (uint32_t i = 0; i < system_temp_oam_4_5_hsc.size(); i++) {
				if (system_temp_oam_4_5_hsc[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_oam_4_5_hsc[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					baseboard_json["oam_4_5_hsc"] = {
						{"value", system_temp_oam_4_5_hsc[i].val},
						{"unit", system_temp_oam_4_5_hsc[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (system_temp_oam_6_7_hsc.size() == 0) {
			baseboard_json["oam_6_7_hsc"] = {
				{"value", "N/A"},
				{"unit", ""}
			};
		} else {
			for (uint32_t i = 0; i < system_temp_oam_6_7_hsc.size(); i++) {
				if (system_temp_oam_6_7_hsc[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_oam_6_7_hsc[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					baseboard_json["oam_6_7_hsc"] = {
						{"value", system_temp_oam_6_7_hsc[i].val},
						{"unit", system_temp_oam_6_7_hsc[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (system_temp_ubb_fpga_0v72_vr.size() == 0) {
			baseboard_json["ubb_fpga_0v72_vr"] = {
				{"value", "N/A"},
				{"unit", ""}
			};
		} else {
			for (uint32_t i = 0; i < system_temp_ubb_fpga_0v72_vr.size(); i++) {
				if (system_temp_ubb_fpga_0v72_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ubb_fpga_0v72_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					baseboard_json["ubb_fpga_0v72_vr"] = {
						{"value", system_temp_ubb_fpga_0v72_vr[i].val},
						{"unit", system_temp_ubb_fpga_0v72_vr[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (system_temp_ubb_fpga_3v3_vr.size() == 0) {
			baseboard_json["ubb_fpga_3v3_vr"] = {
				{"value", "N/A"},
				{"unit", ""}
			};
		} else {
			for (uint32_t i = 0; i < system_temp_ubb_fpga_3v3_vr.size(); i++) {
				if (system_temp_ubb_fpga_3v3_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ubb_fpga_3v3_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					baseboard_json["ubb_fpga_3v3_vr"] = {
						{"value", system_temp_ubb_fpga_3v3_vr[i].val},
						{"unit", system_temp_ubb_fpga_3v3_vr[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (system_temp_retimer_0_1_2_3_1v2_vr.size() == 0) {
			baseboard_json["retimer_0_1_2_3_1v2_vr"] = {
				{"value", "N/A"},
				{"unit", ""}
			};
		} else {
			for (uint32_t i = 0; i < system_temp_retimer_0_1_2_3_1v2_vr.size(); i++) {
				if (system_temp_retimer_0_1_2_3_1v2_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_retimer_0_1_2_3_1v2_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					baseboard_json["retimer_0_1_2_3_1v2_vr"] = {
						{"value", system_temp_retimer_0_1_2_3_1v2_vr[i].val},
						{"unit", system_temp_retimer_0_1_2_3_1v2_vr[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (system_temp_retimer_4_5_6_7_1v2_vr.size() == 0) {
			baseboard_json["retimer_4_5_6_7_1v2_vr"] = {
				{"value", "N/A"},
				{"unit", ""}
			};
		} else {
			for (uint32_t i = 0; i < system_temp_retimer_4_5_6_7_1v2_vr.size(); i++) {
				if (system_temp_retimer_4_5_6_7_1v2_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_retimer_4_5_6_7_1v2_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					baseboard_json["retimer_4_5_6_7_1v2_vr"] = {
						{"value", system_temp_retimer_4_5_6_7_1v2_vr[i].val},
						{"unit", system_temp_retimer_4_5_6_7_1v2_vr[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (system_temp_retimer_0_1_0v9_vr.size() == 0) {
			baseboard_json["retimer_0_1_0v9_vr"] = {
				{"value", "N/A"},
				{"unit", ""}
			};
		} else {
			for (uint32_t i = 0; i < system_temp_retimer_0_1_0v9_vr.size(); i++) {
				if (system_temp_retimer_0_1_0v9_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_retimer_0_1_0v9_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					baseboard_json["retimer_0_1_0v9_vr"] = {
						{"value", system_temp_retimer_0_1_0v9_vr[i].val},
						{"unit", system_temp_retimer_0_1_0v9_vr[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (system_temp_retimer_4_5_0v9_vr.size() == 0) {
			baseboard_json["retimer_4_5_0v9_vr"] = {
				{"value", "N/A"},
				{"unit", ""}
			};
		} else {
			for (uint32_t i = 0; i < system_temp_retimer_4_5_0v9_vr.size(); i++) {
				if (system_temp_retimer_4_5_0v9_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_retimer_4_5_0v9_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					baseboard_json["retimer_4_5_0v9_vr"] = {
						{"value", system_temp_retimer_4_5_0v9_vr[i].val},
						{"unit", system_temp_retimer_4_5_0v9_vr[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (system_temp_retimer_2_3_0v9_vr.size() == 0) {
			baseboard_json["retimer_2_3_0v9_vr"] = {
				{"value", "N/A"},
				{"unit", ""}
			};
		} else {
			for (uint32_t i = 0; i < system_temp_retimer_2_3_0v9_vr.size(); i++) {
				if (system_temp_retimer_2_3_0v9_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_retimer_2_3_0v9_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					baseboard_json["retimer_2_3_0v9_vr"] = {
						{"value", system_temp_retimer_2_3_0v9_vr[i].val},
						{"unit", system_temp_retimer_2_3_0v9_vr[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (system_temp_retimer_6_7_0v9_vr.size() == 0) {
			baseboard_json["retimer_6_7_0v9_vr"] = {
				{"value", "N/A"},
				{"unit", ""}
			};
		} else {
			for (uint32_t i = 0; i < system_temp_retimer_6_7_0v9_vr.size(); i++) {
				if (system_temp_retimer_6_7_0v9_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_retimer_6_7_0v9_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					baseboard_json["retimer_6_7_0v9_vr"] = {
						{"value", system_temp_retimer_6_7_0v9_vr[i].val},
						{"unit", system_temp_retimer_6_7_0v9_vr[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (system_temp_oam_0_1_2_3_3v3_vr.size() == 0) {
			baseboard_json["oam_0_1_2_3_3v3_vr"] = {
				{"value", "N/A"},
				{"unit", ""}
			};
		} else {
			for (uint32_t i = 0; i < system_temp_oam_0_1_2_3_3v3_vr.size(); i++) {
				if (system_temp_oam_0_1_2_3_3v3_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_oam_0_1_2_3_3v3_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					baseboard_json["oam_0_1_2_3_3v3_vr"] = {
						{"value", system_temp_oam_0_1_2_3_3v3_vr[i].val},
						{"unit", system_temp_oam_0_1_2_3_3v3_vr[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (system_temp_oam_4_5_6_7_3v3_vr.size() == 0) {
			baseboard_json["oam_4_5_6_7_3v3_vr"] = {
				{"value", "N/A"},
				{"unit", ""}
			};
		} else {
			for (uint32_t i = 0; i < system_temp_oam_4_5_6_7_3v3_vr.size(); i++) {
				if (system_temp_oam_4_5_6_7_3v3_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_oam_4_5_6_7_3v3_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					baseboard_json["oam_4_5_6_7_3v3_vr"] = {
						{"value", system_temp_oam_4_5_6_7_3v3_vr[i].val},
						{"unit", system_temp_oam_4_5_6_7_3v3_vr[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (system_temp_ibc_hsc.size() == 0) {
			baseboard_json["ibc_hsc"] = {
				{"value", "N/A"},
				{"unit", ""}
			};
		} else {
			for (uint32_t i = 0; i < system_temp_ibc_hsc.size(); i++) {
				if (system_temp_ibc_hsc[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ibc_hsc[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					baseboard_json["ibc_hsc"] = {
						{"value", system_temp_ibc_hsc[i].val},
						{"unit", system_temp_ibc_hsc[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (system_temp_ibc.size() == 0) {
			baseboard_json["ibc"] = {
				{"value", "N/A"},
				{"unit", ""}
			};
		} else {
			for (uint32_t i = 0; i < system_temp_ibc.size(); i++) {
				if (system_temp_ibc[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ibc[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					baseboard_json["ibc"] = {
						{"value", system_temp_ibc[i].val},
						{"unit", system_temp_ibc[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		formatted_string = baseboard_json.dump(4);
	} else if (arg.output == csv) {
		if (system_temp_ubb_fpga.size() == 0) {
			formatted_string += string_format("%s", "N/A");
		} else {
			for (uint32_t i = 0; i < system_temp_ubb_fpga.size(); i++) {
				if (system_temp_ubb_fpga[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ubb_fpga[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					formatted_string += string_format("%d", system_temp_ubb_fpga[i].val);
				}
			}
		}
		if (system_temp_ubb_front.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < system_temp_ubb_front.size(); i++) {
				if (system_temp_ubb_front[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ubb_front[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", system_temp_ubb_front[i].val);
				}
			}
		}
		if (system_temp_ubb_back.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < system_temp_ubb_back.size(); i++) {
				if (system_temp_ubb_back[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ubb_back[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", system_temp_ubb_back[i].val);
				}
			}
		}
		if (system_temp_ubb_oam7.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < system_temp_ubb_oam7.size(); i++) {
				if (system_temp_ubb_oam7[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ubb_oam7[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", system_temp_ubb_oam7[i].val);
				}
			}
		}
		if (system_temp_ubb_ibc.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < system_temp_ubb_ibc.size(); i++) {
				if (system_temp_ubb_ibc[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ubb_ibc[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", system_temp_ubb_ibc[i].val);
				}
			}
		}
		if (system_temp_ubb_ufpga.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < system_temp_ubb_ufpga.size(); i++) {
				if (system_temp_ubb_ufpga[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ubb_ufpga[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", system_temp_ubb_ufpga[i].val);
				}
			}
		}
		if (system_temp_ubb_oam1.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < system_temp_ubb_oam1.size(); i++) {
				if (system_temp_ubb_oam1[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ubb_oam1[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", system_temp_ubb_oam1[i].val);
				}
			}
		}
		if (system_temp_oam_0_1_hsc.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < system_temp_oam_0_1_hsc.size(); i++) {
				if (system_temp_oam_0_1_hsc[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_oam_0_1_hsc[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", system_temp_oam_0_1_hsc[i].val);
				}
			}
		}
		if (system_temp_oam_2_3_hsc.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < system_temp_oam_2_3_hsc.size(); i++) {
				if (system_temp_oam_2_3_hsc[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_oam_2_3_hsc[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", system_temp_oam_2_3_hsc[i].val);
				}
			}
		}
		if (system_temp_oam_4_5_hsc.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < system_temp_oam_4_5_hsc.size(); i++) {
				if (system_temp_oam_4_5_hsc[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_oam_4_5_hsc[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", system_temp_oam_4_5_hsc[i].val);
				}
			}
		}
		if (system_temp_oam_6_7_hsc.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < system_temp_oam_6_7_hsc.size(); i++) {
				if (system_temp_oam_6_7_hsc[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_oam_6_7_hsc[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", system_temp_oam_6_7_hsc[i].val);
				}
			}
		}
		if (system_temp_ubb_fpga_0v72_vr.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < system_temp_ubb_fpga_0v72_vr.size(); i++) {
				if (system_temp_ubb_fpga_0v72_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ubb_fpga_0v72_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", system_temp_ubb_fpga_0v72_vr[i].val);
				}
			}
		}
		if (system_temp_ubb_fpga_3v3_vr.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < system_temp_ubb_fpga_3v3_vr.size(); i++) {
				if (system_temp_ubb_fpga_3v3_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ubb_fpga_3v3_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", system_temp_ubb_fpga_3v3_vr[i].val);
				}
			}
		}
		if (system_temp_retimer_0_1_2_3_1v2_vr.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < system_temp_retimer_0_1_2_3_1v2_vr.size(); i++) {
				if (system_temp_retimer_0_1_2_3_1v2_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_retimer_0_1_2_3_1v2_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", system_temp_retimer_0_1_2_3_1v2_vr[i].val);
				}
			}
		}
		if (system_temp_retimer_4_5_6_7_1v2_vr.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < system_temp_retimer_4_5_6_7_1v2_vr.size(); i++) {
				if (system_temp_retimer_4_5_6_7_1v2_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_retimer_4_5_6_7_1v2_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", system_temp_retimer_4_5_6_7_1v2_vr[i].val);
				}
			}
		}
		if (system_temp_retimer_0_1_0v9_vr.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < system_temp_retimer_0_1_0v9_vr.size(); i++) {
				if (system_temp_retimer_0_1_0v9_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_retimer_0_1_0v9_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", system_temp_retimer_0_1_0v9_vr[i].val);
				}
			}
		}
		if (system_temp_retimer_4_5_0v9_vr.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < system_temp_retimer_4_5_0v9_vr.size(); i++) {
				if (system_temp_retimer_4_5_0v9_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_retimer_4_5_0v9_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", system_temp_retimer_4_5_0v9_vr[i].val);
				}
			}
		}
		if (system_temp_retimer_2_3_0v9_vr.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < system_temp_retimer_2_3_0v9_vr.size(); i++) {
				if (system_temp_retimer_2_3_0v9_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_retimer_2_3_0v9_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", system_temp_retimer_2_3_0v9_vr[i].val);
				}
			}
		}
		if (system_temp_retimer_6_7_0v9_vr.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < system_temp_retimer_6_7_0v9_vr.size(); i++) {
				if (system_temp_retimer_6_7_0v9_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_retimer_6_7_0v9_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", system_temp_retimer_6_7_0v9_vr[i].val);
				}
			}
		}
		if (system_temp_oam_0_1_2_3_3v3_vr.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < system_temp_oam_0_1_2_3_3v3_vr.size(); i++) {
				if (system_temp_oam_0_1_2_3_3v3_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_oam_0_1_2_3_3v3_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", system_temp_oam_0_1_2_3_3v3_vr[i].val);
				}
			}
		}
		if (system_temp_oam_4_5_6_7_3v3_vr.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < system_temp_oam_4_5_6_7_3v3_vr.size(); i++) {
				if (system_temp_oam_4_5_6_7_3v3_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_oam_4_5_6_7_3v3_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", system_temp_oam_4_5_6_7_3v3_vr[i].val);
				}
			}
		}
		if (system_temp_ibc_hsc.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < system_temp_ibc_hsc.size(); i++) {
				if (system_temp_ibc_hsc[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ibc_hsc[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", system_temp_ibc_hsc[i].val);
				}
			}
		}
		if (system_temp_ibc.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < system_temp_ibc.size(); i++) {
				if (system_temp_ibc[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ibc[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", system_temp_ibc[i].val);
				}
			}
		}
	} else {
		formatted_string = BaseBoardHeaderTemplate;
		if (system_temp_ubb_fpga.size() == 0) {
			formatted_string += string_format(baseboardSystemTempUbbFpgaTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < system_temp_ubb_fpga.size(); i++) {
				if (system_temp_ubb_fpga[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ubb_fpga[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					std::string system_temp_ubb_fpga_unit = system_temp_ubb_fpga[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string system_temp_ubb_fpga_chiplet_val = string_format("%d", system_temp_ubb_fpga[i].val);
					formatted_string += string_format(baseboardSystemTempUbbFpgaTemplate, system_temp_ubb_fpga_chiplet_val.c_str(), system_temp_ubb_fpga_unit.c_str());
				}
			}
		}
		if (system_temp_ubb_front.size() == 0) {
			formatted_string += string_format(baseboardSystemTempUbbFrontTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < system_temp_ubb_front.size(); i++) {
				if (system_temp_ubb_front[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ubb_front[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					std::string system_temp_ubb_front_unit = system_temp_ubb_front[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string system_temp_ubb_front_chiplet_val = string_format("%d", system_temp_ubb_front[i].val);
					formatted_string += string_format(baseboardSystemTempUbbFrontTemplate, system_temp_ubb_front_chiplet_val.c_str(), system_temp_ubb_front_unit.c_str());
				}
			}
		}
		if (system_temp_ubb_back.size() == 0) {
			formatted_string += string_format(baseboardSystemTempUbbBackTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < system_temp_ubb_back.size(); i++) {
				if (system_temp_ubb_back[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ubb_back[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					std::string system_temp_ubb_back_unit = system_temp_ubb_back[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string system_temp_ubb_back_chiplet_val = string_format("%d", system_temp_ubb_back[i].val);
					formatted_string += string_format(baseboardSystemTempUbbBackTemplate, system_temp_ubb_back_chiplet_val.c_str(), system_temp_ubb_back_unit.c_str());
				}
			}
		}
		if (system_temp_ubb_oam7.size() == 0) {
			formatted_string += string_format(baseboardSystemTempUbbOam7Template, "N/A", "");
		} else {
			for (uint32_t i = 0; i < system_temp_ubb_oam7.size(); i++) {
				if (system_temp_ubb_oam7[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ubb_oam7[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					std::string system_temp_ubb_oam7_unit = system_temp_ubb_oam7[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string system_temp_ubb_oam7_chiplet_val = string_format("%d", system_temp_ubb_oam7[i].val);
					formatted_string += string_format(baseboardSystemTempUbbOam7Template, system_temp_ubb_oam7_chiplet_val.c_str(), system_temp_ubb_oam7_unit.c_str());
				}
			}
		}
		if (system_temp_ubb_ibc.size() == 0) {
			formatted_string += string_format(baseboardSystemTempUbbIbcTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < system_temp_ubb_ibc.size(); i++) {
				if (system_temp_ubb_ibc[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ubb_ibc[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					std::string system_temp_ubb_ibc_unit = system_temp_ubb_ibc[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string system_temp_ubb_ibc_chiplet_val = string_format("%d", system_temp_ubb_ibc[i].val);
					formatted_string += string_format(baseboardSystemTempUbbIbcTemplate, system_temp_ubb_ibc_chiplet_val.c_str(), system_temp_ubb_ibc_unit.c_str());
				}
			}
		}
		if (system_temp_ubb_ufpga.size() == 0) {
			formatted_string += string_format(baseboardSystemTempUbbUfpgaTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < system_temp_ubb_ufpga.size(); i++) {
				if (system_temp_ubb_ufpga[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ubb_ufpga[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					std::string system_temp_ubb_ufpga_unit = system_temp_ubb_ufpga[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string system_temp_ubb_ufpga_chiplet_val = string_format("%d", system_temp_ubb_ufpga[i].val);
					formatted_string += string_format(baseboardSystemTempUbbUfpgaTemplate, system_temp_ubb_ufpga_chiplet_val.c_str(), system_temp_ubb_ufpga_unit.c_str());
				}
			}
		}
		if (system_temp_ubb_oam1.size() == 0) {
			formatted_string += string_format(baseboardSystemTempUbbOam1Template, "N/A", "");
		} else {
			for (uint32_t i = 0; i < system_temp_ubb_oam1.size(); i++) {
				if (system_temp_ubb_oam1[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ubb_oam1[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					std::string system_temp_ubb_oam1_unit = system_temp_ubb_oam1[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string system_temp_ubb_oam1_chiplet_val = string_format("%d", system_temp_ubb_oam1[i].val);
					formatted_string += string_format(baseboardSystemTempUbbOam1Template, system_temp_ubb_oam1_chiplet_val.c_str(), system_temp_ubb_oam1_unit.c_str());
				}
			}
		}
		if (system_temp_oam_0_1_hsc.size() == 0) {
			formatted_string += string_format(baseboardSystemTempOam01HscTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < system_temp_oam_0_1_hsc.size(); i++) {
				if (system_temp_oam_0_1_hsc[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_oam_0_1_hsc[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					std::string system_temp_oam_0_1_hsc_unit = system_temp_oam_0_1_hsc[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string system_temp_oam_0_1_hsc_chiplet_val = string_format("%d", system_temp_oam_0_1_hsc[i].val);
					formatted_string += string_format(baseboardSystemTempOam01HscTemplate, system_temp_oam_0_1_hsc_chiplet_val.c_str(), system_temp_oam_0_1_hsc_unit.c_str());
				}
			}
		}
		if (system_temp_oam_2_3_hsc.size() == 0) {
			formatted_string += string_format(baseboardSystemTempOam23HscTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < system_temp_oam_2_3_hsc.size(); i++) {
				if (system_temp_oam_2_3_hsc[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_oam_2_3_hsc[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					std::string system_temp_oam_2_3_hsc_unit = system_temp_oam_2_3_hsc[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string system_temp_oam_2_3_hsc_chiplet_val = string_format("%d", system_temp_oam_2_3_hsc[i].val);
					formatted_string += string_format(baseboardSystemTempOam23HscTemplate, system_temp_oam_2_3_hsc_chiplet_val.c_str(), system_temp_oam_2_3_hsc_unit.c_str());
				}
			}
		}
		if (system_temp_oam_4_5_hsc.size() == 0) {
			formatted_string += string_format(baseboardSystemTempOam45HscTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < system_temp_oam_4_5_hsc.size(); i++) {
				if (system_temp_oam_4_5_hsc[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_oam_4_5_hsc[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					std::string system_temp_oam_4_5_hsc_unit = system_temp_oam_4_5_hsc[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string system_temp_oam_4_5_hsc_chiplet_val = string_format("%d", system_temp_oam_4_5_hsc[i].val);
					formatted_string += string_format(baseboardSystemTempOam45HscTemplate, system_temp_oam_4_5_hsc_chiplet_val.c_str(), system_temp_oam_4_5_hsc_unit.c_str());
				}
			}
		}
		if (system_temp_oam_6_7_hsc.size() == 0) {
			formatted_string += string_format(baseboardSystemTempOam67HscTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < system_temp_oam_6_7_hsc.size(); i++) {
				if (system_temp_oam_6_7_hsc[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_oam_6_7_hsc[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					std::string system_temp_oam_6_7_hsc_unit = system_temp_oam_6_7_hsc[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string system_temp_oam_6_7_hsc_chiplet_val = string_format("%d", system_temp_oam_6_7_hsc[i].val);
					formatted_string += string_format(baseboardSystemTempOam67HscTemplate, system_temp_oam_6_7_hsc_chiplet_val.c_str(), system_temp_oam_6_7_hsc_unit.c_str());
				}
			}
		}
		if (system_temp_ubb_fpga_0v72_vr.size() == 0) {
			formatted_string += string_format(baseboardSystemTempUbbFpga0v72VrTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < system_temp_ubb_fpga_0v72_vr.size(); i++) {
				if (system_temp_ubb_fpga_0v72_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ubb_fpga_0v72_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					std::string system_temp_ubb_fpga_0v72_vr_unit = system_temp_ubb_fpga_0v72_vr[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string system_temp_ubb_fpga_0v72_vr_chiplet_val = string_format("%d", system_temp_ubb_fpga_0v72_vr[i].val);
					formatted_string += string_format(baseboardSystemTempUbbFpga0v72VrTemplate, system_temp_ubb_fpga_0v72_vr_chiplet_val.c_str(), system_temp_ubb_fpga_0v72_vr_unit.c_str());
				}
			}
		}
		if (system_temp_ubb_fpga_3v3_vr.size() == 0) {
			formatted_string += string_format(baseboardSystemTempUbbFpga3v3VrTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < system_temp_ubb_fpga_3v3_vr.size(); i++) {
				if (system_temp_ubb_fpga_3v3_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ubb_fpga_3v3_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					std::string system_temp_ubb_fpga_3v3_vr_unit = system_temp_ubb_fpga_3v3_vr[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string system_temp_ubb_fpga_3v3_vr_chiplet_val = string_format("%d", system_temp_ubb_fpga_3v3_vr[i].val);
					formatted_string += string_format(baseboardSystemTempUbbFpga3v3VrTemplate, system_temp_ubb_fpga_3v3_vr_chiplet_val.c_str(), system_temp_ubb_fpga_3v3_vr_unit.c_str());
				}
			}
		}
		if (system_temp_retimer_0_1_2_3_1v2_vr.size() == 0) {
			formatted_string += string_format(baseboardSystemTempRetimer01231v2VrTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < system_temp_retimer_0_1_2_3_1v2_vr.size(); i++) {
				if (system_temp_retimer_0_1_2_3_1v2_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_retimer_0_1_2_3_1v2_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					std::string system_temp_retimer_0_1_2_3_1v2_vr_unit = system_temp_retimer_0_1_2_3_1v2_vr[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string system_temp_retimer_0_1_2_3_1v2_vr_chiplet_val = string_format("%d", system_temp_retimer_0_1_2_3_1v2_vr[i].val);
					formatted_string += string_format(baseboardSystemTempRetimer01231v2VrTemplate, system_temp_retimer_0_1_2_3_1v2_vr_chiplet_val.c_str(), system_temp_retimer_0_1_2_3_1v2_vr_unit.c_str());
				}
			}
		}
		if (system_temp_retimer_4_5_6_7_1v2_vr.size() == 0) {
			formatted_string += string_format(baseboardSystemTempRetimer45671v2VrTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < system_temp_retimer_4_5_6_7_1v2_vr.size(); i++) {
				if (system_temp_retimer_4_5_6_7_1v2_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_retimer_4_5_6_7_1v2_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					std::string system_temp_retimer_4_5_6_7_1v2_vr_unit = system_temp_retimer_4_5_6_7_1v2_vr[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string system_temp_retimer_4_5_6_7_1v2_vr_chiplet_val = string_format("%d", system_temp_retimer_4_5_6_7_1v2_vr[i].val);
					formatted_string += string_format(baseboardSystemTempRetimer45671v2VrTemplate, system_temp_retimer_4_5_6_7_1v2_vr_chiplet_val.c_str(), system_temp_retimer_4_5_6_7_1v2_vr_unit.c_str());
				}
			}
		}
		if (system_temp_retimer_0_1_0v9_vr.size() == 0) {
			formatted_string += string_format(baseboardSystemTempRetimer010v9VrTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < system_temp_retimer_0_1_0v9_vr.size(); i++) {
				if (system_temp_retimer_0_1_0v9_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_retimer_0_1_0v9_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					std::string system_temp_retimer_0_1_0v9_vr_unit = system_temp_retimer_0_1_0v9_vr[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string system_temp_retimer_0_1_0v9_vr_chiplet_val = string_format("%d", system_temp_retimer_0_1_0v9_vr[i].val);
					formatted_string += string_format(baseboardSystemTempRetimer010v9VrTemplate, system_temp_retimer_0_1_0v9_vr_chiplet_val.c_str(), system_temp_retimer_0_1_0v9_vr_unit.c_str());
				}
			}
		}
		if (system_temp_retimer_4_5_0v9_vr.size() == 0) {
			formatted_string += string_format(baseboardSystemTempRetimer450v9VrTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < system_temp_retimer_4_5_0v9_vr.size(); i++) {
				if (system_temp_retimer_4_5_0v9_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_retimer_4_5_0v9_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					std::string system_temp_retimer_4_5_0v9_vr_unit = system_temp_retimer_4_5_0v9_vr[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string system_temp_retimer_4_5_0v9_vr_chiplet_val = string_format("%d", system_temp_retimer_4_5_0v9_vr[i].val);
					formatted_string += string_format(baseboardSystemTempRetimer450v9VrTemplate, system_temp_retimer_4_5_0v9_vr_chiplet_val.c_str(), system_temp_retimer_4_5_0v9_vr_unit.c_str());
				}
			}
		}
		if (system_temp_retimer_2_3_0v9_vr.size() == 0) {
			formatted_string += string_format(baseboardSystemTempRetimer230v9VrTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < system_temp_retimer_2_3_0v9_vr.size(); i++) {
				if (system_temp_retimer_2_3_0v9_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_retimer_2_3_0v9_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					std::string system_temp_retimer_2_3_0v9_vr_unit = system_temp_retimer_2_3_0v9_vr[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string system_temp_retimer_2_3_0v9_vr_chiplet_val = string_format("%d", system_temp_retimer_2_3_0v9_vr[i].val);
					formatted_string += string_format(baseboardSystemTempRetimer230v9VrTemplate, system_temp_retimer_2_3_0v9_vr_chiplet_val.c_str(), system_temp_retimer_2_3_0v9_vr_unit.c_str());
				}
			}
		}
		if (system_temp_retimer_6_7_0v9_vr.size() == 0) {
			formatted_string += string_format(baseboardSystemTempRetimer670v9VrTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < system_temp_retimer_6_7_0v9_vr.size(); i++) {
				if (system_temp_retimer_6_7_0v9_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_retimer_6_7_0v9_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					std::string system_temp_retimer_6_7_0v9_vr_unit = system_temp_retimer_6_7_0v9_vr[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string system_temp_retimer_6_7_0v9_vr_chiplet_val = string_format("%d", system_temp_retimer_6_7_0v9_vr[i].val);
					formatted_string += string_format(baseboardSystemTempRetimer670v9VrTemplate, system_temp_retimer_6_7_0v9_vr_chiplet_val.c_str(), system_temp_retimer_6_7_0v9_vr_unit.c_str());
				}
			}
		}
		if (system_temp_oam_0_1_2_3_3v3_vr.size() == 0) {
			formatted_string += string_format(baseboardSystemTempOam01233v3VrTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < system_temp_oam_0_1_2_3_3v3_vr.size(); i++) {
				if (system_temp_oam_0_1_2_3_3v3_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_oam_0_1_2_3_3v3_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					std::string system_temp_oam_0_1_2_3_3v3_vr_unit = system_temp_oam_0_1_2_3_3v3_vr[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string system_temp_oam_0_1_2_3_3v3_vr_chiplet_val = string_format("%d", system_temp_oam_0_1_2_3_3v3_vr[i].val);
					formatted_string += string_format(baseboardSystemTempOam01233v3VrTemplate, system_temp_oam_0_1_2_3_3v3_vr_chiplet_val.c_str(), system_temp_oam_0_1_2_3_3v3_vr_unit.c_str());
				}
			}
		}
		if (system_temp_oam_4_5_6_7_3v3_vr.size() == 0) {
			formatted_string += string_format(baseboardSystemTempOam45673v3VrTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < system_temp_oam_4_5_6_7_3v3_vr.size(); i++) {
				if (system_temp_oam_4_5_6_7_3v3_vr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_oam_4_5_6_7_3v3_vr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					std::string system_temp_oam_4_5_6_7_3v3_vr_unit = system_temp_oam_4_5_6_7_3v3_vr[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string system_temp_oam_4_5_6_7_3v3_vr_chiplet_val = string_format("%d", system_temp_oam_4_5_6_7_3v3_vr[i].val);
					formatted_string += string_format(baseboardSystemTempOam45673v3VrTemplate, system_temp_oam_4_5_6_7_3v3_vr_chiplet_val.c_str(), system_temp_oam_4_5_6_7_3v3_vr_unit.c_str());
				}
			}
		}
		if (system_temp_ibc_hsc.size() == 0) {
			formatted_string += string_format(baseboardSystemTempIbcHscTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < system_temp_ibc_hsc.size(); i++) {
				if (system_temp_ibc_hsc[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ibc_hsc[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					std::string system_temp_ibc_hsc_unit = system_temp_ibc_hsc[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string system_temp_ibc_hsc_chiplet_val = string_format("%d", system_temp_ibc_hsc[i].val);
					formatted_string += string_format(baseboardSystemTempIbcHscTemplate, system_temp_ibc_hsc_chiplet_val.c_str(), system_temp_ibc_hsc_unit.c_str());
				}
			}
		}
		if (system_temp_ibc.size() == 0) {
			formatted_string += string_format(baseboardSystemTempIbcTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < system_temp_ibc.size(); i++) {
				if (system_temp_ibc[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && system_temp_ibc[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD) {
					is_supported = true;
					std::string system_temp_ibc_unit = system_temp_ibc[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string system_temp_ibc_chiplet_val = string_format("%d", system_temp_ibc[i].val);
					formatted_string += string_format(baseboardSystemTempIbcTemplate, system_temp_ibc_chiplet_val.c_str(), system_temp_ibc_unit.c_str());
				}
			}
		}
	}

	if (is_supported) {
		return ret;
	} else {
		return AMDSMI_STATUS_NOT_SUPPORTED;
	}
}

int AmdSmiApiHost::amdsmi_get_node_npm_info_command(uint64_t processor_bdf, Arguments arg, std::string &formatted_string)
{
	int ret;
	amdsmi_node_handle node;
	amdsmi_npm_info_t npm_info;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_get_node_handle(processor, &node);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = host_fill_node_npm_info(arg, "N/A");
		return ret;
	}

	ret = host_amdsmi_get_npm_info(node, &npm_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = host_fill_node_npm_info(arg, "N/A");
		return ret;
	}

	std::string npm_status_string = npm_info.status == 0 ? "DISABLED" : "ENABLED";
	std::string npm_limit_string = string_format("%d", npm_info.limit);

	if (arg.output == json) {
		nlohmann::ordered_json npm_limit{};
		if(npm_info.limit == UINT64_MAX) {
			npm_limit["value"] = "N/A";
			npm_limit["unit"] = "N/A";
		} else {
			npm_limit["value"] = npm_info.limit;
			npm_limit["unit"] = "W";
		}

		nlohmann::ordered_json npm_info_json = {
			{ "limit", npm_limit },
			{ "status", npm_status_string.c_str() }
		};

		formatted_string = npm_info_json.dump(4);
	} else if (arg.output == csv) {
		formatted_string = string_format("%s,%s", npm_limit_string.c_str(), npm_status_string.c_str());
	} else {
		formatted_string = string_format(nodePowerManagementTemplate, npm_limit_string.c_str(), npm_status_string.c_str());
	}

	return ret;
}
