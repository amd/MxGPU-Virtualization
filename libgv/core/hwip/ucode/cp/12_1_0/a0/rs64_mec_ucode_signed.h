/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef RS64_MEC_PRODUCTION_UCODE_H
#define RS64_MEC_PRODUCTION_UCODE_H

//-------------------------------------------//
// Addresses are described as byte addresses //
// while offsets and sizes are in DWords.    //
//-------------------------------------------//
#define RS64_MEC_PRODUCTION_IRQ_START_ADDR_LO     4096
#define RS64_MEC_PRODUCTION_IRQ_START_ADDR_HI     3758096384

#define RS64_MEC_PRODUCTION_UC_START_ADDR_LO      12288
#define RS64_MEC_PRODUCTION_UC_START_ADDR_HI      3758096384

#define RS64_MEC_PRODUCTION_UCODE_START_ADDR_LO   0
#define RS64_MEC_PRODUCTION_UCODE_START_ADDR_HI   0
#ifndef RS64_MEC_PRODUCTION_UCODE_SIZE
#define RS64_MEC_PRODUCTION_UCODE_SIZE            61240         /* size in DWORDs of only ucode (signing overhead not included) */
#define RS64_MEC_PRODUCTION_UCODE_SIZE_SIGNED_BYTES 246800 /* size in bytes of signed ucode */
#define RS64_MEC_PRODUCTION_UCODE_SIZE_SIGNED_BODY_OFFSET 256 /* size in bytes of signature header */
#endif

#define RS64_MEC_PRODUCTION_MAIN_OFFSET           0

#define TMZ_CLEANER_SHADER_BASE_OFFSET 61440

#ifdef RS64_MEC_PRODUCTION_CP_VERSION
#if RS64_MEC_PRODUCTION_CP_VERSION != 2440
#error 'RS64_MEC_PRODUCTION_CP_VERSION is already defined but with value different than 2440'
#endif
#else
#define RS64_MEC_PRODUCTION_CP_VERSION          2440
#define RS64_MEC_PRODUCTION_CP_VERSION_STR      "2440"
#define RS64_MEC_PRODUCTION_CP_QA_VERSION       0
#define RS64_MEC_PRODUCTION_CP_FEATURE_VERSION  29
#endif


unsigned char aRS64_MEC_PRODUCTION_UCODE[RS64_MEC_PRODUCTION_UCODE_SIZE_SIGNED_BYTES] = { 0 };


#endif
