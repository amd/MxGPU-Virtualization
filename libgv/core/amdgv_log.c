/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv_device.h"

#define AMDGV_GET_PCI_DOMAIN(x)	  (((x)&0xFFFF0000) >> 16)
#define AMDGV_GET_PCI_BUS(x)	  (((x)&0x0000FF00) >> 8)
#define AMDGV_GET_PCI_DEVICE(x)	  (((x)&0x000000F8) >> 3)
#define AMDGV_GET_PCI_FUNCTION(x) ((x)&0x00000007)

#define USEC_PER_SEC 1000000L

#define AMDGV_SHIFT_ERROR_SEVERITY_LEVEL(level) (level ? (1 << (level - 1)) : 0)

extern const struct amdgv_log_info amdgv_log_list[AMDGV_LOG_CATEGORY_MAX];

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

struct amdgv_log_severity_info {
	enum AMDGV_LOG_RING_TYPE ring;
	uint8_t                  print_level;
	const char              *header;
};

/* Out-of-range severity falls back to print_level = DEBUG_LEVEL4 -> no sink prints. */
static const struct amdgv_log_severity_info severity_map[] = {
	[AMDGV_LOG_SEVERITY_ERROR_HIGH] = { AMDGV_LOG_RING_ERROR, AMDGV_ERROR_LEVEL, LIBGV_ERR_HEADER },
	[AMDGV_LOG_SEVERITY_ERROR_MED]  = { AMDGV_LOG_RING_ERROR, AMDGV_ERROR_LEVEL, LIBGV_ERR_HEADER },
	[AMDGV_LOG_SEVERITY_ERROR_LOW]  = { AMDGV_LOG_RING_ERROR, AMDGV_ERROR_LEVEL, LIBGV_ERR_HEADER },
	[AMDGV_LOG_SEVERITY_WARNING]    = { AMDGV_LOG_RING_ERROR, AMDGV_WARN_LEVEL,  LIBGV_WARN_HEADER },
	[AMDGV_LOG_SEVERITY_INFO]       = { AMDGV_LOG_RING_INFO,  AMDGV_INFO_LEVEL,  LIBGV_EVENT_HEADER },
	[AMDGV_LOG_SEVERITY_DEBUG]      = { AMDGV_LOG_RING_DEBUG, AMDGV_DEBUG_LEVEL, LIBGV_EVENT_HEADER },
};

static const struct amdgv_log_severity_info severity_unknown_info = {
	AMDGV_LOG_RING_ERROR, AMDGV_DEBUG_LEVEL4, LIBGV_ERR_HEADER
};

struct amdgv_log_ring_config {
	uint32_t entry_count;
};

static const struct amdgv_log_ring_config ring_config[AMDGV_LOG_RING_TYPE_MAX] = {
	[AMDGV_LOG_RING_ERROR] = { AMDGV_LOG_ERROR_ENTRY_COUNT },
	[AMDGV_LOG_RING_INFO]  = { AMDGV_LOG_INFO_ENTRY_COUNT  },
	[AMDGV_LOG_RING_DEBUG] = { AMDGV_LOG_DEBUG_ENTRY_COUNT },
};

static inline const struct amdgv_log_severity_info *
amdgv_log_severity_info(uint8_t severity)
{
	if (severity >= ARRAY_SIZE(severity_map))
		return &severity_unknown_info;
	return &severity_map[severity];
}

static void amdgv_log_list_sanity_test(struct amdgv_adapter *adapt)
{
	int i, j;
	uint32_t expect;

	for (i = 0; i < AMDGV_LOG_CATEGORY_MAX; i++) {
		if (amdgv_log_list[i].category != i) {
			AMDGV_WARN("The category index in amdgv_log_list "
				   "does not match AMDGV_LOG_CATEGORY! "
				   "Mismatch at category idx(%d)\n",
				   i);
		}
		for (j = 0; j < amdgv_log_list[i].count; j++) {
			expect = AMDGV_LOG_CODE(i, j);
			if (amdgv_log_list[i].log_msg[j].code != expect) {
				AMDGV_WARN(
					"The error code index in Category(%d) does not match "
					"the enum! Mismatch at error code idx(0x%X)\n",
					i, j);
			}
		}
	}
}

int amdgv_log_alloc_new_notifier(amdgv_dev_t dev, uint64_t event_mask, void *priv,
				 struct amdgv_log_notifier **ctx)
{
	struct amdgv_log_notifier *notifier = OSS_INVALID_HANDLE;
	struct amdgv_log_ring_buffer *log_rb = OSS_INVALID_HANDLE;
	struct amdgv_adapter *adapt;

	adapt = (struct amdgv_adapter *)dev;

	oss_mutex_lock(adapt->log.notifier_list_lock);
	if (adapt->log.notifier_count >= AMDGV_MAX_LOG_NOTIFIER_COUNT) {
		oss_mutex_unlock(adapt->log.notifier_list_lock);
		AMDGV_WARN("Log notifier count has reached the max limit, current count: %u, max limit: %u\n",
			   adapt->log.notifier_count, AMDGV_MAX_LOG_NOTIFIER_COUNT);
		return AMDGV_FAILURE;
	}
	adapt->log.notifier_count++;
	oss_mutex_unlock(adapt->log.notifier_list_lock);

	notifier = oss_malloc(sizeof(struct amdgv_log_notifier));
	if (notifier == OSS_INVALID_HANDLE)
		goto fail;

	log_rb = oss_zalloc(sizeof(struct amdgv_log_ring_buffer));
	if (log_rb == OSS_INVALID_HANDLE)
		goto fail;
	log_rb->entry_count = AMDGV_LOG_ERROR_ENTRY_COUNT;
	log_rb->log_entry_buffer = oss_zalloc(AMDGV_LOG_ERROR_ENTRY_COUNT *
					      sizeof(struct amdgv_log_entry));
	if (log_rb->log_entry_buffer == OSS_INVALID_HANDLE)
		goto fail;

	notifier = (struct amdgv_log_notifier *)notifier;
	log_rb = (struct amdgv_log_ring_buffer *)log_rb;
	AMDGV_INIT_LIST_HEAD(&notifier->head);
	notifier->event_mask = event_mask;
	notifier->log_ring_buffer = log_rb;
	notifier->priv = priv;
	oss_mutex_lock(adapt->log.notifier_list_lock);
	amdgv_list_add_tail(&notifier->head, &adapt->log.notifier_list.head);
	oss_mutex_unlock(adapt->log.notifier_list_lock);
	*ctx = notifier;

	/* signal log process to iterate ERROR ring for this notifier */
	oss_signal_event(adapt->log.new_error_event);

	return 0;

fail:
	oss_mutex_lock(adapt->log.notifier_list_lock);
	if (adapt->log.notifier_count > 0)
		adapt->log.notifier_count--;
	else
		AMDGV_WARN("Log notifier count is already 0 when failed to allocate new notifier.\n");
	oss_mutex_unlock(adapt->log.notifier_list_lock);

	if (log_rb != OSS_INVALID_HANDLE) {
		if (log_rb->log_entry_buffer != OSS_INVALID_HANDLE)
			oss_free(log_rb->log_entry_buffer);
		oss_free(log_rb);
	}

	if (notifier != OSS_INVALID_HANDLE)
		oss_free(notifier);

	return AMDGV_FAILURE;

}

void amdgv_log_delete_notifier(amdgv_dev_t dev, struct amdgv_log_notifier *notifier)
{
	struct amdgv_adapter *adapt;

	if (notifier == OSS_INVALID_HANDLE)
		return;

	adapt = (struct amdgv_adapter *)dev;

	oss_mutex_lock(adapt->log.notifier_list_lock);
	amdgv_list_del(&notifier->head);
	if (adapt->log.notifier_count > 0)
		adapt->log.notifier_count--;
	else
		AMDGV_WARN("Error notifier count is already 0 when deleting notifier.\n");
	oss_mutex_unlock(adapt->log.notifier_list_lock);

	oss_free(notifier->log_ring_buffer->log_entry_buffer);
	oss_free(notifier->log_ring_buffer);
	notifier->log_ring_buffer = OSS_INVALID_HANDLE;
	oss_free(notifier);
}

static void amdgv_log_copy_error(struct amdgv_adapter *adapt,
				   struct amdgv_log_notifier *notifier,
				   struct amdgv_log_entry *entry)
{
	struct amdgv_log_ring_buffer *log_rb;
	int index;

	log_rb = notifier->log_ring_buffer;

	/* allow to over-write oldest entry in notifier */
	index = AMDGV_LOG_INDEX(log_rb, log_rb->write_count);

	if (entry) {
		log_rb->log_entry_buffer[index] = *entry;
		log_rb->write_count++;
	} else
		AMDGV_WARN("Assignment of uninitialized entry attempted.\n");
}

static bool amdgv_log_check_mask(uint32_t log_code, uint64_t event_mask)
{
	uint8_t log_category = AMDGV_LOG_CATEGORY(log_code);
	uint16_t log_sub_code = AMDGV_LOG_SUBCODE(log_code);
	const struct log_text *log_text;
	uint8_t log_level;

	uint64_t mask_category = AMDGV_LOG_MASK_CATEGORY(event_mask);
	uint8_t mask_level = AMDGV_LOG_MASK_SEVERITY(event_mask);

	log_text = &amdgv_log_list[log_category].log_msg[log_sub_code];
	log_level = AMDGV_SHIFT_ERROR_SEVERITY_LEVEL(log_text->severity);

	/* mask level 0xF means including all events,
	 * mask level 0x0 means only including high severity events
	 * other mask levels are in between */
	if (((1ULL << log_category) & mask_category) && (mask_level >= log_level))
		return true;

	return false;
}

static void amdgv_log_iterate_ring_buffer(struct amdgv_adapter *adapt, uint32_t write_count)
{
	struct amdgv_log_ring_buffer *log_rb;
	struct amdgv_log_entry *entry;
	struct amdgv_log_notifier *notifier;
	uint32_t index;

	struct amdgv_log_entry overflow_entry = { 0 };
	uint32_t wr_diff;

	log_rb = adapt->log.rings[AMDGV_LOG_RING_ERROR];

	while (write_count != log_rb->read_count) {
		wr_diff = write_count - log_rb->read_count;

		if (wr_diff > log_rb->entry_count) {
			overflow_entry.timestamp = oss_get_time_stamp();
			overflow_entry.log_code = AMDGV_LOG_DRIVER_BUFFER_OVERFLOW;
			overflow_entry.log_level = AMDGV_LOG_SEVERITY_ERROR_MED;
			overflow_entry.vf_idx = AMDGV_PF_IDX;

			/*
			 * We need the "+1" in below code.
			 *
			 * adapt's RB and notifier's RB use the same struct
			 * and this function will sync these 2 RBs
			 *
			 * The new overflow entry takes 1 extra space in
			 * notifier's RB. If do not +1, then this overflow
			 * message will be OVERFLOWED after the copy
			 */

			/* how many entries we dropped */
			overflow_entry.log_data = wr_diff - log_rb->entry_count + 1;

			log_rb->read_count = write_count - log_rb->entry_count + 1;

			entry = &overflow_entry;
		} else {
			index = AMDGV_LOG_INDEX(log_rb, log_rb->read_count);
			log_rb->read_count++;

			entry = &log_rb->log_entry_buffer[index];
		}

		oss_mutex_lock(adapt->log.notifier_list_lock);
		amdgv_list_for_each_entry (notifier, &adapt->log.notifier_list.head,
					   struct amdgv_log_notifier, head) {
			if (amdgv_log_check_mask(entry->log_code, notifier->event_mask))
				amdgv_log_copy_error(adapt, notifier, entry);
		}
		oss_mutex_unlock(adapt->log.notifier_list_lock);
	}
}

static void amdgv_log_notify_users(struct amdgv_adapter *adapt)
{
	struct amdgv_log_notifier *notifier;

	oss_mutex_lock(adapt->log.notifier_list_lock);
	amdgv_list_for_each_entry (notifier, &adapt->log.notifier_list.head,
				   struct amdgv_log_notifier, head) {
		oss_notifier_wakeup(notifier->priv, 1);
	}
	oss_mutex_unlock(adapt->log.notifier_list_lock);
}

static void amdgv_log_process(struct amdgv_adapter *adapt)
{
	struct amdgv_log_ring_buffer *err_rb;
	uint32_t write_count;

	if (adapt == NULL)
		return;

	err_rb = adapt->log.rings[AMDGV_LOG_RING_ERROR];
	if (err_rb == NULL)
		return;

	amdgv_diag_data_add_error(adapt);

	if (amdgv_list_empty(&adapt->log.notifier_list.head) ||
		(err_rb->write_count == err_rb->read_count))
		return;

	write_count = err_rb->write_count;
	while (write_count != err_rb->read_count) {
		amdgv_log_iterate_ring_buffer(adapt, write_count);
		write_count = err_rb->write_count;
	}
	amdgv_log_notify_users(adapt);
}

static int amdgv_log_process_thread(void *context)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)context;

	while (!oss_thread_should_stop(adapt->log.process_thread)) {
		amdgv_log_process(adapt);
		oss_wait_event(adapt->log.new_error_event, 0);
	}
	return 0;
}

/*
 * amdgv_log_init will not trigger sw_fini sequence
 * MUST call fini function inside if this call failed
 */
int amdgv_log_init(struct amdgv_adapter *adapt)
{
	int i;
	struct amdgv_log_ring_buffer *rb;

	if (adapt == NULL)
		return AMDGV_FAILURE;

	amdgv_log_list_sanity_test(adapt);

	for (i = 0; i < AMDGV_LOG_RING_TYPE_MAX; i++)
		adapt->log.rings[i] = NULL;
	adapt->log.new_error_event = OSS_INVALID_HANDLE;
	adapt->log.notifier_list_lock = OSS_INVALID_HANDLE;
	adapt->log.process_thread = OSS_INVALID_HANDLE;
	AMDGV_INIT_LIST_HEAD(&adapt->log.notifier_list.head);
	adapt->log.notifier_count = 0;

	for (i = 0; i < AMDGV_LOG_RING_TYPE_MAX; i++) {
		rb = (struct amdgv_log_ring_buffer *)oss_zalloc(
			sizeof(struct amdgv_log_ring_buffer));
		if (rb == NULL)
			goto fini;
		rb->entry_count = ring_config[i].entry_count;
		rb->log_entry_buffer = oss_zalloc(rb->entry_count *
						  sizeof(struct amdgv_log_entry));
		if (rb->log_entry_buffer == OSS_INVALID_HANDLE) {
			oss_free(rb);
			goto fini;
		}
		rb->log_lock = oss_spin_lock_init(AMDGV_SPIN_LOCK_HIGHEST_RANK);
		if (rb->log_lock == OSS_INVALID_HANDLE) {
			AMDGV_ERROR("Cannot create spin lock for log ring %d.\n", i);
			oss_free(rb->log_entry_buffer);
			oss_free(rb);
			goto fini;
		}
		adapt->log.rings[i] = rb;
	}

	adapt->log.new_error_event = oss_event_init();
	if (adapt->log.new_error_event == OSS_INVALID_HANDLE) {
		AMDGV_ERROR("Cannot create synchronization event.\n");
		goto fini;
	}

	adapt->log.notifier_list_lock = oss_mutex_init();
	if (adapt->log.notifier_list_lock == OSS_INVALID_HANDLE) {
		AMDGV_ERROR("Cannot create mutex.\n");
		goto fini;
	}

	adapt->log.process_thread = oss_create_thread(amdgv_log_process_thread,
							(void *)adapt, "log_process_thread");
	if (adapt->log.process_thread == OSS_INVALID_HANDLE) {
		AMDGV_ERROR("Cannot create thread.\n");
		goto fini;
	}

	return 0;

fini:
	amdgv_log_fini(adapt);

	return AMDGV_FAILURE;
}

void amdgv_log_fini(struct amdgv_adapter *adapt)
{
	struct amdgv_list_head *start;
	struct amdgv_log_notifier *notifier, *tmp;
	int i;

	if (adapt == NULL)
		return;

	if (adapt->log.process_thread) {
		/* signal event forever permanently makes any subsquent
		 * calls to wait_event() return immediatelly.
		 * This prevents a deadlock in closing the thread if the
		 * signal races the kernel exit check
		 */
		oss_signal_event_forever(adapt->log.new_error_event);
		oss_close_thread(adapt->log.process_thread);
	}

	if (adapt->log.notifier_list_lock)
		oss_mutex_lock(adapt->log.notifier_list_lock);
	start = &adapt->log.notifier_list.head;
	if (!amdgv_list_empty(start)) {
		AMDGV_ASSERT(amdgv_list_empty(start));
		amdgv_list_for_each_entry_safe (notifier, tmp, start,
						struct amdgv_log_notifier, head) {
			amdgv_list_del(&notifier->head);
		}
	}

	adapt->log.notifier_count = 0;

	if (adapt->log.notifier_list_lock)
		oss_mutex_unlock(adapt->log.notifier_list_lock);

	if (adapt->log.notifier_list_lock)
		oss_mutex_fini(adapt->log.notifier_list_lock);

	if (adapt->log.new_error_event)
		oss_event_fini(adapt->log.new_error_event);

	for (i = 0; i < AMDGV_LOG_RING_TYPE_MAX; i++) {
		if (adapt->log.rings[i]) {
			if (adapt->log.rings[i]->log_lock)
				oss_spin_lock_fini(adapt->log.rings[i]->log_lock);
			oss_free(adapt->log.rings[i]->log_entry_buffer);
			oss_free(adapt->log.rings[i]);
			adapt->log.rings[i] = NULL;
		}
	}

	adapt->log.new_error_event = OSS_INVALID_HANDLE;
	adapt->log.notifier_list_lock = OSS_INVALID_HANDLE;
	adapt->log.process_thread = OSS_INVALID_HANDLE;
}

bool amdgv_log_is_valid_vf_code(uint16_t log_code)
{
	uint8_t log_category = AMDGV_LOG_CATEGORY(log_code);
	uint16_t log_sub_code = AMDGV_LOG_SUBCODE(log_code);

	if (log_category != AMDGV_LOG_CATEGORY_VF)
		return false;

	if (log_sub_code >= amdgv_log_list[log_category].count)
		return false;

	return true;
}

static int amdgv_log_format_text(const struct log_text *log_text, uint64_t data,
				 char *buf, uint32_t size)
{
	const char *text_ptr;
	uint32_t len;

	text_ptr = log_text->text + oss_strlen(AMDGV_LOG_PRINT_HEADER);

	switch (log_text->arg_type) {
	case LOG_DATA_ARG_NONE:
		oss_vsnprintf(buf, size, text_ptr);
		break;
	case LOG_DATA_ARG_64:
		oss_vsnprintf(buf, size, text_ptr, data);
		break;
	case LOG_DATA_ARG_32_32:
		oss_vsnprintf(buf, size, text_ptr, (uint32_t)(data >> 32),
			      (uint32_t)(data & 0xFFFFFFFF));
		break;
	case LOG_DATA_ARG_16_16_32:
		oss_vsnprintf(buf, size, text_ptr, (uint16_t)(data >> 48),
			      (uint16_t)((data >> 32) & 0xFFFF),
			      (uint32_t)(data & 0xFFFFFFFF));
		break;
	case LOG_DATA_ARG_16_16_16_16:
		oss_vsnprintf(buf, size, text_ptr, (uint16_t)(data >> 48),
			      (uint16_t)((data >> 32) & 0xFFFF),
			      (uint16_t)((data >> 16) & 0xFFFF),
			      (uint16_t)(data & 0xFFFF));
		break;
	default:
		return 0;
	}

	/* skip the last \n */
	len = oss_strlen(buf);
	if (len)
		buf[len - 1] = 0;

	return oss_strlen(buf);
}

static int amdgv_log_format_text_ext(const struct log_text *log_text, uint64_t data,
				     char *buf, uint32_t size, uint64_t *data_ext)
{
	const char *text_ptr;
	uint32_t len;

	if (!(log_text->flags & AMDGV_LOG_FLAG_EXT))
		return amdgv_log_format_text(log_text, data, buf, size);

	text_ptr = log_text->text + oss_strlen(AMDGV_LOG_PRINT_HEADER);

	switch (log_text->arg_type) {
	case LOG_DATA_ARG_FIVE_64_EXT:
		oss_vsnprintf(buf, size, text_ptr, data,
			data_ext[0], data_ext[1], data_ext[2], data_ext[3]);
		break;
	case LOG_DATA_ARG_THREE_64_EXT:
		oss_vsnprintf(buf, size, text_ptr, data,
			data_ext[0], data_ext[1]);
		break;
	default:
		return 0;
	}

	/* skip the last \n */
	len = oss_strlen(buf);
	if (len)
		buf[len - 1] = 0;

	return oss_strlen(buf);
}

int amdgv_log_get_text(uint32_t log_code, uint64_t data, char *buf, uint32_t size)
{
	uint8_t log_category = AMDGV_LOG_CATEGORY(log_code);
	uint16_t log_sub_code = AMDGV_LOG_SUBCODE(log_code);

	if ((log_category == AMDGV_LOG_CATEGORY_NON_USED) ||
	    (log_category >= AMDGV_LOG_CATEGORY_MAX))
		return 0;

	if (log_sub_code >= amdgv_log_list[log_category].count)
		return 0;

	return amdgv_log_format_text(
		&amdgv_log_list[log_category].log_msg[log_sub_code],
		data, buf, size);
}

int amdgv_log_get_text_ext(uint32_t log_code, uint64_t data, char *buf, uint32_t size, uint64_t *data_ext)
{
	uint8_t log_category = AMDGV_LOG_CATEGORY(log_code);
	uint16_t log_sub_code = AMDGV_LOG_SUBCODE(log_code);

	if ((log_category == AMDGV_LOG_CATEGORY_NON_USED) ||
	    (log_category >= AMDGV_LOG_CATEGORY_MAX))
		return 0;

	if (log_sub_code >= amdgv_log_list[log_category].count)
		return 0;

	return amdgv_log_format_text_ext(
		&amdgv_log_list[log_category].log_msg[log_sub_code],
		data, buf, size, data_ext);
}

static int amdgv_log_kprint(uint32_t pf_bdf, uint32_t idx_vf,
				      const char *func_name, uint32_t line_num,
				      const struct log_text *log_text,
				      uint8_t print_level, const char *print_header,
				      uint64_t data)
{
	char str_vf[5] = { 0, 'F', 0, 0, 0 };

	if (idx_vf == AMDGV_PF_IDX)
		str_vf[0] = 'P';
	else {
		str_vf[0] = 'V';
		str_vf[2] = '0' + idx_vf / 10;
		str_vf[3] = '0' + idx_vf % 10;
	}

	switch (log_text->arg_type) {
	case LOG_DATA_ARG_NONE:
		oss_print(print_level, log_text->text, print_header, (pf_bdf >> 16), (pf_bdf >> 8) & (0xff),
			  (pf_bdf >> 3) & (0x1f), (pf_bdf) & (0x7), str_vf, func_name,
			  line_num);
		break;
	case LOG_DATA_ARG_64:
		oss_print(print_level, log_text->text, print_header, (pf_bdf >> 16), (pf_bdf >> 8) & (0xff),
			  (pf_bdf >> 3) & (0x1f), (pf_bdf) & (0x7), str_vf, func_name,
			  line_num, data);
		break;
	case LOG_DATA_ARG_32_32:
		oss_print(print_level, log_text->text, print_header, (pf_bdf >> 16), (pf_bdf >> 8) & (0xff),
			  (pf_bdf >> 3) & (0x1f), (pf_bdf) & (0x7), str_vf, func_name,
			  line_num, (uint32_t)(data >> 32), (uint32_t)(data & 0xFFFFFFFF));
		break;
	case LOG_DATA_ARG_16_16_32:
		oss_print(print_level, log_text->text, print_header, (pf_bdf >> 16), (pf_bdf >> 8) & (0xff),
			  (pf_bdf >> 3) & (0x1f), (pf_bdf) & (0x7), str_vf, func_name,
			  line_num, (uint16_t)(data >> 48), (uint16_t)((data >> 32) & 0xFFFF),
			  (uint32_t)(data & 0xFFFFFFFF));
		break;
	case LOG_DATA_ARG_16_16_16_16:
		oss_print(print_level, log_text->text, print_header, (pf_bdf >> 16), (pf_bdf >> 8) & (0xff),
			  (pf_bdf >> 3) & (0x1f), (pf_bdf) & (0x7), str_vf, func_name,
			  line_num, (uint16_t)(data >> 48), (uint16_t)((data >> 32) & 0xFFFF),
			  (uint16_t)((data >> 16) & 0xFFFF), (uint16_t)(data & 0xFFFF));
		break;
	default:
		return 0;
	}

	return 0;
}

static int amdgv_log_kprint_ext(uint32_t pf_bdf, uint32_t idx_vf,
					  const char *func_name, uint32_t line_num,
					  const struct log_text *log_text,
					  uint8_t print_level, const char *print_header,
					  uint64_t data, uint64_t *data_ext)
{
	char str_vf[5] = { 0, 'F', 0, 0, 0 };

	if (!(log_text->flags & AMDGV_LOG_FLAG_EXT)) {
		return amdgv_log_kprint(pf_bdf, idx_vf, func_name, line_num,
			log_text, print_level, print_header, data);
	}

	if (idx_vf == AMDGV_PF_IDX)
		str_vf[0] = 'P';
	else {
		str_vf[0] = 'V';
		str_vf[2] = '0' + idx_vf / 10;
		str_vf[3] = '0' + idx_vf % 10;
	}

	switch (log_text->arg_type) {
	case LOG_DATA_ARG_FIVE_64_EXT:
		oss_print(print_level, log_text->text, print_header, (pf_bdf >> 16), (pf_bdf >> 8) & (0xff),
			  (pf_bdf >> 3) & (0x1f), (pf_bdf) & (0x7), str_vf, func_name,
			  line_num, data, data_ext[0], data_ext[1],
			  data_ext[2], data_ext[3]);
		break;
	case LOG_DATA_ARG_THREE_64_EXT:
		oss_print(print_level, log_text->text, print_header, (pf_bdf >> 16), (pf_bdf >> 8) & (0xff),
			  (pf_bdf >> 3) & (0x1f), (pf_bdf) & (0x7), str_vf, func_name,
			  line_num, data, data_ext[0], data_ext[1]);
		break;
	default:
		return 0;
	}

	return 0;
}

static int amdgv_log_prep_syslog_buf(struct amdgv_adapter *adapt,
		uint32_t idx_vf, char *buf, uint32_t size)
{
	/* Append DBDF & VF into the syslog message*/
	oss_vsnprintf(buf, size, "[%x:%d:%d:%d][%s] ",
			AMDGV_GET_PCI_DOMAIN(adapt->bdf),
			AMDGV_GET_PCI_BUS(adapt->bdf),
			AMDGV_GET_PCI_DEVICE(adapt->bdf),
			AMDGV_GET_PCI_FUNCTION(adapt->bdf),
			amdgv_idx_to_str(idx_vf));

	return oss_strlen(buf);
}

/**
 * amdgv_log_dump_stack_filter_set - Add or remove a log code from the dump stack filter list.
 * @dev: The device handle, which is a pointer to the amdgv_adapter structure.
 * @code: The log code to be added or removed from the filter list.
 * @add_enter: A boolean flag indicating whether to add (true) or remove (false) the log code.

  * Return: 0 on success, AMDGV_FAILURE on failure.
 */

int amdgv_log_dump_stack_filter_set(amdgv_dev_t dev, uint32_t code, bool add_enter)
{
	int index;
	uint32_t *filter_list = NULL;
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;

	if (adapt == NULL) {
		AMDGV_PRINT("Adapt should not been NULL\n");
		return AMDGV_FAILURE;
	}

	filter_list = adapt->log.dump_stack_filter_list;

	if (add_enter) {
		for (index = 0; index < AMDGV_ERROR_FILTER_LIST_SIZE_MAX; ++index) {
			if (filter_list[index] == code)
				return 0;

			if (filter_list[index] == 0) {
				filter_list[index] = code;
				return 0;
			}
		}
	} else {
		/*
		* Deletion: replace the item to be deleted with the last non-zero element, and clear the last non-zero element
		*/
		int aim_index = -1;
		for (index = 0; index < AMDGV_ERROR_FILTER_LIST_SIZE_MAX; ++index) {
			if (filter_list[index] == code)
				aim_index = index;

			if (filter_list[index] == 0) {
				if (aim_index != -1) {
					filter_list[aim_index] = filter_list[index - 1];
					filter_list[index - 1] = 0;
					return 0;
				}
				return AMDGV_FAILURE;
			}
		}

		if (aim_index != -1) {
			filter_list[aim_index] = filter_list[AMDGV_ERROR_FILTER_LIST_SIZE_MAX - 1];
			filter_list[AMDGV_ERROR_FILTER_LIST_SIZE_MAX - 1] = 0;
			return 0;
		}
	}

	return AMDGV_FAILURE;
}

void amdgv_log_initialize_default_filters(amdgv_dev_t dev)
{
	int i;
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;

	for (i = 0; i < sizeof(default_filter_table) / sizeof(default_filter_table[0]); ++i) {
		if (amdgv_log_dump_stack_filter_set(adapt, default_filter_table[i], true)) {
			AMDGV_PRINT("Failed to set default filter: %u\n", default_filter_table[i]);
		}
	}
}

static void amdgv_log_dump_stack(struct amdgv_adapter *adapt, uint8_t log_level,
				 uint32_t log_code)
{
	int index;
	uint32_t *filter_list = NULL;
	uint8_t log_category = AMDGV_LOG_CATEGORY(log_code);
	uint16_t log_sub_code = AMDGV_LOG_SUBCODE(log_code);
	const struct log_text *log_text;

	if (adapt == NULL) {
		AMDGV_PRINT("Adapt should not been NULL\n");
		return;
	}

	filter_list = adapt->log.dump_stack_filter_list;
	log_text = &amdgv_log_list[log_category].log_msg[log_sub_code];

	for (index = 0; index < AMDGV_ERROR_FILTER_LIST_SIZE_MAX; ++index) {
		if (filter_list[index] == log_code)
			return;

		if (filter_list[index] == 0) {
			break;
		}
	}

	if (adapt->log.dump_stack_count < adapt->log.dump_stack_max &&
		log_level < AMDGV_LOG_SEVERITY_INFO) {
		oss_dump_stack();
		adapt->log.dump_stack_count++;
	}
}

void amdgv_put_event(amdgv_dev_t dev, uint32_t idx_vf, uint32_t log_code,
		     uint64_t log_data, const char *func_name, uint32_t line_num)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;
	/* Do not zero-init this 512 B buffer. */
	char syslog_msg[AMDGV_LOG_SYS_MSG_SIZE];
	uint32_t used_len;
	struct amdgv_log_ring_buffer *log_rb;
	struct amdgv_log_entry *entry;
	enum AMDGV_LOG_RING_TYPE ring_type;
	int index;

	uint8_t log_category = AMDGV_LOG_CATEGORY(log_code);
	uint16_t log_sub_code = AMDGV_LOG_SUBCODE(log_code);
	uint8_t log_level;
	uint8_t print_level;
	const char *print_header;
	const struct log_text *log_text;
	const struct amdgv_log_severity_info *sev;

	syslog_msg[0] = '\0';

	if (adapt == NULL) {
		AMDGV_PRINT("Adapt should not been NULL\n");
		return;
	}

	if ((log_category == AMDGV_LOG_CATEGORY_NON_USED) ||
	    (log_category >= AMDGV_LOG_CATEGORY_MAX))
		return;

	if (log_sub_code >= amdgv_log_list[log_category].count)
		return;

	log_text = &amdgv_log_list[log_category].log_msg[log_sub_code];
	log_level = log_text->severity;
	sev = amdgv_log_severity_info(log_level);
	ring_type    = sev->ring;
	print_level     = sev->print_level;
	print_header = sev->header;

	if (adapt->log_level >= print_level)
		amdgv_log_kprint(adapt->bdf, idx_vf, func_name, line_num, log_text,
				 print_level, print_header, log_data);

	amdgv_log_dump_stack(adapt, log_level, log_code);

	if (adapt->sys_log_level >= print_level) {
		used_len = amdgv_log_prep_syslog_buf(adapt, idx_vf, syslog_msg, AMDGV_LOG_SYS_MSG_SIZE);
		amdgv_log_format_text(log_text, log_data, syslog_msg + used_len,
			AMDGV_LOG_SYS_MSG_SIZE - used_len);
		oss_notify_shim_ext(adapt->dev, log_code, log_level, syslog_msg);
	}

	log_rb = adapt->log.rings[ring_type];
	if (log_rb == NULL)
		return;

	/* overflow is handled in iterate_ring_buffer (read) call */
	oss_spin_lock_irq(log_rb->log_lock);
	index = AMDGV_LOG_INDEX(log_rb, log_rb->write_count);
	entry = &log_rb->log_entry_buffer[index];
	entry->timestamp = oss_get_time_stamp();
	entry->log_code = log_code;
	entry->log_level = log_level;
	entry->vf_idx = idx_vf;
	entry->log_data = log_data;
	log_rb->write_count++;
	oss_spin_unlock_irq(log_rb->log_lock);

	/* Only ERROR ring drives the process thread / notifier wakeups. */
	if (ring_type == AMDGV_LOG_RING_ERROR)
		oss_signal_event(adapt->log.new_error_event);
}

static int amdgv_log_arg_type_to_arg_ext_num(uint8_t arg_type)
{
	switch (arg_type) {
	case LOG_DATA_ARG_FIVE_64_EXT:
		return 4; /* data + 4 variadic args*/
	case LOG_DATA_ARG_THREE_64_EXT:
		return 2; /* data + 2 variadic args*/
	default:
		return 0;
	}
}

static void amdgv_log_get_va_list_args(int num_args, uint64_t *args_arr, va_list args)
{
	int i = 0;
	for (i = 0; i < num_args; i++) {
		args_arr[i] = va_arg(args, uint64_t);
	}
}

void amdgv_put_event_ext(amdgv_dev_t dev, uint32_t idx_vf, uint32_t log_code,
		     uint64_t log_data, const char *func_name, uint32_t line_num, ...)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;
	struct amdgv_log_ring_buffer *log_rb;
	struct amdgv_log_entry *entry;
	enum AMDGV_LOG_RING_TYPE ring_type;
	int index, i;
	va_list args_ext;
	int args_ext_num;
	uint64_t data_ext[AMDGV_LOG_EXT_MAX_ARGS];
	uint32_t used_len;
	/* Do not zero-init this 512 B buffer. */
	char syslog_msg[AMDGV_LOG_SYS_MSG_SIZE];

	uint8_t log_category = AMDGV_LOG_CATEGORY(log_code);
	uint16_t log_sub_code = AMDGV_LOG_SUBCODE(log_code);
	uint8_t log_level;
	uint8_t print_level;
	const char *print_header;
	const struct log_text *log_text;
	const struct amdgv_log_severity_info *sev;

	syslog_msg[0] = '\0';

	if (adapt == NULL) {
		AMDGV_PRINT("Adapt should not been NULL\n");
		return;
	}

	if ((log_category == AMDGV_LOG_CATEGORY_NON_USED) ||
	    (log_category >= AMDGV_LOG_CATEGORY_MAX))
		return;

	if (log_sub_code >= amdgv_log_list[log_category].count)
		return;

	log_text = &amdgv_log_list[log_category].log_msg[log_sub_code];
	log_level = log_text->severity;
	sev = amdgv_log_severity_info(log_level);
	ring_type    = sev->ring;
	print_level     = sev->print_level;
	print_header = sev->header;

	args_ext_num = amdgv_log_arg_type_to_arg_ext_num(log_text->arg_type);

	va_start(args_ext, line_num);
	amdgv_log_get_va_list_args(args_ext_num, data_ext, args_ext);
	va_end(args_ext);

	if (adapt->log_level >= print_level)
		amdgv_log_kprint_ext(adapt->bdf, idx_vf, func_name, line_num, log_text,
				     print_level, print_header, log_data, data_ext);

	amdgv_log_dump_stack(adapt, log_level, log_code);

	if (adapt->sys_log_level >= print_level) {
		used_len = amdgv_log_prep_syslog_buf(adapt, idx_vf, syslog_msg, AMDGV_LOG_SYS_MSG_SIZE);
		amdgv_log_format_text_ext(log_text, log_data, syslog_msg + used_len,
			AMDGV_LOG_SYS_MSG_SIZE - used_len, data_ext);
		oss_notify_shim_ext(adapt->dev, log_code, log_level, syslog_msg);
	}

	log_rb = adapt->log.rings[ring_type];
	if (log_rb == NULL)
		return;

	/* overflow is handled in iterate_ring_buffer (read) call */
	oss_spin_lock_irq(log_rb->log_lock);
	index = AMDGV_LOG_INDEX(log_rb, log_rb->write_count);
	entry = &log_rb->log_entry_buffer[index];
	entry->timestamp = oss_get_time_stamp();
	entry->log_code = log_code;
	entry->log_level = log_level;
	entry->vf_idx = idx_vf;
	entry->log_data = log_data;
	entry->log_flags = log_text->flags;
	log_rb->write_count++;
	for (i = 0; i < args_ext_num; i++)
		entry->log_data_ext[i] = data_ext[i];
	oss_spin_unlock_irq(log_rb->log_lock);

	/* Only ERROR ring drives the process thread / notifier wakeups. */
	if (ring_type == AMDGV_LOG_RING_ERROR)
		oss_signal_event(adapt->log.new_error_event);
}

int amdgv_log_is_pending(amdgv_dev_t dev, struct amdgv_log_notifier *notifier)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;
	struct amdgv_log_ring_buffer *log_rb;
	uint32_t read_count, write_count;

	if (notifier == NULL || adapt == NULL)
		return false;

	log_rb = notifier->log_ring_buffer;

	write_count = log_rb->write_count;
	read_count = log_rb->read_count;

	return (write_count != read_count);
}

int amdgv_log_get_entry(amdgv_dev_t dev, struct amdgv_log_notifier *notifier,
			struct amdgv_log_entry **log_entry)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;
	struct amdgv_log_ring_buffer *log_rb;
	uint32_t read_count, write_count;
	int index;

	if (!log_entry)
		return AMDGV_FAILURE;

	if (notifier == NULL || adapt == NULL) {
		*log_entry = NULL;
		return AMDGV_FAILURE;
	}

	log_rb = notifier->log_ring_buffer;

	write_count = log_rb->write_count;
	read_count = log_rb->read_count;

	if (write_count != read_count) {
		index = AMDGV_LOG_INDEX(log_rb, read_count);
		*log_entry = &log_rb->log_entry_buffer[index];
		log_rb->read_count++;
	} else {
		*log_entry = NULL;
		AMDGV_DEBUG("Nothing to read from log ring buffer\n");
	}

	return 0;
}

/*
 * Format a single ring entry as one syslog/dmesg-style line:
 *   <prefix><severity header>[<timestamp>][<domain:bus:dev:func>][<vf>] <message>
 * @prefix: optional ring tag (e.g. "[INFO] ") for the combined dump, or NULL.
 * Returns true if the entry was written, false if the buffer is full
 * (in which case the caller should stop).
 */
static bool amdgv_log_format_entry(struct amdgv_adapter *adapt,
				   struct amdgv_log_entry *entry, const char *prefix,
				   char *buf, int buf_size, int *len)
{
	int ret_len;

	/*
	 * Fixed field widths keep the message text column-aligned across lines:
	 * timestamp is right-justified to 20 (max digits of a uint64) and the VF
	 * tag left-justified to 4 (widest is "VF30").
	 */
	ret_len = oss_vsnprintf(buf + *len, buf_size - *len,
			"%s[%20llu][%-4s] ",
			prefix ? prefix : "",
			entry->timestamp,
			amdgv_idx_to_str(entry->vf_idx));
	if (ret_len < 0 || *len + ret_len >= buf_size)
		return false;
	*len += ret_len;

	ret_len = amdgv_log_get_text_ext(entry->log_code, entry->log_data,
			buf + *len, buf_size - *len, entry->log_data_ext);
	if (ret_len < 0 || *len + ret_len >= buf_size)
		return false;
	*len += ret_len;

	ret_len = oss_vsnprintf(buf + *len, buf_size - *len, "\n");
	if (ret_len < 0 || *len + ret_len >= buf_size)
		return false;
	*len += ret_len;

	return true;
}

/*
 * Dump a single ring buffer oldest->newest, skipping empty slots.
 * Holds the ring's log_lock for the whole walk so no writer can modify
 * the ring while it is being dumped.
 */
static int amdgv_log_dump_ring_buffer(struct amdgv_adapter *adapt,
				      struct amdgv_log_ring_buffer *log_rb,
				      char *buf, int buf_size)
{
	struct amdgv_log_entry *entry;
	uint32_t write_count, pos, index;
	int len = 0;

	if (log_rb == NULL)
		return 0;

	oss_spin_lock_irq(log_rb->log_lock);

	write_count = log_rb->write_count;

	for (pos = 0; pos < log_rb->entry_count; pos++) {
		index = AMDGV_LOG_INDEX(log_rb, write_count + 1 + pos);
		entry = &log_rb->log_entry_buffer[index];
		if (entry->log_code == 0)
			continue;

		if (!amdgv_log_format_entry(adapt, entry, NULL, buf, buf_size, &len))
			break;
	}

	oss_spin_unlock_irq(log_rb->log_lock);

	return len;
}

int amdgv_log_get_all(struct amdgv_adapter *adapt, char *buf, int buf_size)
{
	return amdgv_log_dump_ring_buffer(adapt, adapt->log.rings[AMDGV_LOG_RING_ERROR],
					  buf, buf_size);
}

int amdgv_log_dump_ring(struct amdgv_adapter *adapt, enum AMDGV_LOG_RING_TYPE ring_type,
			char *buf, int buf_size)
{
	if (ring_type >= AMDGV_LOG_RING_TYPE_MAX)
		return 0;

	return amdgv_log_dump_ring_buffer(adapt, adapt->log.rings[ring_type], buf, buf_size);
}

struct ring_cursor {
	struct amdgv_log_ring_buffer *rb;
	uint32_t write_count;
	uint32_t pos;
	struct amdgv_log_entry *entry; /* current entry, or NULL if drained */
};

/* Fixed-width tags so the columns after them line up across rings. */
static const char *const ring_prefix[AMDGV_LOG_RING_TYPE_MAX] = {
	[AMDGV_LOG_RING_ERROR] = "[ERROR] ",
	[AMDGV_LOG_RING_INFO]  = "[INFO]  ",
	[AMDGV_LOG_RING_DEBUG] = "[DEBUG] ",
};

int amdgv_log_dump_combined(struct amdgv_adapter *adapt, char *buf, int buf_size)
{
	int len = 0;
	int i, min_i;
	struct ring_cursor cur[AMDGV_LOG_RING_TYPE_MAX];
	/* Freeze every ring for the whole merge so no writer can modify any
	 * ring while it is being dumped.  */
	for (i = 0; i < AMDGV_LOG_RING_TYPE_MAX; i++) {
		cur[i].rb = adapt->log.rings[i];
		cur[i].pos = 0;
		cur[i].entry = NULL;
		if (cur[i].rb)
			oss_spin_lock_irq(cur[i].rb->log_lock);
		cur[i].write_count = cur[i].rb ? cur[i].rb->write_count : 0;
	}

	while (1) {
		/* refill any drained cursor with its next valid entry */
		for (i = 0; i < AMDGV_LOG_RING_TYPE_MAX; i++) {
			if (cur[i].entry != NULL || cur[i].rb == NULL)
				continue;
			while (cur[i].pos < cur[i].rb->entry_count) {
				uint32_t index = AMDGV_LOG_INDEX(cur[i].rb,
						cur[i].write_count + 1 + cur[i].pos);
				cur[i].pos++;
				if (cur[i].rb->log_entry_buffer[index].log_code != 0) {
					cur[i].entry = &cur[i].rb->log_entry_buffer[index];
					break;
				}
			}
		}

		/* pick the oldest pending entry across all rings */
		min_i = -1;
		for (i = 0; i < AMDGV_LOG_RING_TYPE_MAX; i++) {
			if (cur[i].entry == NULL)
				continue;
			if (min_i < 0 ||
			    cur[i].entry->timestamp < cur[min_i].entry->timestamp)
				min_i = i;
		}
		if (min_i < 0)
			break;

		if (!amdgv_log_format_entry(adapt, cur[min_i].entry, ring_prefix[min_i],
					    buf, buf_size, &len))
			break;

		cur[min_i].entry = NULL; /* consume */
	}

	for (i = AMDGV_LOG_RING_TYPE_MAX - 1; i >= 0; i--) {
		if (cur[i].rb)
			oss_spin_unlock_irq(cur[i].rb->log_lock);
	}

	return len;
}

void amdgv_log_put_test_entry(amdgv_dev_t dev, int category)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;
	static uint64_t log_data;

	if ((category <= AMDGV_LOG_CATEGORY_NON_USED) ||
	    (category >= AMDGV_LOG_CATEGORY_MAX))
		return;

	log_data++;

	AMDGV_PRINT("Adapt 0x%p added 1 test log entry Cate(%d) ID(%u)\n", dev, category,
		    log_data);

	amdgv_put_log(AMDGV_PF_IDX, amdgv_log_list[category].count - 1, log_data);
}
