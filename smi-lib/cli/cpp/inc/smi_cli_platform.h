/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>

class AmdSmiPlatform
{
private:
	std::string operating_system;
	bool is_host_{ false };
	bool is_guest_os_{ false };
	bool is_baremetal_{ false };
	bool unknown_platform { false };
	bool is_linux_{ false };
	bool is_windows_{ false };
	bool is_esxi_ { false };
	bool is_mi300_{ false };
	bool is_mi308_{ false };
	bool is_mi350_{ false };
	bool is_nv_ { false };
	bool is_apu_ { false };
	bool is_mi200_ { false };
	bool is_mixxx_ { false };
	/**
	 * @brief Construct a new Amd Smi Helpers object
	 *
	 */
	AmdSmiPlatform();
	/**
	 * @brief Executes command in command line
	 *
	 * @param[in] cmd command for execution
	 * @return command output
	 */
	std::string exec(const char *cmd);
public:
	/**
	 * @brief Get the static class object
	 *
	 * @return static instance of AmdSmiPlatform class
	 */
	static AmdSmiPlatform &getInstance()
	{
		static AmdSmiPlatform instance;
		return instance;
	}
	bool is_host();
	/**
	 * @brief Check if it is platform guest
	 *
	 * @return true if platform is guest else false
	 */
	bool is_guest();
	/**
	 * @brief Check if it is platform baremetal
	 *
	 * @return true if platform is baremetal else false
	 */
	bool is_baremetal();
	/**
	 * @brief Check if it is operating system windows
	 *
	 * @return true if operating system is windows else false
	 */
	bool is_windows();
	/**
	 * @brief Check if it is operating system linux
	 *
	 * @return true if operating system is linux else false
	 */
	bool is_linux();
	/**
	 * @brief Check if it is operating system ESXi
	 *
	 * @return true if operating system is ESXi else false
	 */
	bool is_esxi();
	/**
	 * @brief Check if it is mi300 gpu
	 *
	 * @return true if it is mi300 gpu
	 */
	bool is_mi300();
	/**
	 * @brief Check if it is mi308 gpu
	 *
	 * @return true if it is mi308 gpu
	 */
	bool is_mi308();
	/**
	 * @brief Check if it is mi350 gpu
	 *
	 * @return true if it is mi350 gpu
	 */
	bool is_mi350();
	/**
	 * @brief Check if it is nv32 gpu
	 *
	 * @return true if it is nv32 gpu
	 */
	bool is_nv();
	/**
	 * @brief Check if it is apu
	 *
	 * @return true if it is apu
	 */
	bool is_apu();
	/**
	 * @brief Check if it is mi200 gpu
	 *
	 * @return true if it is mi200 gpu
	 */
	bool is_mi200();
	/**
	 * @brief Check if it is mixxx gpu
	 *
	 * @return true if it is mixxx gpu
	 */
	bool is_mixxx();
	/**
	 * @brief Get the platform string
	 *
	 * @return platform string
	 */
	std::string get_platform();
};
