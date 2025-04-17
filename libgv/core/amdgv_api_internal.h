/*
 * Copyright (c) 2020-2022 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef AMDGV_API_INTERNAL_H
#define AMDGV_API_INTERNAL_H

/* Driver & GPU is healthy */
#define SET_ADAPT_AND_CHECK_STATUS(adapt, dev)                                                \
	do {                                                                                  \
		if (dev == AMDGV_INVALID_HANDLE)                                              \
			return AMDGV_ERROR_GPU_DEVICE_LOST;                                   \
		adapt = (struct amdgv_adapter *)dev;                                          \
		if (adapt->status == AMDGV_STATUS_SW_INIT)                                    \
			return AMDGV_ERROR_GPU_NOT_INITIALIZED;                               \
		if (adapt->status != AMDGV_STATUS_HW_INIT)                                    \
			return AMDGV_ERROR_DRIVER_DEV_INIT_FAIL;                              \
	} while (0)

/* GPU is in RMA state due to excessive bad pages, but otherwise HW is still healthy. */
#define SET_ADAPT_AND_CHECK_STATUS_NOT_LOST(adapt, dev)                                       \
	do {                                                                                  \
		if (dev == AMDGV_INVALID_HANDLE)                                              \
			return AMDGV_ERROR_GPU_DEVICE_LOST;                                   \
		adapt = (struct amdgv_adapter *)dev;                                          \
		if (adapt->status == AMDGV_STATUS_SW_INIT)                                    \
			return AMDGV_ERROR_GPU_NOT_INITIALIZED;                               \
		if (!(adapt->status == AMDGV_STATUS_HW_INIT) &&                               \
		    !(adapt->status == AMDGV_STATUS_HW_RMA)  &&                               \
		    !(adapt->status == AMDGV_STATUS_HW_HIVE_RMA))                             \
			return AMDGV_ERROR_DRIVER_DEV_INIT_FAIL;                              \
	} while (0)

/* GPU is in a bad state. Limit driver to SW operations only */
#define SET_ADAPT_AND_CHECK_STATUS_MINIMAL(adapt, dev)                                        \
	do {                                                                                  \
		if (dev == AMDGV_INVALID_HANDLE)                                              \
			return AMDGV_ERROR_GPU_DEVICE_LOST;                                   \
		adapt = (struct amdgv_adapter *)dev;                                          \
		if (adapt->status == AMDGV_STATUS_SW_INIT)                                    \
			return AMDGV_ERROR_GPU_NOT_INITIALIZED;                               \
		if (!(adapt->status == AMDGV_STATUS_HW_INIT)     &&                           \
		    !(adapt->status == AMDGV_STATUS_HW_RMA)      &&                           \
		    !(adapt->status == AMDGV_STATUS_HW_HIVE_RMA) &&                           \
		    !(adapt->status == AMDGV_STATUS_HW_LOST))                                 \
			return AMDGV_ERROR_DRIVER_DEV_INIT_FAIL;                              \
	} while (0)

int amdgv_int_allocate_vf(struct amdgv_adapter *adapt, struct amdgv_vf_option *option);
int amdgv_int_free_vf(struct amdgv_adapter *adapt, uint32_t idx_vf);
int amdgv_int_set_vf_number(struct amdgv_adapter *adapt, uint32_t num_vf);

int amdgv_int_ras_trigger_error(struct amdgv_adapter *adapt, struct amdgv_smi_ras_error_inject_info *data);
int amdgv_int_ras_ta_load(struct amdgv_adapter *adapt, struct amdgv_smi_cmd_ras_ta_load *data);
int amdgv_int_ras_ta_unload(struct amdgv_adapter *adapt, struct amdgv_smi_cmd_ras_ta_unload *data);

int amdgv_int_stop_to_pf_helper(struct amdgv_adapter *adapt);


#endif // AMDGV_API_INTERNAL_H
