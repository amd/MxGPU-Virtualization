/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "amdsmi.h"

#ifdef SMI_ESXI_BUILD
	#define CPER_RAW_DATA_BUFFER_SIZE (1024 * 10)
	#define CPER_HDRS_ARRAY_SIZE 10
#else
	#define CPER_RAW_DATA_BUFFER_SIZE (1024 * 1024)
	#define CPER_HDRS_ARRAY_SIZE 1024
#endif

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

	char cper_data[CPER_RAW_DATA_BUFFER_SIZE];   // the buffer to hold the raw cper data
	amdsmi_cper_hdr_t* cper_hdrs[CPER_HDRS_ARRAY_SIZE];  // the buffer to hold the parsed cper headers
	uint32_t severity_mask = 3;
	uint64_t buf_size = sizeof(cper_data);
	uint64_t entry_count = CPER_HDRS_ARRAY_SIZE;

	uint64_t cursor = 0;  // The cursor to get more data

	uint64_t afids[AMDSMI_MAX_NUMBER_OF_AFIDS_PER_RECORD];
	uint32_t num_afids = 0;
	char *cper_buffer;

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
		ret = amdsmi_get_gpu_cper_entries(processors[0], severity_mask, cper_data, &buf_size, cper_hdrs, &entry_count, &cursor);
		if (ret != AMDSMI_STATUS_SUCCESS && ret != AMDSMI_STATUS_MORE_DATA) {
			goto fini;
		}

		for(uint32_t i = 0; i < entry_count; i++) {
			printf("Record id: 		%s \n", cper_hdrs[i]->record_id);
			printf("Error severity: %d \n", cper_hdrs[i]->error_severity);
			print_cper_timestamp(&cper_hdrs[i]->timestamp);

			cper_buffer = (char*)malloc(cper_hdrs[i]->record_length * sizeof(char));
			memcpy(cper_buffer, cper_hdrs[i], cper_hdrs[i]->record_length);
			ret = amdsmi_get_afids_from_cper(cper_buffer, cper_hdrs[i]->record_length, afids, &num_afids);
			if (ret != AMDSMI_STATUS_SUCCESS) {
				free(cper_buffer);
				goto fini;
			}

			for(uint32_t i = 0; i < num_afids; i++){
				printf("Sec: %d AFID: %lu \n", i, afids[i]);
			}
			printf("\n");
			free(cper_buffer);
		}

	} while(ret == AMDSMI_STATUS_MORE_DATA);

fini:
	ret = amdsmi_shut_down();
	if (ret != AMDSMI_STATUS_SUCCESS)
		printf("AMDSMI failed to finish\n");
	free(processors);

	return ret;
}
