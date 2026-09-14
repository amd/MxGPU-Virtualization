/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include "smu_v15_0_8_internal.h"
#include "asic_reg/MP/mp_15_0_8_offset.h"
#include "asic_reg/MP/mp_15_0_8_sh_mask.h"

static const uint32_t this_block = AMDGV_POWER_BLOCK;

static int smu_v15_0_8_wait_for_response(struct amdgv_adapter *adapt, uint32_t *val,
					 enum amdgv_wait_for_types wait_type)
{
	int ret;
	uint32_t tmp;

	ret = amdgv_wait_for_smu_msg_resp(adapt, SOC15_REG_OFFSET_NAME(MP1, 0, regMP1_SMN_C2PMSG_41),
					  MP1_SMN_C2PMSG_41__CONTENT_MASK, 0,
					  AMDGV_TIMEOUT(TIMEOUT_SMU_REG), AMDGV_WAIT_CHECK_NE,
					  wait_type);

	tmp = RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_41));
	if (val)
		*val = tmp;

	return ret;
}

static void smu_v15_0_8_write_args(struct amdgv_adapter *adapt, struct smu_15_0_8_msg *msg)
{
	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_42), msg->in_arg[0]);
	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_43), msg->in_arg[1]);
	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_44), msg->in_arg[2]);
	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_45), msg->in_arg[3]);
}

static void smu_v15_0_8_read_args(struct amdgv_adapter *adapt, struct smu_15_0_8_msg *msg)
{
	msg->out_arg[0] = RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_42));
	msg->out_arg[1] = RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_43));
	msg->out_arg[2] = RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_44));
	msg->out_arg[3] = RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_45));
}

void smu_v15_0_8_put_timeout(struct amdgv_adapter *adapt, uint64_t elapsed)
{
	uint32_t msg  = RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_40));
	uint32_t resp = RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_41));
	uint32_t a0   = RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_42));
	uint32_t a1   = RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_43));
	uint32_t a2   = RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_44));
	uint32_t a3   = RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_45));

	amdgv_put_log_ext(AMDGV_PF_IDX, AMDGV_LOG_PP_SMU_TIMEOUT_EX,
			  elapsed, msg, resp, a0, a1, a2, a3);
}

static int smu_v15_0_8_msg_allowed_in_sync_flood(struct amdgv_adapter *adapt, uint32_t msg)
{
	int ret = false;
	/* TODO: get allowed messages */
	return ret;
}

int smu_v15_0_8_send_msg(struct amdgv_adapter *adapt, struct smu_15_0_8_msg *msg)
{
	int ret = 0;
	uint32_t resp;

	if (oss_atomic_read(adapt->in_sync_flood) &&
	    !smu_v15_0_8_msg_allowed_in_sync_flood(adapt, msg->id)) {
		AMDGV_ERROR("Skip msg:0x%x due to fatal error interrupt\n", msg->id);
		return AMDGV_FAILURE;
	}

	oss_mutex_lock(adapt->pp.smu_lock);

	amdgv_put_log_ext(AMDGV_PF_IDX, AMDGV_LOG_PP_SMU_WRITE_EX,
			  msg->id,
			  msg->in_arg[0], msg->in_arg[1],
			  msg->in_arg[2], msg->in_arg[3]);

	/* clear previous response */
	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_41), 0);

	smu_v15_0_8_write_args(adapt, msg);
	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_40), msg->id);

	ret = smu_v15_0_8_wait_for_response(adapt, &resp, AMDGV_WAIT_FOR_SMU_MSG_RESPONSE);
	if (ret) {
		ret = AMDGV_FAILURE;
		goto end;
	}

	if (resp != PPSMC_Result_OK) {
		amdgv_put_log_ext(AMDGV_PF_IDX, AMDGV_LOG_PP_SMU_FAIL_EX,
				msg->id, resp,
				msg->in_arg[0], msg->in_arg[1],
				msg->in_arg[2], msg->in_arg[3]);
		ret = AMDGV_FAILURE;
		goto end;
	}

	smu_v15_0_8_read_args(adapt, msg);

	amdgv_put_log_ext(AMDGV_PF_IDX, AMDGV_LOG_PP_SMU_RECV_EX,
			msg->id, resp,
			msg->out_arg[0], msg->out_arg[1],
			msg->out_arg[2], msg->out_arg[3]);
end:
	oss_mutex_unlock(adapt->pp.smu_lock);

	return ret;
}

void smu_v15_0_8_ack_irq(struct amdgv_adapter *adapt)
{
	uint32_t val;

	val = RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_IH_SW_INT_CTRL));
	val = REG_SET_FIELD(val, MP1_SMN_IH_SW_INT_CTRL, INT_ACK, 1);
	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_IH_SW_INT_CTRL), val);
}

static int smu_v15_0_8_send_test_msg(struct amdgv_adapter *adapt)
{
	struct smu_15_0_8_msg msg = { 0 };

	msg.id = PPSMC_MSG_TestMessage;
	msg.in_arg[0] = 0x22334455;

	if (smu_v15_0_8_send_msg(adapt, &msg))
		return AMDGV_FAILURE;

	if (msg.out_arg[0] != (msg.in_arg[0] + 1)) {
		AMDGV_ERROR("PMFW test message response mismatch\n");
		return AMDGV_FAILURE;
	}

	return 0;
}

static int smu_v15_0_8_get_version(struct amdgv_adapter *adapt, uint32_t *smu_version,
				   uint32_t *driver_if_version)
{
	struct smu_15_0_8_msg msg = { 0 };
	int ret;

	if (!smu_version && !driver_if_version)
		return AMDGV_FAILURE;

	if (smu_version) {
		msg.id = PPSMC_MSG_GetSmuVersion;
		ret = smu_v15_0_8_send_msg(adapt, &msg);
		if (ret)
			return ret;
		*smu_version = msg.out_arg[0];
	}

	if (driver_if_version) {
		msg.id = PPSMC_MSG_GetDriverIfVersion;
		ret = smu_v15_0_8_send_msg(adapt, &msg);
		if (ret)
			return ret;
		*driver_if_version = msg.out_arg[0];
	}

	return 0;
}

static int smu_v15_0_8_check_fw_status(struct amdgv_adapter *adapt)
{
	uint32_t mp1_flags, mp1_intr_en = 0;
	int retries_max = 2, retries;

	for (retries = 0; retries < retries_max; retries++) {
		mp1_flags = RREG32_PCIE_EXT(SOC15_REG_OFFSET_SMN(MP1, 0,
					    regMP1_CRU0_MP1_FIRMWARE_FLAGS, MP1_Public));

		if (mp1_flags == 0xffffffff) {
			AMDGV_WARN("MP1_FIRMWARE_FLAGS read 0xffffffff, try again...\n");
			oss_msleep(100);
			continue;
		}

		mp1_intr_en = REG_GET_FIELD(mp1_flags, MP1_CRU0_MP1_FIRMWARE_FLAGS,
					    INTERRUPTS_ENABLED);
		break;
	}

	AMDGV_DEBUG("Read MP1 FW flags: 0x%x, retries: %d\n", mp1_flags, retries);

	return mp1_intr_en ? 0 : AMDGV_FAILURE;
}

bool smu_v15_0_8_is_fw_alive(struct amdgv_adapter *adapt)
{
	int ret;

	ret = smu_v15_0_8_check_fw_status(adapt);
	if (ret)
		return false;

	ret = smu_v15_0_8_send_test_msg(adapt);
	if (ret)
		return false;

	return true;
}

static int smu_v15_0_8_enable_smu_features(struct amdgv_adapter *adapt)
{
	struct smu_15_0_8_msg msg = { 0 };
	int ret = 0;

	msg.id = PPSMC_MSG_EnableAllSmuFeatures;

	ret = smu_v15_0_8_send_msg(adapt, &msg);
	if (ret) {
		AMDGV_ERROR("Failed to Enable all SMU Features\n");
		ret = AMDGV_FAILURE;
	}

	return ret;
}

static void smu_v15_0_8_disable_smu_features(struct amdgv_adapter *adapt)
{
	struct smu_15_0_8_msg msg = { 0 };

	if (in_whole_gpu_reset())
		return;

	msg.id = PPSMC_MSG_PrepareForDriverUnload;

	smu_v15_0_8_send_msg(adapt, &msg);
}

static int smu_v15_0_8_irq_control(struct amdgv_adapter *adapt, bool enable)
{
	uint32_t val;

	if (enable) {
		AMDGV_DEBUG("Enable PMFW Interrupts\n");
		val = RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_IH_SW_INT));
		val = REG_SET_FIELD(val, MP1_SMN_IH_SW_INT, ID, 0xFE);
		val = REG_SET_FIELD(val, MP1_SMN_IH_SW_INT, VALID, 0);
		WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_IH_SW_INT), val);

		val = RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_IH_SW_INT_CTRL));
		val = REG_SET_FIELD(val, MP1_SMN_IH_SW_INT_CTRL, INT_MASK, 0);
		WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_IH_SW_INT_CTRL), val);
	} else {
		AMDGV_DEBUG("Disable PMFW Interrupts\n");
		val = RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_IH_SW_INT_CTRL));
		val = REG_SET_FIELD(val, MP1_SMN_IH_SW_INT_CTRL, INT_MASK, 0);
		WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_IH_SW_INT_CTRL), val);
	}

	return 0;
}

static int smu_v15_0_8_early_sw_init(struct amdgv_adapter *adapt)
{
	adapt->pp.smu_lock = oss_mutex_init();
	if (adapt->pp.smu_lock == OSS_INVALID_HANDLE) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_CREATE_MUTEX_FAIL, 0);
		return AMDGV_FAILURE;
	}

	return 0;
}

static int smu_v15_0_8_early_sw_fini(struct amdgv_adapter *adapt)
{
	if (adapt->pp.smu_lock) {
		oss_mutex_fini(adapt->pp.smu_lock);
		adapt->pp.smu_lock = OSS_INVALID_HANDLE;
	}

	return 0;
}

static int smu_v15_0_8_early_hw_init(struct amdgv_adapter *adapt)
{
	uint32_t pmfw_if_version;

	if (!smu_v15_0_8_is_fw_alive(adapt))
		return AMDGV_FAILURE;

	if (smu_v15_0_8_get_version(adapt, &adapt->pp.smu_fw_version, &pmfw_if_version))
		return AMDGV_FAILURE;

	amdgv_put_log_ext(AMDGV_PF_IDX, AMDGV_LOG_PP_SMU_VERSION,
			  (uint64_t)adapt->pp.smu_fw_version,
			  (uint64_t)pmfw_if_version,
			  (uint64_t)DRIVER_IF_SMU_V15_0_8_VERSION);

	if (adapt->psp.fw_info)
		adapt->psp.fw_info[AMDGV_FIRMWARE_ID__SMU] = adapt->pp.smu_fw_version;

	return 0;
}

static int smu_v15_0_8_early_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

const struct amdgv_init_func smu_v15_0_8_early_func = {
	.name = "smu_v15_0_8_early_func",
	.sw_init = smu_v15_0_8_early_sw_init,
	.sw_fini = smu_v15_0_8_early_sw_fini,
	.hw_init = smu_v15_0_8_early_hw_init,
	.hw_fini = smu_v15_0_8_early_hw_fini,
};

static int smu_v15_0_8_late_hw_init(struct amdgv_adapter *adapt)
{
	if (smu_v15_0_8_enable_smu_features(adapt))
		return AMDGV_FAILURE;

	smu_v15_0_8_irq_control(adapt, true);

	return 0;
}

static int smu_v15_0_8_late_hw_fini(struct amdgv_adapter *adapt)
{
	smu_v15_0_8_irq_control(adapt, false);
	smu_v15_0_8_disable_smu_features(adapt);

	return 0;
}

const struct amdgv_init_func smu_v15_0_8_late_func = {
	.name = "smu_v15_0_8_late_func",
	.hw_init = smu_v15_0_8_late_hw_init,
	.hw_fini = smu_v15_0_8_late_hw_fini,
};