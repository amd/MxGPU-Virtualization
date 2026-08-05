/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __AMDGV_LOG_INTERNAL_H__
#define __AMDGV_LOG_INTERNAL_H__


/* Ring entry counts — must be powers of 2 (index uses bitmask, not modulo) */
#define AMDGV_LOG_ERROR_ENTRY_COUNT  128
#define AMDGV_LOG_INFO_ENTRY_COUNT   128
#define AMDGV_LOG_DEBUG_ENTRY_COUNT  1024

#define AMDGV_MAX_LOG_NOTIFIER_COUNT 0x1000

/* Index into a ring buffer given its entry_count (power-of-2). */
#define AMDGV_LOG_INDEX(rb, c) ((c) & ((rb)->entry_count - 1))

/* TODO: below Macro are outdated, need to update them */
#define AMDGV_LOG_CODE_FROM_MAILBOX(x)	((uint16_t)(((x) >> 16) & 0xFFFF))
#define AMDGV_LOG_FLAGS_FROM_MAILBOX(x)	((uint16_t)((x)&0xFFFF))
#define AMDGV_LOG_CODE_FLAGS_TO_MAILBOX(c, f) ((((c)&0xFFFF) << 16) | ((f)&0xFFFF))

enum LOG_DATA_TYPE {
	LOG_DATA_ARG_NONE = 0, // No log data
	LOG_DATA_ARG_64,	 // 64-bit
	LOG_DATA_ARG_32_32,	 // 32bit 32bit
	LOG_DATA_ARG_16_16_32, // 16bit 16bit 32bit
	LOG_DATA_ARG_16_16_16_16,
	LOG_DATA_ARG_THREE_64_EXT,
	LOG_DATA_ARG_FIVE_64_EXT,
};

struct log_text {
	const uint32_t code; /* full log code */
	const char	  *code_string;
	const uint8_t  arg_type;
	const uint8_t  severity;
	const char    *text;
	const uint32_t flags;
};

struct amdgv_log_info {
	const uint8_t		 category;
	const struct log_text *log_msg;
	const int		 count;
};

/**
 * Log Capture Ring Buffer for AMD SRIOV GPU Virtualization
 *
 * @ write_count:       Total entries written (wraps, used as ring head).
 * @ read_count:        Read pointer for the notifier consumer.
 * @ entry_count:       Capacity of log_entry_buffer (power of 2).
 * @ log_entry_buffer:  Heap-allocated entry array; size set per ring type.
 */
struct amdgv_log_ring_buffer {
	uint32_t write_count;
	uint32_t read_count;
	uint32_t entry_count;

	spin_lock_t log_lock;
	struct amdgv_log_entry *log_entry_buffer;
};

struct amdgv_log_notifier {
	struct amdgv_list_head		head;
	uint64_t			event_mask;
	struct amdgv_log_ring_buffer *log_ring_buffer;
	void			       *priv;
};

struct amdgv_log {
	struct amdgv_log_ring_buffer	*rings[AMDGV_LOG_RING_TYPE_MAX];
	thread_t			process_thread;
	event_t				new_error_event;
	struct amdgv_log_notifier	notifier_list;
	mutex_t				notifier_list_lock;
	uint32_t			notifier_count;
	uint32_t			dump_stack_max;
	uint32_t			dump_stack_count;
	uint32_t			dump_stack_filter_list[AMDGV_ERROR_FILTER_LIST_SIZE_MAX];
};

int  amdgv_log_init(struct amdgv_adapter *adapt);
void amdgv_log_fini(struct amdgv_adapter *adapt);

bool amdgv_log_is_valid_vf_code(uint16_t log_code);

#endif
