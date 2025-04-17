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
#include "smi_cli_parser.h"
#include "smi_cli_logger_err.h"
#include "smi_cli_templates.h"
#include "smi_cli_device.h"
#include "smi_cli_exception.h"

#include "json/json.h"

#include <sstream>
#ifdef _WIN64
#include <windows.h>
#include <sysinfoapi.h>
#endif

typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_BAD_PAGE_INFO)(amdsmi_processor_handle,
		uint32_t *,
		amdsmi_eeprom_table_record_t *);


extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF host_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_GPU_BAD_PAGE_INFO host_amdsmi_get_gpu_bad_page_info;


int AmdSmiApiHost::amdsmi_get_bad_pages_command(uint64_t processor_bdf, Arguments arg,
		std::string& out, std::string* gpu_id)
{
	amdsmi_status_t ret;
	amdsmi_processor_handle processor;
	amdsmi_eeprom_table_record_t *eeprom_table_records;
	uint32_t bad_page_count = AMDSMI_MAX_BAD_PAGE_RECORD;
	struct tm dtime;
	auto bad_pages_list_json = nlohmann::ordered_json::array();
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}

	ret = host_amdsmi_get_gpu_bad_page_info(
			  processor, &bad_page_count, NULL);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}

	eeprom_table_records = (amdsmi_eeprom_table_record_t *)malloc(sizeof(amdsmi_eeprom_table_record_t)
						   *bad_page_count);
	if (eeprom_table_records == NULL) {
		throw SmiToolNotEnoughMemException();
	}

	ret = host_amdsmi_get_gpu_bad_page_info(
			  processor, &bad_page_count, &eeprom_table_records[0]);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		free(eeprom_table_records);
		return ret;
	}

	for (uint8_t bad_pages_iterator = 0; bad_pages_iterator < bad_page_count;
			bad_pages_iterator++) {
		std::string timestamp{"N/A"};
		time_t rawtime = eeprom_table_records[bad_pages_iterator].ts;
#ifdef WIN64
		if(localtime_s(&dtime, &rawtime) != NULL) {
			timestamp = string_format(
							"%d/%d/%d:%d/%d/%d", dtime.tm_year + 1900, dtime.tm_mon + 1,
							dtime.tm_mday, dtime.tm_hour, dtime.tm_min, dtime.tm_sec);
		}
#else
		if(localtime_r(&rawtime, &dtime) != NULL) {
			timestamp = string_format(
							"%d/%d/%d:%d/%d/%d", dtime.tm_year + 1900, dtime.tm_mon + 1,
							dtime.tm_mday, dtime.tm_hour, dtime.tm_min, dtime.tm_sec);
		}
#endif
		std::string retired_page { string_format(
									   "0x%X",
									   eeprom_table_records[bad_pages_iterator].retired_page)
								 };

		if (arg.output == json) {
			bad_pages_list_json.push_back(nlohmann::ordered_json::object( {
				{ "bad_page", bad_pages_iterator },
				{
					"retired_bad_page",
					retired_page
				},
				{ "timestamp", timestamp },
				{
					"mem_channel",
					eeprom_table_records[bad_pages_iterator].mem_channel
				},
				{
					"mcumc_id",
					eeprom_table_records[bad_pages_iterator].mcumc_id
				} }));

		} else if (arg.output == csv) {
			out += string_format("%s,%u,%s,%s,%u,%u\n", (*gpu_id).c_str(),
								 bad_pages_iterator, retired_page.c_str(),
								 timestamp.c_str(),
								 eeprom_table_records[bad_pages_iterator].mem_channel,
								 eeprom_table_records[bad_pages_iterator].mcumc_id);
		} else {
			out += string_format(
					   badPagesTemplate, bad_pages_iterator, retired_page.c_str(),
					   timestamp.c_str(),
					   eeprom_table_records[bad_pages_iterator].mem_channel,
					   eeprom_table_records[bad_pages_iterator].mcumc_id);
		}
	}

	if (arg.output == json) {
		out = bad_pages_list_json.dump(4);
	}
	free(eeprom_table_records);
	return ret;
}
