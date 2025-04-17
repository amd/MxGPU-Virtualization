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
#include "smi_cli_parser.h"
#include "smi_cli_api_base.h"
#include "smi_cli_exception.h"
#include "smi_cli_platform.h"
#include "smi_cli_helpers.h"

#include <regex>
#include <unordered_set>

AmdSmiParser::AmdSmiParser()
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_gpu_count(gpu_count);
	if (ret != 0) {
		throw SmiToolSMILIBErrorException(ret);
	}
}

void AmdSmiParser::parseAndFillVector(const std::string& input,
									  std::vector<std::vector<uint64_t>>& groupsVector,
									  Arguments &parsed_arguments)
{
	std::istringstream iss(input);
	std::string token;

	while (std::getline(iss, token, ' ')) {
		std::istringstream groupStream(token);
		std::vector<uint64_t> groupBDFs;
		std::vector<uint8_t> groupIndexes;
		std::string numberToken;

		while (std::getline(groupStream, numberToken, '-')) {
			int number = 0;
			try {
				number = std::stoi(numberToken);
			} catch(...) {
				throw SmiToolInvalidParameterValueException(token);
			}

			if (number >= parsed_arguments.devices.size()) {
				throw SmiToolDeviceNotFoundException(std::to_string(number));
			}
			groupBDFs.push_back(parsed_arguments.devices[number]->get_bdf());
			groupIndexes.push_back(number);
		}
		std::unordered_set<uint64_t> seenNumbers;

		for (auto number : groupIndexes) {
			if (seenNumbers.find(number) != seenNumbers.end()) {
				throw SmiToolInvalidParameterValueException(std::to_string(number));
			} else {
				seenNumbers.insert(number);
			}
		}
		groupsVector.push_back(groupBDFs);
	}
}

bool AmdSmiParser::is_argument_present(const std::vector<std::string>& command_arguments,
									   const std::string& argument_prefix)
{
	auto it = std::find_if(command_arguments.begin(),
	command_arguments.end(), [&](const std::string& arg) {
		return arg.find(argument_prefix) == 0;
	});

	return it != command_arguments.end();
}

bool AmdSmiParser::is_number(const std::string &s)
{
	std::regex isNumberRegex("^[0-9]+$");
	return std::regex_match(s, isNumberRegex);
}

bool AmdSmiParser::is_negative_number(const std::string &s)
{
	std::regex isNumberRegex("^-\\d+$");
	return std::regex_match(s, isNumberRegex);
}

std::shared_ptr<Device> AmdSmiParser::get_device_from_input(std::string gpu)
{
	if (is_number(gpu)) {
		if (std::stoi(gpu) >= gpu_count) {
			throw SmiToolDeviceNotFoundException(gpu);
		} else {
			return std::shared_ptr<Device>(new Device(std::stoi(gpu), GPU_INDEX));
		}
	} else if (is_BDF(gpu)) {
		return std::shared_ptr<Device>(new Device(gpu, BDF, "--gpu"));
	} else if (is_UUID(gpu)) {
		return std::shared_ptr<Device>(new Device(gpu, UUID, "--gpu"));
	} else {
		if(gpu.size() == 0) {
			throw SmiToolMissingParameterValueException("--gpu=");
		} else {
			throw SmiToolInvalidParameterValueException(gpu);
		}
	}
}

void AmdSmiParser::parse_command_object(std::vector<std::string> command_argument_list,
										Arguments &parsed_arguments)
{
	if (command_argument_list.size() == 0) {
		parsed_arguments.command = "help";
		return;
	}

	if (std::find(std::begin(SUPPORTED_COMMANDS), std::end(SUPPORTED_COMMANDS),
				  command_argument_list[0]) == SUPPORTED_COMMANDS.end()) {
		throw SmiToolInvalidCommandException(std::string(command_argument_list[0]));
	} else {
		parsed_arguments.command = command_argument_list[0];
	}

	if (parsed_arguments.command == "help") {
		if (parsed_arguments.output != human) {
			if (parsed_arguments.output == json) {
				throw SmiToolInvalidParameterException("--json");
			} else {
				throw SmiToolInvalidParameterException("--csv");
			}
		}
	}

}

std::string AmdSmiParser::get_device_type(std::vector<std::string> &command_argument_list,
		Arguments &parsed_arguments)
{
	std::string ret = "--gpu";
	std::regex is_gpu("--gpu=.*");
	std::regex is_g("-g=.*");
	std::regex is_vf("--vf=.*");
	for (int i = 1; i < command_argument_list.size(); i++) {
		if (std::regex_match(command_argument_list[i], is_gpu) ||
				std::regex_match(command_argument_list[i], is_g)) {
			if (parsed_arguments.command == "help" || parsed_arguments.command == "version") {
				throw SmiToolInvalidParameterException(command_argument_list[i]);
			}
			if (std::regex_match(command_argument_list[i], is_g)) {
				parsed_arguments.devices.push_back(
											get_device_from_input(command_argument_list[i].substr(3)));
			} else {
				parsed_arguments.devices.push_back(
											get_device_from_input(command_argument_list[i].substr(6)));
			}
			command_argument_list.erase(command_argument_list.begin() + i);
			i--;
		} else {
			if (std::regex_match(command_argument_list[i], is_vf)) {
				if (parsed_arguments.command == "list" || parsed_arguments.command == "discovery"
						|| parsed_arguments.command == "help" || parsed_arguments.command == "version") {
					throw SmiToolInvalidParameterException(command_argument_list[i]);
				}
				if(!AmdSmiPlatform::getInstance().is_host()) {
					throw SmiToolParameterNotSupportedException(command_argument_list[i].substr(0, 4));
				}
				ret = "--vf";
				parsed_arguments.vf_id = (command_argument_list[i].substr(5));
				command_argument_list.erase(command_argument_list.begin() + i);
				i--;
			}
		}
	}

	//check errors

	return ret;
}

bool AmdSmiParser::is_option_argument(std::string option, Arguments &parsed_arguments)
{
	if (parsed_arguments.command == "help") {
		if (option == "--json" || option == "--csv") {
			throw SmiToolInvalidParameterException(option);
		}
	}
	if (parsed_arguments.command == "set") {
		if (option == "--gpu" || option == "--vf") {
			throw SmiToolInvalidParameterException(option);
		}
	}

	if (option == "--json") {
		return true;
	}
	if (option == "--csv") {
		return true;
	}
	if (option.substr(0, 6) == "--file" && parsed_arguments.command != "ras") {
		if (parsed_arguments.command == "reset") {
			throw SmiToolInvalidParameterException(option);
		}

		if (option.substr(6, 1) == "=") {
			parsed_arguments.is_file = true;
			parsed_arguments.file_path = option.substr(7);
		} else {
			throw SmiToolInvalidParameterException(option);
		}
		return true;
	}

	if (option.substr(0, 12) == "--watch_time") {
		if ((parsed_arguments.command != "metric") && (parsed_arguments.command != "process")
				&& (parsed_arguments.command != "monitor")) {
			throw SmiToolInvalidParameterException(option.substr(0, 12));
		}
		if (parsed_arguments.iterations > -1) {
			throw SmiToolInvalidParameterException(option.substr(0, 13));
		}
		if (parsed_arguments.watch == -1) {
			throw SmiToolInvalidParameterException(option.substr(0, 12));
		}
		if (option.substr(12,1) != "=") {
			if (option.substr(12,1) == "") {
				throw SmiToolInvalidParameterException(option);
			}
			throw SmiToolInvalidParameterException(option);
		} else {
			if (option.substr(13,1) == "") {
				throw SmiToolMissingParameterValueException(option.substr(0,12));
			}
		}
		if (is_negative_number(option.substr(13))) {
			throw SmiToolInvalidParameterValueException(option.substr(13));
		}
		if (is_number(option.substr(13))) {
			parsed_arguments.watch_time = std::stoi(option.substr(13));
		} else {
			throw SmiToolInvalidParameterValueException(option.substr(13));
		}

		return true;
	}
	if (option.substr(0, 7) == "--watch") {
		if ((parsed_arguments.command != "metric") && (parsed_arguments.command != "process")
				&& (parsed_arguments.command != "monitor")) {
			throw SmiToolInvalidParameterException(option.substr(0, 7));
		}
		if (option.substr(7,1) != "=") {
			if (option.substr(7,1) == "") {
				throw SmiToolInvalidParameterException(option);
			}
			throw SmiToolInvalidParameterException(option);
		} else {
			if (option.substr(8,1) == "") {
				throw SmiToolMissingParameterValueException(option.substr(0,12));
			}
		}
		if (is_negative_number(option.substr(8))) {
			throw SmiToolInvalidParameterValueException(option.substr(8));
		}
		if (is_number(option.substr(8))) {
			parsed_arguments.watch = std::stoi(option.substr(8));
		} else {
			throw SmiToolInvalidParameterValueException(option.substr(8));
		}

		return true;
	}
	if (option.substr(0, 12) == "--iterations") {
		if ((parsed_arguments.command != "metric") && (parsed_arguments.command != "process")
				&& (parsed_arguments.command != "monitor")) {
			throw SmiToolInvalidParameterException(option.substr(0, 12));
		}
		if (parsed_arguments.watch_time > -1) {
			throw SmiToolInvalidParameterException(option.substr(0, 13));
		}
		if (parsed_arguments.watch == -1) {
			throw SmiToolMissingParameterValueException(option.substr(0, 12));
		}

		if (option.substr(12,1) != "=") {
			if (option.substr(12,1) == "") {
				throw SmiToolInvalidParameterException(option);
			}
			throw SmiToolInvalidParameterException(option);
		} else {
			if (option.substr(13,1) == "") {
				throw SmiToolMissingParameterValueException(option.substr(0,12));
			}
		}
		if (is_negative_number(option.substr(13))) {
			throw SmiToolInvalidParameterValueException(option.substr(13));
		}
		if (is_number(option.substr(13))) {
			parsed_arguments.iterations = std::stoi(option.substr(13));
		} else {
			throw SmiToolInvalidParameterValueException(option.substr(13));
		}

		return true;
	}
	if (option.substr(0, 6) == "--name") {
		if (parsed_arguments.command != "process") {
			throw SmiToolInvalidParameterException(option.substr(0, 6));
		}
		if (option.substr(6,1) != "=") {
			if (option.substr(6,1) == "") {
				throw SmiToolInvalidParameterException(option);
			}
			throw SmiToolInvalidParameterException(option);
		} else {
			if (option.substr(7,1) == "") {
				throw SmiToolMissingParameterValueException(option.substr(0,12));
			}
		}
		parsed_arguments.options.push_back("name");
		parsed_arguments.process_map[option.substr(7)] = ProcessType::name;
		return true;
	}
	if (option.substr(0, 5) == "--pid") {
		if (parsed_arguments.command != "process") {
			throw SmiToolInvalidParameterException(option.substr(0, 5));
		}
		if (option.substr(5,1) != "=") {
			if (option.substr(5,1) == "") {
				throw SmiToolInvalidParameterException(option);
			}
			throw SmiToolInvalidParameterException(option);
		} else {
			if (option.substr(6,1) == "") {
				throw SmiToolMissingParameterValueException(option.substr(0,12));
			}
		}
		if (is_number(option.substr(6))) {
			parsed_arguments.options.push_back("pid");
			parsed_arguments.process_map[option.substr(6)] = ProcessType::pid;
			return true;
		} else {
			throw SmiToolInvalidParameterValueException(option.substr(6));
			return false;
		}
	}

	if (option.substr(0, 6) == "--mode") {
		try {
			parsed_arguments.xgmi_mode = option.substr(7);
		} catch (...) {
			throw SmiToolMissingParameterValueException(option.substr(0, 6));
		}
		return true;
	}

	if (option.substr(0, 17) == "--fb-sharing-mode") {
		try {
			parsed_arguments.fb_sharing_mode = option.substr(18);
		} catch (...) {
			throw SmiToolMissingParameterValueException(option.substr(0, 17));
		}
		return true;
	}

	if (option.substr(0, 23) == "--accelerator-partition") {
		try {
			parsed_arguments.accelerator_partition_setting = std::stoul(option.substr(24));
		} catch (...) {
			throw SmiToolMissingParameterValueException(option.substr(0, 13));
		}
		if (option.substr(23,1) != "=") {
			if (option.substr(23,1) == "") {
				throw SmiToolInvalidParameterException(option);
			}
			throw SmiToolInvalidParameterException(option);
		} else {
			if (option.substr(24,1) == "") {
				throw SmiToolMissingParameterValueException(option.substr(0,23));
			}
		}
		parsed_arguments.options.push_back("accelerator-partition");
		parsed_arguments.accelerator_partition_setting = std::stoul(option.substr(24));
		return true;
	}

	if (option.substr(0, 18) == "--memory-partition") {
		try {
			parsed_arguments.memory_partition_setting = option.substr(19);
		} catch (...) {
			throw SmiToolMissingParameterValueException(option.substr(0, 18));
		}
		if (option.substr(18,1) != "=") {
			if (option.substr(18,1) == "") {
				throw SmiToolInvalidParameterException(option);
			}
			throw SmiToolInvalidParameterException(option);
		} else {
			if (option.substr(19,1) == "") {
				throw SmiToolMissingParameterValueException(option.substr(0,18));
			}
		}
		parsed_arguments.options.push_back("memory-partition");
		parsed_arguments.memory_partition_setting = option.substr(19);
		return true;
	}

	if (option.substr(0, 7) == "--group") {
		if (option.substr(7,1) != "=") {
			if (option.substr(7,1) == "") {
				throw SmiToolInvalidParameterException(option);
			}
			throw SmiToolInvalidParameterException(option);
		} else {
			if (option.substr(8,1) == "") {
				throw SmiToolMissingParameterValueException(option.substr(0,7));
			}
		}
		parseAndFillVector(option.substr(8), parsed_arguments.groups, parsed_arguments);
		return true;
	}

	if (option.substr(0, 19) == "--process-isolation") {
		try {
			parsed_arguments.process_isolation_set = option.substr(20);
		} catch (...) {
			throw SmiToolMissingParameterValueException(option.substr(0, 19));
		}
		if (option.substr(19,1) != "=") {
			if (option.substr(19,1) == "") {
				throw SmiToolInvalidParameterException(option);
			}
			throw SmiToolInvalidParameterException(option);
		} else {
			if (option.substr(20,1) == "") {
				throw SmiToolMissingParameterValueException(option.substr(0,19));
			}
		}
		return true;
	}

	if (option.substr(0, 2) == "-R") {
		try {
			parsed_arguments.process_isolation_set = option.substr(3);
		} catch (...) {
			throw SmiToolMissingParameterValueException(option.substr(0, 2));
		}
		if (option.substr(2,1) != "=") {
			if (option.substr(2,1) == "") {
				throw SmiToolInvalidParameterException(option);
			}
			throw SmiToolInvalidParameterException(option);
		} else {
			if (option.substr(3,1) == "") {
				throw SmiToolMissingParameterValueException(option.substr(0,2));
			}
		}
		return true;
	}

	if (option.substr(0, 12) == "--soc-pstate") {
		try {
			parsed_arguments.pstate_set = option.substr(13);
		} catch (...) {
			throw SmiToolMissingParameterValueException(option.substr(0, 12));
		}
		parsed_arguments.options.push_back("soc-pstate");
		return true;
	}

	if (option.substr(0, 3) == "-ps") {
		try {
			parsed_arguments.pstate_set = option.substr(4);
		} catch (...) {
			throw SmiToolMissingParameterValueException(option.substr(0, 3));
		}
		parsed_arguments.options.push_back("soc-pstate");
		return true;
	}
	if (option.substr(0, 11) == "--power-cap") {
		try {
			parsed_arguments.power_cap_set = std::stoul(option.substr(12));
			parsed_arguments.options.push_back("power-cap");
		} catch (...) {
			throw SmiToolMissingParameterValueException(option.substr(0, 13));
		}
		if (option.substr(11,1) != "=") {
			throw SmiToolInvalidParameterException(option);
		} else {
			if (option.substr(12,1) == "") {
				throw SmiToolMissingParameterValueException(option.substr(0,11));
			}
		}
		return true;
	}

	if (option.substr(0, 3) == "-pc") {
		try {
			parsed_arguments.power_cap_set = std::stoul(option.substr(4));
			parsed_arguments.options.push_back("power-cap");
		} catch (...) {
			throw SmiToolMissingParameterValueException(option.substr(0, 3));
		}
		if (option.substr(3,1) != "=") {
			throw SmiToolInvalidParameterException(option);
		} else {
			if (option.substr(4,1) == "") {
				throw SmiToolMissingParameterValueException(option.substr(0,3));
			}
		}
		return true;
	}

	if (option.substr(0, 10) == "--severity") {
		try {
			std::string severities = option.substr(11);
			std::vector<std::string> severities_vector = split_string(severities, ',');

			const std::vector<std::string> valid_severities = {"fatal", "nonfatal-corrected", "nonfatal-uncorrected", "all"};

			for (const std::string& severity : severities_vector) {
				if (std::find(valid_severities.begin(), valid_severities.end(),
							  severity) == valid_severities.end()) {
					throw SmiToolInvalidParameterException(severity);
				}
			}

			parsed_arguments.severities.insert(parsed_arguments.severities.begin(),
											   severities_vector.begin(), severities_vector.end());
		} catch (const SmiToolInvalidParameterException& ex) {
			throw;
		} catch (...) {
			throw SmiToolMissingParameterValueException(option.substr(0, 10));
		}
		if (option.substr(10,1) != "=") {
			if (option.substr(10,1) == "") {
				throw SmiToolMissingParameterValueException(option);
			}
			throw SmiToolMissingParameterValueException(option);
		} else {
			if (option.substr(11,1) == "") {
				throw SmiToolMissingParameterValueException(option.substr(0,10));
			}
		}
		return true;
	}

	if (option.substr(0, 8) == "--folder") {

		try {
			parsed_arguments.folder_name = option.substr(9);
		} catch (...) {
			throw SmiToolMissingParameterValueException(option.substr(0, 8));
		}
		if (option.substr(8,1) != "=") {
			if (option.substr(8,1) == "") {
				throw SmiToolMissingParameterValueException(option);
			}
			throw SmiToolMissingParameterValueException(option);
		} else {
			if (option.substr(9,1) == "") {
				throw SmiToolMissingParameterValueException(option.substr(0,8));
			}
		}
		return true;
	}

	if (option.substr(0, 12) == "--file_limit") {
		if (parsed_arguments.command != "ras") {
			throw SmiToolInvalidParameterException(option.substr(0, 12));
		}
		if (option.substr(12,1) != "=") {
			if (option.substr(12,1) == "") {
				throw SmiToolMissingParameterValueException(option);
			}
			throw SmiToolMissingParameterValueException(option);
		} else {
			if (option.substr(13,1) == "") {
				throw SmiToolMissingParameterValueException(option.substr(0,12));
			}
		}
		if (is_negative_number(option.substr(13))) {
			throw SmiToolInvalidParameterValueException(option.substr(13));
		}
		if (is_number(option.substr(13))) {
			if (std::stoi(option.substr(13)) == 0) {
				throw SmiToolInvalidParameterValueException(option.substr(13));
			}
			parsed_arguments.file_limit = std::stoi(option.substr(13));
		} else {
			throw SmiToolInvalidParameterValueException(option.substr(13));
		}
		parsed_arguments.options.push_back("file_limit");

		return true;
	}


	return false;
}

void AmdSmiParser::is_options_valid(Arguments &parsed_arguments)
{
	if ((parsed_arguments.watch != -1) &&
			(parsed_arguments.command != "metric") & (parsed_arguments.command != "process")
			&& (parsed_arguments.command != "monitor")) {
		throw SmiToolInvalidParameterException("--watch");
	}
}

void AmdSmiParser::add_all_gpus(Arguments &parsed_arguments)
{
	for (int i = 0; i < gpu_count; i++) {
		parsed_arguments.devices.push_back(get_device_from_input(std::to_string(i)));
	}
}

void AmdSmiParser::parse_arguments(std::vector<std::string> command_argument_list,
								   Arguments &parsed_arguments)
{
	if (command_argument_list.size() <= 1) {
		if(command_argument_list[0] == "set") {
			throw SmiToolRequiredCommandException("set");
		}
		add_all_gpus(parsed_arguments);
		parsed_arguments.all_arguments = true;
		return;
	}

	if((std::find(command_argument_list.begin(), command_argument_list.end(),
				  "--help") != command_argument_list.end()) ||
			(std::find(command_argument_list.begin(), command_argument_list.end(),
					   "-h") != command_argument_list.end())) {

		parsed_arguments.options.push_back(parsed_arguments.command);
		parsed_arguments.command = "help";
		for (int i = 1; i < command_argument_list.size(); i++) {
			if (command_argument_list[i].substr(0, 6) == "--file") {
				if (command_argument_list[i].substr(6, 1) == "=") {
					parsed_arguments.is_file = true;
					parsed_arguments.file_path = command_argument_list[i].substr(7);
				} else {
					throw SmiToolInvalidParameterException(command_argument_list[i]);
				}
			}
		}

		return;
	}

	std::string device_type = get_device_type(command_argument_list, parsed_arguments);
	if (COMMAND_SUPPORTED_ARGUMENTS.count(command_argument_list[0]) != 1) {
		throw SmiToolInvalidParameterException(std::string(command_argument_list[0]));
	}
	parsed_arguments.is_vf = device_type == "--vf";

	if (parsed_arguments.is_vf && !AmdSmiPlatform::getInstance().is_host()) {
		throw SmiToolParameterNotSupportedException("--vf");
	}

	std::vector<std::string> command_arguments;
	if (command_argument_list[0] != "version" && command_argument_list[0] != "help") {
		if (COMMAND_SUPPORTED_ARGUMENTS.at(command_argument_list[0])
				.count(device_type) != 1) {
			throw SmiToolInvalidParameterException(std::string(device_type));
		}
		if (parsed_arguments.devices.size() == 0) {
			add_all_gpus(parsed_arguments);
		}
		command_arguments = COMMAND_SUPPORTED_ARGUMENTS.at(command_argument_list[0])
							.at(device_type);
	}

	for (int i = 1; i < command_argument_list.size(); i++) {
		if (std::find(std::begin(command_arguments), std::end(command_arguments),
					  command_argument_list[i]) == command_arguments.end()) {
			if (!is_option_argument(command_argument_list[i], parsed_arguments)) {
				throw SmiToolInvalidParameterException(std::string(command_argument_list[i]));
			}
		} else {
			if (command_argument_list[0] == "set") {
				if (command_argument_list[i] == "--fb-sharing-mode" || command_argument_list[i] == "--group"
						||	command_argument_list[i] == "--memory-partition"
						|| command_argument_list[i] == "--accelerator-partition"
						|| command_argument_list[i] == "--process-isolation"
						|| command_argument_list[i] == "-R"
						|| command_argument_list[i] == "--soc-pstate" || command_argument_list[i] == "-ps") {
					if (!is_option_argument(command_argument_list[i], parsed_arguments)) {
						throw SmiToolInvalidParameterException(std::string(command_argument_list[i]));
					}
				}
			}
			if (command_argument_list[0] == "ras") {
				if (
					command_argument_list[i] == "--folder" || command_argument_list[i] == "--file_limit" ||
					command_argument_list[i] == "--severity") {
					if (!is_option_argument(command_argument_list[i], parsed_arguments)) {
						throw SmiToolInvalidParameterException(std::string(command_argument_list[i]));
					}
				}
			}
			if (command_argument_list[i].at(1) == '-') {
				parsed_arguments.options.push_back(command_argument_list[i].substr(2));
			} else {
				parsed_arguments.options.push_back(command_argument_list[i].substr(1));
			}
		}
	}

	if (command_argument_list[0] == "set") {
		if ((!is_argument_present(command_argument_list, "--xgmi") ||
				!is_argument_present(command_argument_list, "--fb-sharing-mode"))  &&
				!is_argument_present(command_argument_list, "--memory-partition") &&
				!is_argument_present(command_argument_list, "--accelerator-partition") &&
				!is_argument_present(command_argument_list, "--process-isolation") &&
				!is_argument_present(command_argument_list, "-R") &&
				!is_argument_present(command_argument_list, "--soc-pstate") &&
				!is_argument_present(command_argument_list, "-ps") &&
				!is_argument_present(command_argument_list, "--power-cap") &&
				!is_argument_present(command_argument_list, "-pc")) {
			throw SmiToolRequiredCommandException("set");
		} else {
			if (parsed_arguments.fb_sharing_mode != "CUSTOM") {
				if (is_argument_present(command_argument_list, "--group")) {
					throw SmiToolInvalidParameterException("group");
				}
			} else {
				if (!is_argument_present(command_argument_list, "--group")) {
					throw  SmiToolRequiredCommandException("set");
				}
			}
		}
	}

	if (command_argument_list[0] == "ras") {
		if (!is_argument_present(command_argument_list, "--cper") ||
				!is_argument_present(command_argument_list, "--severity")) {
			throw SmiToolRequiredCommandException("ras");
		}
	}

	if (parsed_arguments.options.size() == 0) {
		parsed_arguments.all_arguments = true;
		if(command_argument_list[0] == "reset") {
			throw SmiToolRequiredCommandException("reset");
		}
	}

	if (parsed_arguments.options.size() == 1 && command_argument_list[0] == "monitor"
			&& (is_argument_present(command_argument_list, "--process")
				|| is_argument_present(command_argument_list, "-q"))) {
		parsed_arguments.all_arguments = true;
	}

	is_options_valid(parsed_arguments);
}

void AmdSmiParser::parse_output_format(std::vector<std::string> command_argument_list,
									   Arguments &parsed_arguments)
{
	for (auto option : command_argument_list) {
		if (option == "--csv") {
			if (parsed_arguments.output != human || command_argument_list[0] == "reset"
					|| command_argument_list[0] == "set") {
				throw SmiToolInvalidParameterException(option);
			}
			parsed_arguments.output = csv;
		}
		if (option == "--json") {
			if (parsed_arguments.output != human || command_argument_list[0] == "reset"
					|| command_argument_list[0] == "set") {
				throw SmiToolInvalidParameterException(option);
			}
			parsed_arguments.output = json;
		}
	}
}

void AmdSmiParser::parse_arg(int argc, char **argv, Arguments &ret)
{
	std::vector<std::string> command_argument_list(argv + 1, argv + argc);

	if (argc == 1) {
		ret.command = "help";
		return;
	}

	parse_output_format(command_argument_list, ret);
	parse_command_object(command_argument_list, ret);
	parse_arguments(command_argument_list, ret);
}
