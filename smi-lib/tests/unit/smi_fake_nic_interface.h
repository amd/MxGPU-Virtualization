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

#ifndef __SMI_FAKE_NIC_INTERFACE_H__
#define __SMI_FAKE_NIC_INTERFACE_H__

#ifdef __cplusplus
extern "C" {
#include "smi_nic_interface.h"
#endif

/**
 * @brief Get the current NIC initialization flag state.
 *
 * @return true if NIC init is performed, false if NIC init is not performed
 */
bool get_nic_init();

/**
 * @brief Set the NIC initialization flag state.
 *
 * When set to true, smi_nic_init() will return SMI_NIC_STATUS_SUCCESS.
 * When set to false, smi_nic_init() will return SMI_NIC_STATUS_ERROR.
 *
 * @param flag false to force NIC init failure, true for normal usage
 */
void set_nic_init(bool flag);

/**
 * @brief Get the current NIC cleanup flag state.
 *
 * @return true if NIC cleanup is performed, false if NIC cleanup is not performed
 */
bool get_nic_cleanup();

/**
 * @brief Set the NIC cleanup flag state.
 *
 * When set to true, smi_nic_cleanup() will return SMI_NIC_STATUS_SUCCESS.
 * When set to false, smi_nic_cleanup() will return SMI_NIC_STATUS_ERROR.
 *
 * @param flag false to force NIC cleanup failure, true for normal usage
 */
void set_nic_cleanup(bool flag);

/**
 * @brief Get the current NIC discovery flag state.
 *
 * @return true if NIC discovery is performed, false if NIC discovery is not performed
 */
bool get_nic_discovery();

/**
 * @brief Set the NIC discovery flag state.
 *
 * When set to true, set_nic_discovery() will return SMI_NIC_STATUS_SUCCESS.
 * When set to false, set_nic_discovery() will return SMI_NIC_STATUS_ERROR.
 *
 * @param flag false to force NIC discovery failure, true for normal usage
 */
void set_nic_discovery(bool flag);

/**
 * @brief Set whether to use a long interface name for testing.
 *
 * @param flag true to return a long interface name, false for normal name
 */
void set_nic_long_interface_name(bool flag);

/**
 * @brief Get the current NIC API status code.
 *
 * @return status code from smi_nic_status_t enum
 */
smi_nic_status_t get_nic_api_status();

/**
 * @brief Set NIC API return status code.
 *
 * @param status code to simulate return status of NIC API
 */
void set_nic_api_status(smi_nic_status_t status);

#ifdef __cplusplus
}
#endif

#endif // __SMI_FAKE_NIC_INTERFACE_H__
