/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_API_INTERNAL_H
#define AMDGV_API_INTERNAL_H

/* Driver & GPU is healthy */
#define SET_ADAPT_AND_CHECK_STATUS(adapt, dev)                                                \
	do {                                                                                  \
		if (dev == AMDGV_INVALID_HANDLE)                                              \
			return AMDGV_LOG_GPU_DEVICE_LOST;                                   \
		adapt = (struct amdgv_adapter *)dev;                                          \
		if (adapt->status == AMDGV_STATUS_SW_INIT)                                    \
			return AMDGV_LOG_GPU_NOT_INITIALIZED;                               \
		if (adapt->status != AMDGV_STATUS_HW_INIT)                                    \
			return AMDGV_LOG_DRIVER_DEV_INIT_FAIL;                              \
	} while (0)

/* GPU is in RMA state due to excessive bad pages, but otherwise HW is still healthy. */
#define SET_ADAPT_AND_CHECK_STATUS_NOT_LOST(adapt, dev)                                       \
	do {                                                                                  \
		if (dev == AMDGV_INVALID_HANDLE)                                              \
			return AMDGV_LOG_GPU_DEVICE_LOST;                                   \
		adapt = (struct amdgv_adapter *)dev;                                          \
		if (adapt->status == AMDGV_STATUS_SW_INIT)                                    \
			return AMDGV_LOG_GPU_NOT_INITIALIZED;                               \
		if (!(adapt->status == AMDGV_STATUS_HW_INIT) &&                               \
		    !(adapt->status == AMDGV_STATUS_HW_RMA)  &&                               \
		    !(adapt->status == AMDGV_STATUS_HW_HIVE_RMA))                             \
			return AMDGV_LOG_DRIVER_DEV_INIT_FAIL;                              \
	} while (0)

/* GPU is in a bad state. Limit driver to SW operations only */
#define SET_ADAPT_AND_CHECK_STATUS_MINIMAL(adapt, dev)                                        \
	do {                                                                                  \
		if (dev == AMDGV_INVALID_HANDLE)                                              \
			return AMDGV_LOG_GPU_DEVICE_LOST;                                   \
		adapt = (struct amdgv_adapter *)dev;                                          \
		if (adapt->status == AMDGV_STATUS_SW_INIT)                                    \
			return AMDGV_LOG_GPU_NOT_INITIALIZED;                               \
		if (!(adapt->status == AMDGV_STATUS_HW_INIT)     &&                           \
		    !(adapt->status == AMDGV_STATUS_HW_RMA)      &&                           \
		    !(adapt->status == AMDGV_STATUS_HW_HIVE_RMA) &&                           \
		    !(adapt->status == AMDGV_STATUS_HW_LOST))                                 \
			return AMDGV_LOG_DRIVER_DEV_INIT_FAIL;                              \
	} while (0)

int amdgv_int_allocate_vf(struct amdgv_adapter *adapt, struct amdgv_vf_option *option);
int amdgv_int_free_vf(struct amdgv_adapter *adapt, uint32_t idx_vf);
int amdgv_int_set_vf_number(struct amdgv_adapter *adapt, uint32_t num_vf);

int amdgv_int_ras_trigger_error(struct amdgv_adapter *adapt, struct amdgv_smi_ras_error_inject_info *data);
int amdgv_int_ras_ta_load(struct amdgv_adapter *adapt, struct amdgv_smi_cmd_ras_ta_load *data);
int amdgv_int_ras_ta_unload(struct amdgv_adapter *adapt, struct amdgv_smi_cmd_ras_ta_unload *data);

int amdgv_int_alloc_dump_cu_resource_memory(struct amdgv_adapter *adapt,
		struct amdgv_dump_cu_resource_size *dump_cu_resource_size, struct amdgv_dump_cu_resource_memory *output_data);
int amdgv_int_dump_cu_data(struct amdgv_adapter *adapt);
int amdgv_int_set_dump_cu_info(struct amdgv_adapter *adapt,
		enum AMDGV_CU_DATA_TYPE cu_dump_type, uint32_t xcc_id, bool use_extra_ring);
void amdgv_int_free_dump_cu_resource_memory(struct amdgv_adapter *adapt);

int amdgv_int_stop_to_pf_helper(struct amdgv_adapter *adapt);


#endif // AMDGV_API_INTERNAL_H
