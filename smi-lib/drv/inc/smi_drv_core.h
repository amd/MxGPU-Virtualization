/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SMI_DRV_CORE_H__
#define __SMI_DRV_CORE_H__

#include <amdgv_api.h>
#include <smi_drv_oss.h>
#include <smi_drv.h>

#include <smi_cmd.h>
#include <smi_drv_ioctl.h>

struct smi_ctx;
typedef int (*smi_cmd_func) (struct smi_ctx *, void *, void *, uint16_t, uint16_t);

extern struct oss_interface *smi_oss_funcs;

struct smi_cmd_entry {
	smi_cmd_func func;
	uint32_t cmd;
	int16_t in_buffer_len;
	int16_t out_buffer_len;
};

struct smi_vf_entry {
	uint64_t handle;
	uint64_t parent_handle;
	void *adev;
	uint32_t bdf;
	uint32_t idx;
};

struct smi_event_ctx {
	void *handle;
	struct amdgv_error_notifier *notifier;
	uint64_t event_id;  /* ESXi event ID*/
};

struct smi_ctx {
	mutex_t				ioctl_mutex;
	uint32_t			version;
	bool				privileged;
	uint8_t				padding[3];
	struct smi_cmd_entry		*tbl_cmd;
	uint32_t			max_cmd;
	struct smi_in_command		in_command;
	struct smi_out_response		out_response;
	struct {
		int64_t parent;
		uint64_t bdf;
		uint64_t handle;
		void *adev;
	} devices[SMI_MAX_DEVICES];
	uint32_t num_devices;
	uint32_t padding_2;
	struct smi_vf_entry *vf_map;

	/* save temporary value */
	union amdgv_vf_info vf_info;
	uint32_t padding_3;
	struct smi_event_ctx *event_ctx;
	bool mutex_flag;
	uint64_t shared_event_id;  /* ESXi: shared event ID */
	uint8_t event_readers;     /* ESXi: threads inside smi_read_event */
	bool releasing;            /* ESXi: context teardown in progress */
	uint8_t padding_4[1];
};

#define SMI_ASSIGN_FUNC(ctx, i, c, f, ins, outs) do {\
	ctx->tbl_cmd[i].cmd              = c;  \
	ctx->tbl_cmd[i].func             = &f; \
	ctx->tbl_cmd[i].in_buffer_len    = ins;  \
	ctx->tbl_cmd[i++].out_buffer_len = outs; \
	} while (0)

int smi_cmd_handshake(struct smi_ctx *ctx,
		void *inb, void *outb,
		uint16_t ins, uint16_t outs);

int smi_vf_map_update(struct smi_ctx *ctx, void *adev);
int smi_get_vf_index(struct smi_ctx *ctx, struct smi_vf_handle *vf_handle);
uint64_t smi_get_vf_handle(struct smi_ctx *ctx, smi_device_handle_t *dev, int idx_vf);

void smi_print(const char *fmt, ...);
int smi_vsnprintf(char *buf, uint32_t size, const char *fmt, ...);

#endif // __SMI_DRV_CORE_H__
