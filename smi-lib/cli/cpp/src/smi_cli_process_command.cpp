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

#include "json/json.h"

#include "smi_cli_commands.h"
#include "smi_cli_helpers.h"
#include "smi_cli_process_command.h"
#include "smi_cli_templates.h"
#include "smi_cli_api_base.h"
#include "smi_cli_platform.h"
#include "smi_cli_exception.h"

auto constexpr process_general_header_csv {",pid,name,mem_usage"};
auto constexpr
process_engine_header_csv {",gfx,enc"};
auto constexpr gpu_header_csv {"gpu"};


int AmdSmiProcessCommand::process_command_all_arguments(uint64_t processor,
		std::string &formatted_string, int &proc_num, int gpu_id)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_all_arguments_process_command(
				  processor,
				  arg, formatted_string, proc_num, gpu_id);
	return ret;
}

int AmdSmiProcessCommand::process_command_general(uint64_t processor, std::string &formatted_string,
		int &proc_num,
		int gpu_id)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_general_process_command(processor,
			  arg, formatted_string, proc_num, gpu_id);
	return ret;
}

int AmdSmiProcessCommand::process_command_engine(uint64_t processor, std::string &formatted_string,
		int &proc_num,
		int gpu_id)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_engine_process_command(processor,
			  arg, formatted_string, proc_num, gpu_id);
	return ret;
}

void AmdSmiProcessCommand::process_command_json()
{
	int ret;
	nlohmann::ordered_json json_format = nlohmann::ordered_json::array();
	nlohmann::ordered_json json;
	std::string out{};
	std::string result{};
	int proc_num = 0;
	int prev_proc_num = 0;
	bool error_message_condition = false;

	for (unsigned int i = 0; i < arg.devices.size(); i++) {
		int gpu_id = arg.devices[i]->get_gpu_index();
		json = {};
		nlohmann::ordered_json values_json;
		uint64_t gpu_bdf = arg.devices[i]->get_bdf();
		prev_proc_num = proc_num;
		if (arg.all_arguments) {
			ret = process_command_all_arguments(gpu_bdf, out, proc_num, gpu_id);
			std::string param{"process"};
			if (ret == 7) {
				throw SmiToolSMILIBErrorException(ret);
			}
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				values_json = nlohmann::ordered_json::parse(out);
				json["gpu"] = gpu_id;
				json["process_list"] = values_json;
				out.clear();
			}
		} else if ((std::find(arg.options.begin(), arg.options.end(), "general") != arg.options.end())) {
			ret = process_command_general(gpu_bdf, out, proc_num, i);
			if (ret == INVALID_PARAM_VALUE) {
				nlohmann::ordered_json process_info;
				process_info["process_info"] =  "No running processes detected";
				json["process_list"] = process_info;
			} else {
				std::string param{"general"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					if (arg.process_map.empty() || (!arg.process_map.empty()
													&& (prev_proc_num + arg.process_map.size()) == proc_num)) {
						values_json = nlohmann::ordered_json::parse(out);
						json["gpu"] = arg.devices[i]->get_gpu_index();
						json["process_list"] = values_json;
						out.clear();
					}
				}
			}
		} else if ((std::find(arg.options.begin(), arg.options.end(), "engine") != arg.options.end())) {
			ret = process_command_engine(gpu_bdf, out, proc_num, i);
			if (ret == INVALID_PARAM_VALUE) {
				nlohmann::ordered_json process_info;
				process_info["process_info"] =  "No running processes detected";
				json["process_list"] = process_info;
			} else {
				std::string param{"engine"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					if (arg.process_map.empty() || (!arg.process_map.empty()
													&& (prev_proc_num + arg.process_map.size()) == proc_num)) {
						values_json = nlohmann::ordered_json::parse(out);
						json["gpu"] = arg.devices[i]->get_gpu_index();
						json["process_list"] = values_json;
						out.clear();
					}
				}
			}
		} else if (!arg.process_map.empty()) {
			ret = process_command_all_arguments(gpu_bdf, out, proc_num, i);
			if (ret == INVALID_PARAM_VALUE) {
				nlohmann::ordered_json process_info;
				process_info["process_info"] =  "No running processes detected";
				json["process_list"] = process_info;
			} else {
				int error = handle_exceptions(ret, arg.process_map.begin()->first, arg);
				if (error == 0 && (prev_proc_num + arg.process_map.size()) == proc_num) {
					values_json = nlohmann::ordered_json::parse(out);
					json["gpu"] = gpu_id;
					json["process_list"] = values_json;
					out.clear();
				}
			}
		}
		if (!json.empty()) {
			json_format.insert(json_format.end(), json);
		}
	}

	if (arg.devices.size() > 1) {
		result = json_format.dump(4);
	} else {
		result = json.dump(4);
	}
	if (arg.is_file) {
		write_to_file(arg.file_path, result);
	} else {
		std::cout << std::setw(4) << result << '\n';
	}
}

void AmdSmiProcessCommand::process_command_human()
{
	int ret;
	std::string formatted_string{};
	std::string out{};
	int proc_num = 0;
	int prev_proc_num = 0;
	bool error_message_condition = false;
	for (unsigned int i = 0; i < arg.devices.size(); i++) {
		uint64_t gpu_bdf = arg.devices[i]->get_bdf();
		int gpu_id = arg.devices[i]->get_gpu_index();
		prev_proc_num = proc_num;
		if (arg.all_arguments) {
			ret = process_command_all_arguments(gpu_bdf, formatted_string, proc_num, gpu_id);
			if (ret == 7) {
				throw SmiToolSMILIBErrorException(ret);
			}
			std::string param{"process"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				out += formatted_string;
				formatted_string.clear();
			}
		} else if ((std::find(arg.options.begin(), arg.options.end(), "general") != arg.options.end())) {
			ret = process_command_general(gpu_bdf, formatted_string, proc_num, i);
			if (ret == INVALID_PARAM_VALUE) {
				std::cout << "PROCESS_INFO: No running processes detected" << std::endl;
			} else {
				std::string param{"general"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					if (arg.process_map.empty() || (!arg.process_map.empty()
													&& (prev_proc_num + arg.process_map.size()) == proc_num)) {
						out += string_format(gpuTemplate, arg.devices[i]->get_gpu_index());
						out += formatted_string;
						formatted_string.clear();
					}
				}
			}
		} else if ((std::find(arg.options.begin(), arg.options.end(), "engine") != arg.options.end())) {
			ret = process_command_engine(gpu_bdf, formatted_string, proc_num, i);
			if (ret == INVALID_PARAM_VALUE) {
				std::cout << "PROCESS_INFO: No running processes detected" << std::endl;
			} else {
				std::string param{"engine"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					if (arg.process_map.empty() || (!arg.process_map.empty()
													&& (prev_proc_num + arg.process_map.size()) == proc_num)) {
						out += string_format(gpuTemplate, arg.devices[i]->get_gpu_index());
						out += formatted_string;
						formatted_string.clear();
					}
				}
			}
		} else if (!arg.process_map.empty()) {
			ret = process_command_all_arguments(gpu_bdf, formatted_string, proc_num, i);
			if (ret == INVALID_PARAM_VALUE) {
				std::cout << "PROCESS_INFO: No running processes detected" << std::endl;
			} else {
				int error = handle_exceptions(ret, arg.process_map.begin()->first, arg);
				if (error == 0 && (prev_proc_num + arg.process_map.size()) == proc_num) {
					out += string_format(gpuTemplate, arg.devices[i]->get_gpu_index());
					out += formatted_string;
					formatted_string.clear();
				}
			}
		}
	}

	if (arg.is_file) {
		write_to_file(arg.file_path, out);
	} else {
		std::cout << out.c_str() << std::endl;
	}
}

void AmdSmiProcessCommand::process_command_csv()
{
	int ret;
	std::string headers{};
	std::string values{};
	std::string formatted_string{};
	std::string out{};
	std::vector<std::shared_ptr<Device> > devices;

	int proc_num = 0;
	int prev_proc_num = 0;
	bool error_message_condition = false;
	headers.append(gpu_header_csv);
	std::string param;

	for (unsigned int i = 0; i < arg.devices.size(); i++) {
		int gpu_id = arg.devices[i]->get_gpu_index();
		uint64_t gpu_bdf = arg.devices[i]->get_bdf();
		prev_proc_num = proc_num;
		if (arg.all_arguments) {
			headers.append(process_general_header_csv);
			headers.append(process_engine_header_csv);
			ret = process_command_all_arguments(gpu_bdf, formatted_string, proc_num, gpu_id);
			if (ret == 7) {
				throw SmiToolSMILIBErrorException(ret);
			}
			std::string param{"process"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				std::string param{"process"};
				values.append(formatted_string);
				formatted_string.clear();
			}
		} else if ((std::find(arg.options.begin(), arg.options.end(), "general") != arg.options.end())) {
			headers.append(process_general_header_csv);
			ret = process_command_general(gpu_bdf, formatted_string, proc_num, gpu_id);
			if (ret == INVALID_PARAM_VALUE) {
				values.append("No running processes detected");
			} else {
				std::string param{"general"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					if (arg.process_map.empty() || (!arg.process_map.empty()
													&& (prev_proc_num + arg.process_map.size()) == proc_num)) {
						values.append(formatted_string);
						formatted_string.clear();
					}
				}
			}
		} else if ((std::find(arg.options.begin(), arg.options.end(), "engine") != arg.options.end())) {
			headers.append(process_engine_header_csv);
			ret = process_command_engine(gpu_bdf, formatted_string, proc_num, gpu_id);
			if (ret == INVALID_PARAM_VALUE) {
				values.append("No running processes detected");
			} else {
				std::string param{"engine"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					if (arg.process_map.empty() || (!arg.process_map.empty()
													&& (prev_proc_num + arg.process_map.size()) == proc_num)) {
						values.append(formatted_string);
						formatted_string.clear();
					}
				}
			}
		} else if (!arg.process_map.empty()) {
			headers.append(process_general_header_csv);
			headers.append(process_engine_header_csv);
			ret = process_command_all_arguments(gpu_bdf, formatted_string, proc_num, gpu_id);
			if (ret == INVALID_PARAM_VALUE) {
				values.append("No running processes detected");
			} else {
				int error = handle_exceptions(ret, arg.process_map.begin()->first, arg);
				if (error == 0 && (prev_proc_num + arg.process_map.size()) == proc_num) {
					values.append(formatted_string);
					formatted_string.clear();
				}
			}
		}
		if (i == 0) {
			out.append(headers).append("\n");
		}
		if (values.size() != 0) {
			out.append(values).append("\n");
		}
		values.clear();
	}

	if (arg.is_file) {
		write_to_file(arg.file_path, out);
	} else {
		std::cout << out.c_str() << std::endl;
	}
}

void AmdSmiProcessCommand::process_command_watch()
{
	int ret;
	std::string out{};
	std::string formatted_string{};
	int MAX_LINE_LENGTH{10};
	std::vector<std::string> first_row{};
	std::vector<std::string> second_row{};
	std::vector<std::string> third_row{};
	std::vector<std::string> fourth_row{};
	std::vector<std::vector<std::string>> table;
	std::vector<std::vector<std::string>> value_rows;
	uint8_t current_iteration{0};
	bool should_break{false};
	std::time_t program_start{std::time(nullptr)};
	int proc_num = 0;
	int prev_proc_num = 0;
	bool error_message_condition = false;
	while (true) {
		std::time_t start_timestamp{std::time(nullptr)};
		if (should_break) {
			break;
		}
		for (unsigned int i = 0; i < arg.devices.size(); i++) {
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();
			std::time_t timestamp{std::time(nullptr)};
			std::string timestamp_sec{string_format("%lld",timestamp)};
			prev_proc_num = proc_num;
			if (i == 0) {
				first_row.push_back("timestamp");
				second_row.push_back(" ");
				third_row.push_back("seconds");
				first_row.push_back("gpu");
				second_row.push_back(" ");
				third_row.push_back("index");
			}
			std::string gpu_index(string_format("%d", arg.devices[i]->get_gpu_index()));

			if (arg.all_arguments) {
				ret = process_command_all_arguments(gpu_bdf, formatted_string, proc_num, i);
				std::string param{"process"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					if (i == 0) {
						first_row.push_back("pid");
						first_row.push_back("name");
						first_row.push_back("mem_usage");
						first_row.push_back("usage");
						second_row.push_back(" ");
						second_row.push_back(" ");
						second_row.push_back(" ");
						second_row.push_back("gfx, enc");
						third_row.push_back("index");
						third_row.push_back("string");
						third_row.push_back("int(MB)");
						third_row.push_back(" ");
					}
					std::vector<std::string> process_list{};
					process_list = split_string(formatted_string, '\n');
					for (int j=0; j< process_list.size(); j++) {
						std::vector<std::string> process{};
						process = split_string(process_list[j], ',');
						fourth_row.push_back(timestamp_sec.c_str());
						fourth_row.push_back(gpu_index.c_str());
						fourth_row.push_back((process[0]).c_str());
						fourth_row.push_back((process[1]).c_str());
						fourth_row.push_back((process[2]).c_str());
						fourth_row.push_back((process[3] + "," + process[4]).c_str());
						value_rows.push_back(fourth_row);
						fourth_row.clear();
					}
				}
			} else if ((std::find(arg.options.begin(), arg.options.end(), "general") != arg.options.end())) {
				ret = process_command_general(gpu_bdf, formatted_string, proc_num, i);
				error_message_condition = !arg.process_map.empty() && ret == INVALID_PARAM_VALUE;
				std::string param = error_message_condition ? arg.process_map.begin()->first : "general";
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					if (i == 0) {
						first_row.push_back("pid");
						first_row.push_back("name");
						first_row.push_back("mem_usage");
						second_row.push_back(" ");
						second_row.push_back(" ");
						second_row.push_back(" ");
						third_row.push_back("index");
						third_row.push_back("string");
						third_row.push_back("int(MB)");
					}
					if (arg.process_map.empty() || (!arg.process_map.empty() && (prev_proc_num + 1) == proc_num)) {
						std::vector<std::string> process_list{};
						process_list = split_string(formatted_string, '\n');
						for (int j=0; j< process_list.size(); j++) {
							std::vector<std::string> process{};
							process = split_string(process_list[j], ',');
							fourth_row.push_back(timestamp_sec.c_str());
							fourth_row.push_back(gpu_index.c_str());
							fourth_row.push_back((process[0]).c_str());
							fourth_row.push_back((process[1]).c_str());
							fourth_row.push_back((process[2]).c_str());
							value_rows.push_back(fourth_row);
							fourth_row.clear();
						}
					}
				}
			} else if ((std::find(arg.options.begin(), arg.options.end(), "engine") != arg.options.end())) {
				ret = process_command_engine(gpu_bdf, formatted_string, proc_num, i);
				error_message_condition = !arg.process_map.empty() && ret == INVALID_PARAM_VALUE;
				std::string param = error_message_condition ? arg.process_map.begin()->first : "engine";
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					if (i == 0) {
						first_row.push_back("usage");
						second_row.push_back("gfx, enc");
						third_row.push_back(" ");
					}
					if (arg.process_map.empty() || (!arg.process_map.empty() && (prev_proc_num + 1) == proc_num)) {
						std::vector<std::string> process_list{};
						process_list = split_string(formatted_string, '\n');
						for (int j=0; j< process_list.size(); j++) {
							std::vector<std::string> process{};
							process = split_string(process_list[j], ',');
							fourth_row.push_back(timestamp_sec.c_str());
							fourth_row.push_back(gpu_index.c_str());
							fourth_row.push_back((process[0] + "," + process[1]).c_str());
							value_rows.push_back(fourth_row);
							fourth_row.clear();
						}
					}
				}
			} else if (!arg.process_map.empty()) {
				ret = process_command_all_arguments(gpu_bdf, formatted_string, proc_num, i);
				std::string param{arg.process_map.begin()->first};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					if (i == 0) {
						first_row.push_back("pid");
						first_row.push_back("name");
						first_row.push_back("mem_usage");
						first_row.push_back("usage");
						second_row.push_back(" ");
						second_row.push_back(" ");
						second_row.push_back(" ");
						second_row.push_back("gfx, enc");
						third_row.push_back("index");
						third_row.push_back("string");
						third_row.push_back("int(MB)");
						third_row.push_back(" ");
					}
					if ((prev_proc_num + 1) == proc_num) {
						std::vector<std::string> process_list{};
						process_list = split_string(formatted_string, '\n');
						for (int j=0; j< process_list.size(); j++) {
							std::vector<std::string> process{};
							process = split_string(process_list[j], ',');
							fourth_row.push_back(timestamp_sec.c_str());
							fourth_row.push_back(gpu_index.c_str());
							fourth_row.push_back((process[0]).c_str());
							fourth_row.push_back((process[1]).c_str());
							fourth_row.push_back((process[2]).c_str());
							fourth_row.push_back((process[3] + "," + process[4]).c_str());
							value_rows.push_back(fourth_row);
							fourth_row.clear();
						}
					}
				}
			}
			formatted_string.clear();
		}

		int result_lines{num_of_lines(first_row, MAX_LINE_LENGTH)};
		std::vector<std::vector<std::string>> first_table(result_lines,
				std::vector<std::string>(first_row.size(), " "));

		for (int i = 0; i < first_row.size(); i++) {
			std::vector<std::string> res{split_string_by_size(first_row[i], MAX_LINE_LENGTH)};
			for (int j = 0; j < res.size(); j++) {
				first_table[j][i] = res[j];
			}
		}

		result_lines = num_of_lines(second_row, MAX_LINE_LENGTH);
		std::vector<std::vector<std::string>> second_table(result_lines,
				std::vector<std::string>(second_row.size(), " "));

		for (int i = 0; i < second_row.size(); i++) {
			std::vector<std::string> res{split_string_by_size(second_row[i], MAX_LINE_LENGTH)};
			for (int j = 0; j < res.size(); j++) {
				second_table[j][i] = res[j];
			}
		}

		result_lines = num_of_lines(third_row, MAX_LINE_LENGTH);
		std::vector<std::vector<std::string>> third_table(result_lines,
				std::vector<std::string>(third_row.size(), " "));

		for (int i = 0; i < third_row.size(); i++) {
			std::vector<std::string> res{split_string_by_size(third_row[i], MAX_LINE_LENGTH)};
			for (int j = 0; j < res.size(); j++) {
				third_table[j][i] = res[j];
			}
		}

		for (auto elem : first_table) {
			table.push_back(elem);
		}
		for (auto elem : second_table) {
			table.push_back(elem);
		}
		for (auto elem : third_table) {
			table.push_back(elem);
		}

		for (auto fourth_row : value_rows) {
			result_lines = num_of_lines(fourth_row, MAX_LINE_LENGTH);
			std::vector<std::vector<std::string>> fourth_table(result_lines,
					std::vector<std::string>(fourth_row.size(), " "));

			for (int i = 0; i < fourth_row.size(); i++) {
				std::vector<std::string> res{split_string_by_size(fourth_row[i], MAX_LINE_LENGTH)};
				for (int j = 0; j < res.size(); j++) {
					fourth_table[j][i] = res[j];
				}
			}

			for (auto elem : fourth_table) {
				table.push_back(elem);
			}
		}
		value_rows.clear();

		align_table(table);

		out.append("\n");
		for (auto row : table) {
			for (auto elem : row) {
				out.append(elem.c_str());
			}
			out.append("\n");
		}

		out.append("\n\n");

		while(std::difftime(std::time(nullptr), start_timestamp) < arg.watch) {}

		if (arg.is_file) {
			write_to_file(arg.file_path, out, true);
		} else {
			std::cout << out.c_str() << std::endl;
		}

		out.clear();
		first_row.clear();
		second_row.clear();
		third_row.clear();
		fourth_row.clear();
		table.clear();
		formatted_string.clear();

		current_iteration++;
		if (current_iteration == arg.iterations) {
			should_break = true;
			break;
		}

		if (arg.watch_time > -1) {
			if (std::difftime(std::time(nullptr), program_start) >= arg.watch_time) {
				should_break = true;
				break;
			}
		}
	}


}

void AmdSmiProcessCommand::execute_command()
{
	if(AmdSmiPlatform::getInstance().is_host()) {
		std::string command("process");
		throw SmiToolCommandNotSupportedException(command);
	}

	if (arg.watch > -1) {
		if (arg.output == json) {
			throw SmiToolInvalidParameterException("--json");
		}
		if (arg.output == csv) {
			throw SmiToolInvalidParameterException("--csv");
		}
		std::cout << "If you want to exit looping, press 'CTRL' + 'C' \n\n";
		process_command_watch();
	} else if (arg.output == OutputFormat::json) {
		process_command_json();
	} else if (arg.output == OutputFormat::csv) {
		process_command_csv();
	} else if (arg.output == OutputFormat::human) {
		process_command_human();
	}
};
