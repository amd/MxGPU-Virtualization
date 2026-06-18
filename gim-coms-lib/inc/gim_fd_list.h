/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _GIM_FD_LIST_H_
#define _GIM_FD_LIST_H_

#include <pthread.h>
#define __USE_GNU
#include <search.h>
#include <sys/queue.h>

typedef struct fd_list_entry {
	int fd;
	pthread_t tid;
	SLIST_ENTRY(fd_list_entry) entries;
} fd_list_entry;

typedef struct fd_list_head {
	struct fd_list_entry *slh_first;
	pthread_mutex_t fd_list_mutex;
} fd_list_head;

int create_fd_list(int fd);
struct fd_list_head *get_fd_list(int fd);
int search_fd_from_fd_list(int fd);
int insert_fd_into_fd_list(int curr_fd, int fd);
int destroy_fd_list(int fd);

#endif // _GIM_FD_LIST_H_