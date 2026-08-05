/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv_device.h"
#include "amdgv_vfmgr.h"
#include "amdgv_sched.h"
#include "amdgv_sched_internal.h"
#include "amdgv_gpuiov.h"
#include "amdgv_guard.h"
#include "amdgv_notify.h"
#include "amdgv_psp_gfx_if.h"
#include "ta_ras_if.h"
#include "amdgv_irqmgr.h"
#include "amdgv_api_internal.h"
#include "amdgv_gpumon_internal.h"
#include "amdgv_ecc.h"
#include "amdgv_gfx.h"
#include "amdgv_marketing_name.h"
#include "amdgv_gart.h"
#include "amdgv_live_migration.h"

#define AMDGV_API

static const uint32_t this_block = AMDGV_API_BLOCK;

/* global amdgv flags */
uint64_t amdgv_flags;

/* global os service callback */
struct oss_interface *amdgv_oss_funcs;

typedef void (*amdgv_funcptr_t)(void);

const char *const amdgv_idx2str[AMDGV_MAX_VF_SLOT] = {
	"VF0",	"VF1",	"VF2",	"VF3",	"VF4",	"VF5",	"VF6",	"VF7",	"VF8",	"VF9",	"VF10",
	"VF11", "VF12", "VF13", "VF14", "VF15", "VF16", "VF17", "VF18", "VF19", "VF20", "VF21",
	"VF22", "VF23", "VF24", "VF25", "VF26", "VF27", "VF28", "VF29", "VF30", "PF"
};

const char *const amdgv_inf_name[] = {
	"get_vf_dev_from_bdf",
	"put_vf_dev",
	"map_vf_dev_res",
	"unmap_vf_dev_res",
	"map_framebuffer",
	"unmap_framebuffer",
	"mm_readb",
	"mm_readw",
	"mm_readl",
	"mm_writeb",
	"mm_writew",
	"mm_writel",
	"mm_writeq",
	"io_readb",
	"io_readw",
	"io_readl",
	"io_writeb",
	"io_writew",
	"io_writel",
	"pci_read_config_byte",
	"pci_read_config_word",
	"pci_read_config_dword",
	"pci_write_config_byte",
	"pci_write_config_word",
	"pci_write_config_dword",
	"pci_find_cap",
	"pci_find_ext_cap",
	"pci_find_next_ext_cap",
	"pci_restore_vf_rebar",
	"pci_enable_sriov",
	"pci_disable_sriov",
	"pci_map_rom",
	"pci_unmap_rom",
	"pci_read_rom",
	"register_interrupt",
	"unregister_interrupt",
	"pci_bus_reset",
	"alloc_small_memory",
	"alloc_small_memory_atomic",
	"alloc_small_zero_memory",
	"free_small_memory",
	"get_physical_addr",
	"alloc_memory",
	"free_memory",
	"alloc_dma_mem",
	"alloc_dma_mem_with_attr",
	"free_dma_mem",
	"flush_dma_mem",
	"sg_dma_address",
	"memremap",
	"memunmap",
	"memset",
	"memcpy",
	"memcmp",
	"strncmp",
	"strlen",
	"strnlen",
	"do_div",
	"spin_lock_init",
	"spin_lock",
	"spin_unlock",
	"spin_lock_irq",
	"spin_unlock_irq",
	"spin_lock_fini",
	"spin_lock_init_raw",
	"spin_lock_irqsave_raw",
	"spin_unlock_irqrestore_raw",
	"mutex_init",
	"mutex_init_raw",
	"mutex_lock",
	"mutex_unlock",
	"mutex_fini",
	"mutex_destroy_raw",
	"rwlock_init",
	"rwlock_read_lock",
	"rwlock_read_trylock",
	"rwlock_read_unlock",
	"rwlock_write_lock",
	"rwlock_write_trylock",
	"rwlock_write_unlock",
	"rwlock_fini",
	"rwsema_init",
	"rwsema_read_lock",
	"rwsema_read_trylock",
	"rwsema_read_unlock",
	"rwsema_write_lock",
	"rwsema_write_trylock",
	"rwsema_write_unlock",
	"rwsema_fini",
	"event_init",
	"signal_event",
	"signal_event_with_flag",
	"signal_event_forever",
	"signal_event_forever_with_flag",
	"wait_event",
	"event_fini",
	"notifier_wakeup",
	"atomic_init",
	"atomic_read",
	"atomic_set",
	"atomic_inc",
	"atomic_dec",
	"atomic_sub",
	"atomic_inc_return",
	"atomic_dec_return",
	"atomic_cmpxchg",
	"atomic_fini",
	"create_thread",
	"get_current_thread",
	"is_current_running_thread",
	"close_thread",
	"thread_should_stop",
	"timer_init",
	"timer_init_ex",
	"start_timer",
	"pause_timer",
	"try_pause_timer",
	"close_timer",
	"get_assigned_vf_count",
	"udelay",
	"msleep",
	"usleep",
	"yield",
	"memory_fence",
	"get_time_stamp",
	"get_utc_time_stamp",
	"get_utc_time_stamp_str",
	"ari_supported",
	"print",
	"vsnprintf",
	"send_msg",
	"store_dump",
	"get_random_bytes",
	"sema_up",
	"sema_down",
	"sema_init",
	"sema_fini",
	"access_ok",
	"copy_from_user",
	"copy_to_user",
	"strnstr",
	"detect_fw",
	"get_fw",
	"get_discovery_binary",
	"create_hash_64",
	"dump_stack",
	"copy_call_trace_buffer",
#ifdef WS_RECORD
	"store_record",
#endif
	"store_rlcv_timestamp",
	"hbm_dax_init",
	"hbm_dax_fini",
	"hbm_drv_mgmt_init",
	"hbm_drv_mgmt_fini",
	"get_ih_rb_info",
#ifndef EXCLUDE_DCORE_DEBUG
	"signal_reset_happened",
	"signal_diag_data_ready",
	"diag_data_collect_disabled",
	"signal_manual_dump_happened",
#endif
	"get_device_numa_node",
	"save_fb_sharing_mode",
	"save_accelerator_partition_mode",
	"save_memory_partition_mode",
	"save_cc_mode",
	"clear_conf_file",
	"schedule_work",
	"mb",
	"notify_shim_ext",
	"store_gfx_dump_data",
	"map_queue",
	"bh_init",
	"bh_queue",
	"bh_fini",
	"in_virtual_machine",
	"get_device_list",
	"init_vf_sysmem_xchg",
	"fini_vf_sysmem_xchg",
	"write_vf_sysmem_xchg",
	"read_vf_sysmem_xchg",
	"vf_sysmem_xchg_gpa_to_spa",
	"set_dma_mask",
#ifdef SHIM_LAYER_OSS_RADIX_TREE
	"radix_tree_init",
	"radix_tree_fini",
	"radix_tree_insert",
	"radix_tree_delete",
	"radix_tree_lookup",
	"radix_tree_gang_lookup_tag",
	"radix_tree_tag_set",
	"radix_tree_tag_clear",
	"radix_tree_iter_init",
	"radix_tree_next_chunk",
	"radix_tree_next_slot",
	"radix_tree_deref_slot",
	"radix_tree_delete_iter",
#endif
#ifdef SHIM_LAYER_OSS_KFIFO
	"kfifo_alloc",
	"kfifo_in_spinlocked_raw",
	"kfifo_out_spinlocked_raw",
	"kfifo_out_peek",
	"kfifo_len",
	"kfifo_free",
#endif
#ifdef AMDGV_UNIRAS_SUPPORT
	"init_waitqueue_head",
	"wait_event_interruptible_timeout",
	"msecs_to_jiffies",
#endif
#ifdef SHIM_LAYER_OSS_MEMPOOL
	"mempool_create_kmalloc_pool",
	"mempool_destroy",
	"mempool_alloc_preallocated",
	"mempool_free",
#endif
	"register_mce_notifier",
	"unregister_mce_notifier",
};

int AMDGV_API amdgv_init(struct oss_interface *funcs, uint16_t *dev_id_array, uint32_t flags)
{
	int i;
	int ret = 0;
	struct amdgv_device_ids *dev_entry;
	struct amdgv_device_ids dev_ids[AMDGV_MAX_SUPPORT_DEVICE_TYPE_NUM];

	ret = amdgv_init_ex(funcs, dev_ids, flags);
	if (ret)
		return ret;

	for (i = 0; i < ARRAY_SIZE(dev_ids); i++) {
		dev_entry = (struct amdgv_device_ids *)&dev_ids[i];
		dev_id_array[i] = dev_entry->dev_id;
	}

	return 0;
}

int AMDGV_API amdgv_init_ex(struct oss_interface *funcs, struct amdgv_device_ids *dev_ids,
			    uint32_t flags)
{
	amdgv_funcptr_t *p;
	int i, ret, length, inf_name_num;

	length = sizeof(struct oss_interface) / sizeof(amdgv_funcptr_t);
	inf_name_num = sizeof(amdgv_inf_name) / sizeof(char *);

	/* if print interface exists then we need output one warning
	 * to notify user if no interface implementation otherwise we
	 * need directly return error
	 */
	if (funcs->print == NULL)
		return AMDGV_FAILURE;

	amdgv_oss_funcs = funcs;
	/* check here to ensure the interface patch submitter to add
	 * interface into oss_interface and amdgv_inf_name together
	 */
	if (inf_name_num != length) {
		AMDGV_PRINT("error: oss interface num not match interface name num\n");
		return AMDGV_FAILURE;
	}

	/* For interface expansion we have to adopt warning message
	 * instead of return error otherwise we can't add interface
	 * anymore because CI test will always be failed
	 */
	for (p = (amdgv_funcptr_t *)funcs, i = 0; i < length; i++) {
		if (p[i] == NULL)
			AMDGV_PRINT("warn: no implementation for interface: %s\n",
				    amdgv_inf_name[i]);
	}

	amdgv_flags = flags;

	ret = amdgv_get_supported_dev_ids(dev_ids, AMDGV_MAX_SUPPORT_DEVICE_TYPE_NUM);
	if (ret) {
		amdgv_oss_funcs = NULL;
		amdgv_flags = 0;
		return ret;
	}

	if (MAJOR_VERSION == 0)
		AMDGV_PRINT("module loaded - libgv staging\n");
	else
		AMDGV_PRINT("module loaded - libgv %d.%d.%d\n", MAJOR_VERSION, MINOR_VERSION,
			    SUBMINOR_VERSION);

	amdgv_diag_data_cache_buf_init();

	return 0;
}

void AMDGV_API amdgv_fini(void)
{
	amdgv_diag_data_cache_buf_fini();

	amdgv_oss_funcs = NULL;

	amdgv_flags = 0;
}

void amdgv_get_version(int *major, int *minor)
{
	*major = MAJOR_VERSION;
	*minor = (MINOR_VERSION << 8) | SUBMINOR_VERSION;
}

static const char *libgv_build_date(void)
{
	/* "YYYY-MM-DD HH:MM:SS\0" = 20 bytes */
	static char date_buf[20];

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdate-time"
	const char *d = __DATE__;	/* "Mmm dd yyyy" */
	const char *t = __TIME__;	/* "hh:mm:ss" */
#pragma GCC diagnostic pop
	int month;

	switch (d[0]) {
	case 'J':
		month = (d[1] == 'a') ? 1 : (d[2] == 'n') ? 6 : 7;
		break;
	case 'F': month = 2; break;
	case 'M': month = (d[2] == 'r') ? 3 : 5; break;
	case 'A': month = (d[1] == 'p') ? 4 : 8; break;
	case 'S': month = 9; break;
	case 'O': month = 10; break;
	case 'N': month = 11; break;
	case 'D': month = 12; break;
	default:  month = 0; break;
	}

	date_buf[0]  = d[7];
	date_buf[1]  = d[8];
	date_buf[2]  = d[9];
	date_buf[3]  = d[10];
	date_buf[4]  = '-';
	date_buf[5]  = '0' + (month / 10);
	date_buf[6]  = '0' + (month % 10);
	date_buf[7]  = '-';
	date_buf[8]  = (d[4] == ' ') ? '0' : d[4];
	date_buf[9]  = d[5];
	date_buf[10] = ' ';
	date_buf[11] = t[0];
	date_buf[12] = t[1];
	date_buf[13] = ':';
	date_buf[14] = t[3];
	date_buf[15] = t[4];
	date_buf[16] = ':';
	date_buf[17] = t[6];
	date_buf[18] = t[7];
	date_buf[19] = '\0';

	return date_buf;
}

void amdgv_get_date(char *str)
{
	int len_min;
	int date_len;
	const char *build_date = libgv_build_date();

	date_len = oss_strlen(build_date);
	len_min = date_len < (STRLEN_NORMAL - 1) ? date_len : (STRLEN_NORMAL - 1);

	oss_memcpy(str, build_date, len_min);
	str[len_min] = 0;
}

uint32_t AMDGV_API amdgv_get_asic_type(uint32_t device_id, uint32_t revision_id)
{
	return amdgv_internal_get_asic_type(device_id, revision_id);
}

amdgv_dev_t AMDGV_API amdgv_device_init(struct amdgv_init_data *init_data)
{
	struct amdgv_adapter *adapt;

	if (amdgv_oss_funcs == NULL)
		return AMDGV_INVALID_HANDLE;

	if (init_data == NULL) {
		AMDGV_PRINT("Error: invalid init data\n");
		return AMDGV_INVALID_HANDLE;
	}

	adapt = amdgv_device_internal_init(init_data);
	if (adapt == NULL)
		return AMDGV_INVALID_HANDLE;

	return (amdgv_dev_t)adapt;
}

void AMDGV_API amdgv_adapt_list_update(amdgv_dev_t dev, amdgv_dev_t *adapt_list)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;

	if (dev == NULL || adapt_list == NULL)
		return;

	adapt->adapt_list = (struct amdgv_adapter **)adapt_list;
}

void AMDGV_API amdgv_device_fini_ex(amdgv_dev_t dev, struct amdgv_fini_config_opt *fini_opt)
{
	if (dev == AMDGV_INVALID_HANDLE)
		return;

	amdgv_device_internal_fini((struct amdgv_adapter *)dev, fini_opt);
}

void AMDGV_API amdgv_device_fini(amdgv_dev_t dev)
{
	struct amdgv_fini_config_opt fini_opt = { 0 };

	if (dev == AMDGV_INVALID_HANDLE)
		return;

	amdgv_device_fini_ex(dev, &fini_opt);
}

enum amdgv_dev_status AMDGV_API amdgv_get_dev_status(amdgv_dev_t dev)
{
	struct amdgv_adapter *adapt;

	if (dev == AMDGV_INVALID_HANDLE)
		return AMDGV_STATUS_HW_LOST;

	adapt = (struct amdgv_adapter *)dev;
	return adapt->status;
}

int AMDGV_API amdgv_is_customized_vf_mode(amdgv_dev_t dev)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;
	bool ret;

	oss_mutex_lock(adapt->api_lock);

	ret = adapt->customized_vf_config_mode;

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int AMDGV_API amdgv_allocate_vf(amdgv_dev_t dev, struct amdgv_vf_option *option)
{
	int ret;
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	data.gpumon_data.type = GPUMON_ALLOC_VF;
	data.gpumon_data.ptr = option;
	data.gpumon_data.result = &event_ret;

	/* Allocation requires atomic operation */
	oss_mutex_lock(adapt->api_lock);

	ret = amdgv_sched_queue_event_and_wait_ex(
		adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_GPUMON, AMDGV_SCHED_BLOCK_ALL, data);
	if (!ret)
		ret = event_ret;

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int AMDGV_API amdgv_free_vf(amdgv_dev_t dev, uint32_t idx_vf)
{
	int ret;
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (idx_vf >= adapt->num_vf)
		return AMDGV_LOG_GPUMON_INVALID_VF_INDEX;

	oss_mutex_lock(adapt->api_lock);

	data.gpumon_data.type = GPUMON_FREE_VF;
	data.gpumon_data.val = idx_vf;
	data.gpumon_data.result = &event_ret;

	ret = amdgv_sched_queue_event_and_wait_ex(
		adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_GPUMON, AMDGV_SCHED_BLOCK_ALL, data);
	if (!ret)
		ret = event_ret;

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int AMDGV_API amdgv_set_vf(amdgv_dev_t dev, enum amdgv_set_vf_opt_type opt_type,
			   struct amdgv_vf_option *opt)
{
	int ret;
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	enum amdgv_set_vf_opt_type type = opt_type;

	bool valid_option = amdgv_vf_option_valid(dev, type, opt) == 0;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!valid_option)
		return AMDGV_FAILURE;

	amdgv_vf_get_option_type(adapt, &type, opt);

	/* extra restrictions of memory related setting */
	if (type & AMDGV_SET_VF_FB) {
		if (opt->idx_vf == AMDGV_PF_IDX)
			return AMDGV_LOG_GPUMON_VF_BUSY;
	}

	data.gpumon_data.type = GPUMON_SET_VF;
	data.gpumon_data.vf.opt_type = type;
	data.gpumon_data.vf.opt = opt;
	data.gpumon_data.result = &event_ret;

	oss_mutex_lock(adapt->api_lock);

	ret = amdgv_sched_queue_event_and_wait_ex(
		adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_GPUMON, AMDGV_SCHED_BLOCK_ALL, data);
	if (!ret)
		ret = event_ret;

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int AMDGV_API amdgv_flr_vf(amdgv_dev_t dev, uint32_t idx_vf)
{
	int ret;
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	ret = AMDGV_FAILURE;
	oss_mutex_lock(adapt->api_lock);

	if (idx_vf == AMDGV_PF_IDX && !(adapt->flags & AMDGV_FLAG_USE_PF)) {
		ret = AMDGV_LOG_GPUMON_INVALID_VF_INDEX;
		goto unlock;
	}

	if (!AMDGV_IS_IDX_INVALID(idx_vf))
		ret = amdgv_sched_queue_flr_vf(adapt, idx_vf);

unlock:
	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int AMDGV_API amdgv_stop_vf(amdgv_dev_t dev, uint32_t idx_vf)
{
	int ret;
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	ret = AMDGV_FAILURE;
	oss_mutex_lock(adapt->api_lock);

	if (!AMDGV_IS_IDX_INVALID(idx_vf))
		ret = amdgv_sched_queue_stop_vf(adapt, idx_vf);

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int AMDGV_API amdgv_force_reset_gpu(amdgv_dev_t dev)
{
	int ret;
	struct amdgv_adapter *adapt;

	/* This API is for testing purposes only.
	 * Allow attempts to reset even if the GPU is in a bad state
	 * to stress test the driver resiliance. */
	SET_ADAPT_AND_CHECK_STATUS_MINIMAL(adapt, dev);

	oss_mutex_lock(adapt->api_lock);

	ret = amdgv_sched_queue_force_reset_gpu(adapt);

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int AMDGV_API amdgv_suspend_vf(amdgv_dev_t dev, uint32_t idx_vf)
{
	int ret;
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	ret = AMDGV_FAILURE;
	oss_mutex_lock(adapt->api_lock);

	if (idx_vf == AMDGV_PF_IDX && !(adapt->flags & AMDGV_FLAG_USE_PF)) {
		ret = AMDGV_LOG_GPUMON_INVALID_VF_INDEX;
		goto unlock;
	}

	if (!AMDGV_IS_IDX_INVALID(idx_vf))
		ret = amdgv_sched_queue_suspend_vf(adapt, idx_vf);

unlock:
	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int AMDGV_API amdgv_resume_vf(amdgv_dev_t dev, uint32_t idx_vf)
{
	int ret;
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);

	ret = AMDGV_FAILURE;

	if (idx_vf == AMDGV_PF_IDX && !(adapt->flags & AMDGV_FLAG_USE_PF)) {
		ret = AMDGV_LOG_GPUMON_INVALID_VF_INDEX;
		goto unlock;
	}

	if (!AMDGV_IS_IDX_INVALID(idx_vf))
		ret = amdgv_sched_queue_resume_vf(adapt, idx_vf);

unlock:
	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int AMDGV_API amdgv_set_vf_number(amdgv_dev_t dev, uint32_t num_vf)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int event_ret = 0, ret;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	data.gpumon_data.type = GPUMON_SET_VF_NUM;
	data.gpumon_data.val = num_vf;
	data.gpumon_data.result = &event_ret;

	oss_mutex_lock(adapt->api_lock);

	ret = amdgv_sched_queue_event_and_wait_ex(
		adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_GPUMON, AMDGV_SCHED_BLOCK_ALL, data);
	if (!ret)
		ret = event_ret;

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int AMDGV_API amdgv_set_all_vf(amdgv_dev_t dev)
{
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);

	amdgv_vfmgr_set_all_vf(adapt);

	oss_mutex_unlock(adapt->api_lock);

	return 0;
}

int AMDGV_API amdgv_set_dump_cu_info(amdgv_dev_t dev, enum AMDGV_CU_DATA_TYPE cu_dump_type,
				     uint32_t xcc_id, bool use_extra_ring)
{
	struct amdgv_adapter *adapt;
	int ret;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->bp_lock);

	ret = amdgv_int_set_dump_cu_info(adapt, cu_dump_type, xcc_id, use_extra_ring);

	oss_mutex_unlock(adapt->bp_lock);

	return ret;
}

int AMDGV_API amdgv_alloc_dump_cu_resource_memory(amdgv_dev_t dev,
				struct amdgv_dump_cu_resource_size *dump_cu_resource_size, struct amdgv_dump_cu_resource_memory *output_data)
{
	struct amdgv_adapter *adapt;
	int ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);
	oss_mutex_lock(adapt->bp_lock);

	ret = amdgv_int_alloc_dump_cu_resource_memory(adapt, dump_cu_resource_size, output_data);

	oss_mutex_unlock(adapt->bp_lock);

	return ret;
}

int AMDGV_API amdgv_dump_cu_data(amdgv_dev_t dev, enum AMDGV_CU_DATA_TYPE type)
{
	struct amdgv_adapter *adapt;
	int ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->bp_lock);

	// Cannot use pushing event queue method. Since when bp mode works, event
	// will not be handled.
	ret = amdgv_int_dump_cu_data(adapt);

	oss_mutex_unlock(adapt->bp_lock);

	return ret;
}

int AMDGV_API amdgv_free_dump_cu_resource_memory(amdgv_dev_t dev)
{
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->bp_lock);

	amdgv_int_free_dump_cu_resource_memory(adapt);

	oss_mutex_unlock(adapt->bp_lock);

	return 0;
}

int AMDGV_API amdgv_notify_event(amdgv_dev_t dev, enum amdgv_notify_event event)
{
	int ret = 0;
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (in_whole_gpu_reset() && !amdgv_sched_is_unrecov_err(adapt)) {
		// For already in WGR, there must be WGR thread waiting to proceed.
		// Add a signal event to notify thread of PF reinit completion
		if (event == AMDGV_NOTIFY_EVENT_REL_GPU_INIT)
			oss_signal_event(adapt->reset.pf_rel_gpu_init);
		else if (event == AMDGV_NOTIFY_EVENT_REQ_GPU_RESET)
			AMDGV_INFO("PF in full access = %d\n", is_full_access_vf(AMDGV_PF_IDX));

		return ret;
	}

	oss_mutex_lock(adapt->api_lock);

	switch (event) {
	case AMDGV_NOTIFY_EVENT_REQ_GPU_INIT:
		/* This event does not require any handling because the PF
		 * is inited in amdgv_sched_world_context_init_pf_state
		 */
		break;

	case AMDGV_NOTIFY_EVENT_REL_GPU_INIT:
		amdgv_sched_queue_event_and_wait(adapt, AMDGV_PF_IDX, AMDGV_EVENT_REL_GPU_INIT,
						 AMDGV_SCHED_BLOCK_ALL);

		break;

	case AMDGV_NOTIFY_EVENT_REQ_GPU_FINI:
		amdgv_sched_queue_event_and_wait(adapt, AMDGV_PF_IDX, AMDGV_EVENT_REQ_GPU_FINI,
						 AMDGV_SCHED_BLOCK_ALL);
		break;

	case AMDGV_NOTIFY_EVENT_REL_GPU_FINI:
		amdgv_sched_queue_event_and_wait(adapt, AMDGV_PF_IDX, AMDGV_EVENT_REL_GPU_FINI,
						 AMDGV_SCHED_BLOCK_ALL);
		break;

	case AMDGV_NOTIFY_EVENT_REQ_GPU_RESET:
		amdgv_sched_queue_event_and_wait(
			adapt, AMDGV_PF_IDX, AMDGV_EVENT_REQ_GPU_RESET, AMDGV_SCHED_BLOCK_ALL);

		break;

	default:
		ret = AMDGV_FAILURE;
		break;
	}

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int AMDGV_API amdgv_get_mitigation_range(amdgv_dev_t dev, struct amdgv_reg_range *table,
					 uint32_t *length)
{
	int ret = 0;
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);

	if (table == NULL) {
		*length = adapt->num_miti;
		goto out;
	}

	if (*length < (uint32_t)adapt->num_miti) {
		ret = AMDGV_FAILURE;
	} else {
		oss_memcpy(table, adapt->miti_table,
			   sizeof(struct amdgv_reg_range) * adapt->num_miti);
		*length = adapt->num_miti;
	}

out:
	oss_mutex_unlock(adapt->api_lock);
	return ret;
}

int AMDGV_API amdgv_get_dev_info(amdgv_dev_t dev, enum amdgv_dev_info_type type,
				 union amdgv_dev_info *info)
{
	int ret = 0;
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);

	switch (type) {
	case AMDGV_GET_ENABLED_VF_NUM:
		if (adapt->status == AMDGV_STATUS_HW_INIT)
			info->vf.num_enabled_vf = adapt->num_vf;
		else
			info->vf.num_enabled_vf = 0;
		break;

	case AMDGV_GET_DEBUG_LEVEL:
		info->log.log_level = adapt->log_level;
		info->log.log_mask = adapt->log_mask;
		break;

	case AMDGV_GET_RESV_AREA:
		amdgv_gpuiov_get_resv_area(adapt, &info->map.offset, &info->map.size);
		break;

	case AMDGV_GET_FB_LAYOUT:
		amdgv_vfmgr_get_fb_layout(adapt, &info->layout);
		break;

	case AMDGV_GET_PSP_VBFLASH_SUPPORT:
		info->vbflash_support = adapt->psp.update_spirom ? true : false;
		break;

	case AMDGV_GET_XGMI_INFO:
		amdgv_vfmgr_get_xgmi_info(adapt, info);
		break;

	case AMDGV_GET_OAM_IDX:
		/*
		 * The OAM ID identifies the GPU's physical OAM slot on the
		 * baseboard and is read from SMUIO_MCM_CONFIG.SOCKET_ID.
		 * It is valid for any ASIC populated on an OAM-class board,
		 * including single-GPU configurations that are not part of an
		 * XGMI hive. ASICs without an OAM concept (e.g. consumer parts)
		 * do not register a get_socket_id callback and report -1.
		 */
		if (adapt->smuio.funcs && adapt->smuio.funcs->get_socket_id)
			info->oam.oam_idx =
				adapt->smuio.funcs->get_socket_id(adapt);
		else
			info->oam.oam_idx = -1;
		break;
	case AMDGV_GET_COMPUTE_PROFILE:
		ret = amdgv_gfx_get_compute_cap(adapt, true, &(info->compute_cap.min));
		if (!ret)
			ret = amdgv_gfx_get_compute_cap(adapt, false, &(info->compute_cap.max));
		break;
	case AMDGV_GET_BASIC_INFO:
		info->basic_info.vendor_id = adapt->vendor_id;
		info->basic_info.device_id = adapt->dev_id;
		info->basic_info.revision_id = adapt->rev_id;
		info->basic_info.sub_system_id = adapt->sub_sys_id;
		info->basic_info.sub_vendor_id = adapt->sub_vnd_id;
		info->basic_info.bdf = adapt->bdf;
		info->basic_info.dsn = adapt->serial;
		break;
	default:
		ret = AMDGV_FAILURE;
		break;
	}

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

static void amdgv_force_switch_idx_vf(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t i = 0;
	struct amdgv_sched_world_switch *world_switch = NULL;

	for (i = 0; i < adapt->sched.num_world_switch; ++i) {
		world_switch = &adapt->sched.world_switch[i];
		if (!world_switch->enabled)
			continue;
		world_switch->funcs->stop(adapt, world_switch);
	}

	amdgv_sched_context_load(adapt, idx_vf, AMDGV_SCHED_BLOCK_ALL);
}

int AMDGV_API amdgv_set_dev_conf(amdgv_dev_t dev, enum amdgv_dev_conf_type type,
				 union amdgv_dev_conf *conf)
{
	int ret = 0;
	struct amdgv_adapter *adapt;
#ifdef WS_RECORD
	enum amdgv_sched_mode gfx_sched_mode;
#endif

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);

	switch (type) {
	case AMDGV_CONF_LOG_LEVEL_MASK:
		adapt->log_level = conf->log.log_level;
		adapt->log_mask = conf->log.log_mask;
		break;

	case AMDGV_CONF_GUARD_FLAG:
		if (conf->flag_switch) {
			AMDGV_INFO("enable sensitive event guard\n");
			adapt->flags |= AMDGV_FLAG_SENSITIVE_EVENT_GUARD;
		} else {
			AMDGV_INFO("disable sensitive event guard\n");
			adapt->flags &= ~AMDGV_FLAG_SENSITIVE_EVENT_GUARD;
		}

		break;

	case AMDGV_CONF_FORCE_RESET_FLAG:
		if (conf->flag_switch == 0) {
			AMDGV_INFO("disable force reset\n");
			adapt->flags &= ~AMDGV_FLAG_VF_HANG_GPU_RESET;
		} else {
			adapt->flags |= AMDGV_FLAG_VF_HANG_GPU_RESET;
			AMDGV_INFO("SET RESET MODE to MODE1\n");
			adapt->reset.reset_mode = AMDGV_RESET_MODE1;
		}
		break;

	case AMDGV_CONF_HANG_DEBUG_FLAG:
		amdgv_debug_set_mode(adapt, conf->flag_switch & AMDGV_DEBUG_MODE_MASK);
		break;
#ifdef WS_RECORD
	case AMDGV_CONF_WS_RECORD:
		if (conf->flag_switch) {
			AMDGV_INFO("ws record enabled\n");
			adapt->flags |= AMDGV_FLAG_WS_RECORD;

			/* Signal event to wake record thread */
			if (adapt->sched.record_event != OSS_INVALID_HANDLE) {
				oss_signal_event(adapt->sched.record_event);
			}
		} else {
			AMDGV_INFO("ws record disabled\n");
			adapt->flags &= ~AMDGV_FLAG_WS_RECORD;
		}
		/* If in auto sched mode, ws_record must cowork with debug dump so try applying same operation on debug_dump */
		ret = amdgv_sched_get_sched_mode(adapt, AMDGV_SCHED_BLOCK_GFX, &gfx_sched_mode);
		if (!ret && gfx_sched_mode <= AMDGV_SCHED_MAX_HW_SCHED_MODE) {
			if (conf->flag_switch) {
				if (!adapt->opt.debug_dump_reserve_size) {
					AMDGV_INFO("No fb size reserved for debug dump when loading driver.\n");
					ret = AMDGV_FAILURE;
				} else {
					adapt->flags |= AMDGV_FLAG_DEBUG_DUMP_ENABLE;
				}
			} else {
				adapt->flags &= ~AMDGV_FLAG_DEBUG_DUMP_ENABLE;
			}
			if (!ret)
				ret = amdgv_sched_set_ws_log_op(adapt, AMDGV_SCHED_DEBUG_DUMP, !!conf->flag_switch);
		}

		break;
#endif
	case AMDGV_CONF_DISABLE_SELF_SWITCH_FLAG:
		if ((adapt->flags & AMDGV_FLAG_IPS_POWER_SAVING) && !conf->flag_switch) {
			AMDGV_WARN(
				"Failed to enable self switch, please disable power saving feature first\n");
			ret = AMDGV_FAILURE;
			break;
		}
		if (conf->flag_switch) {
			AMDGV_INFO("disable self switch\n");
			amdgv_sched_setup_self_switch(adapt, false);
			adapt->flags |= AMDGV_FLAG_DISABLE_SELF_SWITCH;
		} else {
			AMDGV_INFO("enable self switch\n");
			if (adapt->flags & AMDGV_FLAG_DISABLE_SELF_SWITCH) {
				amdgv_sched_setup_self_switch(adapt, true);
				adapt->flags &= ~AMDGV_FLAG_DISABLE_SELF_SWITCH;
			}
		}

		break;

	case AMDGV_CONF_FULL_ACCESS_TIMEOUT:
		if (conf->f_timeout <= 500000)
			adapt->sched.allow_time_full_access = conf->f_timeout * 1000;
		else
			AMDGV_WARN("timeout(%d) is bigger than 500000ms\n", conf->f_timeout);
		break;

	case AMDGV_CONF_ENABLE_CLEAR_VF_FB:
		if (conf->flag_switch) {
			AMDGV_INFO("enable clear_vf_fb\n");
			adapt->flags |= AMDGV_FLAG_ENABLE_CLEAR_VF_FB;
		} else {
			AMDGV_INFO("disable clear_vf_fb\n");
			adapt->flags &= ~AMDGV_FLAG_ENABLE_CLEAR_VF_FB;
		}

		break;

	case AMDGV_CONF_RESET_GPU:
		/* extract "sysfs"/smi_lib option for reset_mode */
		if (conf->flag_switch == 0) {
			AMDGV_INFO("clear whole_gpu_reset\n");
			adapt->reset.reset_mode = 0;
		} else {
			adapt->flags |= AMDGV_FLAG_VF_HANG_GPU_RESET;
			AMDGV_INFO("enable whole_gpu_reset(MODE1_RESET)\n");
			adapt->reset.reset_mode = AMDGV_RESET_MODE1;
		}
		break;

	case AMDGV_CONF_CMD_TIMEOUT:
		adapt->gpuiov.allow_time_cmd_complete = (uint64_t)conf->cmd_timeout * 1000;
		AMDGV_INFO("set iov cmd timeout to %lu\n",
			   adapt->gpuiov.allow_time_cmd_complete);
		break;

	case AMDGV_CONF_FORCE_SWITCH_VF_FLAG:
		if (AMDGV_IS_IDX_INVALID(conf->switch_vf_idx)) {
			AMDGV_ERROR("Invalid VF index\n");
			ret = AMDGV_FAILURE;
			break;
		}
		adapt->force_switch_vf_idx = conf->switch_vf_idx;
		AMDGV_INFO("set force switch vf to %llu\n", adapt->force_switch_vf_idx);
		amdgv_force_switch_idx_vf(adapt, adapt->force_switch_vf_idx);
		break;

	case AMDGV_CONF_DISABLE_MMIO_PROTECTION:
		if (conf->flag_switch) {
			AMDGV_INFO("disable vf mmio protection\n");
			adapt->flags |= AMDGV_FLAG_DISABLE_MMIO_PROTECTION;
		} else {
			AMDGV_INFO("enable vf mmio protection\n");
			adapt->flags &= ~AMDGV_FLAG_DISABLE_MMIO_PROTECTION;
		}

		break;

	case AMDGV_CONF_DISABLE_PSP_VF_GATE:
		if (conf->flag_switch) {
			AMDGV_INFO("disable psp vf gate\n");
			adapt->flags |= AMDGV_FLAG_DISABLE_PSP_VF_GATE;
		} else {
			AMDGV_INFO("enable psp vf gate\n");
			adapt->flags &= ~AMDGV_FLAG_DISABLE_PSP_VF_GATE;
		}

		break;

	case AMDGV_CONF_HYBRID_LIQUID_VF_MIN_TS:
		ret = amdgv_sched_set_hliquid_min_ts(adapt, conf->hliquid_min_ts);
		break;

#ifndef EXCLUDE_DCORE_DEBUG
	case AMDGV_CONF_DISABLE_DCORE_DEBUG:
		if (conf->flag_switch) {
			AMDGV_INFO("disable vf dcore debug\n");
			adapt->flags |= AMDGV_FLAG_DISABLE_DCORE_DEBUG;
		} else {
			AMDGV_INFO("enable vf dcore debug\n");
			adapt->flags &= ~AMDGV_FLAG_DISABLE_DCORE_DEBUG;
		}

		break;
#endif

	case AMDGV_CONF_GPUV_LIVE_UPDATE:
		if (conf->flag_switch) {
			AMDGV_INFO("enable GPUV live update related functionality\n");
			adapt->flags |= AMDGV_FLAG_GPUV_LIVE_UPDATE;
		} else {
			AMDGV_INFO("disable GPUV live update related functionality\n");
			adapt->flags &= ~AMDGV_FLAG_GPUV_LIVE_UPDATE;
		}

		break;

	case AMDGV_CONF_SKIP_PAGE_RETIREMENT:
		if (conf->flag_switch) {
			AMDGV_INFO("skip bad page retirement for data poison!\n");
			adapt->flags |= AMDGV_FLAG_SKIP_BAD_PAGE_RETIREMENT;
		} else {
			AMDGV_INFO("enable bad page retirement for data poison!\n");
			adapt->flags &= ~AMDGV_FLAG_SKIP_BAD_PAGE_RETIREMENT;
		}

		break;

	case AMDGV_CONF_PERF_LOG_FLAG:
		if (conf->flag_switch) {
			AMDGV_INFO("enable perf log in rlcv auto scheduler\n");
			if (adapt->flags & AMDGV_FLAG_PERF_LOG_ENABLE)
				ret = AMDGV_ALREADY_SET;
			else
				adapt->flags |= AMDGV_FLAG_PERF_LOG_ENABLE;
		} else {
			AMDGV_INFO("disable perf log in rlcv auto scheduler\n");
			if (adapt->flags & AMDGV_FLAG_PERF_LOG_ENABLE)
				adapt->flags &= ~AMDGV_FLAG_PERF_LOG_ENABLE;
			else
				ret = AMDGV_ALREADY_SET;
		}
		if (!ret)
			ret = amdgv_sched_set_ws_log_op(adapt, AMDGV_SCHED_PERF_LOG, !!conf->flag_switch);
		break;

	case AMDGV_CONF_DEBUG_DUMP_FLAG:
		if (conf->flag_switch) {
			AMDGV_INFO("enable debug dump in rlcv auto scheduler\n");
			if (!adapt->opt.debug_dump_reserve_size) {
				AMDGV_INFO("No fb size reserved for debug dump when loading driver.\n");
				ret = AMDGV_FAILURE;
			} else if (adapt->flags & AMDGV_FLAG_DEBUG_DUMP_ENABLE) {
				ret = AMDGV_ALREADY_SET;
			} else {
				adapt->flags |= AMDGV_FLAG_DEBUG_DUMP_ENABLE;
			}
		} else {
			AMDGV_INFO("disable debug dump in rlcv auto scheduler\n");
			if (adapt->flags & AMDGV_FLAG_DEBUG_DUMP_ENABLE)
				adapt->flags &= ~AMDGV_FLAG_DEBUG_DUMP_ENABLE;
			else
				ret = AMDGV_ALREADY_SET;
		}
		if (!ret)
			ret = amdgv_sched_set_ws_log_op(adapt, AMDGV_SCHED_DEBUG_DUMP, !!conf->flag_switch);
		break;
	case AMDGV_CONF_ASYMMETRIC_TIMESLICE_FLAG:
		if (conf->asymmetric.reset) {
			AMDGV_INFO("reset all VF to default timeslice\n");
			amdgv_sched_setup_default_vfs_timeslice(adapt);
			conf->asymmetric.reset = false;
		} else {
			AMDGV_INFO("setup VF%d for timeslice %d us\n", conf->asymmetric.vf_idx, conf->asymmetric.vf_timeslice);
			if (!adapt->sched.setup_vf_timeslice) {
				AMDGV_ERROR("Not Support - Asic does not have the feature!\n");
				ret = AMDGV_FAILURE;
			} else {
				if ((adapt->num_vf == 1) && (!(adapt->flags & AMDGV_FLAG_USE_PF))) {
					AMDGV_ERROR("Not Support - oneVF and PF is unused!\n");
					ret = AMDGV_FAILURE;
				} else
					ret = amdgv_sched_set_time_slice(adapt, conf->asymmetric.vf_idx, conf->asymmetric.vf_timeslice, AMDGV_SCHED_BLOCK_GFX);
			}
		}
		break;
	case AMDGV_CONF_HANG_DETECTION_THRESHOLD:
		if (adapt->gfx.hang_detection_supported)
			adapt->gfx.hang_detection_threshold_us = conf->u32val;
		else
			ret = AMDGV_FAILURE;
		break;

	case AMDGV_CONF_HANG_DETECTION_DURATION:
		if (adapt->gfx.hang_detection_supported)
			adapt->gfx.hang_detection_duration_us = conf->u32val;
		else
			ret = AMDGV_FAILURE;
		break;

	case AMDGV_CONF_ASYMMETRIC_FB:
		if (conf->asymmetric.defragment) {
			amdgv_vfmgr_vf_fb_defragment(adapt);
			conf->asymmetric.defragment = false;
		} else {
			AMDGV_INFO("setup VF%d FB to %dMB\n", conf->asymmetric.vf_idx, conf->asymmetric.vf_fb_size);
			ret = amdgv_vfmgr_vf_fb_resize(adapt, conf->asymmetric.vf_idx, conf->asymmetric.vf_fb_size);
		}
		break;
	case AMDGV_CONF_ERROR_DUMP_STACK_MAX:
		adapt->log.dump_stack_max = conf->error_dump_stack_max;
		break;
	case AMDGV_CONF_ERROR_DUMP_STACK_FILTER:
		amdgv_log_dump_stack_filter_set(adapt,
			conf->error_dump_stack_entry, conf->error_dump_stack_add);
		break;

	default:
		ret = AMDGV_FAILURE;
		break;
	}

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int AMDGV_API amdgv_get_dev_conf(amdgv_dev_t dev, enum amdgv_dev_conf_type type,
				 union amdgv_dev_conf *conf)
{
	int ret = 0;
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;

	oss_mutex_lock(adapt->api_lock);

	switch (type) {
	case AMDGV_CONF_LOG_LEVEL_MASK:
		conf->log.log_level = adapt->log_level;
		conf->log.log_mask = adapt->log_mask;
		break;

	case AMDGV_CONF_GUARD_FLAG:
		if (adapt->flags & AMDGV_FLAG_SENSITIVE_EVENT_GUARD)
			conf->flag_switch = 1;
		else
			conf->flag_switch = 0;
		break;

	case AMDGV_CONF_FORCE_RESET_FLAG:
		if (adapt->flags & AMDGV_FLAG_VF_HANG_GPU_RESET) {
				conf->flag_switch = 1;
		} else
			conf->flag_switch = 0;
		break;

	case AMDGV_CONF_HANG_DEBUG_FLAG:
		conf->flag_switch = adapt->debug.mode;
		break;

	case AMDGV_CONF_DISABLE_SELF_SWITCH_FLAG:
		if (adapt->flags & AMDGV_FLAG_DISABLE_SELF_SWITCH)
			conf->flag_switch = 1;
		else
			conf->flag_switch = 0;
		break;

	case AMDGV_CONF_FULL_ACCESS_TIMEOUT:
		conf->f_timeout = adapt->sched.allow_time_full_access / 1000;
		break;

	case AMDGV_CONF_ENABLE_CLEAR_VF_FB:
		if (adapt->flags & AMDGV_FLAG_ENABLE_CLEAR_VF_FB)
			conf->flag_switch = 1;
		else
			conf->flag_switch = 0;
		break;

	case AMDGV_CONF_RESET_GPU:
		if (adapt->reset.reset_mode == 0)
			conf->flag_switch = 0;
		else
			conf->flag_switch = 1;
		break;

	case AMDGV_CONF_FORCE_SWITCH_VF_FLAG:
		conf->switch_vf_idx = adapt->force_switch_vf_idx;
		break;

	case AMDGV_CONF_CMD_TIMEOUT:
		conf->cmd_timeout = (uint32_t)(adapt->gpuiov.allow_time_cmd_complete / 1000);
		break;

	case AMDGV_CONF_SKIP_PAGE_RETIREMENT:
		if (adapt->flags & AMDGV_FLAG_SKIP_BAD_PAGE_RETIREMENT)
			conf->flag_switch = 1;
		else
			conf->flag_switch = 0;
		break;

	case AMDGV_CONF_DISABLE_MMIO_PROTECTION:
		if (adapt->flags & AMDGV_FLAG_DISABLE_MMIO_PROTECTION)
			conf->flag_switch = 1;
		else
			conf->flag_switch = 0;
		break;

	case AMDGV_CONF_DISABLE_PSP_VF_GATE:
		if (adapt->flags & AMDGV_FLAG_DISABLE_PSP_VF_GATE)
			conf->flag_switch = 1;
		else
			conf->flag_switch = 0;
		break;
#ifdef WS_RECORD
	case AMDGV_CONF_WS_RECORD:
		if (adapt->flags & AMDGV_FLAG_WS_RECORD)
			conf->flag_switch = 1;
		else
			conf->flag_switch = 0;
		break;
#endif
	case AMDGV_CONF_HYBRID_LIQUID_VF_MIN_TS:
		conf->hliquid_min_ts = amdgv_sched_get_hliquid_min_ts(adapt);
		break;

#ifndef EXCLUDE_DCORE_DEBUG
	case AMDGV_CONF_DISABLE_DCORE_DEBUG:
		if (adapt->flags & AMDGV_FLAG_DISABLE_DCORE_DEBUG)
			conf->flag_switch = 1;
		else
			conf->flag_switch = 0;
		break;
#endif
	case AMDGV_CONF_HANG_DETECTION_THRESHOLD:
		if (adapt->gfx.hang_detection_supported)
			conf->u32val = adapt->gfx.hang_detection_threshold_us;
		else
			ret = AMDGV_FAILURE;
		break;
	case AMDGV_CONF_HANG_DETECTION_DURATION:
		if (adapt->gfx.hang_detection_supported)
			conf->u32val = adapt->gfx.hang_detection_duration_us;
		else
			ret = AMDGV_FAILURE;
		break;
	case AMDGV_CONF_ERROR_DUMP_STACK_MAX:
		conf->error_dump_stack_max = adapt->log.dump_stack_max;
		break;
	default:
		ret = AMDGV_FAILURE;
		break;
	}

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int AMDGV_API amdgv_get_vf_option(amdgv_dev_t dev, uint32_t idx_vf,
				  struct amdgv_vf_option *option)
{
	int ret = 0;
	struct amdgv_adapter *adapt;
	struct amdgv_vf_device *vf_dev;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);

	if (AMDGV_IS_IDX_INVALID(idx_vf)) {
		ret = AMDGV_FAILURE;
		goto out;
	}

	vf_dev = &adapt->array_vf[idx_vf];

	option->idx_vf = idx_vf;
	if (vf_dev->fb_size_tmr != 0) {
		option->fb_offset = vf_dev->fb_offset_tmr;
		option->fb_size = vf_dev->fb_size_tmr;
	} else {
		option->fb_offset = vf_dev->fb_offset;
		option->fb_size = vf_dev->fb_size;
	}
	option->gfx_time_slice = vf_dev->time_slice[AMDGV_SCHED_BLOCK_GFX];
	oss_memcpy(option->mm_bandwidth, vf_dev->mm_bandwidth,
		   sizeof(uint32_t) * AMDGV_MAX_MM_ENGINE);

out:
	oss_mutex_unlock(adapt->api_lock);
	return ret;
}

int AMDGV_API amdgv_get_vf_info(amdgv_dev_t dev, uint32_t idx_vf, enum amdgv_vf_info_type type,
				union amdgv_vf_info *info)
{
	int ret = 0;
	uint32_t hw_sched_id;
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);

	if (AMDGV_IS_IDX_INVALID(idx_vf)) {
		ret = AMDGV_FAILURE;
		goto out;
	}

	switch (type) {
	case AMDGV_GET_VF_BDF:
		ret = amdgv_vfmgr_get_vf_bdf(adapt, idx_vf, &info->id.bdf);
		break;

	case AMDGV_GET_VF_FB:
		ret = amdgv_vfmgr_get_vf_fb(adapt, idx_vf, &info->fb.fb_offset,
					    &info->fb.fb_size);
		break;
	case AMDGV_GET_VF_HBM_MGMT:
		ret = amdgv_vfmgr_get_vf_hbm_mgmt(adapt, idx_vf, &info->vf_hbm_mgmt);
		break;
	case AMDGV_GET_VF_SCHED_STATE:
		if (idx_vf == AMDGV_PF_IDX)
			info->sched.state = AMDGV_SCHED_AVAIL;
		else
			info->sched.state = amdgv_sched_get_vf_status(adapt, idx_vf);
		break;

	case AMDGV_GET_VF_TIME_LOG:
		//For legacy timelog, Find the first GFX HW sched entry and report it back to user
		for_each_id(hw_sched_id, amdgv_sched_get_hw_sched_mask_by_vf(adapt, idx_vf)) {
			if (adapt->gpuiov.ctrl_blocks[hw_sched_id].sched_block ==
			    AMDGV_SCHED_BLOCK_GFX) {
				/* hw scheduler perf log flush */
				if (adapt->gpuiov.ctrl_blocks[hw_sched_id].sched_mode <= AMDGV_SCHED_MAX_HW_SCHED_MODE &&
					 adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state == AMDGV_ENABLE_AUTO_HW_SWITCH) {
					amdgv_sched_toggle_perflog(adapt, false, hw_sched_id);
					amdgv_sched_toggle_perflog(adapt, true, hw_sched_id);
				}
				oss_memcpy(&(info->time_log),
					   &(adapt->array_vf[idx_vf].time_log[hw_sched_id]),
					   sizeof(struct amdgv_time_log));
				break;
			}
		}

		/* PF FLR + whole gpu reset */
		if (idx_vf == AMDGV_PF_IDX)
			info->time_log.reset_count += adapt->reset.reset_num;

		break;
	case AMDGV_GET_VF_FFBM_MAP_LIST:
		ret = amdgv_get_vf_fb_mapping_list(adapt, idx_vf, &info->vf_ffbm_map_list, false);
		break;
	case AMDGV_GET_VF_UNITID:
		ret = amdgv_vfmgr_get_vf_unitid(adapt, idx_vf, &info->unitid);
		break;
	default:
		ret = AMDGV_FAILURE;
		break;
	}

out:
	oss_mutex_unlock(adapt->api_lock);
	return ret;
}

int AMDGV_API amdgv_get_fb_regions_info(amdgv_dev_t dev, uint32_t idx_vf,
					struct amdgv_fb_regions *info)
{
	int ret = 0;
	struct amdgv_adapter *adapt;
	struct amdgv_memmgr_mem *csa_fb_mem;
	struct psp_local_memory *tmr_mem;
	uint64_t total_avail_fb;
	uint64_t fb_offset;
	uint64_t fb_real_size;
	uint32_t total_avail_fb_in_mb;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);

	if (AMDGV_IS_IDX_INVALID(idx_vf)) {
		ret = AMDGV_FAILURE;
		goto out;
	}

	amdgv_gpuiov_get_total_avail_fb_size(adapt, &total_avail_fb_in_mb);

	total_avail_fb = (uint64_t)total_avail_fb_in_mb;
	total_avail_fb = MBYTES_TO_BYTES(total_avail_fb);
	if (adapt->array_vf[idx_vf].fb_size_tmr > 0)
		fb_offset = MBYTES_TO_BYTES(adapt->array_vf[idx_vf].fb_offset_tmr);
	else
		fb_offset = MBYTES_TO_BYTES(adapt->array_vf[idx_vf].fb_offset);
	fb_real_size = MBYTES_TO_BYTES(adapt->array_vf[idx_vf].real_fb_size);

	csa_fb_mem = adapt->gpuiov.csa_fb_mem;
	if (csa_fb_mem == NULL) {
		info->csa.size = 0;
		info->csa.offset = 0;
	} else {
		info->csa.offset = amdgv_memmgr_get_gpu_addr(csa_fb_mem)
					- adapt->memmgr_pf.mc_base;
		info->csa.size = amdgv_memmgr_get_size(csa_fb_mem);
	}

	tmr_mem = &adapt->psp.tmr_context;
	if (tmr_mem == NULL || tmr_mem->mem == NULL) {
		info->tmr.size = 0;
		info->tmr.offset = 0;
	} else {
		info->tmr.offset = amdgv_memmgr_get_gpu_addr(tmr_mem->mem)
					- adapt->memmgr_pf.mc_base;
		info->tmr.size = amdgv_memmgr_get_size(tmr_mem->mem);
	}

	info->vf_dataexchange.size = KBYTES_TO_BYTES(GET_VF_TABLE_SIZE_KB_BY_ID(adapt, idx_vf, DATAEXCHANGE));
	info->vf_dataexchange.offset =
		fb_offset + GET_VF_TABLE_OFFSET_BY_ID(adapt, idx_vf, DATAEXCHANGE);
	info->vf_ipd.size = KBYTES_TO_BYTES(GET_VF_TABLE_SIZE_KB_BY_ID(adapt, idx_vf, IPD));
	info->vf_ipd.offset = fb_offset + GET_VF_TABLE_OFFSET_BY_ID(adapt, idx_vf, IPD);
	info->pf_dataexchange.size = KBYTES_TO_BYTES(AMD_SRIOV_MSG_DATAEXCHANGE_SIZE_KB_V1);
	info->pf_dataexchange.offset = KBYTES_TO_BYTES(AMD_SRIOV_MSG_DATAEXCHANGE_OFFSET_KB_V1);
	info->pf_ipd.size = AMDGV_IP_DISCOVERY_SIZE;
	info->pf_ipd.offset = total_avail_fb - AMDGV_IP_DISCOVERY_OFFSET;

out:
	oss_mutex_unlock(adapt->api_lock);
	return ret;
}

int amdgv_set_guard_config(amdgv_dev_t dev, uint32_t idx_vf, struct amdgv_guard_info *info)
{
	int ret;
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	ret = AMDGV_FAILURE;
	oss_mutex_lock(adapt->api_lock);
	if (!AMDGV_IS_IDX_INVALID(idx_vf))
		ret = amdgv_guard_set_vf_info(adapt, idx_vf, info);
	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int amdgv_get_guard_info(amdgv_dev_t dev, uint32_t idx_vf, struct amdgv_guard_info *info)
{
	int ret;
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	ret = AMDGV_FAILURE;
	oss_mutex_lock(adapt->api_lock);

	if (!AMDGV_IS_IDX_INVALID(idx_vf))
		ret = amdgv_guard_get_vf_info(adapt, idx_vf, info);

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}


static int default_threshold[AMDGV_GUARD_EVENT_MAX] = {
	[AMDGV_GUARD_EVENT_FLR] = AMDGV_DEFAULT_FLR_THRESHOLD,
	[AMDGV_GUARD_EVENT_WGR] = AMDGV_DEFAULT_WGR_THRESHOLD,
	[AMDGV_GUARD_EVENT_EXCLUSIVE_MOD] = AMDGV_DEFAULT_EXCLUSIVE_THRESHOLD,
	[AMDGV_GUARD_EVENT_EXCLUSIVE_TIMEOUT] = AMDGV_DEFAULT_EXCLUSIVE_TIMEOUT_THRESHOLD,
	[AMDGV_GUARD_EVENT_ALL_INT] = AMDGV_DEFAULT_INTERRUPT_THRESHOLD,
	[AMDGV_GUARD_EVENT_RAS_ERR_COUNT] = AMDGV_DEFAULT_RAS_TELEMETRY_THRESHOLD,
	[AMDGV_GUARD_EVENT_RAS_CPER_DUMP] = AMDGV_DEFAULT_RAS_TELEMETRY_THRESHOLD,
	[AMDGV_GUARD_EVENT_RAS_BAD_PAGES] = AMDGV_DEFAULT_RAS_TELEMETRY_THRESHOLD,
	[AMDGV_GUARD_EVENT_RAS_CHK_CRITI] = AMDGV_DEFAULT_RAS_TELEMETRY_THRESHOLD,
	[AMDGV_GUARD_EVENT_RAS_REMOTE_CMD] = AMDGV_DEFAULT_RAS_REMOTE_CMD_THRESHOLD,
};

static int default_interval[AMDGV_GUARD_EVENT_MAX] = {
	[AMDGV_GUARD_EVENT_FLR] = AMDGV_DEFAULT_FLR_INTERVAL,
	[AMDGV_GUARD_EVENT_WGR] = AMDGV_DEFAULT_WGR_INTERVAL,
	[AMDGV_GUARD_EVENT_EXCLUSIVE_MOD] = AMDGV_DEFAULT_EXCLUSIVE_INTERVAL,
	[AMDGV_GUARD_EVENT_EXCLUSIVE_TIMEOUT] = AMDGV_DEFAULT_EXCLUSIVE_TIMEOUT_INTERVAL,
	[AMDGV_GUARD_EVENT_ALL_INT] = AMDGV_DEFAULT_INTERRUPT_INTERVAL,
	[AMDGV_GUARD_EVENT_RAS_ERR_COUNT] = AMDGV_DEFAULT_RAS_TELEMETRY_INTERVAL,
	[AMDGV_GUARD_EVENT_RAS_CPER_DUMP] = AMDGV_DEFAULT_RAS_TELEMETRY_INTERVAL,
	[AMDGV_GUARD_EVENT_RAS_BAD_PAGES] = AMDGV_DEFAULT_RAS_TELEMETRY_INTERVAL,
	[AMDGV_GUARD_EVENT_RAS_CHK_CRITI] = AMDGV_DEFAULT_RAS_TELEMETRY_INTERVAL,
	[AMDGV_GUARD_EVENT_RAS_REMOTE_CMD] = AMDGV_DEFAULT_RAS_REMOTE_CMD_INTERVAL,
};

int amdgv_reset_guard_config(amdgv_dev_t dev, uint32_t idx_vf)
{
	struct amdgv_guard_info guard_info;
	int ret = 0;
	int i;


	for (i = 0; i < AMDGV_GUARD_EVENT_MAX; i++) {
		guard_info.type = i;
		guard_info.parm.event.active = 0;
		guard_info.parm.event.amount = 0;
		guard_info.parm.event.state = AMDGV_GUARD_EVENT_NORMAL;
		guard_info.parm.event.interval = default_interval[i];
		guard_info.parm.event.threshold = default_threshold[i];
		ret = amdgv_set_guard_config(dev, idx_vf, &guard_info);
		if (ret)
			return ret;
	}

	return ret;
}

void AMDGV_API amdgv_print_active_vfs_running_time(amdgv_dev_t dev)
{
	struct amdgv_adapter *adapt;

	adapt = (struct amdgv_adapter *)dev;

	oss_mutex_lock(adapt->api_lock);

	amdgv_sched_print_vfs_running_time(adapt);

	oss_mutex_unlock(adapt->api_lock);
}

uint32_t amdgv_get_vf_candidate(amdgv_dev_t dev)
{
	const struct amdgv_adapter *adapt;
	int vf_candidate = 0;
	uint32_t i = 0;

	if (dev == AMDGV_INVALID_HANDLE)
		return 0;

	adapt = (struct amdgv_adapter *)dev;
	if (adapt->status != AMDGV_STATUS_HW_INIT)
		return 0;

	for (i = 0; i < adapt->num_vf; i++) {
		if (is_active_vf(i))
			vf_candidate |= (1 << i);
	}

	return vf_candidate;
}

const char *amdgv_idx_to_str(uint32_t idx)
{
	if (idx < AMDGV_MAX_VF_SLOT)
		return amdgv_idx2str[idx];
	else
		return "Invalid Index";
}

int amdgv_register_notification_handler(amdgv_notification_handler cb)
{
	if (!cb)
		return AMDGV_FAILURE;

	/* callback already registered */
	if (notification_handler)
		return AMDGV_FAILURE;

	notification_handler = cb;

	return 0;
}

int amdgv_get_fw_version(amdgv_dev_t dev, union amdgv_smi_query_info *info)
{
	int i;
	struct amdgv_adapter *adapt;
	uint32_t valid_ucode_counter;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (adapt->psp.fw_info == NULL)
		return AMDGV_FAILURE;

	valid_ucode_counter = 0;
	for (i = 1; i < AMDGV_FIRMWARE_ID__MAX; i++) {
		bool support = adapt->psp.fw_id_support(i);
		if (support && adapt->psp.fw_info[i] != 0) {
			info->firmware_info.fw_info[valid_ucode_counter].id = i;
			info->firmware_info.fw_info[valid_ucode_counter].version = adapt->psp.fw_info[i];
			valid_ucode_counter++;
		}
	}

	info->firmware_info.fw_num = valid_ucode_counter;

	for (i = valid_ucode_counter; i < AMDGV_FIRMWARE_ID__MAX; i++) {
		info->firmware_info.fw_info[i].id = AMDGV_FIRMWARE_ID__MAX;
		info->firmware_info.fw_info[i].version = 0;
	}

	return 0;
}

int amdgv_get_asic(amdgv_dev_t dev, union amdgv_smi_query_info *info)
{
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);
	/* Just copy over the structure with all asic config */
	oss_memcpy(&info->asic_info.config, &adapt->config, sizeof(info->asic_info.config));

	return 0;
}

int amdgv_get_smi_info(amdgv_dev_t dev, enum amdgv_smi_query_type type,
		       union amdgv_smi_query_info *info)
{
	int ret = 0;
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;
	struct amdgv_vbios_info vbios_info;
	struct amdgv_vf_device *vf = NULL;
	uint32_t i = 0;
	int j = 0;
	// always able to return version and status
	if (type != AMDGV_SMI_LIBGV_VERSION && type != AMDGV_SMI_GET_STATUS)
		SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_memset(&vbios_info, 0, sizeof(vbios_info));
	switch (type) {
	case AMDGV_SMI_LIBGV_VERSION:
		amdgv_get_version((int *)&info->libgv_version.major,
				  (int *)&info->libgv_version.minor);
		break;

	case AMDGV_SMI_VBIOS:
		if (amdgv_gpumon_get_vbios_info(dev, &vbios_info))
			ret = AMDGV_FAILURE;

		info->vbios_info.version = vbios_info.version;

		oss_memcpy(info->vbios_info.vbios_pn, vbios_info.vbios_pn,
			   sizeof(vbios_info.vbios_pn));

		oss_memcpy(info->vbios_info.date, vbios_info.date, sizeof(vbios_info.date));
		oss_memcpy(info->vbios_info.vbios_version_string,
			   vbios_info.vbios_version_string,
			   sizeof(vbios_info.vbios_version_string));
		break;

	case AMDGV_SMI_FIRMWARE_VERSION:
		if (adapt->status != AMDGV_STATUS_HW_INIT) {
			ret = AMDGV_FAILURE;
			break;
		}

		if (info->firmware_info.vf_idx == AMDGV_PF_IDX) {
			if (adapt->psp.fw_info == NULL) {
				AMDGV_ERROR("Error: firmware info list is null\n");
				ret = AMDGV_FAILURE;
				break;
			}

			for (i = 1; i < AMDGV_FIRMWARE_ID__MAX; i++) {
				bool support = adapt->psp.fw_id_support(i);
				if (support) {
					info->firmware_info.fw_info[i - 1].version =
						adapt->psp.fw_info[i];
				} else {
					info->firmware_info.fw_info[i - 1].version = 0;
				}
				info->firmware_info.fw_info[i - 1].id = i;
			}

		} else {
			if (AMDGV_IS_IDX_INVALID(info->firmware_info.vf_idx)) {
				AMDGV_ERROR("Error: invalid VF index\n");
				ret = AMDGV_FAILURE;
				break;
			}

			vf = &adapt->array_vf[info->firmware_info.vf_idx];

			oss_memcpy(
				&info->firmware_info.fw_info, &vf->fw_info,
				(AMDGV_FIRMWARE_ID__MAX * sizeof(struct amdgv_firmware_info)));
		}

		info->firmware_info.fw_num = adapt->psp.fw_num - 1;
		break;

	case AMDGV_SMI_GPU_PERFORMANCE:
		if (adapt->status != AMDGV_STATUS_HW_INIT) {
			ret = AMDGV_FAILURE;
			break;
		}

		if (amdgv_gpumon_get_max_mclk(dev, (int *)&info->gpu_perf_info.max_mclk))
			ret = AMDGV_FAILURE;

		if (amdgv_gpumon_get_gpu_power_usage(dev, (int *)&info->gpu_perf_info.power_usage))
			ret = AMDGV_FAILURE;

		if (amdgv_gpumon_get_gpu_power_capacity(dev, 0,
							(int *)&info->gpu_perf_info.power_capacity))
			ret = AMDGV_FAILURE;

		if (amdgv_gpumon_get_asic_temperature(dev, (int *)&info->gpu_perf_info.temperature))
			ret = AMDGV_FAILURE;

		break;
	case AMDGV_SMI_ASIC:
		/* Just copy over the structure with all asic config */
		oss_memcpy(&info->asic_info.config, &adapt->config,
			   sizeof(info->asic_info.config));
		break;
	case AMDGV_SMI_GET_STATUS:
		info->status_info.status = amdgv_get_dev_status(dev);
		break;
	case AMDGV_SMI_VF_STATUS:
		for (i = 0; i < adapt->num_vf; i++)
			info->vf_status.status[i] = amdgv_sched_get_vf_status(adapt, i);

		break;
	case AMDGV_SMI_ECC_INFO:
		info->ecc_info.uncorrectable_error_count =
			amdgv_ecc_get_uncorrectable_error_count(adapt, AMDGV_PF_IDX);
		info->ecc_info.correctable_error_count =
			amdgv_ecc_get_correctable_error_count(adapt, AMDGV_PF_IDX);
		info->ecc_info.enabled = adapt->ecc.enabled;
		break;
	case AMDGV_SMI_PSP_MB_ERROR_RECORD:
		j = 0;

		for (i = 0; i < AMDGV_MAX_PSP_MB_ERROR_RECORD; i++) {
			if (adapt->error_record[i].valid) {
				oss_memcpy(&info->psp_mb_error_log.err_record[j],
					   &adapt->error_record[i],
					   sizeof(struct amdgv_psp_mb_err_record));
				j++;
			}
		}
		info->psp_mb_error_log.psp_mb_err_record_counter = j;
		break;
	case AMDGV_SMI_DFC_TABLE:
		if (adapt->psp.dfc_fw != NULL) {
			oss_memcpy(&info->dfc_fw, adapt->psp.dfc_fw, sizeof(struct dfc_fw));
		} else {
			AMDGV_ERROR("No DFC FW\n");
			ret = AMDGV_FAILURE;
		}
		break;
	default:
		AMDGV_ERROR("Invalid query type\n");
		ret = AMDGV_FAILURE;
		break;
	}

	return ret;
}

int AMDGV_API amdgv_vf_read_mmio(amdgv_dev_t dev, uint32_t idx_vf, void *buffer,
				 uint32_t offset, uint32_t length)
{
	int r = 0;
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;

	oss_mutex_lock(adapt->mmio_lock);

	if ((!AMDGV_IS_IDX_INVALID(idx_vf)) && (buffer != NULL)) {
		r = amdgv_vfmgr_read_mmio(adapt, idx_vf, buffer, offset, length);
	}

	oss_mutex_unlock(adapt->mmio_lock);

	return r;
}

int AMDGV_API amdgv_vf_write_mmio(amdgv_dev_t dev, uint32_t idx_vf, void *buffer,
				  uint32_t offset, uint32_t length)
{
	int r = 0;
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;

	oss_mutex_lock(adapt->mmio_lock);

	if ((!AMDGV_IS_IDX_INVALID(idx_vf)) && (buffer != NULL)) {
		r = amdgv_vfmgr_write_mmio(adapt, idx_vf, buffer, offset, length);
	}

	oss_mutex_unlock(adapt->mmio_lock);

	return r;
}

int amdgv_get_live_update_state(amdgv_dev_t dev, enum amdgv_live_update_state *state)
{
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (adapt->flags & AMDGV_FLAG_GPUV_LIVE_UPDATE) {
		if (!state) {
			AMDGV_ERROR("Live update state is NULL\n");
			return AMDGV_FAILURE;
		}

		oss_mutex_lock(adapt->api_lock);

		*state = adapt->live_update_state;

		AMDGV_INFO("Get live update state to %d.\n", *state);
		oss_mutex_unlock(adapt->api_lock);
	}

	return 0;
}

int amdgv_set_live_update_state(amdgv_dev_t dev, enum amdgv_live_update_state state)
{
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (adapt->flags & AMDGV_FLAG_GPUV_LIVE_UPDATE) {
		oss_mutex_lock(adapt->api_lock);

		AMDGV_INFO("Set live update state to %d.\n", state);
		adapt->live_update_state = state;

		if (state == AMDGV_LIVE_UPDATE_NONE) {
			adapt->opt.skip_hw_init = false;
			adapt->in_live_update = false;
		}

		oss_mutex_unlock(adapt->api_lock);
	}

	return 0;
}

int amdgv_export_live_info_size(amdgv_dev_t dev, uint32_t *size)
{
	int ret;
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);
	ret = amdgv_live_info_export_size(adapt, size);
	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int amdgv_export_live_info_data(amdgv_dev_t dev, uint32_t data_op, void *data,
				enum amdgv_live_info_status *status)
{
	int ret = AMDGV_FAILURE;
	struct amdgv_adapter *adapt;

	if (dev != NULL)
		adapt = (struct amdgv_adapter *)dev;
	else
		return AMDGV_LOG_GPU_DEVICE_LOST;

	oss_mutex_lock(adapt->api_lock);

	AMDGV_INFO("Exporting live info data with OP#%d.\n", data_op);
	if (data != NULL)
		ret = amdgv_live_info_export_data(adapt, data_op, data, status);
	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int amdgv_export_all_live_info_data(amdgv_dev_t dev, void *data)
{
	int ret = AMDGV_FAILURE;
	struct amdgv_adapter *adapt;

	if (dev != NULL)
		adapt = (struct amdgv_adapter *)dev;
	else
		return AMDGV_LOG_GPU_DEVICE_LOST;

	if (adapt->flags & AMDGV_FLAG_GPUV_LIVE_UPDATE) {
		oss_mutex_lock(adapt->api_lock);

		AMDGV_INFO("Exporting live info data.\n");
		adapt->sys_mem_info.va_ptr = data;

		if (data != NULL) {
			amdgv_live_info_init_metadata(adapt);
			ret = amdgv_export_data(adapt);
		}
		AMDGV_INFO ("Export Live Info Status: %d\n", ret);

		oss_mutex_unlock(adapt->api_lock);
	}

	return ret;
}

int amdgv_import_live_info_data(amdgv_dev_t dev, uint32_t data_op, void *data,
				enum amdgv_live_info_status *status)
{
	int ret = AMDGV_FAILURE;
	struct amdgv_adapter *adapt;

	if (dev != NULL)
		adapt = (struct amdgv_adapter *)dev;
	else
		return AMDGV_LOG_GPU_DEVICE_LOST;

	oss_mutex_lock(adapt->api_lock);
	AMDGV_INFO("Importing live info data with OP#%d.\n", data_op);
	if (data != NULL)
		ret = amdgv_live_info_import_data(adapt, data_op, data, status);
	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int amdgv_restore_ultralite_data(amdgv_dev_t dev)
{
	struct amdgv_adapter* adapt;

	if (dev != NULL)
		adapt = (struct amdgv_adapter*)dev;
	else
		return AMDGV_LOG_GPU_DEVICE_LOST;

	return amdgv_restore_ultralite_vf_data(adapt);
}

int amdgv_lock_sched(amdgv_dev_t dev)
{
	int ret;
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);

	AMDGV_INFO("Locking scheduler.\n");
	ret = amdgv_sched_queue_suspend(adapt);

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int amdgv_unlock_sched(amdgv_dev_t dev)
{
	int ret = AMDGV_FAILURE;
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);

	AMDGV_INFO("Unlocking scheduler: starting world switch.\n");
	ret = amdgv_sched_queue_resume(adapt);

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int amdgv_lock_sched_ex(amdgv_dev_t dev, struct amdgv_lock_sched_opt opt)
{
	int ret;
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);

	AMDGV_INFO("Locking scheduler.\n");
	ret = amdgv_sched_queue_suspend_ex(adapt, opt);

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int amdgv_unlock_sched_ex(amdgv_dev_t dev, struct amdgv_lock_sched_opt opt)
{
	int ret = AMDGV_FAILURE;
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);

	AMDGV_INFO("Unlocking scheduler: starting world switch.\n");
	ret = amdgv_sched_queue_resume_ex(adapt, opt);

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int amdgv_set_time_quanta_option(amdgv_dev_t dev, enum amdgv_sched_block sched_block,
				 uint32_t opt)
{
	int ret = 0;
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);

	ret = amdgv_sched_set_time_quanta_option(adapt, sched_block, opt);

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int amdgv_get_time_quanta_option(amdgv_dev_t dev, enum amdgv_sched_block sched_block,
				 uint32_t *opt)
{
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);

	amdgv_sched_get_time_quanta_option(adapt, sched_block, opt);

	oss_mutex_unlock(adapt->api_lock);

	return 0;
}

int amdgv_toggle_interrupt(amdgv_dev_t dev, bool enable)
{
	int ret;
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);

	if (enable)
		AMDGV_INFO("Enabling interrupts.\n");
	else
		AMDGV_INFO("Disable interrupts.\n");

	ret = amdgv_irqmgr_toggle_interrupt(adapt, enable);

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int amdgv_control_fru_sigout(amdgv_dev_t dev, const uint8_t *passphrase)
{
	int ret = AMDGV_FAILURE;
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);

	if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->control_fru_sigout)
		ret = amdgv_gpumon_control_fru_sigout(adapt, passphrase);

	oss_mutex_unlock(adapt->api_lock);

	return ret >= 0 ? 0 : AMDGV_FAILURE;
}

int amdgv_ras_trigger_error(amdgv_dev_t dev, struct amdgv_smi_ras_error_inject_info *data)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data event_data;
	int ret = AMDGV_FAILURE;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);

	event_data.gpumon_data.type = GPUMON_RAS_ERROR_INJECT;
	event_data.gpumon_data.ptr = data;
	event_data.gpumon_data.result = &event_ret;

	ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
		AMDGV_EVENT_SCHED_GPUMON,
		AMDGV_SCHED_BLOCK_ALL, event_data);
	if (!ret)
		ret = event_ret;

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int amdgv_ras_clean_correctable_error_count(amdgv_dev_t dev,  int *corr)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data event_data;
	int ret = AMDGV_FAILURE;
	int event_ret = 0;
	struct amdgv_smi_ras_query_if info = {0};

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (adapt->gpumon.funcs && adapt->gpumon.funcs->get_ecc_info &&
			adapt->gpumon.funcs->clean_correctable_error_count) {
		event_data.gpumon_data.type = GPUMON_CLEAN_CORRECTABLE_ERROR_COUNT;
		event_data.gpumon_data.ptr = &info;
		event_data.gpumon_data.result = &event_ret;

		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, event_data);
		if (!ret)
			ret = event_ret;
	}

	*corr = info.ce_count;

	return ret;
}

int amdgv_ras_ta_load(amdgv_dev_t dev, struct amdgv_smi_cmd_ras_ta_load *data)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data event_data;
	int ret = AMDGV_FAILURE;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);

	event_data.gpumon_data.type = GPUMON_RAS_TA_LOAD;
	event_data.gpumon_data.ptr = data;
	event_data.gpumon_data.result = &event_ret;

	ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
						AMDGV_EVENT_SCHED_GPUMON,
						AMDGV_SCHED_BLOCK_ALL, event_data);
	if (!ret)
		ret = event_ret;

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int amdgv_ras_ta_unload(amdgv_dev_t dev, struct amdgv_smi_cmd_ras_ta_unload *data)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data event_data;
	int ret = AMDGV_FAILURE;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);

	event_data.gpumon_data.type = GPUMON_RAS_TA_UNLOAD;
	event_data.gpumon_data.ptr = data;
	event_data.gpumon_data.result = &event_ret;

	ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
						AMDGV_EVENT_SCHED_GPUMON,
						AMDGV_SCHED_BLOCK_ALL, event_data);
	if (!ret)
		ret = event_ret;

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int amdgv_ras_eeprom_clear(amdgv_dev_t dev)
{
	int ret;
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);

	ret = amdgv_gpumon_ras_eeprom_clear(dev);

	if (ret == 0)
		ret = amdgv_gpumon_reset_all_error_counts(dev);

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

void amdgv_ras_get_bad_page_record_count(amdgv_dev_t dev, int *bp_cnt)
{
	struct amdgv_adapter *adapt;

	if (dev == AMDGV_INVALID_HANDLE)
		return;

	adapt = (struct amdgv_adapter *)dev;

	oss_mutex_lock(adapt->api_lock);

	amdgv_gpumon_get_bad_page_record_count(dev, bp_cnt);

	oss_mutex_unlock(adapt->api_lock);
}

int amdgv_ras_get_bad_page_info(amdgv_dev_t dev, uint32_t index,
					struct amdgv_smi_ras_eeprom_table_record *record)
{
	int ret;
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);

	ret = amdgv_gpumon_get_bad_page_info(dev, index, record);

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int amdgv_ras_get_ecc_block_info(amdgv_dev_t dev, struct amdgv_smi_ras_query_if *info)
{
	int ret;
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);

	ret = amdgv_gpumon_get_ecc_info(dev, info);

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int amdgv_get_vf2pf_info(amdgv_dev_t dev, uint32_t idx_vf,
			 struct amd_sriov_msg_vf2pf_info *vf2pf_info)
{
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;
	else if (is_unavail_vf(idx_vf))
		return AMDGV_FAILURE;
	else if (!is_active_vf(idx_vf))
		return 0;

	return amdgv_vfmgr_retrieve_vf2pf_message(adapt, idx_vf, vf2pf_info);
}

int amdgv_get_pf2vf_info(amdgv_dev_t dev, uint32_t idx_vf,
			 struct amd_sriov_msg_pf2vf_info *pf2vf_info)
{
	struct amdgv_adapter *adapt;
	int ret;
	uint32_t size = sizeof(struct amd_sriov_msg_pf2vf_info);

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;

	ret = amdgv_vfmgr_copy_from_vf_xchg_table(adapt, idx_vf,
						  AMD_SRIOV_MSG_DATAEXCHANGE_TABLE_ID,
						  0, pf2vf_info, size);
	if (ret)
		return AMDGV_FAILURE;

	return 0;
}

int amdgv_fw_live_update(amdgv_dev_t dev, enum amdgv_firmware_id fw_id)
{
	struct amdgv_adapter *adapt;
	enum amdgv_sched_event_id sched_event;
	int ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);

	switch (fw_id) {
	case AMDGV_FIRMWARE_ID__DFC_FW:
		sched_event = AMDGV_EVENT_SCHED_FW_LIVE_UPDATE_DFC;
		break;
	default:
		AMDGV_PRINT("FW %d live update unsupported\n", fw_id);
		ret = AMDGV_FAILURE;
		goto unlock;
	}

	ret = amdgv_sched_queue_event_and_wait(adapt, 0, sched_event, AMDGV_SCHED_BLOCK_ALL);

unlock:
	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int amdgv_toggle_mmio_access(amdgv_dev_t dev, uint32_t idx_vf, uint32_t vf_access_select,
			     bool enable)
{
	struct amdgv_adapter *adapt;
	int ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;

	ret = amdgv_gpuiov_set_vf_access(adapt, idx_vf, vf_access_select, enable);

	return ret;
}

int amdgv_toggle_psp_vf_gate(amdgv_dev_t dev, uint32_t vf_select, bool enable)
{
	struct amdgv_adapter *adapt;
	int ret = 0;
	union amdgv_sched_event_data data;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	data.psp_vf_gate_data.enable = enable;
	data.psp_vf_gate_data.vf_select = vf_select;

	oss_mutex_lock(adapt->api_lock);

	ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
						  AMDGV_EVENT_SCHED_PSP_VF_GATE,
						  AMDGV_SCHED_BLOCK_ALL, data);

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int amdgv_get_agp_info(amdgv_dev_t dev, void **buf)
{
	int ret;
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);

	ret = amdgv_misc_get_agp_cpu_base(adapt, buf);

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int amdgv_migration_get_dirty_page_size(amdgv_dev_t dev, uint32_t *dirty_page_size)
{
	struct amdgv_adapter *adapt;
	int ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);
	oss_mutex_lock(adapt->api_lock);
	ret = amdgv_dirtybit_get_dirty_page_size(adapt, dirty_page_size);
	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int amdgv_set_vf_migration_state(amdgv_dev_t dev, uint32_t idx_vf, enum amdgv_migration_vf_state state)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int result = 0;
	int ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;

	data.migration_state.state = state;
	data.migration_state.result = &result;

	oss_mutex_lock(adapt->api_lock);
	ret = amdgv_sched_queue_event_and_wait_ex(adapt, idx_vf,
					  AMDGV_EVENT_SET_VF_MIGRATION_STATE,
					  AMDGV_SCHED_BLOCK_ALL, data);

	oss_mutex_unlock(adapt->api_lock);

	if (ret == 0)
		ret = result;

	return ret;
}

static inline bool amdgv_buffer_do_check(uint64_t addr, uint64_t size,
					uint64_t aperture_start, uint64_t aperture_size)
{
	if (addr < aperture_start)
		return false;

	if (size > aperture_size)
		return false;

	if (addr - aperture_start > aperture_size - size)
		return false;

	return true;
}

static bool amdgv_buffer_check(struct amdgv_adapter *adapt, uint64_t gpu_addr, uint64_t size)
{
	if (size == 0)
		return false;

	if (amdgv_memmgr_get_cpu_addr(adapt->pdb0_mem)) {
		if (amdgv_buffer_do_check(gpu_addr, size, GART_START, adapt->gart_size))
			return true;
	}

	if (adapt->sys_mem_info.va_ptr) {
		if (amdgv_buffer_do_check(gpu_addr, size, adapt->mc_agp_loc_addr, AMDGV_AGP_APERTURE_SIZE))
			return true;
	}

	return false;
}

int amdgv_copy_migration_vf_fb(amdgv_dev_t dev, uint32_t idx_vf, uint32_t idx_fb_block,
			       void *buf, bool to_fb)
{
	int ret;
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data = {0};
	int event_ret = 0;
	uint64_t offset = 0;
	uint64_t block_size = AMDGV_MIGRATION_VF_FB_COPY_BLOCK_SIZE;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;

	if (!adapt->sys_mem_info.va_ptr)
		return AMDGV_FAILURE;

	offset = ((uint8_t *)buf) - ((uint8_t *)adapt->sys_mem_info.va_ptr);
	data.vf_fb_copy_data.vaddr = buf;
	data.vf_fb_copy_data.fb_offset = idx_fb_block * block_size;
	data.vf_fb_copy_data.gpu_addr = adapt->mc_agp_loc_addr + offset;
	data.vf_fb_copy_data.size = block_size;
	data.vf_fb_copy_data.to_fb = to_fb;
	data.vf_fb_copy_data.result = &event_ret;

	if (!amdgv_buffer_check(adapt, data.vf_fb_copy_data.gpu_addr,
					data.vf_fb_copy_data.size)) {
		AMDGV_ERROR("Invalid buffer address.\n");
		return AMDGV_FAILURE;
	}

	oss_mutex_lock(adapt->api_lock);
	ret = amdgv_sched_queue_event_and_wait_ex(adapt, idx_vf,
					  AMDGV_EVENT_VF_FB_COPY,
					  AMDGV_SCHED_BLOCK_ALL, data);

	if (!ret)
		ret = event_ret;

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int amdgv_vf_fb_copy(amdgv_dev_t dev, uint32_t idx_vf, uint64_t fb_offset,
			     uint64_t size, uint64_t gpu_addr, bool to_fb, void *vaddr)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data = {0};
	int event_ret = 0;
	int ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;

	data.vf_fb_copy_data.fb_offset = fb_offset;
	data.vf_fb_copy_data.size = size;
	data.vf_fb_copy_data.gpu_addr = gpu_addr;
	data.vf_fb_copy_data.to_fb = to_fb;
	data.vf_fb_copy_data.vaddr = vaddr;
	data.vf_fb_copy_data.result = &event_ret;

	if (!amdgv_buffer_check(adapt, data.vf_fb_copy_data.gpu_addr,
					data.vf_fb_copy_data.size)) {
		AMDGV_ERROR("Invalid buffer address.\n");
		return AMDGV_FAILURE;
	}

	oss_mutex_lock(adapt->api_lock);
	ret = amdgv_sched_queue_event_and_wait_ex(adapt, idx_vf,
					  AMDGV_EVENT_VF_FB_COPY,
					  AMDGV_SCHED_BLOCK_ALL, data);

	if (!ret)
		ret = event_ret;

	oss_mutex_unlock(adapt->api_lock);
	return ret;
}

int amdgv_vf_fb_copy_async(amdgv_dev_t dev, uint32_t idx_vf,
				 struct amdgv_fb_copy_entry *entries,
				 uint32_t num_entries, bool to_fb,
				 enum amdgv_buf_transfer_state *buf_transfer_state)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data = {0};
	uint32_t i;
	int ret;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;

	if (!buf_transfer_state || !entries || num_entries == 0)
		return AMDGV_FAILURE;

	if (AMDGV_IS_IDX_INVALID(idx_vf)) {
		*buf_transfer_state = AMDGV_BUF_TRANSFER_ERROR;
		return AMDGV_FAILURE;
	}

	for (i = 0; i < num_entries; i++) {
		if (!amdgv_buffer_check(adapt, entries[i].gpu_addr,
					entries[i].size)) {
			AMDGV_ERROR("Invalid buffer address for entry %u.\n", i);
			*buf_transfer_state = AMDGV_BUF_TRANSFER_ERROR;
			return AMDGV_FAILURE;
		}
	}

	data.vf_fb_copy_data.entries = entries;
	data.vf_fb_copy_data.num_entries = num_entries;
	data.vf_fb_copy_data.to_fb = to_fb;
	data.vf_fb_copy_data.buf_transfer_state = buf_transfer_state;
	*buf_transfer_state = AMDGV_BUF_TRANSFER_IN_PROGRESS;

	ret = amdgv_sched_queue_event_ex(adapt, idx_vf,
					 AMDGV_EVENT_VF_FB_COPY_ASYNC,
					 AMDGV_SCHED_BLOCK_ALL, data);

	if (ret) {
		*buf_transfer_state = AMDGV_BUF_TRANSFER_ERROR;
		return ret;
	}

	return 0;
}

static void amdgv_get_vf_identifier_v1(struct amdgv_adapter *adapt, uint32_t idx_vf, struct amdgv_vf_identifier *vf_id)
{
	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return;

	vf_id->version = 0;
	vf_id->v1_0.vf_fb_size_mb = adapt->array_vf[idx_vf].fb_size;
	vf_id->v1_0.gfx_timeslice_us = adapt->array_vf[idx_vf].time_slice[AMDGV_SCHED_BLOCK_GFX];
	vf_id->v1_0.mm_timeslice_us = 0;
	vf_id->v1_0.vcn_engine_bitmask = 0;
	vf_id->v1_0.jpeg_engine_bitmask = 0;
	vf_id->v1_0.partition_config = 0;

	AMDGV_DEBUG("migration ctx: fb size = %#x\n", vf_id->v1_0.vf_fb_size_mb);
	AMDGV_DEBUG("migration ctx: gfx_timeslice_us = %#x\n",
		    vf_id->v1_0.gfx_timeslice_us);
}

static void amdgv_get_vf_identifier_v2(struct amdgv_adapter *adapt, uint32_t idx_vf, struct amdgv_vf_identifier *vf_id)
{
	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return;

	vf_id->vf_index = idx_vf;
	vf_id->version = LIBGV_VF_VERSION;
	vf_id->v2_0.vf_fb_size_mb = adapt->array_vf[idx_vf].fb_size;
	vf_id->v2_0.partition_config = adapt->mcp.spatial_partition_mode;
	vf_id->v2_0.sdma_engine_bitmask = adapt->mcp.gfx.sdma_mask;
	vf_id->v2_0.hw_sched_engine_bitmask = amdgv_sched_get_hw_sched_mask_by_vf(adapt, idx_vf);
	vf_id->v2_0.timeslice_gfx = adapt->array_vf[idx_vf].time_slice[AMDGV_SCHED_BLOCK_GFX];
	vf_id->v2_0.timeslice_uvd = adapt->array_vf[idx_vf].time_slice[AMDGV_SCHED_BLOCK_UVD];
	vf_id->v2_0.timeslice_vce = adapt->array_vf[idx_vf].time_slice[AMDGV_SCHED_BLOCK_VCE];
	vf_id->v2_0.timeslice_uvd1 = adapt->array_vf[idx_vf].time_slice[AMDGV_SCHED_BLOCK_UVD1];
	vf_id->v2_0.timeslice_vcn = adapt->array_vf[idx_vf].time_slice[AMDGV_SCHED_BLOCK_VCN];
	vf_id->v2_0.timeslice_vcn1 = adapt->array_vf[idx_vf].time_slice[AMDGV_SCHED_BLOCK_VCN1];
	vf_id->v2_0.timeslice_jpeg = adapt->array_vf[idx_vf].time_slice[AMDGV_SCHED_BLOCK_JPEG];

	AMDGV_DEBUG("migration ctx: vf index = %d\n", vf_id->vf_index);
	AMDGV_DEBUG("migration ctx: vf version = %#x\n", vf_id->version);
	AMDGV_DEBUG("migration ctx: fb size = %#x\n", vf_id->v2_0.vf_fb_size_mb);
	AMDGV_DEBUG("migration ctx: partition config = %d\n", vf_id->v2_0.partition_config);
	AMDGV_DEBUG("migration ctx: sdma engine bitmask = %#x\n", vf_id->v2_0.sdma_engine_bitmask);
	AMDGV_DEBUG("migration ctx: hw sched engine bitmask = %#x\n", vf_id->v2_0.hw_sched_engine_bitmask);
	AMDGV_DEBUG("migration ctx: timeslice_gfx = %#x\n", vf_id->v2_0.timeslice_gfx);
	AMDGV_DEBUG("migration ctx: timeslice_uvd = %#x\n", vf_id->v2_0.timeslice_uvd);
	AMDGV_DEBUG("migration ctx: timeslice_vce = %#x\n", vf_id->v2_0.timeslice_vce);
	AMDGV_DEBUG("migration ctx: timeslice_uvd1 = %#x\n", vf_id->v2_0.timeslice_uvd1);
	AMDGV_DEBUG("migration ctx: timeslice_vcn = %#x\n", vf_id->v2_0.timeslice_vcn);
	AMDGV_DEBUG("migration ctx: timeslice_vcn1 = %#x\n", vf_id->v2_0.timeslice_vcn1);
	AMDGV_DEBUG("migration ctx: timeslice_jpeg = %#x\n", vf_id->v2_0.timeslice_jpeg);
}

int amdgv_get_migration_ctx(amdgv_dev_t dev, uint32_t idx_vf, struct amdgv_migration_ctx *ctx)
{
	int ret = 0;
	struct amdgv_adapter *adapt;
	struct amdgv_vbios_info vbios_info;
	int i;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!amdgv_is_migration_supported(dev)) {
		AMDGV_WARN("Live migration is not supported\n");
		return AMDGV_FAILURE;
	}

	oss_mutex_lock(adapt->api_lock);

	if (AMDGV_IS_IDX_INVALID(idx_vf)) {
		ret = AMDGV_FAILURE;
		goto out;
	}

	if (ctx) {
		uint32_t libgv_major_version, libgv_minor_version;

		/* 0. Device ID */
		ctx->gpu.device_id = (uint16_t)adapt->dev_id;
		AMDGV_DEBUG("migration ctx: device_id = %u\n", ctx->gpu.device_id);

		/* 1. Get context version */
		ctx->gpu.context_version = adapt->live_migration.context_version;
		AMDGV_DEBUG("migration ctx: context_version = %u\n", ctx->gpu.context_version);

		/* 2. Get libgv version */
		amdgv_get_version((int *)&libgv_major_version, (int *)&libgv_minor_version);
		ctx->gpu.libgv_version = libgv_major_version << 16 | libgv_minor_version;
		AMDGV_DEBUG("migration ctx: libgv_version = %u\n", ctx->gpu.libgv_version);

		/* 3. Get vbios version */
		oss_memset(&vbios_info, 0, sizeof(vbios_info));
		if (amdgv_gpumon_get_vbios_info(dev, &vbios_info)) {
			ret = AMDGV_FAILURE;
			goto out;
		}
		ctx->gpu.vbios_version = vbios_info.version;
		AMDGV_DEBUG("migration ctx: vbios_version = %u\n", ctx->gpu.vbios_version);

		/*4. Get migration version from PSP */
		ctx->gpu.migration_version = 0;
		if (amdgv_migration_get_migration_version(adapt,
							  &ctx->gpu.migration_version)) {
			ret = AMDGV_FAILURE;
			goto out;
		}
		AMDGV_DEBUG("migration ctx: migration_version = %u\n",
			    ctx->gpu.migration_version);

		/* 5. Get fw info */
		if (adapt->psp.fw_info == NULL) {
			AMDGV_ERROR("failed to get fw info\n");
			ret = AMDGV_FAILURE;
			goto out;
		}

		ctx->gpu.num_fw = adapt->psp.fw_num - 1;

		for (i = 1; i < AMDGV_FIRMWARE_ID__MAX; i++) {
			bool support = adapt->psp.fw_id_support(i);
			if (support) {
				ctx->gpu.fw[i - 1].version = adapt->psp.fw_info[i];
				AMDGV_DEBUG("migration ctx: fw%d's version = %u\n", i,
					    ctx->gpu.fw[i - 1].version);
			} else {
				ctx->gpu.fw[i - 1].version = 0;
			}
			ctx->gpu.fw[i - 1].id = i;
		}

		/* Ignore volatile firmware versions in the migration context. */
		ctx->gpu.fw[AMDGV_FIRMWARE_ID__RAS_TA - 1].version = 0;
		ctx->gpu.fw[AMDGV_FIRMWARE_ID__RLCV_LX7 - 1].version = 0;
		ctx->gpu.fw[AMDGV_FIRMWARE_ID__RLC_P - 1].version = 0;

		switch (adapt->live_migration.context_version) {
		case AMDGV_MIGRATION_CONTEXT_VERSION_V2:
			ctx->gpu.nps_mode = adapt->mcp.memory_partition_mode;
			ctx->gpu.cps_mode = adapt->mcp.accelerator_partition_mode;
			ctx->gpu.xgmi_state = amdgv_xgmi_node_fb_sharing_allowed(adapt);
			ctx->gpu.cu_num = adapt->config.gfx.active_cu_count;
			ctx->gpu.num_vf = adapt->num_vf;
			ctx->gpu.gpu_ordinate = adapt->xgmi.phy_node_id;

			AMDGV_DEBUG("migration ctx: nps mode = %d\n", ctx->gpu.nps_mode);
			AMDGV_DEBUG("migration ctx: cps mode = %d\n", ctx->gpu.cps_mode);
			AMDGV_DEBUG("migration ctx: xgmi state = %d\n", ctx->gpu.xgmi_state);
			AMDGV_DEBUG("migration ctx: cu num = %d\n", ctx->gpu.cu_num);
			AMDGV_DEBUG("migration ctx: num vf = %d\n", ctx->gpu.num_vf);
			AMDGV_DEBUG("migration ctx: gpu ordinate = %d\n", ctx->gpu.gpu_ordinate);

			amdgv_get_vf_identifier_v2(adapt, idx_vf, &ctx->vf);
			break;
		default:
			amdgv_get_vf_identifier_v1(adapt, idx_vf, &ctx->vf);
			break;
		}
	}

out:
	oss_mutex_unlock(adapt->api_lock);
	return ret;
}

bool amdgv_is_migration_supported(amdgv_dev_t dev)
{
	struct amdgv_adapter *adapt;
	bool ret;

	if (dev == AMDGV_INVALID_HANDLE)
		return false;

	adapt = (struct amdgv_adapter *)dev;
	if (adapt->status != AMDGV_STATUS_HW_INIT)
		return false;

	oss_mutex_lock(adapt->api_lock);
	ret = (adapt->flags & AMDGV_FLAG_GPUV_LIVE_MIGRATION) &&
		(adapt->live_migration.migration_version != AMDGV_MIGRATION_VERSION_UNINITIALIZED);
	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

bool amdgv_migration_pre_copy_supported(amdgv_dev_t dev)
{
	struct amdgv_adapter *adapt;

	if (dev == AMDGV_INVALID_HANDLE)
		return false;

	adapt = (struct amdgv_adapter *)dev;
	if (adapt->status != AMDGV_STATUS_HW_INIT)
		return false;

	if (adapt->dirtybit.fb_hash_support)
		return true;

	return !amdgv_xgmi_node_fb_sharing_allowed(adapt);
}

int amdgv_get_migration_data_size(amdgv_dev_t dev, uint32_t idx_vf, uint64_t *size,
				  enum amdgv_migration_data_section section)
{
	int ret = 0;
	struct amdgv_adapter *adapt;
	uint32_t fb_size, fb_offset = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;

	oss_mutex_lock(adapt->api_lock);

	switch (section) {
	case AMDGV_MIGRATION_CONTENT_VF_HW_STATIC_DATA:
	case AMDGV_MIGRATION_CONTENT_VF_HW_DYNAMIC_DATA:
		if (amdgv_migration_get_psp_data_size(adapt, size, section)) {
			ret = AMDGV_FAILURE;
			goto out;
		}
		break;

	case AMDGV_MIGRATION_CONTENT_VF_FB_DATA:
		if (amdgv_vfmgr_get_vf_fb(adapt, idx_vf, &fb_offset, &fb_size)) {
			ret = AMDGV_FAILURE;
			goto out;
		}

		*size = MBYTES_TO_BYTES((uint64_t)fb_size);
		break;

	default:
		*size = 0;
		ret = AMDGV_FAILURE;
	}

out:
	oss_mutex_unlock(adapt->api_lock);
	return ret;
}

static int amdgv_migration_transfer_manifest_data_event(amdgv_dev_t dev, uint32_t idx_vf, void *buf,
				uint64_t gpu_addr, enum amdgv_migration_manifest_data_type type,
				uint64_t size)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data = {0};
	int result = 0;
	int ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;

	oss_mutex_lock(adapt->api_lock);

	data.lm.type = type;
	data.lm.addr = (uint64_t)buf;
	data.lm.gpu_addr = gpu_addr;
	data.lm.result = &result;
	data.lm.size = size;
	ret = amdgv_sched_queue_event_and_wait_ex(adapt, idx_vf,
					AMDGV_EVENT_LIVE_MIGRATION_MANIFEST_DATA,
					AMDGV_SCHED_BLOCK_ALL, data);

	oss_mutex_unlock(adapt->api_lock);

	if (ret == 0)
		ret = result;

	return ret;
}

static enum amdgv_migration_manifest_data_type _amdgv_migration_export_phase_to_manifest_type_mapping(enum amdgv_migration_export_phase type)
{
	enum amdgv_migration_manifest_data_type manifest_data_type = AMDGV_MIGRATION_INVALID;

	switch (type) {
	case AMDGV_MIGRATION_EXPORT_PHASE1_STATIC_DATA:
		manifest_data_type = AMDGV_MIGRATION_EXPORT_STATIC_DATA;
		break;

	case AMDGV_MIGRATION_EXPORT_PHASE2_DYNAMIC_DATA:
		manifest_data_type = AMDGV_MIGRATION_EXPORT_DYNAMIC_DATA;
		break;

	default:
		manifest_data_type = AMDGV_MIGRATION_INVALID;
		break;
	}

	return manifest_data_type;
}

static enum amdgv_migration_manifest_data_type _amdgv_migration_import_phase_to_manifest_type_mapping(enum amdgv_migration_import_phase type)
{
	enum amdgv_migration_manifest_data_type manifest_data_type = AMDGV_MIGRATION_INVALID;

	switch (type) {
	case AMDGV_MIGRATION_IMPORT_PHASE1_PREPARE:
		manifest_data_type = AMDGV_MIGRATION_IMPORT_PREPARE;
		break;

	case AMDGV_MIGRATION_IMPORT_PHASE2_STATIC_DATA:
		manifest_data_type = AMDGV_MIGRATION_IMPORT_STATIC_DATA;
		break;

	case AMDGV_MIGRATION_IMPORT_PHASE3_DYNAMIC_DATA:
		manifest_data_type = AMDGV_MIGRATION_IMPORT_DYNAMIC_DATA;
		break;
	default:
		manifest_data_type = AMDGV_MIGRATION_INVALID;
		break;
	}

	return manifest_data_type;
}

int amdgv_migration_end(amdgv_dev_t dev, uint32_t idx_vf)
{
	struct amdgv_adapter *adapt;
	int ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);
	if (AMDGV_IS_IDX_INVALID(idx_vf) ||
		(adapt->live_migration.mig_state[idx_vf].state != AMDGV_MIGRATION_VF_STATE_EXPORT &&
		adapt->live_migration.mig_state[idx_vf].state != AMDGV_MIGRATION_VF_STATE_IMPORT)) {
		AMDGV_ERROR("Live migration isn't in correct state, can't end live migration.\n");
		ret = AMDGV_FAILURE;
		goto out;
	}

out:
	oss_mutex_unlock(adapt->api_lock);
	return ret;
}

int amdgv_migration_import(amdgv_dev_t dev, uint32_t idx_vf, void *buf, uint64_t size,
			   enum amdgv_migration_import_phase phase)
{
	enum amdgv_migration_manifest_data_type manifest_data_type = _amdgv_migration_import_phase_to_manifest_type_mapping(phase);
	return amdgv_migration_transfer_manifest_data_event(dev, idx_vf, buf, 0,
							    manifest_data_type, size);
}

int amdgv_migration_import_ex(amdgv_dev_t dev, uint32_t idx_vf, void *buf,
			   uint64_t gpu_addr, uint64_t size, enum amdgv_migration_import_phase phase)
{
	enum amdgv_migration_manifest_data_type manifest_data_type = _amdgv_migration_import_phase_to_manifest_type_mapping(phase);
	return amdgv_migration_transfer_manifest_data_event(dev, idx_vf, buf, gpu_addr,
							    manifest_data_type, size);
}

int amdgv_migration_export(amdgv_dev_t dev, uint32_t idx_vf, void *buf,
			   enum amdgv_migration_export_phase phase)
{
	enum amdgv_migration_manifest_data_type manifest_data_type = _amdgv_migration_export_phase_to_manifest_type_mapping(phase);
	return amdgv_migration_transfer_manifest_data_event(dev, idx_vf, buf, 0,
							    manifest_data_type, 0);
}

int amdgv_migration_export_ex(amdgv_dev_t dev, uint32_t idx_vf, void *buf,
			   uint64_t gpu_addr, enum amdgv_migration_export_phase phase)
{
	enum amdgv_migration_manifest_data_type manifest_data_type = _amdgv_migration_export_phase_to_manifest_type_mapping(phase);
	return amdgv_migration_transfer_manifest_data_event(dev, idx_vf, buf, gpu_addr,
							    manifest_data_type, 0);
}

int amdgv_get_migration_static_package(amdgv_dev_t dev, void *buf, uint64_t *size)
{
	int ret = 0;
	struct amdgv_adapter *adapt;
	uint64_t size_return = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (buf == NULL || size == NULL || *size == 0) {
		if (size)
			*size = 0;
		return AMDGV_FAILURE;
	}

	oss_mutex_lock(adapt->api_lock);

	if (!adapt->live_migration.static_data_mem) {
		AMDGV_ERROR("Static data memory on FB for migration isn't ready.\n");
		ret = AMDGV_FAILURE;
		goto out;
	}
	if (amdgv_migration_get_psp_data_size(adapt, &size_return,
					      AMDGV_MIGRATION_CONTENT_VF_HW_STATIC_DATA)) {
		ret = AMDGV_FAILURE;
		goto out;
	}
	if (size_return > *size) {
		AMDGV_ERROR("Static package data size imcompatible, exited.\n");
		ret = AMDGV_FAILURE;
		goto out;
	}

	oss_memcpy(buf, amdgv_memmgr_get_cpu_addr(adapt->live_migration.static_data_mem),
		   size_return);

	AMDGV_INFO("Get migration static data with size = %llu\n", size_return);

	*size = size_return;

out:
	oss_mutex_unlock(adapt->api_lock);
	return ret;
}

int amdgv_get_migration_dynamic_package(amdgv_dev_t dev, void *buf, uint64_t *size)
{
	int ret = 0;
	struct amdgv_adapter *adapt;
	uint64_t size_return = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (buf == NULL || size == NULL || *size == 0) {
		if (size)
			*size = 0;
		return AMDGV_FAILURE;
	}

	oss_mutex_lock(adapt->api_lock);

	if (!adapt->live_migration.dynamic_data_mem) {
		AMDGV_ERROR("Dynamic data memory on FB for migration isn't ready.\n");
		ret = AMDGV_FAILURE;
		goto out;
	}
	if (amdgv_migration_get_psp_data_size(adapt, &size_return,
					      AMDGV_MIGRATION_CONTENT_VF_HW_DYNAMIC_DATA)) {
		ret = AMDGV_FAILURE;
		goto out;
	}
	if (size_return > *size) {
		AMDGV_ERROR("Dynamic package data size imcompatible, exited.\n");
		ret = AMDGV_FAILURE;
		goto out;
	}

	oss_memcpy(buf, amdgv_memmgr_get_cpu_addr(adapt->live_migration.dynamic_data_mem),
		   size_return);

	AMDGV_INFO("Get migration dynamic data with size = %llu\n", size_return);

	*size = size_return;

out:
	oss_mutex_unlock(adapt->api_lock);
	return ret;
}

int amdgv_control_dirtybit(amdgv_dev_t dev, bool enable)
{
	int ret = 0;
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);
	ret = amdgv_dirtybit_control(adapt, enable);
	oss_mutex_unlock(adapt->api_lock);
	return ret;
}

int amdgv_query_dirtybit_data(amdgv_dev_t dev,
				struct amdgv_query_dirty_bit_data *data)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data event_data;
	int event_ret = 0;
	int ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_memcpy(&event_data.dirtybit_query_data.data, data, sizeof(struct amdgv_query_dirty_bit_data));
	event_data.dirtybit_query_data.result = &event_ret;

	oss_mutex_lock(adapt->api_lock);
	ret = amdgv_sched_queue_event_and_wait_ex(adapt, data->idx_vf,
					  AMDGV_EVENT_QUERY_DIRTYBIT_DATA,
					  AMDGV_SCHED_BLOCK_ALL, event_data);
	if (!ret) {
		ret = event_ret;
	}

	oss_mutex_unlock(adapt->api_lock);
	return ret;
}

int amdgv_migration_query_abort(amdgv_dev_t dev, uint32_t idx_vf, bool *should_abort)
{
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;

	oss_mutex_lock(adapt->api_lock);
	*should_abort = AMDGV_MIGRATION_SHOULD_ABORT(adapt, idx_vf);
	oss_mutex_unlock(adapt->api_lock);

	return 0;
}

int amdgv_migration_set_abort(amdgv_dev_t dev, uint32_t idx_vf)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data = {0};

	int ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;

	oss_mutex_lock(adapt->api_lock);
	ret = amdgv_sched_queue_event_and_wait_ex(adapt, idx_vf,
						  AMDGV_EVENT_VF_MIGRATION_SET_ABORT,
						  AMDGV_SCHED_BLOCK_ALL, data);
	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int amdgv_get_diag_data(amdgv_dev_t dev, uint32_t bdf, void *buf, uint32_t *size)
{
	int ret;
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	uint32_t event_ret = 0;

	if (buf == NULL || size == NULL || *size == 0) {
		if (size)
			*size = 0;
		return AMDGV_FAILURE;
	}

#ifndef EXCLUDE_DCORE_DEBUG
	if (oss_diag_data_collect_disabled(dev, bdf)) {
		*size = 0;
		return AMDGV_FAILURE;
	}
#endif

	if (dev) {
		adapt = (struct amdgv_adapter *)dev;
		oss_mutex_lock(adapt->api_lock);

		data.diag_data.data = buf;
		data.diag_data.size = *size;
		data.diag_data.result = &event_ret;

		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_COLLECT_DIAG_DATA,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			*size = event_ret;
		else
			*size = 0;

		oss_mutex_unlock(adapt->api_lock);
	} else {
		if (bdf)
			*size = amdgv_diag_data_collect(NULL, bdf, buf, *size);
		else
			*size = 0;
		ret = 0;
	}
	return ret;
}

#ifndef EXCLUDE_FTRACE
void AMDGV_API amdgv_device_exclude_list(char **call_trace_exclude_list[],
					 uint32_t *call_trace_exclude_len)
{
	amdgv_diag_data_exclude_list(call_trace_exclude_list, call_trace_exclude_len);
}

int AMDGV_API amdgv_list_gpu_threads(amdgv_dev_t dev, void **task_list, uint32_t task_list_len,
				     uint32_t *bdf)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;
	uint32_t world_switch = 0;

	*bdf = 0;
	if (dev == NULL || task_list == NULL || task_list_len < AMDGV_GPU_THREAD_COUNT)
		return 0;

	/* World switch threads */
	for (world_switch = 0; world_switch < AMDGV_MAX_NUM_WORLD_SWITCH; world_switch++) {
		task_list[world_switch] =
			adapt->sched.world_switch[world_switch].manual.switch_thread;
	}

	task_list[AMDGV_GPU_THREAD_READ_VBIOS] = adapt->vbios.read_vbios_thread;
	task_list[AMDGV_GPU_THREAD_EVENT] = adapt->sched.event_thread;
	task_list[AMDGV_GPU_THREAD_ERROR_PROCESS] = adapt->log.process_thread;

	*bdf = adapt->bdf;

	return AMDGV_GPU_THREAD_COUNT;
}
#endif

int amdgv_toggle_ih_registration(amdgv_dev_t dev, bool enable)
{
	int ret = 0;
	struct amdgv_adapter *adapt;

	if (dev != NULL)
		adapt = (struct amdgv_adapter *)dev;
	else
		return AMDGV_LOG_GPU_DEVICE_LOST;

	oss_mutex_lock(adapt->api_lock);

	if (enable) {
		AMDGV_INFO("Register interrupt.\n");
		adapt->irqmgr.ih.enabled = true;
		ret = amdgv_irqmgr_register_interrupt(adapt);
	} else {
		AMDGV_INFO("Unregister interrupt.\n");
		adapt->irqmgr.ih.enabled = false;
		ret = amdgv_irqmgr_unregister_interrupt(adapt);
	}

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int amdgv_read_vbios(amdgv_dev_t dev, void *buffer, uint32_t size)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;

	if (dev == AMDGV_INVALID_HANDLE)
		return 0;
	else if (buffer == NULL || size == 0)
		return adapt->vbios.image_size;
	else if (size < adapt->vbios.image_size)
		return 0;
	else if (size > adapt->vbios.image_size)
		size = adapt->vbios.image_size;

	oss_mutex_lock(adapt->api_lock);

	oss_memcpy(buffer, adapt->vbios.image, size);

	oss_mutex_unlock(adapt->api_lock);

	return size;
}

int amdgv_read_ip_data(amdgv_dev_t dev, void *buffer, uint32_t size)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;

	if (dev == AMDGV_INVALID_HANDLE)
		return 0;
	else if (buffer == NULL || size == 0)
		return sizeof(struct amdgv_ip_discovery);
	else if (size < sizeof(struct amdgv_ip_discovery))
		return 0;

	if (size > sizeof(adapt->ip_discovery))
		size = sizeof(adapt->ip_discovery);

	oss_mutex_lock(adapt->api_lock);

	oss_memcpy(buffer, &adapt->ip_discovery, size);

	oss_mutex_unlock(adapt->api_lock);

	return size;
}

int amdgv_read_psp_data(amdgv_dev_t dev, void *buffer, uint32_t size)
{
	struct amdgv_adapter *adapt = NULL;
	struct gpuv_psp_data *psp = NULL;
	uint32_t i;
	int psp_idx;

	if (dev == AMDGV_INVALID_HANDLE)
		return 0;
	else if (buffer == NULL || size == 0)
		return sizeof(struct gpuv_psp_data);
	else if (size < sizeof(struct gpuv_psp_data))
		return 0;

	adapt = (struct amdgv_adapter *)dev;
	psp = (struct gpuv_psp_data *)buffer;
	psp_idx = adapt->psp.idx;

	if (size > sizeof(struct gpuv_psp_data))
		size = sizeof(struct gpuv_psp_data);

	oss_mutex_lock(adapt->api_lock);

	// convert libgv psp data to kmd psp data
	// fill km_ring buffer
	psp->km_ring.ring_type = psp_ring_type_to_gpuv_psp_ring_type(adapt->psp.km_ring[psp_idx].ring_type);
	psp->km_ring.ring_rptr = adapt->psp.km_ring[psp_idx].ring_rptr;
	psp->km_ring.ring_wptr = adapt->psp.km_ring[psp_idx].ring_wptr;
	psp->km_ring.ring_mem.size = adapt->psp.km_ring[psp_idx].ring_mem.size;
	psp->km_ring.ring_mem.alignment = adapt->psp.km_ring[psp_idx].ring_mem.alignment;
	psp->km_ring.ring_mem.mc_address =
		amdgv_memmgr_get_gpu_addr(adapt->psp.km_ring[psp_idx].ring_mem.mem);
	psp->km_ring.ring_mem.virtual_address =
		amdgv_memmgr_get_cpu_addr(adapt->psp.km_ring[psp_idx].ring_mem.mem);

	// fill km cmd context
	psp->km_cmd_context.next_avail_cmd_buf_index =
		adapt->psp.km_cmd_context.next_avail_cmd_buf_index;
	psp->km_cmd_context.km_fence_count = adapt->psp.km_cmd_context.km_fence_count;
	psp->km_cmd_context.km_fence_mem_handle.size =
		adapt->psp.km_cmd_context.km_fence_mem_handle.size;
	psp->km_cmd_context.km_fence_mem_handle.alignment =
		adapt->psp.km_cmd_context.km_fence_mem_handle.alignment;
	psp->km_cmd_context.km_fence_mem_handle.mc_address =
		amdgv_memmgr_get_gpu_addr(adapt->psp.km_cmd_context.km_fence_mem_handle.mem);
	psp->km_cmd_context.km_fence_mem_handle.virtual_address =
		amdgv_memmgr_get_cpu_addr(adapt->psp.km_cmd_context.km_fence_mem_handle.mem);

	for (i = 0; i < GPUV_PSP_KM_CMD_MAX_NUM; i++) {
		psp->km_cmd_context.km_cmd_buf_pool[i].fence_value =
			adapt->psp.km_cmd_context.km_cmd_buf_pool[i].fence_value;
		psp->km_cmd_context.km_cmd_buf_pool[i].used =
			adapt->psp.km_cmd_context.km_cmd_buf_pool[i].used;
		psp->km_cmd_context.km_cmd_buf_pool[i].cmd_mem.size =
			adapt->psp.km_cmd_context.km_cmd_buf_pool[i].cmd_mem.size;
		psp->km_cmd_context.km_cmd_buf_pool[i].cmd_mem.alignment =
			adapt->psp.km_cmd_context.km_cmd_buf_pool[i].cmd_mem.alignment;
		psp->km_cmd_context.km_cmd_buf_pool[i].cmd_mem.mc_address =
			amdgv_memmgr_get_gpu_addr(
				adapt->psp.km_cmd_context.km_cmd_buf_pool[i].cmd_mem.mem);
		psp->km_cmd_context.km_cmd_buf_pool[i].cmd_mem.virtual_address =
			amdgv_memmgr_get_cpu_addr(
				adapt->psp.km_cmd_context.km_cmd_buf_pool[i].cmd_mem.mem);
	}

	oss_mutex_unlock(adapt->api_lock);

	return size;
}

int amdgv_get_engine_queue_data(amdgv_dev_t dev, enum amdgv_engine_id id, void *buffer, uint32_t size)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;
	struct gpuv_engine_queue_data *queue_data = (struct gpuv_engine_queue_data *)buffer;
	struct amdgv_ring *ring = NULL;

	if (dev == AMDGV_INVALID_HANDLE)
		return 0;
	else if (size < sizeof(struct gpuv_engine_queue_data))
		return 0;
	oss_memset(buffer, 0, size);

	switch (id) {
	case ENGINE_SDMA:
		ring = &adapt->sdma.sdma_ring[0];
		break;
	case ENGINE_COMPUTE:
		ring = &adapt->gfx.compute_ring[adapt->gfx.compute_paging_queue_id];
		break;
	default:
		break;
	}
	if (ring) {
		oss_mutex_lock(adapt->api_lock);
		if (ring->ring && ring->ring_size) {
			queue_data->fence_memory_mc_addr = ring->fence_gpu_addr;
			queue_data->fence_memory_cpu_addr = ring->fence_cpu_addr;
		}
		oss_mutex_unlock(adapt->api_lock);
		return size;
	}
	return 0;
}

int amdgv_submit_frame_to_engine(amdgv_dev_t dev, enum amdgv_engine_id id, uint8_t *frame_data)
{
	int result = -1;
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;
	struct amdgv_ring *ring = NULL;

	if (dev != AMDGV_INVALID_HANDLE) {
		switch (id) {
		case ENGINE_SDMA:
			ring = &adapt->sdma.sdma_ring[0];
			break;
		case ENGINE_COMPUTE:
			ring = &adapt->gfx.compute_ring[adapt->gfx.compute_paging_queue_id];
			break;
		default:
			break;
		}
		if (ring && ring->funcs && ring->funcs->submit_frame) {
			oss_mutex_lock(adapt->api_lock);
			ring->funcs->submit_frame(ring, frame_data);
			oss_mutex_unlock(adapt->api_lock);
			result = 0;
		}
	}
	return result;
}

int amdgv_toggle_power_saving(amdgv_dev_t dev, bool enable)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int event_ret = 0;
	uint32_t status;
	uint32_t i = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!(adapt->flags & AMDGV_FLAG_IPS_POWER_SAVING)) {
		AMDGV_WARN("ips power saving not supported\n");
		return AMDGV_LOG_GPUMON_NOT_SUPPORTED;
	}

	if (!(adapt->flags & AMDGV_FLAG_DISABLE_SELF_SWITCH)) {
		AMDGV_WARN(
			"IPS power saving is not compatible with self-worldswitch"
			"if we intend to enable power saving, please disable self-worldswitch\n");
		return AMDGV_LOG_GPUMON_NOT_SUPPORTED;
	}

	if (!adapt->pp.pp_funcs || !adapt->pp.pp_funcs->enter_power_saving ||
	    !adapt->pp.pp_funcs->exit_power_saving ||
	    !adapt->pp.pp_funcs->query_power_saving_status) {
		AMDGV_WARN("enter/exit powersaving function is NULL\n");
		return AMDGV_LOG_GPUMON_NOT_SUPPORTED;
	}

	if (adapt->pp.pp_funcs->query_power_saving_status(adapt, &status)) {
		AMDGV_WARN("power saving feature not supported\n");
		return AMDGV_LOG_GPUMON_NOT_SUPPORTED;
	}

	if (enable && adapt->in_live_update) {
		AMDGV_WARN("skip powersaving since it's in live udpate\n");
		return AMDGV_LOG_GPUMON_VF_BUSY;
	}

	for (i = 0; enable && i < adapt->num_vf; i++) {
		if (is_active_vf(i)) {
			AMDGV_WARN("please shutdown all vf for powersaving\n");
			return AMDGV_LOG_GPUMON_VF_BUSY;
		}
	}

	data.gpumon_data.result = &event_ret;

	/* Allocation requires atomic operation */
	oss_mutex_lock(adapt->api_lock);

	if (enable)
		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_ENTER_POWER_SAVING,
							  AMDGV_SCHED_BLOCK_ALL, data);
	else
		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_EXIT_POWER_SAVING,
							  AMDGV_SCHED_BLOCK_ALL, data);

	if (!ret)
		ret = event_ret;

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int amdgv_query_power_saving_status(amdgv_dev_t dev, uint32_t *status)
{
	int ret;
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!status) {
		AMDGV_ERROR("invalid input parameter\n");
		return AMDGV_LOG_DRIVER_INVALID_VALUE;
	}

	if (!(adapt->flags & AMDGV_FLAG_IPS_POWER_SAVING)) {
		AMDGV_WARN("ips power saving not supported\n");
		return AMDGV_LOG_GPUMON_NOT_SUPPORTED;
	}

	if (!(adapt->flags & AMDGV_FLAG_DISABLE_SELF_SWITCH)) {
		AMDGV_WARN(
			"IPS power saving is not compatible with self-worldswitch"
			"if we intend to enable power saving, please disable self-worldswitch\n");
		return AMDGV_LOG_GPUMON_NOT_SUPPORTED;
	}

	if (!adapt->pp.pp_funcs || !adapt->pp.pp_funcs->query_power_saving_status) {
		AMDGV_WARN("query powersaving function is NULL\n");
		return AMDGV_LOG_GPUMON_NOT_SUPPORTED;
	}

	/* Allocation requires atomic operation */
	oss_mutex_lock(adapt->api_lock);

	ret = adapt->pp.pp_funcs->query_power_saving_status(adapt, status);

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int amdgv_toggle_power_saving_external(amdgv_dev_t dev, bool enable)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	ret = amdgv_toggle_power_saving(dev, enable);

	return ret;
}

int amdgv_query_power_saving_status_external(amdgv_dev_t dev, uint32_t *status)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	ret = amdgv_query_power_saving_status(dev, status);

	return ret;
}

int amdgv_load_vbflash_bin(amdgv_dev_t dev, char *buffer, uint32_t pos, uint32_t count)
{
	int ret = 0;
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);
	ret = amdgv_psp_load_vbflash_bin(adapt, buffer, pos, count);
	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int amdgv_update_spirom(amdgv_dev_t dev)
{
	int ret = 0;
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);
	ret = amdgv_psp_update_spirom(adapt);
	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int amdgv_get_vbflash_status(amdgv_dev_t dev, uint32_t *status)
{
	int ret = 0;
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);
	ret = amdgv_psp_vbflash_status(adapt, status);
	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int AMDGV_API amdgv_set_rlcv_timestamp_dump(amdgv_dev_t dev, uint64_t enable)
{
	int ret = 0;
	struct amdgv_adapter *adapt;
	enum amdgv_gpuiov_event_id event;
	int hw_sched_id;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->api_lock);

	if (!adapt->gpuiov.funcs->setup_sched_debug_log) {
		AMDGV_WARN("TS LOG is not supported on this asic.\n");
		ret = AMDGV_FAILURE;
		goto unlock;
	}
	AMDGV_INFO("TS LOG is %s\n", enable ? "enabled" : "disabled");

	if (enable) {
		ret = adapt->gpuiov.funcs->setup_sched_debug_log(adapt, AMDGV_SCHED_TS_LOG);
		if (ret) {
			AMDGV_WARN("Failed to setup ts_log.\n");
			goto unlock;
		}

		for_each_id(hw_sched_id, amdgv_sched_get_hw_sched_mask_by_sched_block(
						      adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_GFX)) {
			ret = amdgv_sched_world_switch_config_auto_sched_mode(adapt, hw_sched_id);
			if (ret) {
				AMDGV_ERROR("failed to push sched mem desc for ts_log\n");
				goto unlock;
			}
		}
	}

	event = AMDGV_EVENT_TS_LOG;
	for_each_id(hw_sched_id, amdgv_sched_get_hw_sched_mask_by_sched_block(
					      adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_GFX)) {
		ret = amdgv_gpuiov_event_notification(adapt, AMDGV_PF_IDX, hw_sched_id,
							event, enable);
		if (ret) {
			AMDGV_ERROR("failed to send event notification\n");
			goto unlock;
		}
	}

unlock:
	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int AMDGV_API amdgv_dump_rlcv_timestamp_log(amdgv_dev_t dev)
{
	struct amdgv_adapter *adapt;
	int ret;
	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);
	oss_mutex_lock(adapt->api_lock);
	ret = amdgv_sched_dump_ts_log_data(adapt);
	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int AMDGV_API amdgv_ffbm_vf_mapping(amdgv_dev_t dev, uint32_t vf_idx, uint64_t size,
				    uint64_t gpa, uint64_t spa, uint8_t permission)
{
	int ret = 0;
	struct amdgv_adapter *adapt;
	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);
	if (AMDGV_IS_IDX_INVALID(vf_idx)) {
		AMDGV_WARN("invalid VF index\n");
		return AMDGV_FAILURE;
	}

	oss_mutex_lock(adapt->api_lock);
	ret = amdgv_ffbm_manual_map(adapt, vf_idx, size, gpa, spa, permission, AMDGV_FFBM_MEM_TYPE_VF);
	if (ret)
		goto unlock;
	ret = amdgv_ffbm_apply_page_table(adapt);

unlock:
	oss_mutex_unlock(adapt->api_lock);
	return ret;
}

int AMDGV_API amdgv_ffbm_clear_vf_mapping(amdgv_dev_t dev, uint32_t vf_idx)
{
	struct amdgv_adapter *adapt;
	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);
	if (AMDGV_IS_IDX_INVALID(vf_idx)) {
		AMDGV_WARN("invalid VF index\n");
		return AMDGV_FAILURE;
	}

	oss_mutex_lock(adapt->api_lock);
	amdgv_ffbm_unmap_by_fcn(adapt, vf_idx, false);
	oss_mutex_unlock(adapt->api_lock);
	return 0;
}

int AMDGV_API amdgv_ffbm_print_spa_list(amdgv_dev_t dev)
{
	struct amdgv_adapter *adapt;
	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);
	oss_mutex_lock(adapt->api_lock);
	amdgv_ffbm_dump_page_table(adapt);
	oss_mutex_unlock(adapt->api_lock);
	return 0;
}

int AMDGV_API amdgv_ffbm_read_spa_list(amdgv_dev_t dev, char *page_table_content,
				       int restore_length, int *len)
{
	struct amdgv_adapter *adapt;
	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);
	oss_mutex_lock(adapt->api_lock);
	amdgv_ffbm_read_page_table(adapt, page_table_content, restore_length, len);
	oss_mutex_unlock(adapt->api_lock);
	return 0;
}

int AMDGV_API amdgv_ffbm_copy_spa_data(amdgv_dev_t dev, void *page_table_content, int max_num, int *len)
{
	struct amdgv_adapter *adapt;
	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);
	oss_mutex_lock(adapt->api_lock);
	amdgv_ffbm_copy_page_table(adapt, page_table_content, max_num, len);
	oss_mutex_unlock(adapt->api_lock);
	return 0;
}

int AMDGV_API amdgv_ffbm_mark_badpage(amdgv_dev_t dev, uint64_t spa)
{
	struct amdgv_adapter *adapt;
	struct eeprom_table_record *bp;
	int ret = 0;
	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	bp = oss_zalloc(sizeof(*bp));
	if (bp == NULL) {
		AMDGV_ERROR("Failed to allocate memory during ffbm mark badpage\n");
		return AMDGV_FAILURE;
	}
	bp->retired_page = spa >> AMDGV_GPU_PAGE_SHIFT;
	oss_mutex_lock(adapt->api_lock);
	ret = amdgv_ffbm_replace_bad_pages(adapt, bp, 1);
	oss_mutex_unlock(adapt->api_lock);
	oss_free(bp);
	return ret;
}

int AMDGV_API amdgv_ffbm_find_spa(amdgv_dev_t dev, uint64_t gpa, int idx_vf, uint64_t *spa)
{
	struct amdgv_adapter *adapt;
	int ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);
	oss_mutex_lock(adapt->api_lock);
	if (!spa) {
		AMDGV_ERROR("Null pointer passed to AMDGV_API\n ");
		ret = AMDGV_FAILURE;
		goto api_exit;
	}
	*spa = amdgv_ffbm_gpa_to_spa(adapt, gpa, idx_vf);
	if (*spa == AMDGV_FFBM_INVALID_ADDR) {
		AMDGV_ERROR("Failed to get spa for gpa 0x%llx vf%u\n", gpa, idx_vf);
		ret = AMDGV_FAILURE;
		goto api_exit;
	}
	AMDGV_INFO("gpa 0x%llx vf%u: spa 0x%llx\n", gpa, idx_vf, *spa);
	ret = 0;

api_exit:
	oss_mutex_unlock(adapt->api_lock);
	return ret;
}

int AMDGV_API amdgv_ffbm_find_gpa(amdgv_dev_t dev, uint64_t spa, uint64_t *gpa, int *idx_vf)
{
	struct amdgv_adapter *adapt;
	int ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);
	oss_mutex_lock(adapt->api_lock);
	if (!gpa || !idx_vf) {
		AMDGV_ERROR("Null pointer passed to AMDGV_API\n");
		ret = AMDGV_FAILURE;
		goto api_exit;
	}
	*gpa = amdgv_ffbm_spa_to_gpa(adapt, spa, idx_vf);
	if (*gpa == AMDGV_FFBM_INVALID_ADDR) {
		AMDGV_ERROR("Failed to get gpa for spa 0x%llx\n", spa);
		ret = AMDGV_FAILURE;
		goto api_exit;
	}
	AMDGV_INFO("spa 0x%llx: gpa 0x%llx vf%u\n", spa, *gpa, *idx_vf);
	ret = 0;

api_exit:
	oss_mutex_unlock(adapt->api_lock);
	return ret;
}

int AMDGV_API amdgv_set_timeslice(amdgv_dev_t dev, uint32_t vf_idx, uint32_t time_slice,
				  enum amdgv_sched_block sched_id)
{
	struct amdgv_adapter *adapt;
	int ret = 0;
	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (sched_id < 0 || sched_id >= AMDGV_SCHED_BLOCK_MAX) {
		AMDGV_ERROR("Unsupported scheduler id\n");
		return AMDGV_FAILURE;
	}

	if (adapt->num_vf == 1) {
		AMDGV_ERROR("Not supported on 1 vf\n");
		return AMDGV_FAILURE;
	}

	if (time_slice < 1000 || time_slice > 500000) {
		AMDGV_ERROR("Invalid timeslice value\n");
		return AMDGV_FAILURE;
	}

	if (AMDGV_IS_IDX_INVALID(vf_idx) && (vf_idx != 0xffffffff)) {
		AMDGV_ERROR("Invalid vf idx\n");
		return AMDGV_FAILURE;
	}

	oss_mutex_lock(adapt->api_lock);
	ret = amdgv_sched_set_time_slice(adapt, vf_idx, time_slice, sched_id);
	oss_mutex_unlock(adapt->api_lock);
	return ret;
}

int AMDGV_API amdgv_get_timeslice(amdgv_dev_t dev, uint32_t vf_idx,
				  enum amdgv_sched_block sched_id)
{
	struct amdgv_adapter *adapt;
	int ret = 0;
	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (sched_id < 0 || sched_id >= AMDGV_SCHED_BLOCK_MAX) {
		AMDGV_ERROR("Unsupported scheduler id\n");
		return AMDGV_FAILURE;
	}

	if (adapt->num_vf == 1) {
		AMDGV_ERROR("Not supported on 1 vf\n");
		return AMDGV_FAILURE;
	}

	if (AMDGV_IS_IDX_INVALID(vf_idx)) {
		AMDGV_ERROR("Invalid vf idx\n");
		return AMDGV_FAILURE;
	}

	oss_mutex_lock(adapt->api_lock);
	ret = adapt->array_vf[vf_idx].time_slice[sched_id];
	oss_mutex_unlock(adapt->api_lock);
	return ret;
}

int AMDGV_API amdgv_disable_ras_feature(amdgv_dev_t dev)
{
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	return amdgv_ecc_disable_ras_feature(adapt);
}

int AMDGV_API amdgv_enable_ras_feature(amdgv_dev_t dev)
{
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	return amdgv_ecc_enable_ras_feature(adapt);
}

int AMDGV_API amdgv_copy_ip_discovery_data_to_vf(amdgv_dev_t dev, uint32_t vf_idx)
{
	int ret = 0;
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (AMDGV_IS_IDX_INVALID(vf_idx))
		return AMDGV_FAILURE;

	oss_mutex_lock(adapt->api_lock);
	ret = amdgv_vfmgr_copy_ip_data_to_vf(adapt, vf_idx, false);
	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int AMDGV_API amdgv_read_auto_sched_perf_log(amdgv_dev_t dev, uint32_t *data_size, uint32_t *data)
{
	struct amdgv_adapter *adapt;
	int ret;
	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);
	oss_mutex_lock(adapt->api_lock);
	ret = amdgv_sched_read_perf_log_data(adapt);
	if (!ret) {
		oss_memcpy(data, &adapt->perf_log, sizeof(struct amdgv_perf_log_info));
		*data_size = sizeof(struct amdgv_perf_log_info);
	}
	oss_mutex_unlock(adapt->api_lock);
	return ret;
}

int AMDGV_API amdgv_query_debug_dump_fb_addr(amdgv_dev_t dev, uint32_t *offset, uint32_t *size)
{
	struct amdgv_adapter *adapt;
	int ret = 0;
	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);
	oss_mutex_lock(adapt->api_lock);
	if (!adapt->gpuiov.debug_dump_mem) {
		AMDGV_WARN("debug dump memory not allocated\n");
		ret = AMDGV_FAILURE;
	} else {
		*offset = amdgv_memmgr_get_gpu_addr(adapt->gpuiov.debug_dump_mem);
		*size = amdgv_memmgr_get_size(adapt->gpuiov.debug_dump_mem);
	}
	oss_mutex_unlock(adapt->api_lock);
	return ret;
}

bool AMDGV_API amdgv_in_whole_gpu_reset(amdgv_dev_t dev)
{
	struct amdgv_adapter *adapt;

	if (dev == AMDGV_INVALID_HANDLE)
		return false;

	adapt = (struct amdgv_adapter *)dev;
	return in_whole_gpu_reset();
}

int AMDGV_API amdgv_set_mes_info_dump_enable(amdgv_dev_t dev, bool enable)
{
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);
	oss_mutex_lock(adapt->api_lock);

	if (enable)
		adapt->flags |= AMDGV_FLAG_MES_INFO_DUMP_ENABLE;
	else
		adapt->flags &= ~AMDGV_FLAG_MES_INFO_DUMP_ENABLE;

	oss_mutex_unlock(adapt->api_lock);

	return 0;
}

int AMDGV_API amdgv_get_mes_info_dump_enable(amdgv_dev_t dev, uint32_t *data_adapt, uint32_t *data_vf, unsigned int *num_vf)
{
	struct amdgv_adapter *adapt;
	uint32_t i = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);
	oss_mutex_lock(adapt->api_lock);

	*data_adapt = 0;

	if (adapt->flags & AMDGV_FLAG_MES_INFO_DUMP_ENABLE) {
		*data_adapt = 1;
		for (i = 0; i < adapt->num_vf; ++i) {
			if (adapt->array_vf[i].mes_info_dump_enabled)
				data_vf[i] = 1;
			else
				data_vf[i] = 0;
		}
	}

	*num_vf = adapt->num_vf;

	oss_mutex_unlock(adapt->api_lock);
	return 0;
}

static void amdgv_disable_bp_mode_1(struct amdgv_adapter *adapt)
{
	uint32_t i;
	struct amdgv_sched_world_switch *world_switch;

	for (i = 0; i < adapt->sched.num_world_switch; i++) {
		world_switch = &adapt->sched.world_switch[i];
		if (world_switch->enabled && world_switch->sched_block == AMDGV_SCHED_BLOCK_GFX) {
			if (world_switch->sched_mode > AMDGV_SCHED_MAX_HW_SCHED_MODE) {
				oss_mutex_lock(world_switch->manual.switching_lock);
				world_switch->switch_running = true;
				adapt->bp_gfx_ws_pause_flag = 0;
				oss_mutex_unlock(world_switch->manual.switching_lock);
			} else {
				world_switch->switch_running = true;
				adapt->bp_gfx_ws_pause_flag = 0;
			}
			AMDGV_INFO("sched switch %d is resumed\n", i);
			break;
		}
	}
}

static void amdgv_disable_bp_mode_2(struct amdgv_adapter *adapt)
{
	uint32_t i;
	struct amdgv_sched_world_switch *world_switch;

	for (i = 0; i < adapt->sched.num_world_switch; i++) {
		world_switch = &adapt->sched.world_switch[i];
		if (world_switch->sched_mode == AMDGV_SCHED_FAIRNESS) {
			world_switch->manual.array_vf[world_switch->curr_idx_vf].skip_next_punish = true;
		}
	}
}

int AMDGV_API amdgv_set_bp_mode(amdgv_dev_t dev, int bp_mode)
{
	struct amdgv_adapter *adapt;
	int ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);
	oss_mutex_lock(adapt->bp_lock);
	switch (bp_mode) {
	case AMDGV_BP_MODE_1:
		/* LibGV doesn't yet support switching to mode 1 on runtime */
		ret = AMDGV_FAILURE;
		break;
	case AMDGV_BP_MODE_2:
		AMDGV_INFO("enable Host Driver Break Point Mode %d\n", bp_mode);
		// fall through
	case AMDGV_BP_MODE_DISABLE:
		if (adapt->bp_mode == AMDGV_BP_MODE_1) {
			amdgv_disable_bp_mode_1(adapt);
		} else if (adapt->bp_mode == AMDGV_BP_MODE_2) {
			amdgv_disable_bp_mode_2(adapt);
		}
		adapt->bp_mode = bp_mode;
		adapt->bp_go_flag = 0;
		break;
	default:
		ret = AMDGV_FAILURE;
		break;
	}
	oss_mutex_unlock(adapt->bp_lock);

	return ret;
}

int AMDGV_API amdgv_get_bp_mode(amdgv_dev_t dev)
{
	struct amdgv_adapter *adapt;
	int bp_mode;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_mutex_lock(adapt->bp_lock);
	bp_mode = adapt->bp_mode;
	oss_mutex_unlock(adapt->bp_lock);

	return bp_mode;
}

int AMDGV_API amdgv_bp_go(amdgv_dev_t dev)
{
	struct amdgv_adapter *adapt;
	int ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);
	oss_mutex_lock(adapt->bp_lock);
	if (adapt->bp_mode != AMDGV_BP_MODE_1) {
		adapt->bp_go_flag = 1;
	} else {
		AMDGV_INFO("command not supported.\n");
		ret = AMDGV_FAILURE;
	}
	oss_mutex_unlock(adapt->bp_lock);

	return ret;
}

int AMDGV_API amdgv_get_current_bp(amdgv_dev_t dev, struct amdgv_bp_info *bp_info)
{
	struct amdgv_adapter *adapt;
	int ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);
	oss_mutex_lock(adapt->bp_lock);
	if (adapt->bp_mode == AMDGV_BP_MODE_1) {
		ret = AMDGV_FAILURE;
		goto unlock;
	}

	if (adapt->bp_info.is_in_bp)
		oss_memcpy(bp_info, &(adapt->bp_info), sizeof(struct amdgv_bp_info));
	else
		bp_info->is_in_bp = false;

unlock:
	oss_mutex_unlock(adapt->bp_lock);

	return ret;
}

int AMDGV_API amdgv_send_ws_cmd(amdgv_dev_t dev, uint32_t ws_event, uint32_t hw_sched_id,
					uint32_t idx_vf)
{
	struct amdgv_adapter *adapt;
	int ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	switch (ws_event) {
	case AMDGV_IDLE_GPU:
	case AMDGV_SAVE_GPU_STATE:
	case AMDGV_LOAD_GPU_STATE:
	case AMDGV_RUN_GPU:
	case AMDGV_INIT_GPU:
	case AMDGV_SHUTDOWN_GPU:
		if (AMDGV_IS_IDX_INVALID(idx_vf))
			return AMDGV_FAILURE;
		// fall through
	case AMDGV_ENABLE_AUTO_HW_SWITCH:
	case AMDGV_DISABLE_AUTO_HW_SCHED:
		if (hw_sched_id >= adapt->gpuiov.num_ctrl_blocks) {
			amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_SCHED_INVALID_HW_SCHED_ID, hw_sched_id);
			return AMDGV_FAILURE;
		}
		break;
	default:
		break;
	}

	oss_mutex_lock(adapt->bp_lock);
	adapt->is_user_ws_cmd = true;
	switch (ws_event) {
	case AMDGV_IDLE_GPU:
		if (amdgv_gpuiov_idle_vf(adapt, idx_vf, hw_sched_id))
			ret = AMDGV_LOG_IOV_WS_IDLE_TIMEOUT;
		break;
	case AMDGV_SAVE_GPU_STATE:
		if (amdgv_gpuiov_save_vf(adapt, idx_vf, hw_sched_id))
			ret = AMDGV_LOG_IOV_WS_SAVE_TIMEOUT;
		break;
	case AMDGV_LOAD_GPU_STATE:
		if (amdgv_gpuiov_load_vf(adapt, idx_vf, hw_sched_id))
			ret = AMDGV_LOG_IOV_WS_LOAD_TIMEOUT;
		break;
	case AMDGV_RUN_GPU:
		if (amdgv_gpuiov_run_vf(adapt, idx_vf, hw_sched_id))
			ret = AMDGV_LOG_IOV_WS_SAVE_TIMEOUT;
		break;
	case AMDGV_INIT_GPU:
		if (amdgv_gpuiov_init_vf(adapt, idx_vf, hw_sched_id))
			ret = AMDGV_LOG_IOV_WS_LOAD_TIMEOUT;
		break;
	case AMDGV_SHUTDOWN_GPU:
		if (amdgv_gpuiov_shutdown_vf(adapt, idx_vf, hw_sched_id))
			ret = AMDGV_LOG_IOV_WS_SHUTDOWN_TIMEOUT;
		break;
	case AMDGV_ENABLE_AUTO_HW_SWITCH:
		if (amdgv_gpuiov_enable_auto_sched(adapt, hw_sched_id))
			ret = AMDGV_FAILURE;
		break;
	case AMDGV_DISABLE_AUTO_HW_SCHED:
		if (amdgv_gpuiov_disable_auto_sched(adapt, hw_sched_id))
			ret = AMDGV_FAILURE;
		break;
	}
	adapt->is_user_ws_cmd = false;
	oss_mutex_unlock(adapt->bp_lock);
	return ret;
}
int AMDGV_API amdgv_dump_asymmetric_timeslice(amdgv_dev_t dev, uint32_t idx_vf, bool *is_active_vf, uint32_t *timeslice_vf)
{
	struct amdgv_adapter *adapt;
	struct amdgv_vf_device *entry;
	int ret = 0;
	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;

	oss_mutex_lock(adapt->api_lock);

	if (!adapt->sched.setup_vf_timeslice)
		ret = AMDGV_FAILURE;
	else {
		*is_active_vf = is_active_vf(idx_vf);
		entry = &adapt->array_vf[idx_vf];
		*timeslice_vf = entry->time_slice[AMDGV_SCHED_BLOCK_GFX];
	}
	oss_mutex_unlock(adapt->api_lock);
	return ret;
}

int AMDGV_API amdgv_dump_asymmetric_fb_layout(amdgv_dev_t dev, char *buf, int *len, uint32_t resv_size)
{
	struct amdgv_adapter *adapt;
	int ret = 0;
	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);
	oss_mutex_lock(adapt->api_lock);
	if (!adapt->asymmetric_fb_enabled)
		ret = AMDGV_FAILURE;
	else
		amdgv_vfmgr_read_asymmetric_fb_layout(adapt, buf, len, resv_size);

	oss_mutex_unlock(adapt->api_lock);
	return ret;
}

const char * AMDGV_API amdgv_get_market_name(uint32_t dev_id, uint32_t rev_id)
{
	const char *marketing_name = amdgv_get_marketing_name(dev_id, rev_id);
	return marketing_name;
}

int AMDGV_API amdgv_set_sysmem_va_ptr(amdgv_dev_t dev, void *ptr)
{
	struct amdgv_adapter *adapt;
	int ret = 0;
	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);
	oss_mutex_lock(adapt->api_lock);
	adapt->sys_mem_info.va_ptr = ptr;
	oss_mutex_unlock(adapt->api_lock);
	return ret;
}

int amdgv_register_interrupt_handler(amdgv_dev_t dev, enum amdgv_interrupt_handler_id id,
		void *handler, void *context)
{
	struct amdgv_adapter *adapt;
	int result = 0;
	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);
	switch (id) {
	case SUBMISSION_INTERRUPT:
		amdgv_irqmgr_register_submission_interrupt_handler(adapt, handler, context);
		break;
	case GPU_DISP_TIMER:
		amdgv_irqmgr_register_gpu_timer_handler(adapt, handler, context);
		break;
	default:
		result = AMDGV_FAILURE;
		break;
	}
	return result;
}

int amdgv_gpu_timer(amdgv_dev_t dev, uint64_t micro_seconds)
{
	struct amdgv_adapter *adapt;
	int result = AMDGV_FAILURE;
	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);
	if (adapt->irqmgr.ih_funcs->start_gpu_timer) {
		adapt->irqmgr.ih_funcs->start_gpu_timer(adapt, micro_seconds);
		result = 0;
	}
	return result;
}

int AMDGV_API amdgv_write_virtualized_interrupt(amdgv_dev_t dev, uint32_t idx_vf, uint32_t idx_table, uint64_t message_address,
			uint32_t message_data, uint32_t vector_control, bool is_direct_write)
{
	struct amdgv_adapter *adapt;
	int ret = 0;
	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;

	oss_mutex_lock(adapt->api_lock);
	ret = amdgv_irqmgr_write_virtualized_interrupt(adapt, idx_vf, idx_table, message_address, message_data, vector_control, is_direct_write);
	oss_mutex_unlock(adapt->api_lock);
	return ret;
}

int AMDGV_API amdgv_error_ring_buffer_dump(amdgv_dev_t dev, char *buf, int buf_size)
{
	struct amdgv_adapter *adapt;
	int len;
	SET_ADAPT_AND_CHECK_STATUS_MINIMAL(adapt, dev);
	oss_mutex_lock(adapt->api_lock);
	len = amdgv_log_get_all(adapt, buf, buf_size);
	oss_mutex_unlock(adapt->api_lock);
	return len;
}

int AMDGV_API amdgv_info_ring_buffer_dump(amdgv_dev_t dev, char *buf, int buf_size)
{
	struct amdgv_adapter *adapt;
	int len;
	SET_ADAPT_AND_CHECK_STATUS_MINIMAL(adapt, dev);
	oss_mutex_lock(adapt->api_lock);
	len = amdgv_log_dump_ring(adapt, AMDGV_LOG_RING_INFO, buf, buf_size);
	oss_mutex_unlock(adapt->api_lock);
	return len;
}

int AMDGV_API amdgv_debug_ring_buffer_dump(amdgv_dev_t dev, char *buf, int buf_size)
{
	struct amdgv_adapter *adapt;
	int len;
	SET_ADAPT_AND_CHECK_STATUS_MINIMAL(adapt, dev);
	oss_mutex_lock(adapt->api_lock);
	len = amdgv_log_dump_ring(adapt, AMDGV_LOG_RING_DEBUG, buf, buf_size);
	oss_mutex_unlock(adapt->api_lock);
	return len;
}

int AMDGV_API amdgv_combined_ring_buffer_dump(amdgv_dev_t dev, char *buf, int buf_size)
{
	struct amdgv_adapter *adapt;
	int len;
	SET_ADAPT_AND_CHECK_STATUS_MINIMAL(adapt, dev);
	oss_mutex_lock(adapt->api_lock);
	len = amdgv_log_dump_combined(adapt, buf, buf_size);
	oss_mutex_unlock(adapt->api_lock);
	return len;
}

bool AMDGV_API amdgv_is_service_vm_enabled(amdgv_dev_t dev)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;
	bool ret = false;

	oss_mutex_lock(adapt->api_lock);
	if (adapt->flags & AMDGV_FLAG_ENABLE_SVM)
		ret = true;
	oss_mutex_unlock(adapt->api_lock);

	return ret;
}

int AMDGV_API amdgv_set_product_info_invalid(amdgv_dev_t dev)
{
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS_MINIMAL(adapt, dev);

	if (true == adapt->product_info.visit) {
		adapt->product_info.visit = false;
	}

	return 0;
}

void *AMDGV_API amdgv_map_sysmem(amdgv_dev_t dev, uint64_t len, void **va_ptr, uint64_t *gpu_addr)
{
	return amdgv_map_sysmem_with_attr(dev, len,
				OSS_PAGE_READWRITE | OSS_PAGE_WRITECOMBINE,
				va_ptr, gpu_addr);
}

void *AMDGV_API amdgv_map_sysmem_with_attr(amdgv_dev_t dev, uint64_t len,
					   enum oss_page_attr page_attr,
					   void **va_ptr, uint64_t *gpu_addr)
{
	struct amdgv_adapter *adapt;
	struct amdgv_memmgr_mem *mem;

	if (dev == AMDGV_INVALID_HANDLE)
		return NULL;

	adapt = (struct amdgv_adapter *)dev;
	if ((adapt->status == AMDGV_STATUS_SW_INIT)
		|| (adapt->status != AMDGV_STATUS_HW_INIT)
		|| (!adapt->memmgr_sys.is_init) || !va_ptr || !gpu_addr)
		return NULL;

	oss_mutex_lock(adapt->api_lock);
	mem = amdgv_memmgr_alloc_sys_align_with_attr(&adapt->memmgr_sys, len,
						     PAGE_SIZE, page_attr,
						     gpu_addr, *va_ptr);

	if (!mem) {
		oss_mutex_unlock(adapt->api_lock);
		return NULL;
	}

	if (!*va_ptr)
		*va_ptr = mem->sys_mem.va_ptr;
	mem->sys_mem.size = len;
	oss_mutex_unlock(adapt->api_lock);

	return (void *)(&mem->sys_mem);
}

int AMDGV_API amdgv_unmap_sysmem(amdgv_dev_t dev,
			     void *handle)
{
	struct amdgv_adapter *adapt;
	int ret = 0;
	struct amdgv_memmgr_mem *mem;
	struct oss_dma_mem_info *dma_mem_info = (struct oss_dma_mem_info *)handle;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);
	if (!adapt->memmgr_sys.is_init || !handle)
		return AMDGV_FAILURE;

	oss_mutex_lock(adapt->api_lock);
	mem = container_of(dma_mem_info, struct amdgv_memmgr_mem, sys_mem);
	ret = amdgv_memmgr_free(mem);
	oss_mutex_unlock(adapt->api_lock);
	return ret;
}

int amdgv_handle_ras_ioctl_cmd(amdgv_dev_t dev, struct amdgv_ras_ioctl_cmd *data)
{
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	return amdgv_ras_ioctl_handler(adapt, data);
}

bool amdgv_compare_mig_ctx(amdgv_dev_t dev, uint32_t idx_vf,
			   struct amdgv_migration_ctx *remote_ctx)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;
	bool match = true;
	char field_name_buffer[64];
	int i;

	struct amdgv_migration_ctx *local_ctx = NULL;

	local_ctx =
		(struct amdgv_migration_ctx *)oss_zalloc(sizeof(struct amdgv_migration_ctx));
	if (!local_ctx) {
		AMDGV_ERROR("failed to allocate buffer to do ctx compare.\n");
		return false;
	}

	if (amdgv_get_migration_ctx(dev, idx_vf, local_ctx)) {
		AMDGV_ERROR("failed to get local migration ctx.\n");
		match = false;
		goto exit;
	}
#define FIELD_MISMATCH(field_name, local_value, remote_value) \
	do { \
		AMDGV_ERROR("local_ctx.%s (0x%llx) != remote_ctx.%s (0x%llx)\n", field_name, \
			    (uint64_t)(local_value), field_name, (uint64_t)(remote_value)); \
		match = false; \
		goto exit; \
	} while (0)

#define COMPARE_FIELD(field) \
	do { \
		if (local_ctx->field != remote_ctx->field) { \
			FIELD_MISMATCH(#field, local_ctx->field, remote_ctx->field); \
		} \
	} while (0)

#define COMPARE_STRUCT_ARRAY_FIELD_FOREACH(array_field) \
	for (i = 0; i < ARRAY_SIZE(local_ctx->array_field); i++)

#define COMPARE_STRUCT_ARRAY_FIELD_SUBFIELD(array_field,subfield) \
	do { \
		if (local_ctx->array_field[i].subfield != remote_ctx->array_field[i].subfield) { \
			oss_memset(field_name_buffer, 0, sizeof(field_name_buffer)); \
			oss_vsnprintf(field_name_buffer, sizeof(field_name_buffer), #array_field "[%d]." #subfield, i); \
			FIELD_MISMATCH(field_name_buffer, local_ctx->array_field[i].subfield, \
				remote_ctx->array_field[i].subfield); \
		} \
	} while (0)

	COMPARE_FIELD(gpu.device_id);
	COMPARE_FIELD(gpu.context_version);
	COMPARE_FIELD(gpu.libgv_version);
	COMPARE_FIELD(gpu.vbios_version);
	COMPARE_FIELD(gpu.migration_version);
	COMPARE_FIELD(gpu.num_fw);

	if (local_ctx->gpu.context_version == AMDGV_MIGRATION_CONTEXT_VERSION_V2) {
		COMPARE_FIELD(gpu.gpu_ordinate);
		COMPARE_FIELD(gpu.num_vf);
		COMPARE_FIELD(gpu.xgmi_state);
		COMPARE_FIELD(gpu.cu_num);
		COMPARE_FIELD(gpu.nps_mode);
		COMPARE_FIELD(gpu.cps_mode);
	}

	COMPARE_STRUCT_ARRAY_FIELD_FOREACH(gpu.fw)
	{
		COMPARE_STRUCT_ARRAY_FIELD_SUBFIELD(gpu.fw, id);
		COMPARE_STRUCT_ARRAY_FIELD_SUBFIELD(gpu.fw, version);
	}

	COMPARE_FIELD(vf.version);
	COMPARE_FIELD(vf.vf_index);

	switch (local_ctx->gpu.context_version) {
	case AMDGV_MIGRATION_CONTEXT_VERSION_V1: {
		COMPARE_FIELD(vf.v1_0.vf_fb_size_mb);
		COMPARE_FIELD(vf.v1_0.gfx_timeslice_us);
		COMPARE_FIELD(vf.v1_0.mm_timeslice_us);
		COMPARE_FIELD(vf.v1_0.vcn_engine_bitmask);
		COMPARE_FIELD(vf.v1_0.jpeg_engine_bitmask);
		COMPARE_FIELD(vf.v1_0.partition_config);
		break;
	}
	case AMDGV_MIGRATION_CONTEXT_VERSION_V2: {
		COMPARE_FIELD(vf.v2_0.vf_fb_size_mb);
		COMPARE_FIELD(vf.v2_0.partition_config);
		COMPARE_FIELD(vf.v2_0.sdma_engine_bitmask);
		COMPARE_FIELD(vf.v2_0.hw_sched_engine_bitmask);
		COMPARE_FIELD(vf.v2_0.timeslice_gfx);
		COMPARE_FIELD(vf.v2_0.timeslice_uvd);
		COMPARE_FIELD(vf.v2_0.timeslice_vce);
		COMPARE_FIELD(vf.v2_0.timeslice_uvd1);
		COMPARE_FIELD(vf.v2_0.timeslice_vcn);
		COMPARE_FIELD(vf.v2_0.timeslice_vcn1);
		COMPARE_FIELD(vf.v2_0.timeslice_jpeg);
		break;
	}
	default:
		AMDGV_ERROR("unsupported vf context version %d\n", local_ctx->gpu.context_version);
		match = false;
		goto exit;
	}

	COMPARE_FIELD(vf_status);

#undef COMPARE_STRUCT_ARRAY_FIELD_SUBFIELD
#undef COMPARE_STRUCT_ARRAY_FIELD_FOREACH
#undef COMPARE_FIELD
#undef FIELD_MISMATCH

exit:
	oss_free(local_ctx);
	return match;
}

int AMDGV_API amdgv_reset_vf_arbiters(amdgv_dev_t dev, uint32_t idx_vf)
{
	struct amdgv_adapter *adapt;
	int ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;

	oss_mutex_lock(adapt->api_lock);

	if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->reset_vf_arbiters)
		ret = adapt->pp.pp_funcs->reset_vf_arbiters(adapt, idx_vf);

	oss_mutex_unlock(adapt->api_lock);

	return ret;
}
