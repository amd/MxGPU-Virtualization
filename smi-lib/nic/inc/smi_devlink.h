/*
 * Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SMI_DEVLINK_H__
#define __SMI_DEVLINK_H__

#include <string>
#include <vector>

struct nl_conn;

enum class FwVersionType { Fixed, Running, Stored };

struct FwVersion {
	FwVersionType type;
	std::string name;
	std::string version;
};

class SmiDevlink {
public:
	SmiDevlink();
	~SmiDevlink();

	SmiDevlink(const SmiDevlink&) = delete;
	SmiDevlink& operator=(const SmiDevlink&) = delete;

	int open(const std::string& bdf);
	void close();
	bool is_open() const;

	int get_fw_versions(std::vector<FwVersion>& versions);
	int get_port_flavour(const std::string& netdev, std::string& flavour);

private:
	struct nl_conn *conn_;
	std::string bdf_;
};

#endif // __SMI_DEVLINK_H__
