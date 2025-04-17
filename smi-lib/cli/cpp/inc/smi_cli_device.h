/* * Copyright (C) 2023-2024 Advanced Micro Devices. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

#include <string>
#include <cstdint>

enum DeviceType { GPU_INDEX, VF_INDEX, BDF, UUID = 3 };

class Device
{
private:
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
	Device(int gpu, int vf, DeviceType device_type);

	/**
	 * @brief Construct a new Device object
	 *
	 * @param[in] gpu gpu index
	 * @param[in] device_type device type
	 */
	Device(int gpu, DeviceType device_type);

	/**
	 * @brief Construct a new Device object
	 *
	 * @param[in] device bdf or uuid
	 * @param[in] device_type device type
	 * @param[in] domain domain
	 */
	Device(std::string device, DeviceType device_type, std::string domain);

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
};
