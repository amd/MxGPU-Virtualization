/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "amdsmi.h"

void print_cper_timestamp(amdsmi_cper_timestamp_t *timestamp);
void print_cper_timestamp(amdsmi_cper_timestamp_t *timestamp) {
	if (timestamp == NULL) {
		printf("Invalid timestamp\n");
		return;
	}

	printf("Date: %02d/%02d/%02d%02d\n", timestamp->day, timestamp->month, timestamp->century, timestamp->year);
	printf("Time: %02d:%02d:%02d\n\n", timestamp->hours, timestamp->minutes, timestamp->seconds);
}

int main(void)
{
	amdsmi_status_t ret;
	amdsmi_socket_handle socket = NULL;
	unsigned int gpu_count;

	char cper_data[1024*1024];   // the buffer to hold the raw cper data
	amdsmi_cper_hdr* cper_hdrs[1024];  // the buffer to hold the parsed cper headers
	uint32_t severity_mask = 3;
	uint64_t buf_size = sizeof(cper_data);
	uint64_t entry_count = 1024; //sizeof(cper_hdrs) / sizeof(cper_hdrs[0]);

	uint64_t cursor = 0;  // The cursor to get more data

	ret = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	if (ret != AMDSMI_STATUS_SUCCESS)
		return ret;


	ret = amdsmi_get_processor_handles(socket, &gpu_count, NULL);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}

	amdsmi_processor_handle *processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*gpu_count);

	ret = amdsmi_get_processor_handles(socket, &gpu_count, &processors[0]);
	if (ret != AMDSMI_STATUS_SUCCESS)
		goto fini;

	do {
		ret = amdsmi_gpu_get_cper_entries(processors[0], severity_mask, cper_data, &buf_size, cper_hdrs, &entry_count, &cursor);
		for(uint32_t i = 0; i < entry_count; i++) {
			printf("Record id: 		%s \n", cper_hdrs[i]->record_id);
			printf("Error severity: %d \n", cper_hdrs[i]->error_severity);
			print_cper_timestamp(&cper_hdrs[i]->timestamp);
		}

	} while(ret == AMDSMI_STATUS_MORE_DATA);

fini:
	ret = amdsmi_shut_down();
	if (ret != AMDSMI_STATUS_SUCCESS)
		printf("AMDSMI failed to finish\n");
	free(processors);

	return ret;
}
