/* * Copyright (C) 2023-2024 Advanced Micro Devices. All rights reserved.
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
#include "smi_cli_event_command.h"
#include "smi_cli_exception.h"

#include "json/json.h"

#include <iostream>
#include <sstream>
#ifdef _WIN64
#include <windows.h>
#include <sysinfoapi.h>
#endif

typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLES)(amdsmi_socket_handle, uint32_t *,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_EVENT_CREATE)(amdsmi_processor_handle *, uint32_t,
		uint64_t, amdsmi_event_set *);
typedef amdsmi_status_t (*AMDSMI_EVENT_READ)(amdsmi_event_set, int64_t, amdsmi_event_entry_t *);
typedef amdsmi_status_t (*AMDSMI_EVENT_DESTROY)(amdsmi_event_set);


extern AMDSMI_GET_PROCESSOR_HANDLES host_amdsmi_get_processor_handles;
extern AMDSMI_EVENT_CREATE host_amdsmi_event_create;
extern AMDSMI_EVENT_READ host_amdsmi_event_read;
extern AMDSMI_EVENT_DESTROY host_amdsmi_event_destroy;


amdsmi_event_set set;
amdsmi_processor_handle *processors;


int AmdSmiApiHost::initEvent()
{
	amdsmi_status_t ret;
	unsigned int gpu_count;
	amdsmi_socket_handle socket = NULL;

	amdsmi_get_gpu_count(gpu_count);

	processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*gpu_count);
	if (processors == NULL) {
		throw SmiToolNotEnoughMemException();
	}

	ret = host_amdsmi_get_processor_handles(socket, &gpu_count, &processors[0]);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		free(processors);
		return ret;
	}

	ret = host_amdsmi_event_create(&processors[0], gpu_count, AMDSMI_MASK_ALL, &set);
	return ret;
}

void thread_func_human(char *stopped, amdsmi_event_set set, Arguments arg)
{
	amdsmi_status_t ret;
	amdsmi_event_entry_t event;
	std::string formatted_string{};
	int gpu_index;
	std::string event_msg{};

	while(*stopped != 'q') {
		ret = host_amdsmi_event_read(set, 10*1000*1000, &event);
		if (ret == AMDSMI_STATUS_TIMEOUT)
			continue;
		if ((ret == AMDSMI_STATUS_SUCCESS) && (event.category != AMDSMI_EVENT_CATEGORY_NON_USED)) {
			for (unsigned int i = 0; i < arg.devices.size(); i++) {
				if((*((uint64_t*)processors[i])) == event.dev_id) {
					gpu_index = arg.devices[i]->get_gpu_index();
					event_msg = std::string(event.message);
					event_msg = std::regex_replace(event_msg, std::regex("\n"), " ");
					formatted_string = string_format(
										   eventMessageTemplate, gpu_index, event_msg.c_str(), EVENT_CATEGORY_STR[unsigned(event.category)],
										   event.date);
					formatted_string.append("\n");
				}
			}
			if (arg.is_file) {
				write_to_file(arg.file_path, formatted_string, true);
			} else {
				std::cout << formatted_string.c_str();
			}
		}
		event = {};
	}
}
void thread_func_json(char *stopped, amdsmi_event_set set, Arguments arg)
{

	amdsmi_status_t ret;
	amdsmi_event_entry_t event;
	std::string formatted_string{};
	int gpu_index;
	std::string event_msg{};

	while(*stopped != 'q') {
		ret = host_amdsmi_event_read(set, 10*1000*1000, &event);
		if (ret == AMDSMI_STATUS_TIMEOUT)
			continue;
		if ((ret == AMDSMI_STATUS_SUCCESS) && (event.category != AMDSMI_EVENT_CATEGORY_NON_USED)) {
			for (unsigned int i = 0; i < arg.devices.size(); i++) {
				if((*((uint64_t*)processors[i])) == event.dev_id) {
					gpu_index = arg.devices[i]->get_gpu_index();
					event_msg = std::string(event.message);
					event_msg = std::regex_replace(event_msg, std::regex("\n"), " ");
					nlohmann::ordered_json event_json = {
						{ "gpu", gpu_index },
						{ "message", event_msg },
						{ "category", EVENT_CATEGORY_STR[unsigned(event.category)]},
						{ "date", event.date }
					};
					formatted_string = event_json.dump(4);
					formatted_string.append("\n");
				}
			}
			if (arg.is_file) {
				write_to_file(arg.file_path, formatted_string, true);
			} else {
				std::cout << formatted_string.c_str();
			}
		}
		event = {};
	}
}
void thread_func_csv(char *stopped, amdsmi_event_set set, Arguments arg)
{

	amdsmi_status_t ret;
	amdsmi_event_entry_t event;
	std::string formatted_string{};
	std::string out{};
	int gpu_index;
	std::string event_msg{};

	while(*stopped != 'q') {
		ret = host_amdsmi_event_read(set, 10*1000*1000, &event);
		if (ret == AMDSMI_STATUS_TIMEOUT)
			continue;
		if ((ret == AMDSMI_STATUS_SUCCESS) && (event.category != AMDSMI_EVENT_CATEGORY_NON_USED)) {
			out.append(event_csv_header).append("\n");
			for (unsigned int i = 0; i < arg.devices.size(); i++) {
				if((*((uint64_t*)processors[i])) == event.dev_id) {
					gpu_index = arg.devices[i]->get_gpu_index();
					event_msg = std::string(event.message);
					event_msg = std::regex_replace(event_msg, std::regex("\n"), " ");
					formatted_string = string_format("%d,%s,%s,%s", gpu_index,
													 event_msg.c_str(), EVENT_CATEGORY_STR[unsigned(event.category)], event.date);
					out.append(formatted_string).append("\n");
				}
			}
			if (arg.is_file) {
				write_to_file(arg.file_path, formatted_string, true);
			} else {
				std::cout << out.c_str() << std::endl;
			}
		}
		event = {};
	}
}


int AmdSmiApiHost::amdsmi_get_event_command(Arguments arg, char &stop,
		std::vector<std::thread> &threads)
{
	amdsmi_status_t ret;
	std::string out{};

	for (unsigned int i = 0; i < arg.devices.size(); i++) {
		if (arg.output == csv) {
			std::thread thread_object(thread_func_csv, &stop, set, arg);
			threads.push_back(move(thread_object));
		} else if (arg.output == json) {
			std::thread thread_object(thread_func_json, &stop, set, arg);
			threads.push_back(move(thread_object));
		} else {
			std::thread thread_object(thread_func_human, &stop, set, arg);
			threads.push_back(move(thread_object));
		}
	}

	out.append("Press q and hit ENTER when you want to stop\n\n");
	out.append(eventTemplate);

	std::cout << out.c_str() << std::endl;

	while (stop != 'q') {
		std::cin >> stop;
	}

	for (auto& thread : threads) {
		thread.join();
	}

	ret = host_amdsmi_event_destroy(set);
	free(processors);
	processors = NULL;
	std::cout << "Done!\n";

	return ret;
}
