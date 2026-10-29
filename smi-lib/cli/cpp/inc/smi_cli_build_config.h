/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

/*
 * Central compile-time configuration for platform-neutral CLI code.
 *
 * OS-specific declarations and includes must remain behind preprocessor
 * boundaries or move to platform translation units. These constants are for
 * code whose names and types are valid on every supported build.
 */
namespace smi::config {

inline constexpr bool is_windows =
#ifdef _WIN64
    true;
#else
    false;
#endif

inline constexpr bool is_linux =
#ifdef __linux__
    true;
#else
    false;
#endif

inline constexpr bool is_esxi =
#ifdef SMI_ESXI_BUILD
    true;
#else
    false;
#endif

inline constexpr bool is_nic_enabled =
#if defined(__linux__) && defined(AMD_SMI_NIC_SUPPORT)
    true;
#else
    false;
#endif

inline constexpr bool is_logger_enabled =
#if defined(ENABLE_LOGGER) && ENABLE_LOGGER
    true;
#else
    false;
#endif

inline constexpr long long event_read_timeout_us =
    is_esxi ? 500LL * 1000LL : 10LL * 1000LL * 1000LL;
inline constexpr unsigned int cper_raw_data_buffer_size = is_esxi ? 1024U * 128U : 1024U * 1024U;
inline constexpr unsigned int cper_headers_array_size	= is_esxi ? 128U : 1024U;

} // namespace smi::config
