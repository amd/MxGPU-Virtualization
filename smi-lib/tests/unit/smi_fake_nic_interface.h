/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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

/**
 * @brief Set the link type returned by the mocked smi_topo_get_nic_link_type.
 *
 * @param link_type link type to return on subsequent NIC topology queries
 */
void set_nic_link_type(smi_nic_link_type_t link_type);

#ifdef __cplusplus
}
#endif

#endif // __SMI_FAKE_NIC_INTERFACE_H__
