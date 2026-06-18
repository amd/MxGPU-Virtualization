/*
 * Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __NL_H__
#define __NL_H__

#include <stddef.h>
#include <stdint.h>

#ifdef LIBMNL_INSTALLED
#include <libmnl/libmnl.h>
#else
#include <linux/netlink.h>
typedef int (*mnl_cb_t)(const struct nlmsghdr *nlh, void *data);
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque generic netlink connection handle.
 */
struct nl_conn;

/**
 * @brief Open a generic netlink connection for the given family.
 *
 * Allocates a connection, opens a NETLINK_GENERIC socket, binds it, and
 * resolves @p family_name to a generic netlink family ID.
 *
 * @param[out] conn         Receives the newly allocated connection handle.
 * @param[in]  family_name  Generic netlink family name to resolve.
 *
 * @return 0 on success, positive @c errno value on failure.
 */
int nl_open(struct nl_conn **conn, const char *family_name);

/**
 * @brief Close a generic netlink connection.
 *
 * Closes the underlying socket and frees the connection handle.
 *
 * @param[in] conn  Connection handle returned by ::nl_open, or @c NULL.
 */
void nl_close(struct nl_conn *conn);

/**
 * @brief Return the generic netlink family ID for a connection.
 *
 * @param[in] conn  Connection handle.
 *
 * @return The resolved family ID, or 0 if @p conn is @c NULL.
 */
uint16_t nl_family_id(const struct nl_conn *conn);

/**
 * @brief Initialize a generic netlink request message.
 *
 * Populates netlink and generic netlink headers in the caller provided
 * buffer with NLM_F_REQUEST | NLM_F_ACK flags and an auto incremented
 * sequence number. The caller may append attributes to the returned
 * header before sending.
 *
 * @param[in]  conn     Connection handle.
 * @param[out] buf      Buffer to build the request in.
 * @param[in]  buflen   Size of @p buf in bytes.
 * @param[in]  cmd      Generic netlink command.
 * @param[in]  version  Generic netlink family version.
 *
 * @return Pointer to the prepared nlmsghdr inside @p buf.
 */
struct nlmsghdr *nl_req_init(struct nl_conn *conn,
			     char *buf, size_t buflen,
			     uint8_t cmd, uint8_t version);

/**
 * @brief Send a netlink request and process responses.
 *
 * Sends the message in @p buf and enters a receive loop, invoking
 * @p data_cb for each data message until all responses are consumed.
 *
 * @param[in]     conn     Connection handle.
 * @param[in,out] buf      Buffer containing the request - reused for receives.
 * @param[in]     buflen   Size of @p buf in bytes.
 * @param[in]     nlh      Netlink message header (inside @p buf).
 * @param[in]     data_cb  Callback invoked for each data response.
 * @param[in]     data     Opaque pointer forwarded to @p data_cb.
 *
 * @return 0 on success, positive @c errno value on failure.
 */
int nl_req_run(struct nl_conn *conn,
	       char *buf, size_t buflen,
	       struct nlmsghdr *nlh,
	       mnl_cb_t data_cb, void *data);

/**
 * @brief Send a netlink request and process responses with a custom error
 *        callback.
 *
 * Behaves like ::nl_req_run but additionally invokes @p err_cb for
 * NLMSG_ERROR responses instead of the default libmnl error handler.
 *
 * @param[in]     conn     Connection handle.
 * @param[in,out] buf      Buffer containing the request; reused for receives.
 * @param[in]     buflen   Size of @p buf in bytes.
 * @param[in]     nlh      Netlink message header (inside @p buf).
 * @param[in]     data_cb  Callback invoked for each data response.
 * @param[in]     data     Opaque pointer forwarded to @p data_cb.
 * @param[in]     err_cb   Callback invoked for NLMSG_ERROR responses.
 *
 * @return 0 on success, positive @c errno value on failure.
 */
int nl_req_run2(struct nl_conn *conn,
		char *buf, size_t buflen,
		struct nlmsghdr *nlh,
		mnl_cb_t data_cb, void *data,
		mnl_cb_t err_cb);

#ifdef __cplusplus
}
#endif

#endif /* __NL_H__ */
