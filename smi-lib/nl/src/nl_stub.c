/*
 * Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <nl.h>

int nl_open(struct nl_conn **conn, const char *family_name)
{
	(void)conn;
	(void)family_name;
	return ENOTSUP;
}

void nl_close(struct nl_conn *conn)
{
	(void)conn;
}

uint16_t nl_family_id(const struct nl_conn *conn)
{
	(void)conn;
	return 0;
}

struct nlmsghdr *nl_req_init(struct nl_conn *conn,
			     char *buf, size_t buflen,
			     uint8_t cmd, uint8_t version)
{
	(void)conn;
	(void)buf;
	(void)buflen;
	(void)cmd;
	(void)version;
	return NULL;
}

int nl_req_run(struct nl_conn *conn,
	       char *buf, size_t buflen,
	       struct nlmsghdr *nlh,
	       mnl_cb_t data_cb, void *data)
{
	(void)conn;
	(void)buf;
	(void)buflen;
	(void)nlh;
	(void)data_cb;
	(void)data;
	return ENOTSUP;
}

int nl_req_run2(struct nl_conn *conn,
		char *buf, size_t buflen,
		struct nlmsghdr *nlh,
		mnl_cb_t data_cb, void *data,
		mnl_cb_t err_cb)
{
	(void)conn;
	(void)buf;
	(void)buflen;
	(void)nlh;
	(void)data_cb;
	(void)data;
	(void)err_cb;
	return ENOTSUP;
}
