/*
 * Copyright Advanced Micro Devices, Inc. All rights reserved.
 */

#include "amdgv.h"
#include "amdgv_device.h"
#include "amdgv_psp_gfx_if.h"
#include "amdgv_oss_wrapper.h"
#include "amdgv_iovm_drv.h"

static const uint32_t this_block = AMDGV_IOVM_DRV_BLOCK;

const char *const amdgv_iovm_drv_access_mask_name[] = {
	"VF MMR Read Access",           // Bit[0]
	"VF MMR Write Access",          // Bit[1]
	"", "", "", "", "", "", "", "", // Reserved [2:9]
	"", "", "", "", "", "", "", "", // Reserved [10:17]
	"", "", "", "", "", "", "",     // Reserved [18:24]
	"VF FB Disable",                // Bit[25]
	"VF FB Enable",                 // Bit[26]
	"VF Doorbell Disable",          // Bit[27]
	"VF Doorbell Enable",           // Bit[28]
	"VF MMR No Access",             // Bit[29]
	"VF MMR Range Access",          // Bit[30]
	"VF MMR Full Access",           // Bit[31]
};

uint32_t amdgv_iovm_drv_sw_init(struct amdgv_adapter *adapt)
{
	adapt->iovm_drv.gpuiov_cmd_lock = oss_mutex_init();
	if (adapt->iovm_drv.gpuiov_cmd_lock == OSS_INVALID_HANDLE) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_CREATE_MUTEX_FAIL, 0);
		return AMDGV_FAILURE;
	}

	adapt->iovm_drv.gpuiov_status_lock = oss_mutex_init();
	if (adapt->iovm_drv.gpuiov_status_lock == OSS_INVALID_HANDLE) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_CREATE_MUTEX_FAIL, 0);
		return AMDGV_FAILURE;
	}

	adapt->iovm_drv.iovm_drv_gpuiov_cmd_resp_mem =
		amdgv_memmgr_alloc_align(&adapt->memmgr_pf, IOVM_DRV_GPUIOV_RESP_SIZE, IOVM_DRV_GPUIOV_RESP_ALIGNMENT, MEM_PSP_SRIOV_DRV);
	if (!adapt->iovm_drv.iovm_drv_gpuiov_cmd_resp_mem) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_ALLOC_SYSTEM_MEM_FAIL, IOVM_DRV_GPUIOV_RESP_SIZE);
		return AMDGV_FAILURE;
	}

	adapt->iovm_drv.iovm_drv_gpuiov_status_resp_mem =
		amdgv_memmgr_alloc_align(&adapt->memmgr_pf, IOVM_DRV_GPUIOV_RESP_SIZE, IOVM_DRV_GPUIOV_RESP_ALIGNMENT, MEM_PSP_SRIOV_DRV);
	if (!adapt->iovm_drv.iovm_drv_gpuiov_status_resp_mem) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_ALLOC_SYSTEM_MEM_FAIL, IOVM_DRV_GPUIOV_RESP_SIZE);
		return AMDGV_FAILURE;
	}

	adapt->iovm_drv.enabled = true;

	return 0;
}

uint32_t amdgv_iovm_drv_sw_fini(struct amdgv_adapter *adapt)
{
	if (adapt->iovm_drv.gpuiov_cmd_lock != OSS_INVALID_HANDLE) {
		oss_mutex_fini(adapt->iovm_drv.gpuiov_cmd_lock);
		adapt->iovm_drv.gpuiov_cmd_lock = OSS_INVALID_HANDLE;
	}

	if (adapt->iovm_drv.gpuiov_status_lock != OSS_INVALID_HANDLE) {
		oss_mutex_fini(adapt->iovm_drv.gpuiov_status_lock);
		adapt->iovm_drv.gpuiov_status_lock = OSS_INVALID_HANDLE;
	}

	if (NULL != adapt->iovm_drv.iovm_drv_gpuiov_cmd_resp_mem) {
		amdgv_memmgr_free(adapt->iovm_drv.iovm_drv_gpuiov_cmd_resp_mem);
		adapt->iovm_drv.iovm_drv_gpuiov_cmd_resp_mem = NULL;
	}

	if (NULL != adapt->iovm_drv.iovm_drv_gpuiov_status_resp_mem) {
		amdgv_memmgr_free(adapt->iovm_drv.iovm_drv_gpuiov_status_resp_mem);
		adapt->iovm_drv.iovm_drv_gpuiov_status_resp_mem = NULL;
	}

	adapt->iovm_drv.enabled = false;

	return 0;
}

uint32_t amdgv_iovm_drv_hw_init(struct amdgv_adapter *adapt)
{
	return amdgv_iovm_drv_query_version(adapt);
}

uint32_t amdgv_iovm_drv_query_version(struct amdgv_adapter *adapt)
{
	enum psp_status ret;
	struct psp_cmd_km iovm_drv_km_cmd = { 0 };
	struct psp_gfx_resp psp_resp = { 0 };

	struct iovm_drv_query_version_resp *pResp = NULL;

	iovm_drv_km_cmd.cmd_id = PSP_CMD_KM_TYPE__SRIOV_DRIVER_PASSTHROUGH;
	iovm_drv_km_cmd.cmd.cmd_sriov_drv.sriov_drv_int_ver = IOVM_DRV_INTF_VERSION;
	iovm_drv_km_cmd.cmd.cmd_sriov_drv.command_id = IOVM_DRV_CMD_QUERY_VERSION;

	/* Submit iovm drv cmd */
	ret = amdgv_psp_cmd_km_submit(adapt, &iovm_drv_km_cmd, &psp_resp);

	pResp = (struct iovm_drv_query_version_resp *)&psp_resp.uresp;
	AMDGV_INFO("SR-IOV Drv Query Version Resp. Major Version:%d, Minor Version:%d\n",
		pResp->major_version, pResp->minor_version);

	if (ret != PSP_STATUS__SUCCESS || (pResp->major_version == 0 && pResp->minor_version == 0)) {
		AMDGV_ERROR("SR-IOV Drv Query Version failed.\n");
		ret = PSP_STATUS__ERROR_GENERIC;
	}

	return ret;
}

uint32_t amdgv_iovm_drv_vf_access(struct amdgv_adapter *adapt, uint32_t vf_idx, uint32_t access_mode)
{
	enum psp_status ret;
	struct psp_cmd_km iovm_drv_km_cmd = { 0 };
	struct psp_gfx_resp psp_resp = { 0 };
	struct iovm_drv_vf_access_command *vf_access_cmd_buf =
		(struct iovm_drv_vf_access_command *)&iovm_drv_km_cmd.cmd.cmd_sriov_drv.command_buf;
	struct iovm_drv_vf_access_resp *pResp = NULL;
	uint32_t i;

	if (vf_idx > adapt->num_vf) {
		AMDGV_ERROR("SR-IOV Drv Vf Access: invalid vf index.\n");
		return PSP_STATUS__ERROR_GENERIC;
	}

	for (i = 0; i < sizeof(access_mode)*8; i++) {
		if (access_mode & BIT(i))
			AMDGV_DEBUG("Access mask %s set\n", amdgv_iovm_drv_access_mask_name[i]);
	}

	iovm_drv_km_cmd.cmd_id = PSP_CMD_KM_TYPE__SRIOV_DRIVER_PASSTHROUGH;
	iovm_drv_km_cmd.cmd.cmd_sriov_drv.sriov_drv_int_ver = IOVM_DRV_INTF_VERSION;
	iovm_drv_km_cmd.cmd.cmd_sriov_drv.command_id = IOVM_DRV_CMD_VF_ACCESS;

	vf_access_cmd_buf->version = IOVM_DRV_VF_ACCESS_CMD_VERSION;
	vf_access_cmd_buf->vf_access_cmd.vf_access_cmd_v1_0.vf_id = vf_idx;
	vf_access_cmd_buf->vf_access_cmd.vf_access_cmd_v1_0.vf_access_mode = access_mode;

	/* Submit iovm drv cmd */
	ret = amdgv_psp_cmd_km_submit(adapt, &iovm_drv_km_cmd, &psp_resp);

	pResp = (struct iovm_drv_vf_access_resp *)&psp_resp.uresp;
	AMDGV_DEBUG("SR-IOV Drv Vf Access Resp. version:%d, size:%d, result:%d, cmd_status:%d\n",
		pResp->version, pResp->size, pResp->result, pResp->vf_access_resp_v1_0.vf_access_cmd_status);

	if (ret == PSP_STATUS__SUCCESS &&
	    (pResp->result != IOVM_DRV_CMD_SUCCESS ||
	     pResp->vf_access_resp_v1_0.vf_access_cmd_status != IOVM_DRV_CMD_SUCCESS)) {
		AMDGV_ERROR("SR-IOV Drv Vf Access failed (result=%d cmd_status=%d).\n",
			    pResp->result,
			    pResp->vf_access_resp_v1_0.vf_access_cmd_status);
		ret = PSP_STATUS__ERROR_GENERIC;
	}

	return ret;
}

uint32_t amdgv_iovm_drv_gpuiov_set_command(struct amdgv_adapter *adapt, uint32_t xcd_bitmask, uint32_t mmsch_bitmask, uint32_t vf_id, uint32_t next_vf_id, uint32_t gpuiov_cmd)
{
	enum psp_status ret;
	struct psp_cmd_km iovm_drv_km_cmd = { 0 };
	struct iovm_drv_gpuiov_cmd gpuiov_cmd_buf = { 0 };
	uint64_t gpu_addr = 0;

	if (!adapt->iovm_drv.iovm_drv_gpuiov_cmd_resp_mem) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_ALLOC_SYSTEM_MEM_FAIL, IOVM_DRV_GPUIOV_RESP_SIZE);
		return PSP_STATUS__ERROR_GENERIC;
	}

	oss_mutex_lock(adapt->iovm_drv.gpuiov_cmd_lock);
	iovm_drv_km_cmd.cmd_id = PSP_CMD_KM_TYPE__SRIOV_DRIVER_PASSTHROUGH;
	iovm_drv_km_cmd.cmd.cmd_sriov_drv.sriov_drv_int_ver = IOVM_DRV_INTF_VERSION;
	iovm_drv_km_cmd.cmd.cmd_sriov_drv.command_id = IOVM_DRV_CMD_GPUIOV_CMD;
	// TODO: add mmsch suppot
	gpuiov_cmd_buf.version = IOVM_DRV_GPUIOV_CMD_VERSION;
	gpuiov_cmd_buf.gpuiov_cmd.gpuiov_cmd_v1_0.xcd_bitmask = xcd_bitmask;
	gpuiov_cmd_buf.gpuiov_cmd.gpuiov_cmd_v1_0.vf_id = vf_id;
	gpuiov_cmd_buf.gpuiov_cmd.gpuiov_cmd_v1_0.next_vf_id = next_vf_id;
	gpuiov_cmd_buf.gpuiov_cmd.gpuiov_cmd_v1_0.gpuiov_cmd = gpuiov_cmd;
	gpu_addr = amdgv_memmgr_get_gpu_addr(adapt->iovm_drv.iovm_drv_gpuiov_cmd_resp_mem);
	gpuiov_cmd_buf.gpuiov_cmd.gpuiov_cmd_v1_0.resp_buf_addr_hi = upper_32_bits(gpu_addr);
	gpuiov_cmd_buf.gpuiov_cmd.gpuiov_cmd_v1_0.resp_buf_addr_lo = lower_32_bits(gpu_addr);
	gpuiov_cmd_buf.gpuiov_cmd.gpuiov_cmd_v1_0.resp_buf_size = IOVM_DRV_GPUIOV_RESP_SIZE;

	oss_memcpy(&iovm_drv_km_cmd.cmd.cmd_sriov_drv.command_buf, &gpuiov_cmd_buf, sizeof(struct iovm_drv_gpuiov_cmd));

	ret = amdgv_psp_cmd_km_submit(adapt, &iovm_drv_km_cmd, NULL);

	// TODO: Verify command response
	oss_mutex_unlock(adapt->iovm_drv.gpuiov_cmd_lock);
	return ret;
}

uint32_t amdgv_iovm_drv_gpuiov_query_status(struct amdgv_adapter *adapt, uint32_t xcd_bitmask, uint32_t mmsch_bitmask)
{
	enum psp_status ret;
	struct psp_cmd_km iovm_drv_km_cmd = { 0 };
	struct iovm_drv_gpuiov_query_status gpuiov_status_buf = { 0 };
	struct iovm_drv_gpuiov_resp *gpuiov_resp = NULL;
	uint64_t gpu_addr = 0;

	if (!adapt->iovm_drv.iovm_drv_gpuiov_status_resp_mem) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_ALLOC_SYSTEM_MEM_FAIL, IOVM_DRV_GPUIOV_RESP_SIZE);
		return PSP_STATUS__ERROR_GENERIC;
	}

	oss_mutex_lock(adapt->iovm_drv.gpuiov_status_lock);
	iovm_drv_km_cmd.cmd_id = PSP_CMD_KM_TYPE__SRIOV_DRIVER_PASSTHROUGH;
	iovm_drv_km_cmd.cmd.cmd_sriov_drv.sriov_drv_int_ver = IOVM_DRV_INTF_VERSION;
	iovm_drv_km_cmd.cmd.cmd_sriov_drv.command_id = IOVM_DRV_CMD_GPUIOV_STATUS;

	gpuiov_status_buf.version = IOVM_DRV_GPUIOV_CMD_VERSION;
	gpuiov_status_buf.gpuiov_query_status.gpuiov_query_status_v1_0.xcd_bitmask = xcd_bitmask;
	gpu_addr = amdgv_memmgr_get_gpu_addr(adapt->iovm_drv.iovm_drv_gpuiov_status_resp_mem);
	gpuiov_status_buf.gpuiov_query_status.gpuiov_query_status_v1_0.resp_buf_addr_hi = upper_32_bits(gpu_addr);
	gpuiov_status_buf.gpuiov_query_status.gpuiov_query_status_v1_0.resp_buf_addr_lo = lower_32_bits(gpu_addr);
	gpuiov_status_buf.gpuiov_query_status.gpuiov_query_status_v1_0.resp_buf_size = IOVM_DRV_GPUIOV_RESP_SIZE;

	oss_memcpy(&iovm_drv_km_cmd.cmd.cmd_sriov_drv.command_buf, &gpuiov_status_buf, sizeof(struct iovm_drv_gpuiov_query_status));

	/* Submit iovm drv cmd */
	ret = amdgv_psp_cmd_km_submit(adapt, &iovm_drv_km_cmd, NULL);

	gpuiov_resp = (struct iovm_drv_gpuiov_resp *)(amdgv_memmgr_get_cpu_addr(adapt->iovm_drv.iovm_drv_gpuiov_status_resp_mem));
	// TODO: Verify command response
	oss_mutex_unlock(adapt->iovm_drv.gpuiov_status_lock);
	return ret;
}
