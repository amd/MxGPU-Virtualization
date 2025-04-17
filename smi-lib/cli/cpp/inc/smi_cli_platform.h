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
	bool is_mi300_{ false };
	bool is_nv32_ { false };
	bool is_mi200_ { false };
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
	 * @brief Check if it is mi300 gpu
	 *
	 * @return true if it is mi300 gpu
	 */
	bool is_mi300();
	/**
	 * @brief Check if it is nv32 gpu
	 *
	 * @return true if it is nv32 gpu
	 */
	bool is_nv32();
	/**
	 * @brief Check if it is mi200 gpu
	 *
	 * @return true if it is mi200 gpu
	 */
	bool is_mi200();
};
