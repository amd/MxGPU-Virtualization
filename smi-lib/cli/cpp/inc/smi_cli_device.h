/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>
#include <cstdint>

enum class DeviceIdentifierType { INDEX, VF_INDEX, BDF, UUID = 3 };
enum class DeviceType { GPU, NIC, BRCM_NIC };

class Device
{
private:
	DeviceIdentifierType identifier_type;
	DeviceType type;
	int gpu_index;
	int vf_index;
	uint64_t bdf;
	std::string value;
	std::string domain;

public:
	/**
	 * @brief Construct a new Device object
	 *
	 * @param[in] gpu gpu index
	 * @param[in] vf vf index
	 * @param[in] device_type device type
	 */
	Device(int gpu, int vf, DeviceIdentifierType device_type, DeviceType type);

	/**
	 * @brief Construct a new Device object
	 *
	 * @param[in] gpu gpu index
	 * @param[in] device_type device type
	 */
	Device(int gpu, DeviceIdentifierType device_type, DeviceType type);

	/**
	 * @brief Construct a new Device object
	 *
	 * @param[in] device bdf or uuid
	 * @param[in] device_type device type
	 * @param[in] domain domain
	 */
	Device(std::string device, DeviceIdentifierType device_type, std::string domain, DeviceType type);

	/**
	 * @brief Get the domain object
	 *
	 * @return domain
	 */
	std::string get_domain()
	{
		return domain;
	}

	/**
	 * @brief Get the value object
	 *
	 * @return value
	 */
	std::string get_value()
	{
		return value;
	}

	/**
	 * @brief Get the gpu index object
	 *
	 * @return gpu index
	 */
	int get_gpu_index()
	{
		return gpu_index;
	}

	/**
	 * @brief Get the vf index object
	 *
	 * @return vf index
	 */
	int get_vf_index()
	{
		return vf_index;
	}

	/**
	 * @brief Get the bdf
	 *
	 * @return bdf handle
	 */
	uint64_t get_bdf()
	{
		return bdf;
	}

	/**
	 * @brief Get the device type
	 *
	 * @return device type
	 */
	DeviceType get_type()
	{
		return type;
	}
};
