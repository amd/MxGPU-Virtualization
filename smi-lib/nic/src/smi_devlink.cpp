/*
 * Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "smi_devlink.h"
#include "smi_nic_interface.h"
#include "smi_utils.h"

#ifdef LIBMNL_INSTALLED

#include <cstring>
#include <utility>
#include <linux/devlink.h>
#include <linux/genetlink.h>

extern "C" {
#include <nl.h>
}

namespace {

struct fw_ctx {
	const std::string &bdf;
	std::vector<FwVersion> &versions;
	bool found;
};

int fw_info_cb(const struct nlmsghdr *nlh, void *data)
{
	auto *ctx = static_cast<fw_ctx*>(data);

	const char *dev_name = nullptr;

	auto bdf_cb = [](const struct nlattr *attr, void *data) -> int {
		auto *out = static_cast<const char**>(data);
		if (mnl_attr_get_type(attr) == DEVLINK_ATTR_DEV_NAME)
			*out = static_cast<const char*>(mnl_attr_get_payload(attr));
		return MNL_CB_OK;
	};

	mnl_attr_parse(nlh, sizeof(struct genlmsghdr), bdf_cb, &dev_name);

	if (!dev_name || ctx->bdf != dev_name) {
		return MNL_CB_OK;
	}

	ctx->found = true;

	auto attr_cb = [](const struct nlattr *attr, void *data) -> int {
		auto *versions = static_cast<std::vector<FwVersion>*>(data);
		FwVersionType type;

		switch (mnl_attr_get_type(attr)) {
		case DEVLINK_ATTR_INFO_VERSION_FIXED:
			type = FwVersionType::Fixed;
			break;
		case DEVLINK_ATTR_INFO_VERSION_RUNNING:
			type = FwVersionType::Running;
			break;
		case DEVLINK_ATTR_INFO_VERSION_STORED:
			type = FwVersionType::Stored;
			break;
		default:
			return MNL_CB_OK;
		}

		std::pair<const char*, const char*> kv{nullptr, nullptr};
		auto kv_cb = [](const struct nlattr *attr, void *data) -> int {
			auto *kv = static_cast<std::pair<const char*, const char*>*>(data);
			switch (mnl_attr_get_type(attr)) {
			case DEVLINK_ATTR_INFO_VERSION_NAME:
				kv->first = static_cast<const char*>(mnl_attr_get_payload(attr));
				break;
			case DEVLINK_ATTR_INFO_VERSION_VALUE:
				kv->second = static_cast<const char*>(mnl_attr_get_payload(attr));
				break;
			}
			return MNL_CB_OK;
		};
		mnl_attr_parse_nested(attr, kv_cb, &kv);
		if (kv.first && kv.second) {
			versions->push_back(FwVersion{type, kv.first, kv.second});
		}
		return MNL_CB_OK;
	};

	mnl_attr_parse(nlh, sizeof(struct genlmsghdr), attr_cb, &ctx->versions);
	return MNL_CB_OK;
}

using flavour_ctx = std::pair<const std::string*, std::string*>;

int port_flavour_cb(const struct nlmsghdr *nlh, void *data)
{
	auto *ctx = static_cast<flavour_ctx*>(data);
	if (!ctx->second->empty())
		return MNL_CB_OK;

	std::pair<const char*, uint16_t> port_data{nullptr, UINT16_MAX};

	auto attr_cb = [](const struct nlattr *attr, void *data) -> int {
		auto *pd = static_cast<std::pair<const char*, uint16_t>*>(data);
		switch (mnl_attr_get_type(attr)) {
		case DEVLINK_ATTR_PORT_NETDEV_NAME:
			pd->first = static_cast<const char*>(mnl_attr_get_payload(attr));
			break;
		case DEVLINK_ATTR_PORT_FLAVOUR:
			pd->second = mnl_attr_get_u16(attr);
			break;
		}
		return MNL_CB_OK;
	};

	mnl_attr_parse(nlh, sizeof(struct genlmsghdr), attr_cb, &port_data);

	if (port_data.first && *ctx->first == port_data.first) {
		*ctx->second = smi_utils::flavour_to_string(port_data.second);
	}

	return MNL_CB_OK;
}

}

// **** SmiDevlink ****

SmiDevlink::SmiDevlink()
	: conn_(nullptr)
{
}

SmiDevlink::~SmiDevlink()
{
	close();
}

int SmiDevlink::open(const std::string& bdf)
{
	close();

	int ret = nl_open(&conn_, "devlink");
	if (ret != 0) {
		return SMI_NIC_STATUS_NOT_INIT;
	}
	bdf_ = bdf;
	return SMI_NIC_STATUS_SUCCESS;
}

void SmiDevlink::close()
{
	if (conn_) {
		nl_close(conn_);
		conn_ = nullptr;
	}
	bdf_.clear();
}

bool SmiDevlink::is_open() const
{
	return conn_ != nullptr;
}

int SmiDevlink::get_fw_versions(std::vector<FwVersion>& versions)
{
	versions.clear();

	if (!conn_) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	char buf[MNL_SOCKET_BUFFER_SIZE];
	struct nlmsghdr *nlh = nl_req_init(conn_, buf, sizeof(buf),
					  DEVLINK_CMD_INFO_GET,
					  DEVLINK_GENL_VERSION);
	nlh->nlmsg_flags |= NLM_F_DUMP;
	mnl_attr_put_strz(nlh, DEVLINK_ATTR_BUS_NAME, "pci");
	mnl_attr_put_strz(nlh, DEVLINK_ATTR_DEV_NAME, bdf_.c_str());

	fw_ctx ctx{bdf_, versions, false};
	int ret = nl_req_run(conn_, buf, sizeof(buf), nlh,
			    fw_info_cb, &ctx);

	if (ret != 0) {
		return SMI_NIC_STATUS_ERROR;
	}
	if (!ctx.found) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}
	return versions.empty() ? SMI_NIC_STATUS_NO_DATA : SMI_NIC_STATUS_SUCCESS;
}

int SmiDevlink::get_port_flavour(const std::string& netdev, std::string& flavour)
{
	flavour.clear();

	if (!conn_) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	char buf[MNL_SOCKET_BUFFER_SIZE];
	struct nlmsghdr *nlh = nl_req_init(conn_, buf, sizeof(buf),
					  DEVLINK_CMD_PORT_GET,
					  DEVLINK_GENL_VERSION);
	nlh->nlmsg_flags |= NLM_F_DUMP;
	mnl_attr_put_strz(nlh, DEVLINK_ATTR_BUS_NAME, "pci");
	mnl_attr_put_strz(nlh, DEVLINK_ATTR_DEV_NAME, bdf_.c_str());

	flavour_ctx ctx(&netdev, &flavour);
	int ret = nl_req_run(conn_, buf, sizeof(buf), nlh,
			    port_flavour_cb, &ctx);

	if (ret != 0) {
		return SMI_NIC_STATUS_ERROR;
	}

	return flavour.empty() ? SMI_NIC_STATUS_NOT_FOUND : SMI_NIC_STATUS_SUCCESS;
}

#else /* !LIBMNL_INSTALLED */

SmiDevlink::SmiDevlink()
	: conn_(nullptr)
{
}

SmiDevlink::~SmiDevlink()
{
}

int SmiDevlink::open(const std::string&)
{
	return SMI_NIC_STATUS_NOT_SUPPORTED;
}

void SmiDevlink::close()
{
}

bool SmiDevlink::is_open() const
{
	return false;
}

int SmiDevlink::get_fw_versions(std::vector<FwVersion>& versions)
{
	versions.clear();
	return SMI_NIC_STATUS_NOT_SUPPORTED;
}

int SmiDevlink::get_port_flavour(const std::string&, std::string& flavour)
{
	flavour.clear();
	return SMI_NIC_STATUS_NOT_SUPPORTED;
}

#endif /* LIBMNL_INSTALLED */
