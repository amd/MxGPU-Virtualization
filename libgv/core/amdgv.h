/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _AMDGV_H_
#define _AMDGV_H_

#include "amdgv_basetypes.h"
#include "amdgv_api.h"

#ifndef NULL
#define NULL (void *)0
#endif

#ifndef INLINE
#define INLINE static inline
#endif

/* dummy macro for compatibility */
#define cpu_to_le32(x) (x)
#define le32_to_cpu(x) (x)

#define upper_32_bits(n) ((uint32_t)(((n) >> 16) >> 16))
#define lower_32_bits(n) ((uint32_t)(n))

/*
 * When x = 0, the result is NOT round up, but ZERO
 * make sure do a ZERO check before using this macro
 */
#define roundup(x, y) ((((x) + ((y)-1)) / (y)) * (y))

#define rounddown(x, y) ((x) - ((x) % (y)))

#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))

#ifndef offsetof
#define offsetof(typ, memb) ((uint64_t)((char *)&(((typ *)0)->memb)))
#endif

#define min(x, y) ((x) < (y) ? x : y)

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define AMDGV_INVALID_IDX_VF (adapt->max_num_vf + 1)

#define AMDGV_IS_IDX_INVALID(idx_vf) (idx_vf != AMDGV_PF_IDX && idx_vf >= adapt->num_vf)

#define AMDGV_ACTIVE_FCN_SELECT_IDX_VF(idx_vf)                                                \
	((idx_vf == AMDGV_PF_IDX) ? (1U << 31) : (1U << idx_vf))

#define BIT(x) (1U << (x))
#define BIT64(x) (1ULL << (x))

#define AMDGV_ERROR(fmt, ...)                                                                 \
	do {                                                                                  \
		if (adapt->log_mask & this_block)                                             \
			oss_print(AMDGV_ERROR_LEVEL,                                          \
				  LIBGV_ERR_HEADER "[%x:%x:%x:%x][%s:%d] " fmt,               \
				  ((adapt->bdf) >> 16),                                            \
				  (((adapt->bdf) >> 8) & (0xff)),                             \
				  (((adapt->bdf) >> 3) & (0x1f)), ((adapt->bdf) & (0x7)),     \
				  __func__, __LINE__, ##__VA_ARGS__);                         \
	} while (0)

#define AMDGV_WARN(fmt, ...)                                                                  \
	do {                                                                                  \
		if (adapt->log_level >= AMDGV_WARN_LEVEL && (adapt->log_mask & this_block))   \
			oss_print(AMDGV_WARN_LEVEL,                                           \
				  LIBGV_WARN_HEADER "[%x:%x:%x:%x][%s:%d] " fmt,               \
				  ((adapt->bdf) >> 16),                                            \
				  (((adapt->bdf) >> 8) & (0xff)),                             \
				  (((adapt->bdf) >> 3) & (0x1f)), ((adapt->bdf) & (0x7)),     \
				  __func__, __LINE__, ##__VA_ARGS__);                         \
	} while (0)

#define AMDGV_INFO(fmt, ...)                                                                  \
	do {                                                                                  \
		if (adapt->log_level >= AMDGV_INFO_LEVEL && (adapt->log_mask & this_block))   \
			oss_print(AMDGV_INFO_LEVEL,                                           \
				  LIBGV_INFO_HEADER "[%x:%x:%x:%x][%s:%d] " fmt,               \
				  ((adapt->bdf) >> 16),                                            \
				  (((adapt->bdf) >> 8) & (0xff)),                             \
				  (((adapt->bdf) >> 3) & (0x1f)), ((adapt->bdf) & (0x7)),     \
				  __func__, __LINE__, ##__VA_ARGS__);                         \
	} while (0)

#define AMDGV_DEBUG(fmt, ...)                                                                 \
	do {                                                                                  \
		if (adapt->log_level >= AMDGV_DEBUG_LEVEL && (adapt->log_mask & this_block))  \
			oss_print(AMDGV_DEBUG_LEVEL,                                          \
				  LIBGV_DEBUG_HEADER "[%x:%x:%x:%x][%s:%d] " fmt,               \
				  ((adapt->bdf) >> 16),                                           \
				  (((adapt->bdf) >> 8) & (0xff)),                             \
				  (((adapt->bdf) >> 3) & (0x1f)), ((adapt->bdf) & (0x7)),     \
				  __func__, __LINE__, ##__VA_ARGS__);                         \
	} while (0)

#define AMDGV_DEBUG4(fmt, ...)                                                                \
	do {                                                                                  \
		if (adapt->log_level >= AMDGV_DEBUG_LEVEL4 && (adapt->log_mask & this_block)) \
			oss_print(AMDGV_DEBUG_LEVEL,                                          \
				  LIBGV_DEBUG_HEADER "[%x:%x:%x:%x][%s:%d] " fmt,               \
				  ((adapt->bdf) >> 16),                                            \
				  (((adapt->bdf) >> 8) & (0xff)),                             \
				  (((adapt->bdf) >> 3) & (0x1f)), ((adapt->bdf) & (0x7)),     \
				  __func__, __LINE__, ##__VA_ARGS__);                         \
	} while (0)

#define AMDGV_PRINT(fmt, ...)                                                                 \
	do {                                                                                  \
		oss_print(AMDGV_INFO_LEVEL, LIBGV_PRINT_HEADER "[%s:%d] " fmt, __func__,      \
			  __LINE__, ##__VA_ARGS__);                                           \
	} while (0)

#define AMDGV_ASSERT(expr)                                                                    \
	do {                                                                                  \
		if (!(expr))                                                                  \
			oss_print(AMDGV_ERROR_LEVEL,                                          \
				  LIBGV_ASSERT_HEADER "[%s:%d] "                              \
						      "Assertion fail (%s)\n",                  \
				  __func__, __LINE__, #expr);                                 \
	} while (0)

INLINE unsigned int order_base_2(unsigned int n)
{
	unsigned int order = 0;

	if (n <= 1)
		return 0;

	n--;
	while (n > 0) {
		n >>= 1;
		order++;
	}

	return order;
}

INLINE int amdgv_count_array(void *array)
{
	int count = 0;
	void **array_p = (void **)array;

	while (array_p[count])
		count++;

	return count;
}

#endif
