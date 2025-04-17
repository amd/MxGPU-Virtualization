/*
 * Copyright (c) 2014-2021 Advanced Micro Devices, Inc. All rights reserved.
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

#include "amdgv_device.h"

#define AMDGV_GET_PCI_DOMAIN(x)	  (((x)&0xFFFF0000) >> 16)
#define AMDGV_GET_PCI_BUS(x)	  (((x)&0x0000FF00) >> 8)
#define AMDGV_GET_PCI_DEVICE(x)	  (((x)&0x000000F8) >> 3)
#define AMDGV_GET_PCI_FUNCTION(x) ((x)&0x00000007)

#define USEC_PER_SEC 1000000L

#define AMDGV_SHIFT_ERROR_SEVERITY_LEVEL(level) (level ? (1 << (level - 1)) : 0)

extern const struct amdgv_error_info amdgv_error_list[AMDGV_ERROR_CATEGORY_MAX];

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

/* there is coding error if sanity test failed */
static void amdgv_error_list_sanity_test(struct amdgv_adapter *adapt)
{
	int i, j;
	uint32_t expect;

	for (i = 0; i < AMDGV_ERROR_CATEGORY_MAX; i++) {
		if (amdgv_error_list[i].category != i) {
			AMDGV_WARN("The category index in amdgv_error_list "
				   "does not match AMDGV_ERROR_CATEGORY! "
				   "Mismatch at category idx(%d)\n",
				   i);
		}
		for (j = 0; j < amdgv_error_list[i].count; j++) {
			expect = AMDGV_ERROR_CODE(i, j);
			if (amdgv_error_list[i].error_msg[j].code != expect) {
				AMDGV_WARN(
					"The error code index in Category(%d) does not match "
					"the enum! Mismatch at error code idx(0x%X)\n",
					i, j);
			}
		}
	}
}

int amdgv_error_alloc_new_notifier(amdgv_dev_t dev, uint64_t event_mask, void *priv,
				   struct amdgv_error_notifier **ctx)
{
	struct amdgv_error_notifier *notifier;
	struct amdgv_error_ring_buffer *err_rb;
	struct amdgv_adapter *adapt;

	adapt = (struct amdgv_adapter *)dev;
	notifier = oss_malloc(sizeof(struct amdgv_error_notifier));
	if (notifier == OSS_INVALID_HANDLE)
		return AMDGV_FAILURE;

	err_rb = oss_zalloc(sizeof(struct amdgv_error_ring_buffer));
	if (err_rb == OSS_INVALID_HANDLE)
		return AMDGV_FAILURE;

	notifier = (struct amdgv_error_notifier *)notifier;
	err_rb = (struct amdgv_error_ring_buffer *)err_rb;
	AMDGV_INIT_LIST_HEAD(&notifier->head);
	notifier->event_mask = event_mask;
	notifier->error_ring_buffer = err_rb;
	notifier->priv = priv;
	oss_mutex_lock(adapt->notifier_list_lock);
	amdgv_list_add_tail(&notifier->head, &adapt->notifier_list.head);
	oss_mutex_unlock(adapt->notifier_list_lock);
	*ctx = notifier;

	/* signal error process to iterate error RB for this notifier */
	oss_signal_event(adapt->new_error_event);

	return 0;
}

void amdgv_error_delete_notifier(amdgv_dev_t dev, struct amdgv_error_notifier *notifier)
{
	struct amdgv_adapter *adapt;

	if (notifier == OSS_INVALID_HANDLE)
		return;

	adapt = (struct amdgv_adapter *)dev;

	oss_mutex_lock(adapt->notifier_list_lock);
	amdgv_list_del(&notifier->head);
	oss_mutex_unlock(adapt->notifier_list_lock);

	oss_free(notifier->error_ring_buffer);
	notifier->error_ring_buffer = OSS_INVALID_HANDLE;
	oss_free(notifier);
}

static void amdgv_error_copy_error(struct amdgv_adapter *adapt,
				   struct amdgv_error_notifier *notifier,
				   struct amdgv_error_entry *entry)
{
	struct amdgv_error_ring_buffer *err_rb;
	int index;

	err_rb = notifier->error_ring_buffer;

	/* allow to over-write oldest error in notifier */
	index = AMDGV_ERROR_INDEX(err_rb->write_count);
	if (entry) {
		err_rb->error_entry_buffer[index] = *entry;
		err_rb->write_count++;
	} else
		AMDGV_WARN("Assignment of uninitialized entry attempted.\n");
}

static bool amdgv_error_check_mask(uint32_t error_code, uint64_t event_mask)
{
	uint8_t error_category = AMDGV_ERROR_CATEGORY(error_code);
	uint16_t error_sub_code = AMDGV_ERROR_SUBCODE(error_code);
	const struct error_text *error_text;
	uint8_t error_level;

	uint64_t mask_category = AMDGV_ERROR_MASK_CATEGORY(event_mask);
	uint8_t mask_level = AMDGV_ERROR_MASK_SEVERITY(event_mask);

	error_text = &amdgv_error_list[error_category].error_msg[error_sub_code];
	error_level = AMDGV_SHIFT_ERROR_SEVERITY_LEVEL(error_text->severity);

	/* mask level 0xF means including all events,
	 * mask level 0x0 means only including high severity error events
	 * other mask levels are in between */
	if (((1ULL << error_category) & mask_category) && (mask_level >= error_level))
		return true;

	return false;
}

static void amdgv_error_iterate_ring_buffer(struct amdgv_adapter *adapt, uint32_t write_count)
{
	struct amdgv_error_ring_buffer *err_rb;
	struct amdgv_error_entry *entry;
	struct amdgv_error_notifier *notifier;
	uint32_t index;

	struct amdgv_error_entry overflow_entry = { 0 };
	uint32_t wr_diff;

	err_rb = adapt->error_ring_buffer;

	while (write_count != err_rb->read_count) {
		wr_diff = write_count - err_rb->read_count;

		/* overflowed */
		if (wr_diff > AMDGV_ERROR_BUF_ENTRY_SIZE) {
			// copy an overflow error
			overflow_entry.timestamp = oss_get_time_stamp();
			overflow_entry.error_code = AMDGV_ERROR_DRIVER_BUFFER_OVERFLOW;
			overflow_entry.error_level = AMDGV_ERROR_SEVERITY_ERROR_MED;
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
			overflow_entry.error_data = wr_diff - AMDGV_ERROR_BUF_ENTRY_SIZE + 1;

			/* shift read_count */
			err_rb->read_count = write_count - AMDGV_ERROR_BUF_ENTRY_SIZE + 1;

			entry = &overflow_entry;
		} else {
			index = AMDGV_ERROR_INDEX(err_rb->read_count);
			err_rb->read_count++;

			entry = &err_rb->error_entry_buffer[index];
		}

		oss_mutex_lock(adapt->notifier_list_lock);
		amdgv_list_for_each_entry (notifier, &adapt->notifier_list.head,
					   struct amdgv_error_notifier, head) {
			if (amdgv_error_check_mask(entry->error_code, notifier->event_mask))
				amdgv_error_copy_error(adapt, notifier, entry);
		}
		oss_mutex_unlock(adapt->notifier_list_lock);
	}
}

static void amdgv_error_notify_users(struct amdgv_adapter *adapt)
{
	struct amdgv_error_notifier *notifier;

	oss_mutex_lock(adapt->notifier_list_lock);
	amdgv_list_for_each_entry (notifier, &adapt->notifier_list.head,
				   struct amdgv_error_notifier, head) {
		oss_notifier_wakeup(notifier->priv, 1);
	}
	oss_mutex_unlock(adapt->notifier_list_lock);
}

static void amdgv_error_process(struct amdgv_adapter *adapt)
{
	uint32_t write_count;

	if (adapt == NULL || adapt->error_ring_buffer == NULL)
		return;

	/* log the errors to diagnosis data */
	amdgv_diag_data_add_error(adapt);

	/* if no notifier or new events, do not consume ring buffer */
	if (amdgv_list_empty(&adapt->notifier_list.head) ||
		(adapt->error_ring_buffer->write_count ==
		adapt->error_ring_buffer->read_count))
		return;

	write_count = adapt->error_ring_buffer->write_count;
	while (write_count != adapt->error_ring_buffer->read_count) {
		amdgv_error_iterate_ring_buffer(adapt, write_count);
		write_count = adapt->error_ring_buffer->write_count;
	}
	amdgv_error_notify_users(adapt);
}

static int amdgv_error_process_thread(void *context)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)context;

	while (!oss_thread_should_stop(adapt->error_process_thread)) {
		amdgv_error_process(adapt);
		oss_wait_event(adapt->new_error_event, 0);
	}
	return 0;
}

/*
 * amdgv_error_init will not trigger sw_fini sequence
 * MUST call fini function inside if this call failed
 */
int amdgv_error_init(struct amdgv_adapter *adapt)
{
	if (adapt == NULL)
		return AMDGV_FAILURE;

	amdgv_error_list_sanity_test(adapt);

	adapt->error_ring_buffer = NULL;
	adapt->new_error_event = OSS_INVALID_HANDLE;
	adapt->notifier_list_lock = OSS_INVALID_HANDLE;
	adapt->error_process_thread = OSS_INVALID_HANDLE;
	AMDGV_INIT_LIST_HEAD(&adapt->notifier_list.head);

	adapt->error_ring_buffer = (struct amdgv_error_ring_buffer *)oss_zalloc(
		sizeof(struct amdgv_error_ring_buffer));
	if (adapt->error_ring_buffer == NULL)
		goto fini;

	adapt->error_ring_buffer->error_lock =
		oss_spin_lock_init(AMDGV_SPIN_LOCK_HIGHEST_RANK);
	if (adapt->error_ring_buffer->error_lock == OSS_INVALID_HANDLE) {
		AMDGV_ERROR("Cannot create spin lock.\n");
		goto fini;
	}

	adapt->new_error_event = oss_event_init();
	if (adapt->new_error_event == OSS_INVALID_HANDLE) {
		AMDGV_ERROR("Cannot create synchronization event.\n");
		goto fini;
	}

	adapt->notifier_list_lock = oss_mutex_init();
	if (adapt->notifier_list_lock == OSS_INVALID_HANDLE) {
		AMDGV_ERROR("Cannot create mutex.\n");
		goto fini;
	}

	adapt->error_process_thread = oss_create_thread(amdgv_error_process_thread,
							(void *)adapt, "error_process_thread");
	if (adapt->error_process_thread == OSS_INVALID_HANDLE) {
		AMDGV_ERROR("Cannot create thread.\n");
		goto fini;
	}

	return 0;

fini:
	amdgv_error_fini(adapt);

	return AMDGV_FAILURE;
}

void amdgv_error_fini(struct amdgv_adapter *adapt)
{
	struct amdgv_list_head *start;
	struct amdgv_error_notifier *notifier, *tmp;

	if (adapt == NULL)
		return;

	if (adapt->error_process_thread) {
		/* signal event forever permanently makes any subsquent
		 * calls to wait_event() return immediatelly.
		 * This prevents a deadlock in closing the thread if the
		 * signal races the kernel exit check
		 */
		oss_signal_event_forever(adapt->new_error_event);
		oss_close_thread(adapt->error_process_thread);
	}

	/* need to lock the list to remove notifier */
	if (adapt->notifier_list_lock)
		oss_mutex_lock(adapt->notifier_list_lock);
	start = &adapt->notifier_list.head;
	if (!amdgv_list_empty(start)) {
		AMDGV_ASSERT(amdgv_list_empty(start));
		amdgv_list_for_each_entry_safe (notifier, tmp, start,
						struct amdgv_error_notifier, head) {
			amdgv_list_del(&notifier->head);
		}
	}
	if (adapt->notifier_list_lock)
		oss_mutex_unlock(adapt->notifier_list_lock);

	if (adapt->notifier_list_lock)
		oss_mutex_fini(adapt->notifier_list_lock);

	if (adapt->new_error_event)
		oss_event_fini(adapt->new_error_event);

	if (adapt->error_ring_buffer) {
		if (adapt->error_ring_buffer->error_lock)
			oss_spin_lock_fini(adapt->error_ring_buffer->error_lock);
		oss_free(adapt->error_ring_buffer);
	}

	adapt->error_ring_buffer = NULL;
	adapt->new_error_event = OSS_INVALID_HANDLE;
	adapt->notifier_list_lock = OSS_INVALID_HANDLE;
	adapt->error_process_thread = OSS_INVALID_HANDLE;
}

bool amdgv_error_is_valid_vf_code(uint16_t error_code)
{
	uint8_t error_category = AMDGV_ERROR_CATEGORY(error_code);
	uint16_t error_sub_code = AMDGV_ERROR_SUBCODE(error_code);

	if (error_category != AMDGV_ERROR_CATEGORY_VF)
		return false;

	if (error_sub_code >= amdgv_error_list[error_category].count)
		return false;

	return true;
}

int amdgv_error_get_error_text(uint32_t error_code, uint64_t data, char *buf, uint32_t size)
{
	uint8_t error_category = AMDGV_ERROR_CATEGORY(error_code);
	uint16_t error_sub_code = AMDGV_ERROR_SUBCODE(error_code);
	const struct error_text *error_text;
	const char *text_ptr;

	if ((error_category == AMDGV_ERROR_CATEGORY_NON_USED) ||
	    (error_category >= AMDGV_ERROR_CATEGORY_MAX))
		return 0;

	if (error_sub_code >= amdgv_error_list[error_category].count)
		return 0;

	error_text = &amdgv_error_list[error_category].error_msg[error_sub_code];

	/* skip the header */
	text_ptr = error_text->text;
	text_ptr += oss_strlen(AMDGV_ERROR_PRINT_HEADER);

	switch (error_text->arg_type) {
	case ERROR_DATA_ARG_NONE:
		oss_vsnprintf(buf, size, text_ptr);
		break;
	case ERROR_DATA_ARG_64:
		oss_vsnprintf(buf, size, text_ptr, data);
		break;
	case ERROR_DATA_ARG_32_32:
		oss_vsnprintf(buf, size, text_ptr, (uint32_t)(data >> 32),
			      (uint32_t)(data & 0xFFFFFFFF));
		break;
	case ERROR_DATA_ARG_16_16_32:
		oss_vsnprintf(buf, size, text_ptr, (uint16_t)(data >> 48),
			      (uint16_t)((data >> 32) & 0xFFFF),
			      (uint32_t)(data & 0xFFFFFFFF));
		break;
	case ERROR_DATA_ARG_16_16_16_16:
		oss_vsnprintf(buf, size, text_ptr, (uint16_t)(data >> 48),
			      (uint16_t)((data >> 32) & 0xFFFF),
			      (uint16_t)((data >> 16) & 0xFFFF),
			      (uint16_t)(data & 0xFFFF));
		break;
	default:
		return 0;
	}

	/* skip the last \n */
	buf[oss_strlen(buf) - 1] = 0;

	return oss_strlen(buf);
}

int amdgv_error_get_error_text_ext(uint32_t error_code, uint64_t data, char *buf, uint32_t size, uint64_t *data_ext)
{
	uint8_t error_category = AMDGV_ERROR_CATEGORY(error_code);
	uint16_t error_sub_code = AMDGV_ERROR_SUBCODE(error_code);
	const struct error_text *error_text;
	const char *text_ptr;

	if ((error_category == AMDGV_ERROR_CATEGORY_NON_USED) ||
	    (error_category >= AMDGV_ERROR_CATEGORY_MAX))
		return 0;

	if (error_sub_code >= amdgv_error_list[error_category].count)
		return 0;

	error_text = &amdgv_error_list[error_category].error_msg[error_sub_code];

	if (!(error_text->flags & AMDGV_GPU_ERROR_FLAG_ERROR_EXT))
		return amdgv_error_get_error_text(error_code, data, buf, size);

	/* skip the header */
	text_ptr = error_text->text;
	text_ptr += oss_strlen(AMDGV_ERROR_PRINT_HEADER);

	switch (error_text->arg_type) {
	case ERROR_DATA_ARG_FIVE_64_EXT:
		oss_vsnprintf(buf, size, text_ptr, data,
			data_ext[0], data_ext[1], data_ext[2], data_ext[3]);
		break;
	case ERROR_DATA_ARG_THREE_64_EXT:
		oss_vsnprintf(buf, size, text_ptr, data,
			data_ext[0], data_ext[1]);
		break;
	default:
		return 0;
	}
	/* skip the last \n */
	buf[oss_strlen(buf) - 1] = 0;
	return oss_strlen(buf);
}

static int amdgv_error_print_error_text(uint32_t pf_bdf, uint32_t idx_vf,
					const char *func_name, uint32_t line_num,
					uint32_t error_code, uint64_t data)
{
	uint8_t error_category = AMDGV_ERROR_CATEGORY(error_code);
	uint16_t error_sub_code = AMDGV_ERROR_SUBCODE(error_code);
	uint8_t error_level;
	const struct error_text *error_text;
	const char *print_header;
	char str_vf[5] = { 0, 'F', 0, 0, 0 };

	if ((error_category == AMDGV_ERROR_CATEGORY_NON_USED) ||
	    (error_category >= AMDGV_ERROR_CATEGORY_MAX))
		return 0;

	if (error_sub_code >= amdgv_error_list[error_category].count)
		return 0;

	error_text = &amdgv_error_list[error_category].error_msg[error_sub_code];

	if (idx_vf == AMDGV_PF_IDX)
		str_vf[0] = 'P';
	else {
		str_vf[0] = 'V';
		str_vf[2] = '0' + idx_vf / 10;
		str_vf[3] = '0' + idx_vf % 10;
	}

	error_level = error_text->severity;

	/* match LibGV error level */
	switch (error_level) {
	case AMDGV_ERROR_SEVERITY_ERROR_HIGH:
	case AMDGV_ERROR_SEVERITY_ERROR_MED:
	case AMDGV_ERROR_SEVERITY_ERROR_LOW:
		error_level = AMDGV_ERROR_LEVEL;
		print_header = LIBGV_ERR_HEADER;
		break;
	case AMDGV_ERROR_SEVERITY_WARNING:
		error_level = AMDGV_WARN_LEVEL;
		print_header = LIBGV_WARN_HEADER;
		break;
	case AMDGV_ERROR_SEVERITY_INFO:
		error_level = AMDGV_INFO_LEVEL;
		print_header = LIBGV_EVENT_HEADER;
		break;
	default:
		error_level = AMDGV_ERROR_LEVEL;
		print_header = LIBGV_ERR_HEADER;
		break;
	}

	switch (error_text->arg_type) {
	case ERROR_DATA_ARG_NONE:
		oss_print(error_level, error_text->text, print_header, (pf_bdf >> 16), (pf_bdf >> 8) & (0xff),
			  (pf_bdf >> 3) & (0x1f), (pf_bdf) & (0x7), str_vf, func_name,
			  line_num);
		break;
	case ERROR_DATA_ARG_64:
		oss_print(error_level, error_text->text, print_header, (pf_bdf >> 16), (pf_bdf >> 8) & (0xff),
			  (pf_bdf >> 3) & (0x1f), (pf_bdf) & (0x7), str_vf, func_name,
			  line_num, data);
		break;
	case ERROR_DATA_ARG_32_32:
		oss_print(error_level, error_text->text, print_header, (pf_bdf >> 16), (pf_bdf >> 8) & (0xff),
			  (pf_bdf >> 3) & (0x1f), (pf_bdf) & (0x7), str_vf, func_name,
			  line_num, (uint32_t)(data >> 32), (uint32_t)(data & 0xFFFFFFFF));
		break;
	case ERROR_DATA_ARG_16_16_32:
		oss_print(error_level, error_text->text, print_header, (pf_bdf >> 16), (pf_bdf >> 8) & (0xff),
			  (pf_bdf >> 3) & (0x1f), (pf_bdf) & (0x7), str_vf, func_name,
			  line_num, (uint16_t)(data >> 48), (uint16_t)((data >> 32) & 0xFFFF),
			  (uint32_t)(data & 0xFFFFFFFF));
		break;
	case ERROR_DATA_ARG_16_16_16_16:
		oss_print(error_level, error_text->text, print_header, (pf_bdf >> 16), (pf_bdf >> 8) & (0xff),
			  (pf_bdf >> 3) & (0x1f), (pf_bdf) & (0x7), str_vf, func_name,
			  line_num, (uint16_t)(data >> 48), (uint16_t)((data >> 32) & 0xFFFF),
			  (uint16_t)((data >> 16) & 0xFFFF), (uint16_t)(data & 0xFFFF));
		break;
	default:
		return 0;
	}

	return 0;
}

static int amdgv_error_print_error_text_ext(uint32_t pf_bdf, uint32_t idx_vf,
					const char *func_name, uint32_t line_num,
					uint32_t error_code, uint64_t data,
					uint64_t *data_ext)
{
	uint8_t error_category = AMDGV_ERROR_CATEGORY(error_code);
	uint16_t error_sub_code = AMDGV_ERROR_SUBCODE(error_code);
	uint8_t error_level;
	const struct error_text *error_text;
	const char *print_header;
	char str_vf[5] = { 0, 'F', 0, 0, 0 };

	if ((error_category == AMDGV_ERROR_CATEGORY_NON_USED) ||
	    (error_category >= AMDGV_ERROR_CATEGORY_MAX))
		return 0;

	if (error_sub_code >= amdgv_error_list[error_category].count)
		return 0;

	error_text = &amdgv_error_list[error_category].error_msg[error_sub_code];

	if (!(error_text->flags & AMDGV_GPU_ERROR_FLAG_ERROR_EXT)) {
		return amdgv_error_print_error_text(pf_bdf, idx_vf, func_name, line_num,
			error_code, data);
	}

	if (idx_vf == AMDGV_PF_IDX)
		str_vf[0] = 'P';
	else {
		str_vf[0] = 'V';
		str_vf[2] = '0' + idx_vf / 10;
		str_vf[3] = '0' + idx_vf % 10;
	}

	error_level = error_text->severity;

	/* match LibGV error level */
	switch (error_level) {
	case AMDGV_ERROR_SEVERITY_ERROR_HIGH:
	case AMDGV_ERROR_SEVERITY_ERROR_MED:
	case AMDGV_ERROR_SEVERITY_ERROR_LOW:
		error_level = AMDGV_ERROR_LEVEL;
		print_header = LIBGV_ERR_HEADER;
		break;
	case AMDGV_ERROR_SEVERITY_WARNING:
		error_level = AMDGV_WARN_LEVEL;
		print_header = LIBGV_WARN_HEADER;
		break;
	case AMDGV_ERROR_SEVERITY_INFO:
		error_level = AMDGV_INFO_LEVEL;
		print_header = LIBGV_EVENT_HEADER;
		break;
	default:
		error_level = AMDGV_ERROR_LEVEL;
		print_header = LIBGV_ERR_HEADER;
		break;
	}

	switch (error_text->arg_type) {
	case ERROR_DATA_ARG_FIVE_64_EXT:
		oss_print(error_level, error_text->text, print_header, (pf_bdf >> 16), (pf_bdf >> 8) & (0xff),
			  (pf_bdf >> 3) & (0x1f), (pf_bdf) & (0x7), str_vf, func_name,
			  line_num, data, data_ext[0], data_ext[1],
			  data_ext[2], data_ext[3]);
		break;
	case ERROR_DATA_ARG_THREE_64_EXT:
		oss_print(error_level, error_text->text, print_header, (pf_bdf >> 16), (pf_bdf >> 8) & (0xff),
			  (pf_bdf >> 3) & (0x1f), (pf_bdf) & (0x7), str_vf, func_name,
			  line_num, data, data_ext[0], data_ext[1]);
		break;
	default:
		return 0;
	}

	return 0;
}

static int amdgv_error_prep_syslog_buf(struct amdgv_adapter *adapt,
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
 * amdgv_error_dump_stack_filter_set - Add or remove an error code from the error dump stack filter list.
 * @dev: The device handle, which is a pointer to the amdgv_adapter structure.
 * @code: The error code to be added or removed from the filter list.
 * @add_enter: A boolean flag indicating whether to add (true) or remove (false) the error code.

  * Return: 0 on success, AMDGV_FAILURE on failure.
 */

int amdgv_error_dump_stack_filter_set(amdgv_dev_t dev, uint32_t code, bool add_enter)
{
	int index;
	uint32_t *filter_list = NULL;
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;

	if (adapt == NULL) {
		AMDGV_PRINT("Adapt should not been NULL\n");
		return AMDGV_FAILURE;
	}

	filter_list = adapt->error_dump_stack_filter_list;

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

void amdgv_initialize_default_filters(amdgv_dev_t dev)
{
	int i;
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;

	for (i = 0; i < sizeof(default_filter_table) / sizeof(default_filter_table[0]); ++i) {
		if (amdgv_error_dump_stack_filter_set(adapt, default_filter_table[i], true)) {
			AMDGV_PRINT("Failed to set default filter: %u\n", default_filter_table[i]);
		}
	}
}

static void amdgv_error_dump_stack(struct amdgv_adapter *adapt, uint8_t error_level,
			uint32_t error_code)
{
	int index;
	uint32_t *filter_list = NULL;
	uint8_t error_category = AMDGV_ERROR_CATEGORY(error_code);
	uint16_t error_sub_code = AMDGV_ERROR_SUBCODE(error_code);
	const struct error_text *error_text;

	if (adapt == NULL) {
		AMDGV_PRINT("Adapt should not been NULL\n");
		return;
	}

	filter_list = adapt->error_dump_stack_filter_list;
	error_text = &amdgv_error_list[error_category].error_msg[error_sub_code];
	AMDGV_INFO("put event %s, error code: 0x%x\n", error_text->code_string, error_text->code);

	for (index = 0; index < AMDGV_ERROR_FILTER_LIST_SIZE_MAX; ++index) {
		if (filter_list[index] == error_code)
			return;

		if (filter_list[index] == 0) {
			break;
		}
	}

	if (adapt->error_dump_stack_count < adapt->error_dump_stack_max &&
		error_level < AMDGV_ERROR_SEVERITY_INFO) {
		oss_dump_stack();
		adapt->error_dump_stack_count++;
	}
}

void amdgv_put_event(amdgv_dev_t dev, uint32_t idx_vf, uint32_t error_code,
		     uint64_t error_data, const char *func_name, uint32_t line_num)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;
	char syslog_msg[AMDGV_ERROR_SYS_LOG_MSG_SIZE] = { 0 };
	uint32_t used_len;
	struct amdgv_error_ring_buffer *err_rb;
	struct amdgv_error_entry *entry;
	int index;

	uint8_t error_category = AMDGV_ERROR_CATEGORY(error_code);
	uint16_t error_sub_code = AMDGV_ERROR_SUBCODE(error_code);
	uint8_t error_level;
	const struct error_text *error_text;

	if (adapt == NULL) {
		AMDGV_PRINT("Adapt should not been NULL\n");
		return;
	}

	if ((error_category == AMDGV_ERROR_CATEGORY_NON_USED) ||
	    (error_category >= AMDGV_ERROR_CATEGORY_MAX))
		return;

	if (error_sub_code >= amdgv_error_list[error_category].count)
		return;

	error_text = &amdgv_error_list[error_category].error_msg[error_sub_code];
	error_level = error_text->severity;

	/* output to kernel error log first */
	amdgv_error_print_error_text(adapt->bdf, idx_vf, func_name, line_num, error_code,
				     error_data);

	amdgv_error_dump_stack(adapt, error_level, error_code);

	/* OS System Event Log */
	used_len = amdgv_error_prep_syslog_buf(adapt, idx_vf, syslog_msg, AMDGV_ERROR_SYS_LOG_MSG_SIZE);
	amdgv_error_get_error_text(error_code, error_data, syslog_msg + used_len,
		AMDGV_ERROR_SYS_LOG_MSG_SIZE - used_len);
	oss_notify_shim_ext(adapt->dev, error_code, error_level, syslog_msg);

	if (adapt->error_ring_buffer == NULL)
		return;

	err_rb = adapt->error_ring_buffer;

	/* overflow is handled in iterate_ring_buffer (read) call */
	oss_spin_lock_irq(err_rb->error_lock);
	index = AMDGV_ERROR_INDEX(err_rb->write_count);
	entry = &err_rb->error_entry_buffer[index];
	entry->timestamp = oss_get_time_stamp();
	entry->error_code = error_code;
	entry->error_level = error_level;
	entry->vf_idx = idx_vf;
	entry->error_data = error_data;
	err_rb->write_count++;
	oss_spin_unlock_irq(err_rb->error_lock);

	oss_signal_event(adapt->new_error_event);
}

static int amdgv_error_arg_type_to_arg_ext_num(uint8_t arg_type)
{
	switch (arg_type) {
	case ERROR_DATA_ARG_FIVE_64_EXT:
		return 4; /* data + 4 variadic args*/
	case ERROR_DATA_ARG_THREE_64_EXT:
		return 2; /* data + 2 variadic args*/
	default:
		return 0;
	}
}

static void amdgv_error_get_va_list_args(int num_args, uint64_t *args_arr, va_list args)
{
	int i = 0;
	for (i = 0; i < num_args; i++) {
		args_arr[i] = va_arg(args, uint64_t);
	}
}

void amdgv_put_event_ext(amdgv_dev_t dev, uint32_t idx_vf, uint32_t error_code,
		     uint64_t error_data, const char *func_name, uint32_t line_num, ...)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;
	struct amdgv_error_ring_buffer *err_rb;
	struct amdgv_error_entry *entry;
	int index, i;
	va_list args_ext;
	int args_ext_num;
	uint64_t data_ext[AMDGV_GPU_ERROR_EXT_MAX_ARGS];
	uint32_t used_len;
	char syslog_msg[AMDGV_ERROR_SYS_LOG_MSG_SIZE] = { 0 };

	uint8_t error_category = AMDGV_ERROR_CATEGORY(error_code);
	uint16_t error_sub_code = AMDGV_ERROR_SUBCODE(error_code);
	uint8_t error_level;
	const struct error_text *error_text;

	if (adapt == NULL) {
		AMDGV_PRINT("Adapt should not been NULL\n");
		return;
	}

	if ((error_category == AMDGV_ERROR_CATEGORY_NON_USED) ||
	    (error_category >= AMDGV_ERROR_CATEGORY_MAX))
		return;

	if (error_sub_code >= amdgv_error_list[error_category].count)
		return;

	error_text = &amdgv_error_list[error_category].error_msg[error_sub_code];
	error_level = error_text->severity;

	args_ext_num = amdgv_error_arg_type_to_arg_ext_num(error_text->arg_type);

	va_start(args_ext, line_num);
	amdgv_error_get_va_list_args(args_ext_num, data_ext, args_ext);
	va_end(args_ext);

	/* output to kernel error log first */
	amdgv_error_print_error_text_ext(adapt->bdf, idx_vf, func_name, line_num, error_code,
				     error_data, data_ext);

	amdgv_error_dump_stack(adapt, error_level, error_code);

	/* OS System Event Log */
	used_len = amdgv_error_prep_syslog_buf(adapt, idx_vf, syslog_msg, AMDGV_ERROR_SYS_LOG_MSG_SIZE);
	amdgv_error_get_error_text_ext(error_code, error_data, syslog_msg + used_len,
		AMDGV_ERROR_SYS_LOG_MSG_SIZE - used_len, data_ext);
	oss_notify_shim_ext(adapt->dev, error_code, error_level, syslog_msg);

	if (adapt->error_ring_buffer == NULL)
		return;

	err_rb = adapt->error_ring_buffer;

	/* overflow is handled in iterate_ring_buffer (read) call */
	oss_spin_lock_irq(err_rb->error_lock);
	index = AMDGV_ERROR_INDEX(err_rb->write_count);
	entry = &err_rb->error_entry_buffer[index];
	entry->timestamp = oss_get_time_stamp();
	entry->error_code = error_code;
	entry->error_level = error_level;
	entry->vf_idx = idx_vf;
	entry->error_data = error_data;
	entry->error_flags = error_text->flags;
	err_rb->write_count++;
	for (i = 0; i < args_ext_num; i++)
		entry->error_data_ext[i] = data_ext[i];
	oss_spin_unlock_irq(err_rb->error_lock);


	oss_signal_event(adapt->new_error_event);
}

int amdgv_error_is_pending(amdgv_dev_t dev, struct amdgv_error_notifier *notifier)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;
	struct amdgv_error_ring_buffer *err_rb;
	uint32_t read_count, write_count;

	if (notifier == NULL || adapt == NULL)
		return false;

	err_rb = notifier->error_ring_buffer;

	write_count = err_rb->write_count;
	read_count = err_rb->read_count;

	return (write_count != read_count);
}

int amdgv_error_get_error(amdgv_dev_t dev, struct amdgv_error_notifier *notifier,
			  struct amdgv_error_entry **error_entry)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;
	struct amdgv_error_ring_buffer *err_rb;
	uint32_t read_count, write_count;
	int index;

	if (!error_entry)
		return AMDGV_FAILURE;

	if (notifier == NULL || adapt == NULL) {
		*error_entry = NULL;
		return AMDGV_FAILURE;
	}

	err_rb = notifier->error_ring_buffer;

	write_count = err_rb->write_count;
	read_count = err_rb->read_count;

	if (write_count != read_count) {
		index = AMDGV_ERROR_INDEX(read_count);
		*error_entry = &err_rb->error_entry_buffer[index];
		err_rb->read_count++;
	} else {
		*error_entry = NULL;
		AMDGV_DEBUG("Nothing to read from error ring buffer\n");
	}

	return 0;
}

void amdgv_error_log_put_test_entry(amdgv_dev_t dev, int category)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;
	static uint64_t error_data;

	if ((category <= AMDGV_ERROR_CATEGORY_NON_USED) ||
	    (category >= AMDGV_ERROR_CATEGORY_MAX))
		return;

	error_data++;

	AMDGV_PRINT("Adapt 0x%p added 1 test error Cate(%d) ID(%u)\n", dev, category,
		    error_data);

	amdgv_put_error(AMDGV_PF_IDX, amdgv_error_list[category].count - 1, error_data);
}
