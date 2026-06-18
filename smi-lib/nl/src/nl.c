/*
 * Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <linux/genetlink.h>
#include <linux/netlink.h>

#include <nl_priv.h>

int nl_open(struct nl_conn **conn, const char *family_name)
{
	struct nl_conn *c;
	int ret;

	c = calloc(1, sizeof(*c));
	if (!c) {
		return ENOMEM;
	}

	c->soc = mnl_socket_open(NETLINK_GENERIC);
	if (!c->soc) {
		ret = errno;
		goto err_free;
	}

	if (mnl_socket_bind(c->soc, 0, MNL_SOCKET_AUTOPID) < 0) {
		ret = errno;
		goto err_close;
	}

	c->port_id = mnl_socket_get_portid(c->soc);

	ret = nl_resolve_family_id(c, family_name);
	if (ret) {
		goto err_close;
	}

	*conn = c;
	return 0;

err_close:
	mnl_socket_close(c->soc);
err_free:
	free(c);
	return ret;
}

void nl_close(struct nl_conn *conn)
{
	if (!conn) {
		return;
	}
	mnl_socket_close(conn->soc);
	free(conn);
}

uint16_t nl_family_id(const struct nl_conn *conn)
{
	if (!conn) {
		return 0;
	}
	return conn->family_id;
}

struct nlmsghdr *nl_req_init(struct nl_conn *conn,
			     char *buf, size_t buflen,
			     uint8_t cmd, uint8_t version)
{
	struct genlmsghdr *genlh;
	struct nlmsghdr *nlh;

	memset(buf, 0, buflen);

	nlh = mnl_nlmsg_put_header(buf);
	nlh->nlmsg_type = conn->family_id;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
	nlh->nlmsg_seq = ++conn->seq;
	nlh->nlmsg_pid = conn->port_id;

	genlh = mnl_nlmsg_put_extra_header(nlh, sizeof(*genlh));
	genlh->cmd = cmd;
	genlh->version = version;

	return nlh;
}

int nl_req_run(struct nl_conn *conn,
	       char *buf, size_t buflen,
	       struct nlmsghdr *nlh,
	       mnl_cb_t data_cb, void *data)
{
	int ret;

	if (mnl_socket_sendto(conn->soc, buf, nlh->nlmsg_len) < 0) {
		return errno;
	}

	ret = mnl_socket_recvfrom(conn->soc, buf, buflen);
	while (ret > 0) {
		ret = mnl_cb_run(buf, ret, conn->seq, conn->port_id,
				data_cb, data);
		if (ret <= MNL_CB_STOP) {
			break;
		}
		ret = mnl_socket_recvfrom(conn->soc, buf, buflen);
	}

	if (ret == -1) {
		return errno;
	}
	return 0;
}

int nl_req_run2(struct nl_conn *conn,
		char *buf, size_t buflen,
		struct nlmsghdr *nlh,
		mnl_cb_t data_cb, void *data,
		mnl_cb_t err_cb)
{
	mnl_cb_t cb_ctl_array[NLMSG_MIN_TYPE];
	int ret;

	memset(cb_ctl_array, 0, sizeof(cb_ctl_array));
	cb_ctl_array[NLMSG_ERROR] = err_cb;

	if (mnl_socket_sendto(conn->soc, buf, nlh->nlmsg_len) < 0) {
		return errno;
	}

	ret = mnl_socket_recvfrom(conn->soc, buf, buflen);
	while (ret > 0) {
		ret = mnl_cb_run2(buf, ret, conn->seq, conn->port_id,
				 data_cb, data, cb_ctl_array, NLMSG_MIN_TYPE);
		if (ret <= MNL_CB_STOP) {
			break;
		}
		ret = mnl_socket_recvfrom(conn->soc, buf, buflen);
	}

	if (ret == -1) {
		return errno;
	}

	return 0;
}
