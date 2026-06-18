/*
 * Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __NL_PRIV_H__
#define __NL_PRIV_H__

#include <nl.h>

struct nl_conn {
	struct mnl_socket *soc;
	uint16_t family_id;
	uint32_t seq;
	unsigned int port_id;
};

int nl_resolve_family_id(struct nl_conn *conn, const char *name);

#endif /* NL_PRIV_H */
