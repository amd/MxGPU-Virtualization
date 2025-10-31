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

#ifndef __SMI_PROCESSOR_HANDLE_H__
#define __SMI_PROCESSOR_HANDLE_H__

enum smi_processor_type {
	SMI_PROCESSOR_TYPE_UNKNOWN = 0,
	SMI_PROCESSOR_TYPE_AMD_GPU,
	SMI_PROCESSOR_TYPE_AMD_NIC
};

struct smi_gpu_handle {
	enum smi_processor_type type;
	amdsmi_bdf_t bdf;
	uint64_t handle;
	uint64_t dev_id;
};

struct smi_nic_handle {
	enum smi_processor_type type;
	amdsmi_bdf_t bdf;
};

struct smi_gpu_device_handles {
	uint32_t num_gpus;
	struct smi_gpu_handle gpus[AMDSMI_MAX_DEVICES];
};

struct smi_nic_device_handles {
	uint32_t num_nics;
	struct smi_nic_handle nics[AMDSMI_MAX_DEVICES];
};


#endif // __SMI_PROCESSOR_HANDLE_H__
