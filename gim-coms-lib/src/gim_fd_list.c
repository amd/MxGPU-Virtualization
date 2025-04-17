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

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "gim_fd_list.h"

#define MAX_THREADS	64

pthread_mutex_t ht_mutex = PTHREAD_MUTEX_INITIALIZER;
static int hash_table_cnt;
struct hsearch_data htab_fd_list;

static int __hash_table_create(struct hsearch_data *htab)
{
	if (!hcreate_r(MAX_THREADS, htab))
		return -1;
	return 0;
}

static int __hash_table_insert(struct hsearch_data *htab, char *key, void *data)
{
	ENTRY e, *ep;
	e.key = strdup(key);
	e.data = data;

	if (!hsearch_r(e, ENTER, &ep, htab))
		return -1;

	return 0;
}

static void *__hash_table_search(struct hsearch_data *htab, char *key)
{
	ENTRY e, *ep;
	e.key = key;
	e.data = NULL;

	if (!hsearch_r(e, FIND, &ep, htab) || !ep)
		return NULL;

	return ep->data;
}

static void __hash_table_delete(struct hsearch_data *htab, char *key)
{
	ENTRY e, *ep;
	e.key = key;
	e.data = NULL;

	if (hsearch_r(e, FIND, &ep, htab) && ep) {
		free(ep->key);
		ep->key = NULL;
	}

	return;
}

static void __hash_table_destroy(struct hsearch_data *htab)
{
	hdestroy_r(htab);
	return;
}

int create_fd_list(int fd)
{
	int ret;
	char ht_key[20];
	struct fd_list_head *fd_list = NULL;
	struct fd_list_entry *fd_entry = NULL;

	pthread_mutex_lock(&ht_mutex);
	if (!hash_table_cnt) {
		hash_table_cnt = 1;

		ret = __hash_table_create(&htab_fd_list);
		if (ret < 0) {
			printf("failed to init fd list\n");
			close(fd);
			pthread_mutex_unlock(&ht_mutex);
			return ret;
		}
	} else {
		hash_table_cnt++;
	}

	pthread_mutex_unlock(&ht_mutex);

	fd_entry = (struct fd_list_entry *)calloc(1, sizeof(struct fd_list_entry));
	if (!fd_entry) {
		printf("failed to allocate memory for fd_entry\n");
		close(fd);
		return -ENOMEM;
	}
	fd_entry->fd = fd;
	fd_entry->tid = pthread_self();

	fd_list = (struct fd_list_head *)calloc(1, sizeof(struct fd_list_head));
	if (!fd_list) {
		printf("failed to allocate memory for fd_list\n");
		close(fd);
		free(fd_entry);
		return -ENOMEM;
	}
	SLIST_INIT(fd_list);
	SLIST_INSERT_HEAD(fd_list, fd_entry, entries);

	pthread_mutex_init(&fd_list->fd_list_mutex, NULL);

	sprintf(ht_key, "%d", fd);
	ret = __hash_table_insert(&htab_fd_list, ht_key, (void *)fd_list);
	if (ret < 0) {
		printf("failed to init fd list\n");
		close(fd);
		free(fd_entry);
		free(fd_list);
		return ret;
	}

	return ret;
}

struct fd_list_head *get_fd_list(int fd)
{
	char ht_key[20];

	sprintf(ht_key, "%d", fd);
	return (struct fd_list_head *)__hash_table_search(&htab_fd_list, ht_key);
}

int search_fd_from_fd_list(int fd)
{
	pthread_t tid;
	struct fd_list_head *fd_list;
	struct fd_list_entry *fd_entry;

	tid = pthread_self();

	fd_list = get_fd_list(fd);
	fd = 0;

	if (fd_list) {
		pthread_mutex_lock(&fd_list->fd_list_mutex);
		SLIST_FOREACH(fd_entry, fd_list, entries)
		{
			if (fd_entry && fd_entry->tid == tid) {
				fd = fd_entry->fd;
				break;
			}
		}
		pthread_mutex_unlock(&fd_list->fd_list_mutex);
	} else {
		printf("incorrect fd %d, ioctl open should be called first\n", fd);
		return -EINVAL;
	}

	return fd;
}

int insert_fd_into_fd_list(int curr_fd, int fd)
{
	pthread_t tid;
	struct fd_list_head *fd_list;
	struct fd_list_entry *fd_entry;

	tid = pthread_self();

	fd_list = get_fd_list(curr_fd);
	if (fd_list) {
		pthread_mutex_lock(&fd_list->fd_list_mutex);
		fd_entry = (struct fd_list_entry *)calloc(1, sizeof(struct fd_list_entry));
		if (fd_entry == NULL) {
			printf("failed to allocate memory for fd entry during insert fd into fd list\n");
			pthread_mutex_unlock(&fd_list->fd_list_mutex);
			return -ENOMEM;
		}
		fd_entry->fd = fd;
		fd_entry->tid = tid;
		SLIST_INSERT_HEAD(fd_list, fd_entry, entries);
		pthread_mutex_unlock(&fd_list->fd_list_mutex);
	} else {
		printf("incorrect fd %d, ioctl open should be called first\n", fd);
		return -EINVAL;
	}

	return 0;
}

int destroy_fd_list(int fd)
{
	struct fd_list_head *fd_list;
	struct fd_list_entry *fd_entry;
	char ht_key[20];

	fd_list = get_fd_list(fd);
	if (fd_list) {
		pthread_mutex_lock(&fd_list->fd_list_mutex);
		while (!SLIST_EMPTY(fd_list)) {
			fd_entry = SLIST_FIRST(fd_list);
			shutdown(fd_entry->fd, SHUT_RDWR);
			close(fd_entry->fd);
			SLIST_REMOVE_HEAD(fd_list, entries);
			free(fd_entry);
		}
		pthread_mutex_unlock(&fd_list->fd_list_mutex);
		pthread_mutex_destroy(&fd_list->fd_list_mutex);
	} else {
		printf("incorrect fd %d, ioctl open should be called first\n", fd);
		return -EINVAL;
	}
	free(fd_list);

	sprintf(ht_key, "%d", fd);
	__hash_table_delete(&htab_fd_list, ht_key);

	pthread_mutex_lock(&ht_mutex);
	hash_table_cnt--;
	if (!hash_table_cnt)
		__hash_table_destroy(&htab_fd_list);
	pthread_mutex_unlock(&ht_mutex);

	return 0;
}
