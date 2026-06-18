/*
 * Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "smi_sysfs.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

SmiSysfsReader::SysfsStatus SmiSysfsReader::readAll(const std::string& filepath, std::vector<SysfsValue>& content)
{
	std::ifstream file(filepath);
	std::string line;

	if (!file.is_open()) {
		return SmiSysfsReader::SysfsStatus::FileNotFound;
	}

	if (!SmiSysfsReader::exists(filepath)) {
		return SmiSysfsReader::SysfsStatus::IOError;
	}

	content.clear();
	while (std::getline(file, line)) {
		if (line.find(' ') != std::string::npos) {
			content.push_back(line);
			continue;
		}
		std::stringstream ss(line);
		std::string token;
		while (ss >> token) {
			try {
				if (token.find("0x") == 0 || token.find("0X") == 0) {
					int hex_value = std::stoi(token, nullptr, 16);
					content.push_back(hex_value);
				} else if (std::all_of(token.begin(), token.end(), ::isdigit)) {
					content.push_back(std::stoi(token));
				} else {
					content.push_back(token);
				}
			} catch (const std::invalid_argument&) {
				return SmiSysfsReader::SysfsStatus::ParseError;
			} catch (const std::out_of_range&) {
				return SmiSysfsReader::SysfsStatus::ParseError;
			}
		}
	}

	return SmiSysfsReader::SysfsStatus::Success;
}

SmiSysfsReader::SysfsStatus SmiSysfsReader::readLine(const std::string& filepath, SmiSysfsReader::SysfsValue& content)
{
	std::ifstream file(filepath);
	std::string line;

	if (!file.is_open()) {
		return SmiSysfsReader::SysfsStatus::FileNotFound;
	}

	if (!SmiSysfsReader::exists(filepath)) {
		return SmiSysfsReader::SysfsStatus::IOError;
	}

	if (std::getline(file, line)) {
		if (line.find(' ') != std::string::npos) {
			content = line;
			return SmiSysfsReader::SysfsStatus::Success;
		}
		std::stringstream ss(line);
		std::string token;
		if (ss >> token) {
			try {
				if (token.find("0x") == 0 || token.find("0X") == 0) {
					int hex_value = std::stoi(token, nullptr, 16);
					content = hex_value;
				} else if (std::all_of(token.begin(), token.end(), ::isdigit)) {
					content = std::stoi(token);
				} else {
					content = token;
				}
				return SmiSysfsReader::SysfsStatus::Success;
			} catch (...) {
				return SmiSysfsReader::SysfsStatus::ParseError;
			}
		}
	}

	return SmiSysfsReader::SysfsStatus::Success;
}

SmiSysfsReader::SysfsStatus SmiSysfsReader::readBytes(const std::string& filepath, size_t offset, void* buf, size_t len)
{
	if ((buf == nullptr) || (len == 0)) {
		return SmiSysfsReader::SysfsStatus::IOError;
	}

	std::ifstream file(filepath, std::ios::binary);

	if (!file.is_open()) {
		return SmiSysfsReader::SysfsStatus::FileNotFound;
	}

	file.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
	if (!file) {
		return SmiSysfsReader::SysfsStatus::IOError;
	}

	file.read(reinterpret_cast<char*>(buf), static_cast<std::streamsize>(len));
	if (static_cast<size_t>(file.gcount()) != len) {
		return SmiSysfsReader::SysfsStatus::IOError;
	}

	return SmiSysfsReader::SysfsStatus::Success;
}

bool SmiSysfsReader::exists(const std::string& filepath)
{
	std::ifstream file(filepath);
	return file.good();
}
