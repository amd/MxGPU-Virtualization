/*
 * Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <string.h>
#include <linux/genetlink.h>

#include <nl_priv.h>

static int family_attr_cb(const struct nlattr *attr, void *data)
{
	uint16_t *family_id = data;

	if (mnl_attr_get_type(attr) == CTRL_ATTR_FAMILY_ID) {
		*family_id = mnl_attr_get_u16(attr);
	}

	return MNL_CB_OK;
}

static int family_cb(const struct nlmsghdr *nlh, void *data)
{
	return mnl_attr_parse(nlh, sizeof(struct genlmsghdr),
			      family_attr_cb, data);
}

int nl_resolve_family_id(struct nl_conn *conn, const char *name)
{
	char buf[MNL_SOCKET_BUFFER_SIZE];
	struct genlmsghdr *genlh;
	struct nlmsghdr *nlh;
	int ret;

	memset(buf, 0, sizeof(buf));

	nlh = mnl_nlmsg_put_header(buf);
	nlh->nlmsg_type = GENL_ID_CTRL;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
	nlh->nlmsg_seq = ++conn->seq;
	nlh->nlmsg_pid = conn->port_id;

	genlh = mnl_nlmsg_put_extra_header(nlh, sizeof(*genlh));
	genlh->cmd = CTRL_CMD_GETFAMILY;
	genlh->version = 2;

	mnl_attr_put_strz(nlh, CTRL_ATTR_FAMILY_NAME, name);

	if (mnl_socket_sendto(conn->soc, buf, nlh->nlmsg_len) < 0) {
		return errno;
	}

	ret = mnl_socket_recvfrom(conn->soc, buf, sizeof(buf));
	while (ret > 0) {
		ret = mnl_cb_run(buf, ret, conn->seq, conn->port_id,
				family_cb, &conn->family_id);
		if (ret <= MNL_CB_STOP) {
			break;
		}
		ret = mnl_socket_recvfrom(conn->soc, buf, sizeof(buf));
	}

	if (ret == -1) {
		return errno;
	}

	return 0;
}
