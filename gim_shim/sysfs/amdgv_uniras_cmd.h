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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef AMDGV_UNIRAS_CMD__H_
#define AMDGV_UNIRAS_CMD__H_

#define AMDGV_UNI_CMD _IOWR('R', 0, struct amdgv_uni_cmd)
#define AMDGV_UNI_CMD_MAX_IN_SIZE 128
#define AMDGV_UNI_CMD_MAX_OUT_SIZE 1600

/*
 * This structure is added to mirror the one defined in inc/amdgv_cmd_uni_def.h.
 * The only purpose is to allow the main IOCTL handler to handle both old and
 * new amdgv_cmd calls without header conflicts. In all other cases,
 * inc/amdgv_cmd_uni_def.h should be used to get proper amdgv_cmd definition.
 */
#pragma pack(push, 8)
struct amdgv_uni_cmd {
	uint32_t cmd_id;
	uint32_t input_size;
	uint32_t output_size;
	uint8_t version;
	uint8_t cmd_res;
	uint32_t pid;
	uint32_t reserved[3];
	uint8_t input_buff_raw[AMDGV_UNI_CMD_MAX_IN_SIZE];
	uint8_t output_buff_raw[AMDGV_UNI_CMD_MAX_OUT_SIZE];
};
#pragma pack(pop)

long amdgv_uni_cmd_handler(void *arg);

#endif
