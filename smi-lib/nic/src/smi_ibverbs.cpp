/*
 * Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "smi_ibverbs.h"

#ifdef LIBIBVERBS_INSTALLED

#include <dlfcn.h>

#include <cstring>

namespace {

uint16_t mtu_enum_to_bytes(enum ibv_mtu mtu)
{
	switch (mtu) {
	case IBV_MTU_256:  return 256;
	case IBV_MTU_512:  return 512;
	case IBV_MTU_1024: return 1024;
	case IBV_MTU_2048: return 2048;
	case IBV_MTU_4096: return 4096;
	}
	return 0;
}

} // namespace

namespace smi_ibverbs {

IbverbsLoader& IbverbsLoader::instance()
{
	static IbverbsLoader inst;
	return inst;
}

IbverbsLoader::IbverbsLoader()
{
	/*
	 * dlopen() the libibverbs SONAME, resolved through the usual dynamic
	 * linker search rules (ld.so.cache, LD_LIBRARY_PATH, default dirs).
	 * The SONAME is hardcoded rather than detected at build time.
	 *
	 * If this fails, the loader stays unavailable and queries return nullopt.
	 */
	handle_ = ::dlopen("libibverbs.so.1", RTLD_LAZY | RTLD_LOCAL);
	if (!handle_) {
		return;
	}

	get_device_list  = reinterpret_cast<decltype(get_device_list)>(
		::dlsym(handle_, "ibv_get_device_list"));
	free_device_list = reinterpret_cast<decltype(free_device_list)>(
		::dlsym(handle_, "ibv_free_device_list"));
	get_device_name  = reinterpret_cast<decltype(get_device_name)>(
		::dlsym(handle_, "ibv_get_device_name"));
	open_device      = reinterpret_cast<decltype(open_device)>(
		::dlsym(handle_, "ibv_open_device"));
	close_device     = reinterpret_cast<decltype(close_device)>(
		::dlsym(handle_, "ibv_close_device"));
	query_port       = reinterpret_cast<decltype(query_port)>(
		::dlsym(handle_, "ibv_query_port"));

	available_ = get_device_list && free_device_list && get_device_name &&
		     open_device && close_device && query_port;
}

std::optional<PortMtu> query_port_mtu(const std::string& rdma_dev, uint8_t port_num)
{
	auto& ibv = IbverbsLoader::instance();
	if (!ibv.available()) {
		return std::nullopt;
	}

	int num_devices = 0;
	struct ibv_device **dev_list = ibv.get_device_list(&num_devices);
	if (!dev_list) {
		return std::nullopt;
	}

	std::optional<PortMtu> result;
	for (int i = 0; i < num_devices; i++) {
		const char *name = ibv.get_device_name(dev_list[i]);
		if (!name || rdma_dev != name) {
			continue;
		}

		struct ibv_context *ctx = ibv.open_device(dev_list[i]);
		if (!ctx) {
			break;
		}

		struct ibv_port_attr attr;
		std::memset(&attr, 0, sizeof(attr));
		if (ibv.query_port(ctx, port_num, &attr) == 0) {
			uint16_t active = mtu_enum_to_bytes(attr.active_mtu);
			uint16_t max    = mtu_enum_to_bytes(attr.max_mtu);
			if (active != 0 && max != 0) {
				result = PortMtu{active, max};
			}
		}

		ibv.close_device(ctx);
		break;
	}

	ibv.free_device_list(dev_list);
	return result;
}

} // namespace smi_ibverbs

#else // !LIBIBVERBS_INSTALLED

namespace smi_ibverbs {

std::optional<PortMtu> query_port_mtu(const std::string& /*rdma_dev*/, uint8_t /*port_num*/)
{
	return std::nullopt;
}

} // namespace smi_ibverbs

#endif // LIBIBVERBS_INSTALLED
