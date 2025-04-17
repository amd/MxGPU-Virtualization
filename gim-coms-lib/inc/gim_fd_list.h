/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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