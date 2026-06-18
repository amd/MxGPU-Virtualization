/*
 * Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SMI_IBVERBS_H__
#define __SMI_IBVERBS_H__

#include <cstdint>
#include <optional>
#include <string>

#ifdef LIBIBVERBS_INSTALLED
#include <infiniband/verbs.h>
#endif

namespace smi_ibverbs {

struct PortMtu {
	uint16_t active_mtu;
	uint16_t max_mtu;
};

#ifdef LIBIBVERBS_INSTALLED

/*
 * Lazy loader for libibverbs.
 *
 * This class is only compiled in when nic/CMakeLists.txt finds the libibverbs
 * headers (LIBIBVERBS_INSTALLED is defined). If the headers are missing at
 * build time, the stub in the .cpp's #else branch is used instead (every
 * query returns nullopt; dependent fields report N/A).
 *
 * At runtime the loader dlopen()s the hardcoded SONAME "libibverbs.so.1";
 * details and rationale live in the constructor in smi_ibverbs.cpp.
 *
 * The handle is intentionally never dlclose()'d: libibverbs registers atexit()
 * and pthread_key_create() callbacks internally, so unloading the library can
 * leave dangling pointers. Leaking the handle for process lifetime is the
 * standard pattern for optional dlopen()'d libraries.
 */
class IbverbsLoader {
public:
	static IbverbsLoader& instance();

	bool available() const { return available_; }

private:
	IbverbsLoader();
	~IbverbsLoader() = default;
	IbverbsLoader(const IbverbsLoader&) = delete;
	IbverbsLoader& operator=(const IbverbsLoader&) = delete;

	/*
	 * Only query_port_mtu() consumes these resolved symbols; keep them
	 * private and grant it access rather than exposing the vtable.
	 */
	friend std::optional<PortMtu> query_port_mtu(const std::string& rdma_dev,
						     uint8_t port_num);

	struct ibv_device **(*get_device_list)(int *num_devices) = nullptr;
	void (*free_device_list)(struct ibv_device **list) = nullptr;
	const char *(*get_device_name)(struct ibv_device *device) = nullptr;
	struct ibv_context *(*open_device)(struct ibv_device *device) = nullptr;
	int (*close_device)(struct ibv_context *context) = nullptr;
	int (*query_port)(struct ibv_context *context, uint8_t port_num,
			  struct ibv_port_attr *port_attr) = nullptr;

	void *handle_ = nullptr;
	bool available_ = false;
};

#endif // LIBIBVERBS_INSTALLED

/**
 * @brief Query active and max MTU for an InfiniBand/RoCE port via libibverbs.
 *
 * Uses ibv_query_port() under the hood. libibverbs is resolved lazily through
 * dlopen("libibverbs.so.1") the first time this function is called; if the
 * runtime library is not present, or if the build did not have libibverbs
 * headers available, this function returns std::nullopt and the caller should
 * treat the values as N/A.
 *
 * The returned values are in bytes (256, 512, 1024, 2048 or 4096), already
 * converted from the underlying ibv_mtu enum.
 *
 * @param rdma_dev RDMA device name (e.g. "ionic_0", "bnxt_re0")
 * @param port_num 1-based port number, as exposed under
 *                 /sys/class/infiniband/<dev>/ports/<port_num>
 * @return PortMtu with active/max in bytes, or nullopt on any failure.
 */
std::optional<PortMtu> query_port_mtu(const std::string& rdma_dev, uint8_t port_num);

} // namespace smi_ibverbs

#endif // __SMI_IBVERBS_H__
