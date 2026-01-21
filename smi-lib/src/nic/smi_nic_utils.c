/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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

#include "smi_nic_utils.h"


amdsmi_status_t smi_map_nic_status(smi_nic_status_t status)
{
	int status_code = AMDSMI_STATUS_MAP_ERROR;

	switch(status) {
	case SMI_NIC_STATUS_SUCCESS:
		status_code = AMDSMI_STATUS_SUCCESS;
		break;
	case SMI_NIC_STATUS_ERROR:
		status_code = AMDSMI_STATUS_API_FAILED;
		break;
	case SMI_NIC_STATUS_WRONG_PARAM:
		status_code = AMDSMI_STATUS_INVAL;
		break;
	case SMI_NIC_STATUS_NOT_FOUND:
		status_code = AMDSMI_STATUS_NOT_FOUND;
		break;
	case SMI_NIC_STATUS_NO_RESOURCE:
		status_code = AMDSMI_STATUS_OUT_OF_RESOURCES;
		break;
	case SMI_NIC_STATUS_NOT_SUPPORTED:
		status_code = AMDSMI_STATUS_NOT_SUPPORTED;
		break;
	case SMI_NIC_STATUS_NOT_INIT:
		status_code = AMDSMI_STATUS_NOT_INIT;
		break;
	case SMI_NIC_STATUS_NO_DATA:
		status_code = AMDSMI_STATUS_NO_DATA;
		break;
	case SMI_NIC_STATUS_DRIVER_NOT_LOADED:
		status_code = AMDSMI_STATUS_DRIVER_NOT_LOADED;
		break;
	}

	return status_code;
}
