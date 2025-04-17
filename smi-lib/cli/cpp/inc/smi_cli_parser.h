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

#pragma once

#include <map>
#include <regex>
#include <string>

#include "smi_cli_device.h"

enum OutputFormat { human, json, csv = 2 };

enum ProcessType { name, pid };
using process_value = std::string;

class Arguments
{
public:
	OutputFormat output{ human };
	bool is_file{ false };
	bool is_vf{ false };
	std::string vf_id;
	std::string file_path;
	std::string command;
	std::vector<std::string> options;
	std::vector<std::shared_ptr<Device> > devices;
	bool all_arguments{ false };
	int watch{ -1 };
	int watch_time{ -1 };
	int iterations{ -1 };
	std::string process;
	std::map<process_value, ProcessType> process_map{};
	std::string xgmi_mode;
	std::string process_isolation_set;
	std::string fb_sharing_mode;
	uint32_t accelerator_partition_setting;
	std::string memory_partition_setting;
	std::vector<std::vector<uint64_t>> groups;
	std::string pstate_set;
	uint64_t power_cap_set;
	std::vector<std::string> severities;
	std::string folder_name;
	int file_limit {-1};
	int follow {-1};
	Arguments() {};
};

struct deviceType {
	deviceType(std::string const &val) : value(val)
	{
	}
	std::string value;
};

class AmdSmiParser
{
private:
	unsigned int gpu_count;
	std::vector<std::string> SUPPORTED_COMMANDS = { "help",	     "list",	"static",
		"discovery", "ucode",	"firmware",
		"bad-pages", "metric",	"process",
		"profile",   "version", "event", "topology", "xgmi", "reset", "set", "monitor", "partition",
		"ras"
	};

	std::vector<std::string> FW_SUPPORTED_ARGS_GPU = {
		"--ucode-list", "--fw-list", "-f", "--error-records", "-e"
	};

	std::map<std::string, std::vector<std::string> > FW_SUPPORTED_ARGUMENTS = {
		{ "--gpu", FW_SUPPORTED_ARGS_GPU },
		{ "-g", FW_SUPPORTED_ARGS_GPU },
		{ "--vf", { "--ucode-list", "--fw-list", "-f" } }
	};

	std::map<std::string, std::vector<std::string> > BAD_PAGES_SUPPORTED_ARGUMENTS = {
		{ "--gpu", {} },
		{ "-g", {} }
	};

	std::map<std::string, std::vector<std::string> > LIST_SUPPORTED_ARGUMENTS = {
		{ "--gpu", {} },
		{ "-g", {} }
	};

	std::vector<std::string> STATIC_SUPPORTED_ARGS_GPU = {
		"--asic", "-a", "--bus", "-b", "--vbios", "-V", "--board", "-B",
		"--limit", "-l", "--driver", "-d",
		"--ras", "-r", "--dfc-ucode", "-D", "--fb-info", "-f", "--num-vf", "-n",
		"--vram", "-v", "--cache", "-c", "--partition", "-p", "--process-isolation", "-R",
		"--soc-pstate", "-ps"
	};

	std::vector<std::string> METRIC_SUPPORTED_ARGS_GPU = {
		"--usage", "-u", "--power", "-p", "--clock", "-c", "--temperature",
		"-t", "--ecc", "-e", "--ecc-block", "-k", "--pcie", "-P", "--fb-usage", "--energy", "-E"
	};

	std::vector<std::string> METRIC_SUPPORTED_ARGS_VF = {
		"--schedule", "-s", "--guard", "-G", "--guest-data", "-u"
	};

	std::vector<std::string> WATCH_SUPPORTED_ARGS = {
		"--watch_time", "-W", "--iterations", "-i"
	};

	std::vector<std::string> TOPOLOGY_SUPPORTED_ARGS_GPU = {
		"--weight", "--hops", "--fb-sharing", "--link-type", "--link-status"
	};

	std::vector<std::string> PROCESS_SUPPORTED_ARGS_GPU = {
		"--general", "--engine"
	};

	std::vector<std::string> RESET_SUPPORTED_ARGS_GPU = {
		"--clean-local-data", "-l"
	};

	std::vector<std::string> RESET_SUPPORTED_ARGS_VF = {
		"--vf-fb"
	};

	std::vector<std::string> SET_SUPPORTED_ARGS_GPU = {
		"--xgmi", "--fb-sharing-mode", "--group", "--memory-partition", "--accelerator-partition",
		"process-isolation", "-R", "--soc-pstate", "-ps", "--power-cap", "-pc"
	};

	std::vector<std::string> PARTITION_SUPPORTED_ARGS_GPU = {
		"--accelerator", "-a", "--memory", "-m", "--current", "-c"
	};

	std::map<std::string, std::vector<std::string> > PARTITION_SUPPORTED_ARGUMENTS = {
		{ "--gpu", PARTITION_SUPPORTED_ARGS_GPU },
		{ "-g", PARTITION_SUPPORTED_ARGS_GPU }
	};

	std::map<std::string, std::vector<std::string> > STATIC_SUPPORTED_ARGUMENTS = {
		{ "--gpu", STATIC_SUPPORTED_ARGS_GPU },
		{ "-g", STATIC_SUPPORTED_ARGS_GPU },
		{ "--vf", {} }
	};

	std::map<std::string, std::vector<std::string> > METRIC_SUPPORTED_ARGUMENTS = {
		{ "--gpu", METRIC_SUPPORTED_ARGS_GPU },
		{ "-g", METRIC_SUPPORTED_ARGS_GPU },
		{ "--vf", METRIC_SUPPORTED_ARGS_VF },
		{ "--watch", WATCH_SUPPORTED_ARGS},
		{ "--w", WATCH_SUPPORTED_ARGS}
	};

	std::map<std::string, std::vector<std::string> > TOPOLOGY_SUPPORTED_ARGUMENTS = {
		{ "--gpu", TOPOLOGY_SUPPORTED_ARGS_GPU },
		{ "-g", TOPOLOGY_SUPPORTED_ARGS_GPU }
	};

	std::vector<std::string> XGMI_SUPPORTED_ARGS_GPU = {
		"--caps", "--fb-sharing", "--set", "--mode", "--metric"
	};

	std::map<std::string, std::vector<std::string> > XGMI_SUPPORTED_ARGUMENTS = {
		{ "--gpu", XGMI_SUPPORTED_ARGS_GPU },
	};
	std::map<std::string, std::vector<std::string> > EVENT_SUPPORTED_ARGUMENTS = {
		{ "--gpu", {} },
		{ "-g", {} }
	};

	std::map<std::string, std::vector<std::string> > PROCESS_SUPPORTED_ARGUMENTS = {
		{ "--gpu", {PROCESS_SUPPORTED_ARGS_GPU} },
		{ "-g", {PROCESS_SUPPORTED_ARGS_GPU} }
	};

	std::map<std::string, std::vector<std::string> > RESET_SUPPORTED_ARGUMENTS = {
		{ "--gpu", {RESET_SUPPORTED_ARGS_GPU} },
		{ "-g", {RESET_SUPPORTED_ARGS_GPU} },
		{ "--vf", { RESET_SUPPORTED_ARGS_VF } }
	};

	std::map<std::string, std::vector<std::string> > SET_SUPPORTED_ARGUMENTS = {
		{ "--gpu", {SET_SUPPORTED_ARGS_GPU} },
		{ "-g", {SET_SUPPORTED_ARGS_GPU} }
	};

	std::vector<std::string> MONITOR_SUPPORTED_ARGS_GPU = {
		"-p", "--power-usage", "-t", "--temperature", "-u", "--gfx", "-m", "--mem",
		"-n", "--encoder", "-d", "--decoder", "-e", "--ecc",
		"-v", "--vram-usage", "-r", "--pcie", "-q", "--process"
	};

	std::map<std::string, std::vector<std::string> > MONITOR_SUPPORTED_ARGUMENTS = {
		{ "--gpu", MONITOR_SUPPORTED_ARGS_GPU },
		{ "-g", MONITOR_SUPPORTED_ARGS_GPU },
		{ "--watch", WATCH_SUPPORTED_ARGS},
		{ "--w", WATCH_SUPPORTED_ARGS}
	};

	std::vector<std::string> RAS_SUPPORTED_ARGS_GPU = {
		"--cper", "--severity", "--folder", "--file_limit", "--follow"
	};

	std::map<std::string, std::vector<std::string> > RAS_SUPPORTED_ARGUMENTS = {
		{ "--gpu", RAS_SUPPORTED_ARGS_GPU },
		{ "-g", RAS_SUPPORTED_ARGS_GPU },
	};

	std::map<std::string, std::map<std::string, std::vector<std::string> > >
	COMMAND_SUPPORTED_ARGUMENTS = {
		{ "help", {} },
		{ "version", {} },
		{ "list", LIST_SUPPORTED_ARGUMENTS },
		{ "discovery", LIST_SUPPORTED_ARGUMENTS },
		{ "static", STATIC_SUPPORTED_ARGUMENTS },
		{ "ucode", FW_SUPPORTED_ARGUMENTS },
		{ "firmware", FW_SUPPORTED_ARGUMENTS },
		{ "bad-pages", BAD_PAGES_SUPPORTED_ARGUMENTS },
		{ "metric", METRIC_SUPPORTED_ARGUMENTS },
		{ "profile", { { "--gpu", {} }, { "-g", {} } } },
		{ "topology", TOPOLOGY_SUPPORTED_ARGUMENTS },
		{ "xgmi", XGMI_SUPPORTED_ARGUMENTS },
		{ "event", EVENT_SUPPORTED_ARGUMENTS },
		{ "process", PROCESS_SUPPORTED_ARGUMENTS },
		{ "reset", RESET_SUPPORTED_ARGUMENTS },
		{ "set", SET_SUPPORTED_ARGUMENTS },
		{ "monitor", MONITOR_SUPPORTED_ARGUMENTS },
		{ "partition", PARTITION_SUPPORTED_ARGUMENTS},
		{ "ras", RAS_SUPPORTED_ARGUMENTS }
	};

	/**
	 * @brief Check if a string is positive number
	 *
	 * @param[in] s String for check
	 * @return true if string is number, else false
	 */
	bool is_number(const std::string &s);

	/**
	 * @brief Check if a string is negative number
	 *
	 * @param[in] s String for check
	 * @return true if string is negative number, else false
	 */
	bool is_negative_number(const std::string &s);

	/**
	 * @brief Get the gpu device from string
	 *
	 * @param[in] gpu string from which we extract gpu index
	 * @return gpu device
	 */
	std::shared_ptr<Device> get_device_from_input(std::string gpu);

	/**
	 * @brief Get device type (gpu or vf)
	 *
	 * @param[inout] command_argument_list list of command arguments
	 * @param[inout] parsed_arguments parsed arguments
	 * @return std::string - gpu if type is gpu else vf if type vf
	 */
	std::string get_device_type(std::vector<std::string> &command_argument_list,
								Arguments &parsed_arguments);

	/**
	 * @brief Check is option a valid argument
	 *
	 * @param[in] option option that is checked
	 * @param[inout] parsed_arguments parsed arguments
	 * @return true if option is valid else false
	 */
	bool is_option_argument(std::string option, Arguments &parsed_arguments);

	/**
	 * @brief Parse command from argument list
	 *
	 * @param[in] command_argument_list argument list
	 * @param[inout] parsed_arguments parsed arguments
	 */
	void parse_command_object(std::vector<std::string> command_argument_list,
							  Arguments &parsed_arguments);

	/**
	 * @brief Add all gpus into parsed_arguments
	 *
	 * @param parsed_arguments parsed arguments
	 */
	void add_all_gpus(Arguments &parsed_arguments);

	/**
	 * @brief Parse command options arguments
	 *
	 * @param[in] command_argument_list argument list
	 * @param[inout] parsed_arguments parsed arguments
	 */
	void parse_arguments(std::vector<std::string> command_argument_list,
						 Arguments &parsed_arguments);

	/**
	 * @brief Parse output command output formats
	 *
	 * @param command_argument_list argument list
	 * @param parsed_arguments parsed arguments
	 */
	void parse_output_format(std::vector<std::string> command_argument_list,
							 Arguments &parsed_arguments);

	/**
	 * @brief Check is options a valid with input command
	 *
	 * @param[inout] parsed_arguments parsed arguments
	 */
	void is_options_valid(Arguments &parsed_arguments);

	/**
	 * @brief Parse given string to uint64 vector
	 *
	 * @param[in] input input string
	 * @param[out] groupsVector vector of BDFs from string
	 * @param[inout] parsed_arguments parsed arguments
	 */
	void parseAndFillVector(const std::string& input,
							std::vector<std::vector<uint64_t>>& groupsVector,
							Arguments &parsed_arguments);

	bool is_argument_present(const std::vector<std::string>& command_arguments,
							 const std::string& argument_prefix);

public:
	/**
		* @brief Construct a new Smi Parser object
		*
		*/
	AmdSmiParser();

	/**
	 * @brief Parse input arguments
	 *
	 * @param[in] argc number of arguments
	 * @param[in] argv array of argunemts
	 * @param[in] ret parser arguments
	 * @return parsed arguments
	 */
	void parse_arg(int argc, char **argv,  Arguments &ret);
};
