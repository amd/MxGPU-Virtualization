/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_BASETYPES_H
#define AMDGV_BASETYPES_H

typedef unsigned long long 	uint64_t;
typedef unsigned int 		uint32_t;
typedef unsigned short		uint16_t;
typedef unsigned char		uint8_t;
typedef signed long long	int64_t;
typedef signed int		int32_t;
typedef signed short		int16_t;
typedef signed char		int8_t;

#ifndef __bool_true_false_are_defined

typedef _Bool			bool;

#define false 0
#define true 1

#endif // __bool_true_false_are_defined

#endif
