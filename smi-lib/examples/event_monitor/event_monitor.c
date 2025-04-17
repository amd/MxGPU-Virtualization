/*
 * Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
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

#include <stdlib.h>
#include <stdio.h>
#include "amdsmi.h"
#include <stdbool.h>

const char *const EVENT_CATEGORY_STR[] = {
	"NULL",
	"Driver",
	"Reset",
	"Scheduler",
	"VBIOS",
	"ECC",
	"Powerplay",
	"SRIOV",
	"VF",
	"Ucode",
	"GPU device",
	"Event guard",
	"GPU monitor",
	"MMSCH",
	"XGMI"
};

const char *const EVENT_SEVERITY_STR[] = {
	"High",
	"Medium",
	"Low",
	"Warn",
	"Info"
};

int main(int argc, char *argv[])
{
	int ret;
	unsigned int gpu_count;
	amdsmi_socket_handle socket = NULL;

	amdsmi_event_set monitor;
	amdsmi_event_entry_t event;

	unsigned long category = AMDSMI_EVENT_CATEGORY__MAX;
	unsigned long severity = 5;

	amdsmi_bdf_t gpu_bdf;
	amdsmi_bdf_t vf_bdf;

	/* a clear event mask which only masks high severities */
	uint64_t event_mask = AMDSMI_MASK_INIT;

	if (argc > 1) {
		category = strtoul(argv[1], NULL, 10);
	}

	if (argc > 2) {
		severity = strtoul(argv[2], NULL, 10);
	}

	if (category < AMDSMI_EVENT_CATEGORY__MAX) {
		/* only include event from given category */
		event_mask = AMDSMI_MASK_INCLUDE_CATEGORY(event_mask, category);
		printf("Only log category %s\n", EVENT_CATEGORY_STR[category]);
	} else {
		/* include all events but only error severities without warnings and infos */
		event_mask = AMDSMI_MASK_DEFAULT;
		printf("Log all categories\n");
	}

	/*
	 * severity 0 : highest, critical error level
	 * severity 1 : med, significant error level
	 * severity 2 : low, trivial error level
	 * severity 3 : warn, warning level
	 * severity 4 : info, info level
	 */
	switch (severity) {
	case 0:
		event_mask = AMDSMI_MASK_HIGH_ERROR_SEVERITY_ONLY(event_mask);
		printf("Log high severity event only\n");
		break;
	case 1:
		event_mask = AMDSMI_MASK_INCLUDE_MED_ERROR_SEVERITY(event_mask);
		printf("Log med to high severity event\n");
		break;
	case 2:
		event_mask = AMDSMI_MASK_INCLUDE_LOW_ERROR_SEVERITY(event_mask);
		printf("Log low to high severity event\n");
		break;
	case 3:
		event_mask = AMDSMI_MASK_INCLUDE_WARN_SEVERITY(event_mask);
		printf("Log warning to high severity event\n");
		break;
	case 4:
	default:
		event_mask = AMDSMI_MASK_INCLUDE_INFO_SEVERITY(event_mask);
		printf("Log all severity event\n");
		break;
	}

	ret = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	if (ret != AMDSMI_STATUS_SUCCESS)
		goto finish;

	gpu_count = AMDSMI_MAX_DEVICES;
	ret = amdsmi_get_processor_handles(socket, &gpu_count, NULL);
	if (ret != AMDSMI_STATUS_SUCCESS)
		goto finish;

	amdsmi_processor_handle *processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*gpu_count);

	ret = amdsmi_get_processor_handles(socket, &gpu_count, &processors[0]);
	if (ret != AMDSMI_STATUS_SUCCESS)
		goto finish;

	/* log all events */
	ret = amdsmi_event_create(&processors[0], (uint8_t) gpu_count, event_mask, &monitor);
	if (ret != AMDSMI_STATUS_SUCCESS)
		goto finish;

	while (true) {
		/* wait 10 sec for event */
		ret = amdsmi_event_read(monitor, 1000 * 1000 * 10, &event);

		if (ret == AMDSMI_STATUS_TIMEOUT) {
			/* if timeout, let's quit */
			printf("No more events\n");
			ret = AMDSMI_STATUS_SUCCESS;
			break;
		} else if (ret == AMDSMI_STATUS_SUCCESS) {
			/* if read an event, let's print it and try again */
			amdsmi_get_gpu_device_bdf(&event.dev_id, &gpu_bdf);
			amdsmi_get_gpu_device_bdf(&event.fcn_id, &vf_bdf);

			printf("\n");
			printf("=============== Event ================\n");
			printf("  Time:\n");
			printf("  %s\n", event.date);
			printf("--------------------------------------\n");
			printf("  Processor:\n");
			printf("  GPU BDF %02x:%02x.%01x\n",
				gpu_bdf.bdf.bus_number,
				gpu_bdf.bdf.device_number, gpu_bdf.bdf.function_number);
			printf("  VF  BDF %02x:%02x.%01x\n",
				vf_bdf.bdf.bus_number,
				vf_bdf.bdf.device_number, vf_bdf.bdf.function_number);
			printf("--------------------------------------\n");
			printf("  Event meta:\n");
			printf("  Category: %s\n", EVENT_CATEGORY_STR[event.category]);
			printf("  Subcode : 0x%X\n", event.subcode);
			printf("  Severity: %s\n", EVENT_SEVERITY_STR[event.level]);
			printf("--------------------------------------\n");
			printf("  Message:\n");
			printf("  %s\n", event.message);
			printf("======================================\n");
		} else {
			/* read failed */
			break;
		}
	}

	amdsmi_event_destroy(monitor);

finish:
	if (amdsmi_shut_down() != AMDSMI_STATUS_SUCCESS)
		printf("SMI failed to finish\n");
	free(processors);
	return ret;
}
