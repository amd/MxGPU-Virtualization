#ifndef RLCV_SCHED_CONFIG_INTERFACE_H
#define RLCV_SCHED_CONFIG_INTERFACE_H

/*
 * Copyright (c) 2020-2023 Advanced Micro Devices, Inc. All rights reserved.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */



#pragma pack(push, 4)
#define SCHEDULER_DESCRIPTOR_VERSION    1
#define SCHEDULER_DESCRIPTOR_SIZE       64
#define MAX_FCN_NUMBER                  32

struct buffer_descriptor{
  uint64_t buffer_location;   /* PF MC address */
  uint32_t buffer_size;       /* in unit of byte */
};

union scheduler_feature_flags {
  struct {
    uint32_t config_perf_data_log: 1;           /* set when host config perf_data_log buffer */
    uint32_t config_debug_dump_log: 1;          /* set when host config debug_dump_log buffer */
    uint32_t config_ts_log: 1;                  /* set when host config ts_log buffer */
    uint32_t config_live_migration: 1;          /* set when host config live migration buffer */
    uint32_t reserved: 28;                      /* reserved bits should be 0*/
  } flags;
  uint32_t all;
};

enum ws_scheduler_mode {
  MANUAL_WS_MODE          = 0,    /* manual world switch mode*/
  SOLID_AUTO_WS_MODE      = 1,    /* solid auto world switch mode*/
  LIQUID_AUTO_WS_MODE     = 2,    /* liquid auto world switch mode*/
};

/*
 * The scheduler_memory_descriptor is used on config_scheduler_feature(0xF) command,
 * RLC_GPU_IOV_SCH_1 register will be used as parameter of config_scheduler_feature cmd.
 * RLC_GPU_IOV_SCH_1 = scheduler_memory_descriptor buffer offset in PF frame buffer.
 * The unit of the offset is 4k bytes. Maximum can support 4G * 4K = 16T fb offset.
 * The scheduler_memory_descriptor structure will describe the detailed settings of rlcv scheduler.
 */

typedef struct scheduler_memory_descriptor {
  uint16_t checksum;  /* CRC check sum of the scheduler_memory_descriptor */
  uint16_t version;   /* version of this structure */
  union scheduler_feature_flags feature_flags;    /* The features flags of HOST driver set */
  enum ws_scheduler_mode scheduler_mode;  /* world switch scheduler mode */
  struct auto_scheduler_config {  /* auto_scheduler_config valid when scheduler_mode = 1 (soild auto mode) or 2 (liquid auto mode)*/
    /* each bits indicate one VFs is active VF or not, bit31 indicates PF*/
    uint32_t active_functions;
    /* time quanta of each VF, vf_time_quanta[31] is PF time quanta. unit is us. */
    uint32_t vf_time_quanta[MAX_FCN_NUMBER];
    /* auto world switch command timeout setting, unit is us. Default is 500*1000 us */
    uint32_t ws_cmd_timeout;
    /* valid when scheduler_mode = 1 (solid), max debit settings for credit-debit algrithm of solid world switch. unit is us. Defaut is 200*1000 us */
    uint32_t max_debit;
    /* valid when scheduler_mode = 2 (liquid), max skip ws cycle in liquid mode. unit is us. Default is 8 cycles*/
    uint32_t max_skipped_cycle;
    /* valid when scheduler_mode = 2 (liquid), busy check internval in liquid mode. unit is us. Default is 100us*/
    uint32_t busy_check_interval;
  } auto_config;
  /* valid when scheduler_mode=1 or 2 and config_perf_data_log = 1.
   * Perf_data_log logs each VF's total time quanta, world switch cycles, skipped cycles and auto yield cycles*/
  struct buffer_descriptor perf_data_log;
  /* valid when scheduler_mode=1 or 2 and config_debug_dump_log = 1.
   * Debug dump logs IDLE/SAVE/LOAD/RUN command execution time and time of VF/PF in RUN state of each world switch loops. */
  struct buffer_descriptor debug_dump_log;
  /* valid when scheduler_mode = 0 and config_ts_log = 1. By default RLCV won't support the ts_log feature, it needs TSL_SUPPORT build flag enabled RLCV.
   * ts_log feature logs time stamps of each sub steps in world switch. The feature is to analysis and turing world switch latency.
   * Currently, ts_log is manual switch only feature. */
  struct buffer_descriptor ts_log;
  /* valid when config_live_migration = 1. */
  struct buffer_descriptor live_migration_data;
  uint32_t reserved[SCHEDULER_DESCRIPTOR_SIZE - 52]; /* reserved dwords should be 0*/
} scheduler_memory_descriptor_t;

#pragma pack(pop)
#endif /* RLCV_SCHED_CONFIG_INTERFACE_H */
