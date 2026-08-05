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

#ifndef __RAS_SYS_H__
#define __RAS_SYS_H__
#include "amdgv_basetypes.h"
#include "amdgv_oss_wrapper.h"
#include "amdgv_list.h"
#include "amdgv_api.h"
#include "amdgv_device.h"
#include "amdgv_psp_gfx_if.h"

#define u64  uint64_t
#define u32  uint32_t
#define u16  uint16_t
#define u8   uint8_t
#define i64  int64_t
#define i32  int32_t
#define i16  int16_t
#define i8   int8_t

#define __le64 uint64_t

#ifndef __packed
#ifdef _MSC_VER
/* MSVC: use #pragma pack around packed structs (e.g. ras_eeprom.h); suffix
 * __attribute__((packed)) is not valid here.
 */
#define __packed
#else
#define __packed __attribute__((packed))
#endif
#endif

#ifndef IS_ENABLED
#define IS_ENABLED(x) (0)
#endif

#ifndef BITS_PER_LONG
#define BITS_PER_LONG 64
#endif

typedef u64 uintptr_t;

#define u64_to_user_ptr(x)  (void *)(x)

#ifndef NULL
#define NULL  ((void *)0)
#endif
#define OSS_U16_MAX    0xFFFF

#define OSS_GPU_PAGE_SHIFT  12
#define OSS_GPU_PAGE_SIZE   0x1000

#define OSS_BITS_PER_BYTE 8
#define OSS_BITS_PER_TYPE(type)     (sizeof(type) * OSS_BITS_PER_BYTE)

#define OSS_ALIGN_MASK(x, mask)	(((x) + (mask)) & ~(mask))
#define OSS_ALIGN(x, a)	(OSS_ALIGN_MASK(x, (a) - 1))

#define RAS_DEV_ERR(dev, fmt, ...)                                              \
	do {                                                                    \
		struct amdgv_adapter *adapter = (struct amdgv_adapter *)dev;          \
		if (adapter)                                                          \
			oss_print(AMDGV_ERROR_LEVEL,                                    \
				LIBGV_ERR_HEADER "[%x:%x:%x:%x][%s:%d] " fmt,               \
				((adapter->bdf) >> 16),                                       \
				(((adapter->bdf) >> 8) & (0xff)),                             \
				(((adapter->bdf) >> 3) & (0x1f)), ((adapter->bdf) & (0x7)),     \
				__func__, __LINE__, ##__VA_ARGS__);                         \
		else                                                                \
			oss_print(AMDGV_ERROR_LEVEL,                                    \
				LIBGV_ERR_HEADER "[%s:%d] " fmt,                            \
				__func__, __LINE__, ##__VA_ARGS__);                         \
	} while (0)

#define RAS_DEV_WARN(dev, fmt, ...)                                             \
	do {                                                                    \
		struct amdgv_adapter *adapter = (struct amdgv_adapter *)dev;          \
		if (adapter)                                                          \
			oss_print(AMDGV_WARN_LEVEL,                                     \
				LIBGV_WARN_HEADER "[%x:%x:%x:%x][%s:%d] " fmt,              \
				((adapter->bdf) >> 16),                                       \
				(((adapter->bdf) >> 8) & (0xff)),                             \
				(((adapter->bdf) >> 3) & (0x1f)), ((adapter->bdf) & (0x7)),     \
				__func__, __LINE__, ##__VA_ARGS__);                         \
		else                                                                \
			oss_print(AMDGV_WARN_LEVEL,                                     \
				LIBGV_WARN_HEADER "[%s:%d] " fmt,                           \
				__func__, __LINE__, ##__VA_ARGS__);                         \
	} while (0)

#define RAS_DEV_INFO(dev, fmt, ...)                                             \
	do {                                                                    \
		struct amdgv_adapter *adapter = (struct amdgv_adapter *)dev;          \
		if (adapter)                                                          \
			oss_print(AMDGV_INFO_LEVEL,                                     \
				LIBGV_INFO_HEADER "[%x:%x:%x:%x][%s:%d] " fmt,              \
				((adapter->bdf) >> 16),                                       \
				(((adapter->bdf) >> 8) & (0xff)),                             \
				(((adapter->bdf) >> 3) & (0x1f)), ((adapter->bdf) & (0x7)),     \
				__func__, __LINE__, ##__VA_ARGS__);                         \
		else                                                                \
			oss_print(AMDGV_INFO_LEVEL,                                     \
				LIBGV_WARN_HEADER "[%s:%d] " fmt,                           \
				__func__, __LINE__, ##__VA_ARGS__);                         \
	} while (0)

#define RAS_DEV_DBG(dev, fmt, ...)                                              \
	do {                                                                    \
		struct amdgv_adapter *adapter = (struct amdgv_adapter *)dev;          \
		if (adapter)                                                          \
			oss_print(AMDGV_DEBUG_LEVEL,                                    \
				LIBGV_DEBUG_HEADER "[%x:%x:%x:%x][%s:%d] " fmt,             \
				((adapter->bdf) >> 16),                                       \
				(((adapter->bdf) >> 8) & (0xff)),                             \
				(((adapter->bdf) >> 3) & (0x1f)), ((adapter->bdf) & (0x7)),     \
				__func__, __LINE__, ##__VA_ARGS__);                         \
		else                                                                \
			oss_print(AMDGV_DEBUG_LEVEL,                                    \
				LIBGV_WARN_HEADER "[%s:%d] " fmt,                           \
				__func__, __LINE__, ##__VA_ARGS__);                         \
	} while (0)

#define RAS_INFO(fmt, ...)   oss_print(AMDGV_INFO_LEVEL,                  \
	LIBGV_WARN_HEADER "[%s:%d] " fmt, __func__, __LINE__, ##__VA_ARGS__)

#define GFX_FW_TYPE_REG_LIST GFX_FW_TYPE_REG_ACCESS_WHITELIST

#ifndef OSS_LOCK_STRUCT_DEFINED
struct oss_mutex {
	uint64_t buf[8];
};

struct oss_radix_tree_root {
	uint64_t buf[8];
};

typedef struct oss_spinlock {
	uint64_t buf[8];
} oss_spinlock_t;

typedef struct oss_wait_queue_head {
	uint64_t buf[8];
} oss_wait_queue_head_t;

struct oss_kfifo {
	uint64_t buf[8];
};

typedef struct oss_atomic {
	uint64_t buf[8];
} oss_atomic_t;
#endif

static inline u32 DEV_RREG32_SOC15(void *dev,
		u32 ip_HWIP, u32 inst, u32 reg_BASE_IDX, u32 reg)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;

	return RREG32(adapt->reg_offset[ip_HWIP][inst][reg_BASE_IDX] + reg);
}

static inline void DEV_WREG32_SOC15(void *dev,
		u32 ip_HWIP, u32 inst, u32 reg_BASE_IDX, u32 reg, u32 value)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;

	WREG32(adapt->reg_offset[ip_HWIP][inst][reg_BASE_IDX] + reg, value);
}

static inline u32 DEV_GET_MASK(void *dev, u32 ip_HWIP, u32 mask)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;

	return adapt->ip_map.logical_to_dev_mask ?
		adapt->ip_map.logical_to_dev_mask(adapt,
			(enum amdgv_hw_ip_block_type)ip_HWIP, mask) : mask;
}

#define RAS_DEV_RREG32_SOC15(dev, ip, inst, reg) \
	DEV_RREG32_SOC15(dev, ip##_HWIP, inst, reg##_BASE_IDX, reg)

#define RAS_DEV_WREG32_SOC15(dev, ip, inst, reg, value) \
	DEV_WREG32_SOC15(dev, ip##_HWIP, inst, reg##_BASE_IDX, reg, value)

#define RAS_GET_MASK(dev, ip, mask) \
	DEV_GET_MASK(dev, ip##_HWIP, mask)

#ifdef _MSC_VER
#define min_type(x, y) ((x) < (y) ? (x) : (y))
#define max_type(x, y) ((x) > (y) ? (x) : (y))
#else
#define min_type(x, y) ({    \
	typeof(x) _x = (x);	 \
	typeof(y) _y = (y);	 \
	(void) (&_x == &_y); \
	_x < _y ? _x : _y; })

#define max_type(x, y) ({    \
	typeof(x) _x = (x);	 \
	typeof(y) _y = (y);	 \
	(void) (&_x == &_y); \
	_x > _y ? _x : _y; })
#endif

#define max_t(type, x, y)	(max_type(((type)(x)), ((type)(y))))
#define min_t(type, x, y)	(min_type(((type)(x)), ((type)(y))))

#define _BITS_PER_LONG_ 64
#define GENMASK_ULL(h, l) \
	(((~0ULL) - (1ULL << (l)) + 1) & (~0ULL >> (_BITS_PER_LONG_ - 1 - (h))))

#define BIT_ULL(nr)		(0x1ULL << (nr))

#define le64_to_cpu(x)  (x)
#define cpu_to_le64(x)  (x)

#define ffs(x)   amdgv_ffs(x)

static inline u64 div64_u64(u64 dividend, u64 divisor)
{
	return dividend / divisor;
}

static inline u64 div64_u64_rem(u64 dividend, u64 divisor, u64 *remainder)
{
	*remainder = dividend % divisor;
	return dividend / divisor;
}

#ifdef SHIM_LAYER_OSS_RADIX_TREE
#define oss_radix_tree_for_each_slot(slot, root, iter, start)         \
for (slot = oss_radix_tree_iter_init(iter, start) ;            \
	slot || (slot = oss_radix_tree_next_chunk(root, iter, 0)); \
	slot = oss_radix_tree_next_slot(slot, iter, 0))
#endif

#define OSS_LIST_HEAD(name)  AMDGV_LIST_HEAD(name)
#define OSS_INIT_LIST_HEAD(list)  AMDGV_INIT_LIST_HEAD(list)
#define oss_list_add_tail(new, head)  amdgv_list_add_tail(new, head)
#define oss_list_del(entry)  amdgv_list_del(entry)
#define oss_list_empty(entry)  amdgv_list_empty(entry)

#define oss_kthread_should_stop()  oss_thread_should_stop(NULL)
#define oss_wake_up(x)  oss_notifier_wakeup(x, 1)

#define oss_list_for_each_entry(pos, head, type, member)  \
	amdgv_list_for_each_entry(pos, head, type, member)
#define oss_list_for_each_entry_safe(pos, n, head, type, member)  \
	amdgv_list_for_each_entry_safe(pos, n, head, type, member)

typedef struct amdgv_list_head oss_list_head;

typedef int (*condition_func)(void *param);
typedef int (*ras_threadfn)(void *context);
static inline long ras_wait_event_interruptible_timeout(void *wq_head,
			condition_func condition, void *param, unsigned int timeout)
{
	return oss_wait_event_interruptible_timeout(wq_head, condition, param, timeout);
}

void *ras_calloc(unsigned int n, unsigned int size);

extern const struct ras_sys_func amdgv_ras_sys_fn;

#ifndef SHIM_LAYER_OSS_RADIX_TREE
#include "amdgv_ras_radix_tree.h"
#define oss_radix_tree_init     amdgv_radix_tree_init
#define oss_radix_tree_fini     amdgv_radix_tree_fini
#define oss_radix_tree_insert   amdgv_radix_tree_insert
#define oss_radix_tree_delete   amdgv_radix_tree_delete
#define oss_radix_tree_lookup   amdgv_radix_tree_lookup
#define oss_radix_tree_gang_lookup_tag  amdgv_radix_tree_gang_lookup_tag
#define oss_radix_tree_tag_set  amdgv_radix_tree_tag_set
#define oss_radix_tree_tag_clear  amdgv_radix_tree_tag_clear
#define oss_radix_tree_iter_init  amdgv_radix_tree_iter_init
#define oss_radix_tree_next_chunk  amdgv_radix_tree_next_chunk
#define oss_radix_tree_next_slot   amdgv_radix_tree_next_slot
#define oss_radix_tree_deref_slot  amdgv_radix_tree_deref_slot
#define oss_radix_tree_delete_iter  amdgv_radix_tree_delete_iter
#define oss_radix_tree_for_each_slot  amdgv_radix_tree_for_each_slot
#endif

#ifndef SHIM_LAYER_OSS_KFIFO
#include "amdgv_ras_fifo.h"
#define oss_kfifo_alloc              amdgv_fifo_alloc
#define oss_kfifo_in_spinlocked_raw  amdgv_fifo_in_spinlocked_raw
#define oss_kfifo_out_spinlocked_raw amdgv_fifo_out_spinlocked_raw
#define oss_kfifo_out_peek           amdgv_fifo_out_peek
#define oss_kfifo_len                amdgv_fifo_len
#define oss_kfifo_free               amdgv_fifo_free
#endif

#ifndef SHIM_LAYER_OSS_MEMPOOL
#include "amdgv_ras_mempool.h"
#define oss_mempool_create_kmalloc_pool  amdgv_mempool_create_kmalloc_pool
#define oss_mempool_destroy              amdgv_mempool_destroy
#define oss_mempool_alloc_preallocated   amdgv_mempool_alloc_preallocated
#define oss_mempool_free                 amdgv_mempool_free
#endif
#endif
