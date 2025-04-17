/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
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
#ifdef _WIN64
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <windows.h>
#define SLEEP(x) Sleep(x)
#else
#include <stdlib.h>
#include <unistd.h>
#define SLEEP(x) usleep((x)*1000)
#endif
#include <stdio.h>
#include "amdsmi.h"
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#ifdef WINDOW_MODE
#include <ncurses.h>
#else
#define printw(...) printf(__VA_ARGS__)
#endif
#define CHECKRET(a)                                                            \
	do {                                                                                \
		do {                                                                            \
			ret = a;                                                                    \
			if (ret == AMDSMI_STATUS_RETRY || ret == AMDSMI_STATUS_BUSY) {              \
				SLEEP(500);                                                             \
				continue;                                                               \
			} else                                                                      \
				break;                                                                  \
		} while (true);                                                                 \
		if (ret == AMDSMI_STATUS_NOT_SUPPORTED) {                                       \
			printw("GPU%u Warning in line %d : %s not supported with ret=%d\n", i,      \
				   __LINE__, #a, ret);                                                  \
		} else if (ret != AMDSMI_STATUS_SUCCESS) {                                      \
			printw("GPU%u Error in line %d : %s failed with ret=%d\n", i,               \
			       __LINE__, #a, ret);                                                  \
			goto fini;                                                              \
		}                                                                               \
	} while (0)
static const char *__str_sched_state(amdsmi_vf_sched_state_t status)
{
	switch (status) {
	case AMDSMI_VF_STATE_UNAVAILABLE:
		return "UNAVL";
	case AMDSMI_VF_STATE_AVAILABLE:
		return "AVAIL";
	case AMDSMI_VF_STATE_ACTIVE:
		return "ACTIV";
	case AMDSMI_VF_STATE_SUSPENDED:
		return "SUSPN";
	case AMDSMI_VF_STATE_FULLACCESS:
		return "FACCS";
	case AMDSMI_VF_STATE_DEFAULT_AVAILABLE:
		return "DEFAV";
	default:
		break;
	}
	return "UNKNW";
}
static double convert_volt(uint64_t v)
{
	return (double) (v / 1000);
}
#ifdef _WIN64
void dumpMemoryLeakInfo(void)
{
	HANDLE hLogFile;
	hLogFile = CreateFile(L".\\monitor_memory_leak.txt", GENERIC_WRITE,
			      FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
			      FILE_ATTRIBUTE_NORMAL, NULL);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, hLogFile);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, hLogFile);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, hLogFile);
}
#endif
int main(void)
{
	int ret = 0;

	uint32_t num_vf_supported;
	uint32_t num_vf_enabled;

	amdsmi_socket_handle socket = NULL;
	amdsmi_asic_info_t asic;
	amdsmi_power_cap_info_t power;
	amdsmi_pf_fb_info_t fb;
	amdsmi_vbios_info_t vbios;
	amdsmi_fw_info_t fw;
	amdsmi_vf_info_t config;
	amdsmi_partition_info_t partitions[AMDSMI_MAX_VF_COUNT];
	amdsmi_driver_info_t driver_version;
	amdsmi_engine_usage_t usage;
	amdsmi_power_info_t power_info;
	bool is_power_management_enabled = 0;
	amdsmi_clk_info_t clock;
	int64_t temp_edge, temp_hotspot, temp_mem;
	int64_t temp_edge_limit, temp_hotspot_limit, temp_mem_limit;
	int64_t temp_edge_shutdown, temp_hotspot_shutdown, temp_mem_shutdown;
	amdsmi_pcie_info_t pcie_info;
	amdsmi_vf_data_t vf_data;
	amdsmi_board_info_t board_info;
	amdsmi_bdf_t bdf;
	char uuid[AMDSMI_GPU_UUID_SIZE] = {0};
	char clk_deep_sleep_str[6] = {0};
	char clk_locked_str[6] = {0};
	unsigned int uuid_length = AMDSMI_GPU_UUID_SIZE;
	char uuid_vf[AMDSMI_GPU_UUID_SIZE] = {0};
	unsigned int uuid_vf_length = AMDSMI_GPU_UUID_SIZE;
	uint32_t sensor_ind = 0;
	uint32_t i;
	uint32_t j;

	ret = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	if (ret != AMDSMI_STATUS_SUCCESS)
		return ret;
	unsigned int gpu_count;
	ret = amdsmi_get_processor_handles(socket, &gpu_count, NULL);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}
	amdsmi_processor_handle *processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*gpu_count);
	ret = amdsmi_get_processor_handles(socket, &gpu_count, &processors[0]);
	if (ret != AMDSMI_STATUS_SUCCESS)
		goto fini;
#ifdef WINDOW_MODE
	initscr();
	nodelay(stdscr, true);
	while (true) {
		clear();
		refresh(); /* Print it on to the real screen */
#endif
	for (i = 0; i < gpu_count; i++) {
		CHECKRET(amdsmi_get_gpu_device_bdf(processors[i], &bdf));
		CHECKRET(amdsmi_get_gpu_asic_info(processors[i], &asic));
		CHECKRET(amdsmi_get_gpu_driver_info(processors[i], &driver_version));
		printw("BDF:[%02" PRIx64 ":%02" PRIx64 ".%" PRIu64 "] [%04x:%04" PRIx64" ]\n",
			(uint64_t)bdf.bdf.bus_number,
			(uint64_t)bdf.bdf.device_number,
			(uint64_t)bdf.bdf.function_number,
			asic.vendor_id,
			asic.device_id);
		CHECKRET(amdsmi_get_num_vf(processors[i], &num_vf_enabled, &num_vf_supported));
		printf("Num vf enabled: %d\n", num_vf_enabled);
		CHECKRET(amdsmi_get_gpu_device_uuid(processors[i], &uuid_length, uuid));
		CHECKRET(amdsmi_get_fb_layout(processors[i], &fb));
		CHECKRET(amdsmi_get_power_cap_info(processors[i], sensor_ind, &power));
		printw("GPU[%s] BDF:[%02" PRIx64 ":%02" PRIx64 ".%" PRIu64 "] [%04x:%04" PRIu64 "] %s %" PRIx64 "MB "
			"TDP: %" PRIu64 "W\n",
		       uuid,
		       (uint64_t)bdf.bdf.bus_number,
		       (uint64_t)bdf.bdf.device_number,
		       (uint64_t)bdf.bdf.function_number,
		       asic.vendor_id,
		       asic.device_id,
		       asic.market_name,
		       (uint64_t)fb.total_fb_size,
		       power.power_cap);
		printw(" S/N: %s\n", asic.asic_serial);
		CHECKRET(amdsmi_get_gpu_board_info(processors[i], &board_info));
		printw(" Model: %s Serial: %s FRU_ID: %s Manufacturer name: %s Product name: %s\n",
				board_info.model_number,
				board_info.product_serial,
				board_info.fru_id,
				board_info.manufacturer_name,
				board_info.product_name);
		printw("-->Driver version: %s\n", driver_version.driver_version);
		printw("-->Driver date: %s\n", driver_version.driver_date);
		CHECKRET(amdsmi_get_gpu_vbios_info(processors[i], &vbios));
		printw("+VBIOS ver:%s %s %s\n", vbios.version, vbios.build_date,
				vbios.part_number);
		CHECKRET(amdsmi_get_gpu_activity(processors[i], &usage));
		CHECKRET(amdsmi_get_clock_info(processors[i], AMDSMI_CLK_TYPE_GFX,
					&clock));
		if (clock.clk_deep_sleep != (uint8_t)-1) {
#ifdef _WIN64
			strcpy_s(clk_deep_sleep_str, sizeof(clk_deep_sleep_str), clock.clk_deep_sleep ? "true" : "false");
#else
			strcpy(clk_deep_sleep_str, clock.clk_deep_sleep ? "true" : "false");
#endif
		} else {
#ifdef _WIN64
			strcpy_s(clk_deep_sleep_str, sizeof(clk_deep_sleep_str), "N/A");
#else
			strcpy(clk_deep_sleep_str, "N/A");
#endif
		}
		if (clock.clk_locked != (uint8_t)-1) {
#ifdef _WIN64
			strcpy_s(clk_locked_str, sizeof(clk_locked_str), clock.clk_locked ? "true" : "false");
#else
			strcpy(clk_locked_str, clock.clk_locked ? "true" : "false");
#endif
		} else {
#ifdef _WIN64
			strcpy_s(clk_locked_str, sizeof(clk_locked_str), "N/A");
#else
			strcpy(clk_locked_str, "N/A");
#endif
		}
		printw("+GFX %d%% cur %dMhz is locked %s is in deep sleep %s\n", usage.gfx_activity,
				clock.clk, clk_locked_str, clk_deep_sleep_str);
		CHECKRET(amdsmi_get_clock_info(processors[i], AMDSMI_CLK_TYPE_MEM,
									   &clock));
		printw("+MEM %d%% cur %dMhz\n", usage.umc_activity, clock.clk);
		CHECKRET(amdsmi_get_clock_info(processors[i], AMDSMI_CLK_TYPE_VCLK0,
				&clock));
		printw("+ENC0 cur %uMhz max: %uMhz\n", clock.clk, clock.max_clk);
		CHECKRET(amdsmi_get_clock_info(processors[i], AMDSMI_CLK_TYPE_VCLK1,
				&clock));
		printw("+ENC1 cur %uMhz max: %uMhz\n", clock.clk, clock.max_clk);
		CHECKRET(amdsmi_get_clock_info(processors[i], AMDSMI_CLK_TYPE_DCLK0,
				&clock));
		printw("+DEC0 cur %uMhz max: %uMhz\n", clock.clk, clock.max_clk);
		CHECKRET(amdsmi_get_clock_info(processors[i], AMDSMI_CLK_TYPE_DCLK1,
				&clock));
		printw("+DEC1 cur %uMhz max: %uMhz\n", clock.clk, clock.max_clk);
		if (strstr(asic.market_name, "MI300") != NULL) {
			CHECKRET(amdsmi_is_gpu_power_management_enabled(processors[i], &is_power_management_enabled));
			printw("+Power management enabled: %s\n", is_power_management_enabled ? "true" : "false");
		}
		CHECKRET(amdsmi_get_power_info(processors[i], sensor_ind, &power_info));
		CHECKRET(amdsmi_get_temp_metric(processors[i], AMDSMI_TEMPERATURE_TYPE_EDGE,
										AMDSMI_TEMP_CURRENT, &temp_edge));
		CHECKRET(amdsmi_get_temp_metric(processors[i], AMDSMI_TEMPERATURE_TYPE_VRAM,
										AMDSMI_TEMP_CURRENT, &temp_mem));
		CHECKRET(amdsmi_get_temp_metric(processors[i], AMDSMI_TEMPERATURE_TYPE_HOTSPOT,
										AMDSMI_TEMP_CURRENT, &temp_hotspot));
		printw("+Power: %" PRIu64 "W Voltage: %.4fV Edge: %" PRIu64 "C Hotspot: %" PRIu64 "C Mem: %" PRIu64 "C\n",
			power_info.socket_power,
			convert_volt(power_info.gfx_voltage),
			temp_edge,
			temp_hotspot,
			temp_mem);
		CHECKRET(amdsmi_get_temp_metric(processors[i], AMDSMI_TEMPERATURE_TYPE_EDGE,
										AMDSMI_TEMP_CRITICAL, &temp_edge_limit));
		CHECKRET(amdsmi_get_temp_metric(processors[i], AMDSMI_TEMPERATURE_TYPE_VRAM,
										AMDSMI_TEMP_CRITICAL, &temp_mem_limit));
		CHECKRET(amdsmi_get_temp_metric(processors[i], AMDSMI_TEMPERATURE_TYPE_HOTSPOT,
										AMDSMI_TEMP_CRITICAL, &temp_hotspot_limit));
		printw("+Power cap: %" PRIu64 "W Edge cap: %" PRIu64 "C Hotspot cap: %" PRIu64 "C Mem cap: %" PRIu64 "C\n",
			power.power_cap,
			temp_edge_limit,
			temp_hotspot_limit,
			temp_mem_limit);
		CHECKRET(amdsmi_get_temp_metric(processors[i], AMDSMI_TEMPERATURE_TYPE_EDGE,
										AMDSMI_TEMP_SHUTDOWN, &temp_edge_shutdown));
		CHECKRET(amdsmi_get_temp_metric(processors[i], AMDSMI_TEMPERATURE_TYPE_VRAM,
										AMDSMI_TEMP_SHUTDOWN, &temp_mem_shutdown));
		CHECKRET(amdsmi_get_temp_metric(processors[i], AMDSMI_TEMPERATURE_TYPE_HOTSPOT,
										AMDSMI_TEMP_SHUTDOWN, &temp_hotspot_shutdown));
		printw("+Edge shutdown: %" PRIu64 "C Hotspot shutdown: %" PRIu64 "C Mem shutdown: %" PRIu64 "C\n",
			temp_edge_shutdown,
			temp_hotspot_shutdown,
			temp_mem_shutdown);
		CHECKRET(amdsmi_get_pcie_info(processors[i], &pcie_info));
		printw("+PCIe: lanes %u speed: %u MT/s bandwidth: %u Mb/s replay count: %" PRIu64 " l0 recovery count: %" PRIu64 " replay roll over count: %" PRIu64 " nak sent count: %" PRIu64 " nak received count: %" PRIu64" \n",
			pcie_info.pcie_metric.pcie_width, pcie_info.pcie_metric.pcie_speed, pcie_info.pcie_metric.pcie_bandwidth, pcie_info.pcie_metric.pcie_replay_count, pcie_info.pcie_metric.pcie_l0_to_recovery_count, pcie_info.pcie_metric.pcie_replay_roll_over_count,
			pcie_info.pcie_metric.pcie_nak_sent_count, pcie_info.pcie_metric.pcie_nak_received_count);
		CHECKRET(amdsmi_get_fw_info(processors[i], &fw));
		CHECKRET(amdsmi_get_vf_partition_info(processors[i], num_vf_enabled, partitions));
		printw("+ Num VF Enabled: %d\n", num_vf_enabled);
		for (j = 0; j < num_vf_enabled; j++) {
			CHECKRET(amdsmi_get_vf_bdf(partitions[j].id, &bdf));
			CHECKRET(amdsmi_get_vf_info(partitions[j].id, &config));
			CHECKRET(amdsmi_get_vf_data(partitions[j].id, &vf_data));
			CHECKRET(amdsmi_get_vf_uuid(partitions[j].id,
								&uuid_vf_length, uuid_vf));
			printw("  -- vf[%s] %02" PRIx64 ":%02" PRIx64 ".%" PRIu64 "\tFB size: %" PRIx64 "MB offset: %" PRIx64 "MB GFX ts:%uusec sched[%s]\n",
					uuid_vf,
					(uint64_t)bdf.bdf.bus_number,
					(uint64_t)bdf.bdf.device_number,
					(uint64_t)bdf.bdf.function_number,
					(uint64_t)config.fb.fb_size,
					(uint64_t)config.fb.fb_offset,
					config.gfx_timeslice,
					__str_sched_state(vf_data.sched.state));
		}
	}
#ifdef WINDOW_MODE
		/* Wait for user input or execution error */
		if (getch() == 'q' || ret != AMDSMI_STATUS_SUCCESS)
			break;
		SLEEP(500);
	} /* while (true) */
	endwin();
#endif
fini:
	ret = amdsmi_shut_down();
	if (ret != AMDSMI_STATUS_SUCCESS)
		printf("AMDSMI failed to finish\n");
	free(processors);
#ifdef _WIN64
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	dumpMemoryLeakInfo();
#endif
	return ret;
}
