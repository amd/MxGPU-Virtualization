/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ATOM_TYPES_H
#define ATOM_TYPES_H

/* sync atom types to kernel types */

typedef uint16_t USHORT;
typedef uint32_t ULONG;
typedef uint8_t UCHAR;

#ifndef ATOM_BIG_ENDIAN
#if defined(__BIG_ENDIAN)
#define ATOM_BIG_ENDIAN 1
#else
#define ATOM_BIG_ENDIAN 0
#endif
#endif
#endif
