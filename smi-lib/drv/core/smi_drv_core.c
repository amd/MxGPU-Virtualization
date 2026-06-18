/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <smi_drv.h>
#include <smi_drv_core.h>
#include <smi_drv_core_api.h>
#include <amdgv_gpumon.h>

#include "smi_drv_oss_wrapper.h"

struct oss_interface *smi_oss_funcs;
struct smi_shim_interface *smi_shim_funcs;

/* ESXi only: set by smi_drain_in_flight_ioctls during unload. */
volatile bool smi_shutting_down = false;

#define smi_min(a, b) ((a) < (b) ? (a) : (b))

const char * const smi_shim_inf_name[] = {
	"lock_device_list", "get_device_list",
	"unlock_device_list", "get_shim_log_level",

	"set_file_private_data", "get_file_private_data",
	"verify_file_descriptor",
	"generate_date_string",
	"create_event", "read_event", "destroy_event",
	"put_handle",
	"get_pcie_confs", "get_device_data", "create_hash_64",
	"get_driver_version", "get_driver_id",
	"get_profile_info", "get_driver_date",
	"get_driver_model",
	"get_metric_table", "get_eeprom_table",
	"get_partition", "get_partition_global", "get_cper_data"
};

void smi_print(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);

	smi_oss_funcs->print(0, fmt, args);

	va_end(args);
}

int smi_vsnprintf(char *buf, uint32_t size, const char *fmt, ...)
{
	int num;
	va_list args;

	va_start(args, fmt);

	num = smi_oss_funcs->vsnprintf(buf, size, fmt, args);

	va_end(args);

	return num;
}

int smi_core_init(struct oss_interface *oss_interface,
		struct smi_shim_interface *shim_interface)
{
	int ret = 0;
	func_t *p;
	int i, length, inf_name_num;

	length = sizeof(struct smi_shim_interface) / sizeof(func_t);
	inf_name_num = sizeof(smi_shim_inf_name) / sizeof(char *);

	smi_oss_funcs = oss_interface;
	smi_shim_funcs = shim_interface;

	/* smi requires full shim interface */
	if (inf_name_num != length) {
		smi_print("error: SMI shim interface num not match interface name num\n");
		return -SMI_ENOENT;
	}

	for (p = (func_t *)shim_interface, i = 0; i < length; i++) {
		if (p[i] == NULL) {
			smi_print("error: no implementation for interface: %s\n",
				    smi_shim_inf_name[i]);
			ret = -SMI_ENOENT;
		}
	}

	if (oss_interface->copy_from_user == NULL)
		smi_print("error: copy_from_user is not implemented\n");

	if (oss_interface->copy_to_user == NULL)
		smi_print("error: copy_to_user is not implemented\n");

	return ret;
}

int smi_core_fini(void)
{
	smi_oss_funcs = NULL;
	smi_shim_funcs = NULL;

	return 0;
}

int smi_core_open(file_t filp, bool is_privileged)
{
	struct smi_ctx *ctx;
	struct smi_device_data *dev_list = NULL;
	uint32_t dev_list_size;

	struct amdgv_init_data *data;
	struct amdgv_vbios_info vbios;
	uint64_t pf_id;

	uint32_t i;
	int ret = 0;

	ctx = smi_oss_funcs->alloc_small_zero_memory(sizeof(struct smi_ctx));
	if (!ctx) {
		ret = -SMI_ENOMEM;
		goto failed;
	}

	dev_list = smi_oss_funcs->alloc_small_zero_memory(
			AMDGV_MAX_GPU_NUM * sizeof(struct smi_device_data));
	if (!dev_list) {
		ret = -SMI_ENOMEM;
		goto failed;
	}

	/* Check the privilege level */
	ctx->privileged = is_privileged;

	ctx->ioctl_mutex = smi_oss_funcs->mutex_init();
	if (!ctx->ioctl_mutex) {
		ret = -SMI_ENOMEM;
		goto failed;
	}

	smi_set_file_private_data(filp, ctx);

	/* obtain all adapter information from shim driver */
	smi_lock_device_list();
	smi_get_device_list(dev_list, &dev_list_size);
	smi_unlock_device_list();

	for (i = 0; i < dev_list_size; i++) {
		if (amdgv_gpumon_get_vbios_info(dev_list[i].adev, &vbios))
			vbios.serial = 0ULL;

		data = &dev_list[i].init_data;
		ctx->devices[i].bdf = data->info.bdf;

		/* PF handle format ( parent = pf_id, self = pf_id ) */
		pf_id = smi_hash_64(data->info.bdf ^ vbios.serial, 32);
		ctx->devices[i].handle = pf_id | pf_id << 32;
		ctx->devices[i].parent = dev_list[i].parent;
		ctx->devices[i].adev = dev_list[i].adev;
	}
	/* dev_list is a temporary variable on heap */
	smi_oss_funcs->free_small_memory(dev_list);
	dev_list = NULL;

	ctx->num_devices = (uint8_t)dev_list_size;

	/* create hashmap */
	ctx->vf_map = smi_oss_funcs->alloc_small_zero_memory(
			SMI_MAX_VF_COUNT * ctx->num_devices * sizeof(struct smi_vf_entry));

	if (!ctx->vf_map) {
		ret = -SMI_ENOMEM;
		goto failed;
	}

	for (i = 0; i < ctx->num_devices; i++)
		smi_vf_map_update(ctx, ctx->devices[i].adev);

	ctx->event_ctx = smi_oss_funcs->alloc_small_zero_memory(
		ctx->num_devices * sizeof(struct smi_event_ctx));

	if (!ctx->event_ctx) {
		ret = -SMI_ENOMEM;
		goto failed;
	}

	return 0;

failed:
	if (dev_list)
		smi_oss_funcs->free_small_memory(dev_list);
	if (ctx) {
		if (ctx->vf_map)
			smi_oss_funcs->free_small_memory(ctx->vf_map);
		if (ctx->event_ctx)
			smi_oss_funcs->free_small_memory(ctx->event_ctx);
		if (ctx->ioctl_mutex)
			smi_oss_funcs->mutex_fini(ctx->ioctl_mutex);

		smi_oss_funcs->free_small_memory(ctx);
	}

	return ret;
}

int smi_core_release(file_t filp)
{
	struct smi_ctx *ctx = NULL;
	uint32_t i;

	/* Recover the context from the file descriptor */
	smi_get_file_private_data(filp, &ctx);

	/* lock ioctl to prevent access during ioctl */
	smi_oss_funcs->mutex_lock(ctx->ioctl_mutex);

	if (ctx->tbl_cmd)
		smi_oss_funcs->free_small_memory(ctx->tbl_cmd);

	smi_oss_funcs->mutex_unlock(ctx->ioctl_mutex);

	/* Destroy active notifiers, then free. Skip destroy on shutdown
	 * (adev may already be torn down); always free. */
	if (ctx->event_ctx) {
		if (smi_shim_funcs && !smi_shutting_down) {
			for (i = 0; i < ctx->num_devices; i++) {
				if (ctx->event_ctx[i].notifier != NULL)
					smi_event_destroy(ctx, ctx->devices[i].adev,
							  ctx->devices[i].handle);
			}
		}
		smi_oss_funcs->free_small_memory(ctx->event_ctx);
	}

	smi_oss_funcs->free_small_memory(ctx->vf_map);
	smi_oss_funcs->mutex_fini(ctx->ioctl_mutex);

	smi_oss_funcs->free_small_memory(ctx);

	return 0;
}

int smi_core_ioctl_handler(file_t filp, unsigned int cmd, void *arg)
{
	long ret = 0;
	struct smi_ctx *ctx = NULL;
	unsigned int i;
	uint32_t code = 0;
	int16_t in_len = 0;
	int16_t out_len = 0;
	bool early_unlocked = false;
	void *in_buf = NULL;
	void *out_buf = NULL;
	int status = 0;

	struct smi_ioctl_cmd *uptr = (struct smi_ioctl_cmd *) arg;

	if (uptr == NULL)
		return -SMI_EINVAL;

	/* Check that the file descriptor is correct */
	if (smi_verify_file_descriptor(filp))
		return -SMI_EINVAL;

	/* Recover the context from the file descriptor */
	smi_get_file_private_data(filp, &ctx);
	smi_oss_funcs->mutex_lock(ctx->ioctl_mutex);

	/* decode the ioctl */

	/* Copy header from user */
	if (smi_oss_funcs->copy_from_user(&ctx->in_command, &uptr->in_hdr,
			sizeof(struct smi_in_hdr))) {
		ret = -SMI_EFAULT;
		goto unlock_mutex;
	}

	/* Snapshot the input header into per-call locals. ctx->in_command is
	 * per-fd and a concurrent ioctl on the same fd can overwrite it while
	 * this thread blocks after the early mutex_unlock below, so every
	 * subsequent decision based on hdr.{code,in_len,out_len} MUST use
	 * these snapshots rather than re-reading the shared buffer — most
	 * critically the out_len bound used to gate the out_response memset,
	 * which would otherwise sign-extend a racing negative value into a
	 * giant kernel-memory wipe.
	 */
	code = ctx->in_command.hdr.code;
	in_len = ctx->in_command.hdr.in_len;
	out_len = ctx->in_command.hdr.out_len;
	ctx->mutex_flag=true;
	if (code == SMI_CMD_CODE_CREATE_EVENT || code == SMI_CMD_CODE_READ_EVENT
					|| code == SMI_CMD_CODE_DESTROY_EVENT) {
		ctx->mutex_flag=false;
		early_unlocked = true;
		smi_oss_funcs->mutex_unlock(ctx->ioctl_mutex);
	}

	if (in_len < 0 || out_len < 0) {
		ret = -SMI_EINVAL;
		goto unlock_mutex;
	}

	/* check privilege level */
	if (!ctx->privileged) {
		ret = -SMI_EACCES;
		goto unlock_mutex;
	}

	/* if the context func table is not initialized, and cmd is not
	* the handshake, return error
	*/
	if (!ctx->tbl_cmd) {
		if (code != SMI_CMD_CODE_HANDSHAKE) {
			ret = -SMI_EACCES;
			goto unlock_mutex;
		}

		if (smi_oss_funcs->copy_from_user(&ctx->in_command.payload,
				&uptr->payload,
				smi_min((size_t) in_len,
					sizeof(struct smi_handshake)))) {
			ret = -SMI_EFAULT;
			goto unlock_mutex;
		}

		ctx->out_response.hdr.status =
			smi_cmd_handshake(ctx,
				ctx->in_command.payload,
				ctx->out_response.payload,
				in_len,
				out_len);

		if (ctx->out_response.hdr.status &&
			ctx->out_response.hdr.status !=
				SMI_STATUS_NOT_SUPPORTED) {
			ret = -SMI_EIO;
			goto return_status;
		}

		if (smi_oss_funcs->copy_to_user(&uptr->payload,
				&ctx->out_response.payload,
				sizeof(struct smi_handshake))) {
			ret = -SMI_EFAULT;
			goto unlock_mutex;
		}
		goto return_status;
	}

	/* find the entry */
	for (i = 0; i < ctx->max_cmd; i++)
		if (ctx->tbl_cmd[i].cmd == code)
			break;

	if (i == ctx->max_cmd) {
		ret = -SMI_EINVAL;
		goto unlock_mutex;
	}

	if (out_len  > sizeof(ctx->out_response.payload)) {
		ret = -SMI_ENOMEM;
		goto unlock_mutex;
	}

	/* Use per-call payload buffers, not the per-fd ctx buffers. For EVENT
	 * commands the ioctl_mutex was already dropped above, so a concurrent
	 * ioctl on the same fd could overwrite ctx->in_command.payload /
	 * ctx->out_response.payload while this handler runs. Private heap
	 * buffers (4096B each — too large for the kernel stack) keep the input
	 * arguments and output stable across the unlocked window.
	 */
	in_buf = smi_oss_funcs->alloc_small_zero_memory(
			sizeof(ctx->in_command.payload));
	out_buf = smi_oss_funcs->alloc_small_zero_memory(
			sizeof(ctx->out_response.payload));
	if (in_buf == NULL || out_buf == NULL) {
		ret = -SMI_ENOMEM;
		goto unlock_mutex;
	}

	/* copy the payload */
	if (smi_oss_funcs->copy_from_user(in_buf, &uptr->payload,
			smi_min(in_len,
				ctx->tbl_cmd[i].in_buffer_len))) {
		ret = -SMI_EFAULT;
		goto unlock_mutex;
	}

	/* execute the command */
	status = ctx->tbl_cmd[i].func(ctx,
			in_buf,
			out_buf,
			in_len,
			out_len);

	if (ctx->out_response.hdr.status &&
	    ctx->out_response.hdr.status != SMI_STATUS_NO_DATA &&
	    ctx->out_response.hdr.status != SMI_STATUS_TIMEOUT) {
		ret = -SMI_EIO;
		goto generic_return_status;
	}

	if (smi_oss_funcs->copy_to_user(&uptr->payload, out_buf,
			smi_min(out_len,
				ctx->tbl_cmd[i].out_buffer_len))) {
		ret = -SMI_EFAULT;
		goto unlock_mutex;
	}

generic_return_status:
	smi_oss_funcs->copy_to_user(&uptr->out_hdr.status,
			&status, sizeof(int));
	goto unlock_mutex;

return_status:
	smi_oss_funcs->copy_to_user(&uptr->out_hdr.status,
			&ctx->out_response.hdr.status, sizeof(int));

unlock_mutex:
	if (!early_unlocked)
		smi_oss_funcs->mutex_unlock(ctx->ioctl_mutex);
	if (in_buf)
		smi_oss_funcs->free_small_memory(in_buf);
	if (out_buf)
		smi_oss_funcs->free_small_memory(out_buf);
	return ret;
};

int smi_vf_map_update(struct smi_ctx *ctx, void *dev)
{
	uint32_t i;
	unsigned j;
	union amdgv_dev_info dev_info;
	amdgv_dev_t adev = (amdgv_dev_t) dev;
	uint64_t serial;
	uint64_t vf_id;
	uint64_t pf_id;
	struct smi_vf_entry *vf_ptr;

	for (i = 0; i < ctx->num_devices; i++)
		if (ctx->devices[i].adev == adev)
			break;

	if (i == ctx->num_devices)
		return -SMI_EINVAL;

	if (amdgv_get_dev_info(adev, AMDGV_GET_ENABLED_VF_NUM, &dev_info))
		return -SMI_EINVAL;

	if (!amdgv_gpumon_get_asic_serial(adev, &serial))
		serial = 0ULL;

	for (j = 0; j < dev_info.vf.num_enabled_vf; j++) {
		if (!amdgv_get_vf_info(adev, j, AMDGV_GET_VF_BDF, &ctx->vf_info)) {
			vf_ptr = &(ctx->vf_map[i*SMI_MAX_VF_COUNT + j]);

			vf_ptr->adev = adev;
			vf_ptr->parent_handle = ctx->devices[i].handle;

			vf_ptr->bdf = ctx->vf_info.id.bdf;
			/* VF handle format ( parent = pf_id, self = vf_id ) */
			vf_id = smi_hash_64(serial ^
				((uint64_t) ctx->vf_info.id.bdf |
				((uint64_t) ctx->devices[i].bdf << 32)), 32);
			pf_id = ctx->devices[i].handle  << 32;
			vf_ptr->handle = pf_id | vf_id;
			vf_ptr->idx = j;
		}
	}

	return 0;
};

int smi_get_vf_index(struct smi_ctx *ctx, struct smi_vf_handle *vf_handle)
{
	uint32_t i;

	for (i = 0; i < ctx->num_devices * SMI_MAX_VF_COUNT; i++)
		if ((ctx->vf_map[i].handle) &&
				(ctx->vf_map[i].handle == vf_handle->handle))
			return ctx->vf_map[i].idx;

	for (i = 0; i < ctx->num_devices; i++)
		if (ctx->devices[i].handle == vf_handle->handle)
			return SMI_MAX_VF_COUNT - 1;

	return -SMI_EINVAL;
}

uint64_t smi_get_vf_handle(struct smi_ctx *ctx,
			smi_device_handle_t *dev, int idx_vf)
{
	uint64_t tmp;
	uint32_t gpu;

	if (idx_vf < 0 || idx_vf >= SMI_MAX_VF_COUNT)
		return 0;

	for (gpu = 0; gpu < ctx->num_devices; gpu++) {
		if (ctx->devices[gpu].handle == dev->handle)
			break;
	}

	if (gpu == ctx->num_devices)
		return 0;

	tmp = ctx->vf_map[gpu * SMI_MAX_VF_COUNT + idx_vf].handle;

	if (tmp == 0) {
		smi_vf_map_update(ctx, ctx->devices[gpu].adev);
		tmp = ctx->vf_map[gpu * SMI_MAX_VF_COUNT + idx_vf].handle;
	}

	return tmp;
}
