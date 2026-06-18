/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ATOM_BITS_H
#define ATOM_BITS_H

static inline uint8_t get_u8(void *bios, int ptr)
{
	return ((unsigned char *)bios)[ptr];
}
#define U8(ptr)	 get_u8(ctx->ctx->bios, (ptr))
#define CU8(ptr) get_u8(ctx->bios, (ptr))
static inline uint16_t get_u16(void *bios, int ptr)
{
	return get_u8(bios, ptr) | (((uint16_t)get_u8(bios, ptr + 1)) << 8);
}
#define U16(ptr)  get_u16(ctx->ctx->bios, (ptr))
#define CU16(ptr) get_u16(ctx->bios, (ptr))
static inline uint32_t get_u32(void *bios, int ptr)
{
	return get_u16(bios, ptr) | (((uint32_t)get_u16(bios, ptr + 2)) << 16);
}
#define U32(ptr)  get_u32(ctx->ctx->bios, (ptr))
#define CU32(ptr) get_u32(ctx->bios, (ptr))
#define CSTR(ptr) (((char *)(ctx->bios)) + (ptr))

#endif
