/*
 * Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __SMI_UTILS_H__
#define __SMI_UTILS_H__

#include <stdbool.h>
#include <stdlib.h>

#include "common/smi_cmd.h"
#include "amdsmi.h"
#include "common/smi_handle.h"
#include "common/smi_device_handle.h"

enum smi_metric_type {
	SMI_METRIC_TYPE_COUNTER = 0x30,
	SMI_METRIC_TYPE_CHIPLET,
	SMI_METRIC_TYPE_INST,
	SMI_METRIC_TYPE_ACC
};

/**
 *  \brief  Util function for dispaching IOCTL call to the KMD.
 *
 *  \note   The data for input should be stored in the global handle.input buff,
 *          and the result of the KMD command will be stored in the
 * handle.output buff.
 *
 *  \param [in] cmd_code - Code of the command issued to the KMD.
 *
 *  \param [in] input_size - Size of the input structure expected by the
 * command.
 *
 *  \param [in] output_size - Size of the output structure returned by the
 * command.
 *
 *  \return AMDSMI_RET_CODE indicating result.
 */
amdsmi_status_t amdsmi_request(smi_req_ctx *smi_req, uint32_t cmd_code, size_t input_size,
		size_t output_size);

/**
 *  \brief  Util function for querying vf partitioning info.
 *
 *  \note   The result of the KMD command will be stored in the handle.output
 * buff.
 *
 *  \param [in] handle - Amdsmi processor handle.
 *
 *  \return AMDSMI_RET_CODE indicating result.
 */
amdsmi_status_t amdsmi_ioctl_get_vf_partitioning_info(smi_req_ctx *smi_req, smi_device_handle_t handle);

/**
 *  \brief  Util function for querying vf static info.
 *
 *  \note   The result of the KMD command will be stored in the handle.output
 * buff.
 *
 *  \param [in] vf_handle - VF handle to query.
 *
 *  \return AMDSMI_RET_CODE indicating result.
 */
amdsmi_status_t amdsmi_ioctl_get_vf_static_info(smi_req_ctx *smi_req, smi_device_handle_t vf_handle);

/**
 *  \brief  Util function for querying gpu performance info.
 *
 *  \note   The result of the KMD command will be stored in the handle.output
 * buff.
 *
 *  \param [in] handle - Amdsmi processor handle.
 *
 *  \return AMDSMI_RET_CODE indicating result.
 */
amdsmi_status_t amdsmi_ioctl_get_gpu_performance_info(smi_req_ctx *smi_req, smi_device_handle_t handle);

/**
 *  \brief  Util function for querying ecc info.
 *
 *  \note   The result of the KMD command will be stored in the handle.output
 * buff.
 *
 *  \param [in] handle - Amdsmi processor handle.
 *
 *  \return AMDSMI_RET_CODE indicating result.
 */
amdsmi_status_t amdsmi_ioctl_get_ecc_error_count(smi_req_ctx *smi_req, smi_device_handle_t handle);

/**
 *  \brief  Util function for querying vf dynamic info.
 *
 *  \note   The result of the KMD command will be stored in the handle.output
 * buff.
 *
 *  \param [in] vf_handle - VF handle to query.
 *
 *  \return AMDSMI_RET_CODE indicating result.
 */
amdsmi_status_t amdsmi_ioctl_get_vf_dynamic_info(smi_req_ctx *smi_req, smi_device_handle_t vf_handle);

/**
 *  \brief  Util function for converting pcie speed from pcie type.
 *
 *  \param [in] pcie_type - Pcie type.
 *
 *  \param [out] pcie_speed - Pcie speed.
 *
 *  \return AMDSMI_RET_CODE indicating result.
 */
amdsmi_status_t amdsmi_get_pcie_speed_from_pcie_type(uint32_t pcie_type, uint32_t *pcie_speed, uint64_t dev_id);

/**
 *  \brief  Util function for converting status from code to coresponding string message.
 *
 *  \param [in] status - Status code.
 *
 *  \param [out] out - String message.
 *
 *  \return AMDSMI_RET_CODE indicating result.
 */
amdsmi_status_t amdsmi_get_string_from_status_enum(amdsmi_status_t status, const char **out);

/**
 *  \brief  Generates uuid for device with specified parameters
 *
 *  \param [out] str      String buffer where to output generated uuid
 *
 *  \param [in]  serial   Asic serial
 *
 *  \param [in]  did      Device ID
 *
 *  \param [in]  idx      PF/VF index
 *
 *  \return SMI_RET_CODE indicating result.
 */
int smi_uuid_gen(char *str, uint64_t serial, uint16_t did, uint8_t idx);

/**
 *  \brief  Checks that the passed UUID has a valid format.
 *
 *  \param [in]  uuid The UUID to check for
 *
 *  \return true if uuid format is valid or false if it is not
 */
bool is_uuid_valid(const char *uuid);

/**
 *  \brief  Util function for reading smi lib version from VERSION file.
 *
 *  \param [in,out] lib_version - Pointer to lib version.
 *
 *  \return AMDSMI_RET_CODE indicating result.
 */
amdsmi_status_t amdsmi_read_lib_version(amdsmi_version_t *lib_version);

#endif // __SMI_UTILS_H__

