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

#ifndef AMDGV_RAS_EVENT_H
#define AMDGV_RAS_EVENT_H

#include "amdgv_basetypes.h"

/*
 * RAS event sequencing shared between UniRAS ras_mgr, libgv XGMI hive state,
 * and (when built) AMDGPU ras_mgr. Kept here so libgv headers do not need the
 * full amdgv_ras.h surface just to embed struct ras_event_manager.
 */
enum ras_event_type {
	RAS_EVENT_TYPE_INVALID = 0,
	RAS_EVENT_TYPE_FATAL,
	RAS_EVENT_TYPE_POISON_CREATION,
	RAS_EVENT_TYPE_POISON_CONSUMPTION,
	RAS_EVENT_TYPE_COUNT,
};

struct ras_event_state {
	uint64_t last_seqno;
	uint64_t count;
};

struct ras_event_manager {
	uint64_t seqno;
	struct ras_event_state event_state[RAS_EVENT_TYPE_COUNT];
};

#endif /* AMDGV_RAS_EVENT_H */
