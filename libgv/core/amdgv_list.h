/*
 * Copyright (c) 2017-2021 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef _AMDGV_LIST_H_
#define _AMDGV_LIST_H_

#include "amdgv.h"

#ifndef INLINE
#define INLINE static inline
#endif

struct amdgv_list_head {
	struct amdgv_list_head *next, *prev;
};

/* Initialize the list as an empty list. */
#define AMDGV_LIST_HEAD_INIT(name) { &(name), &(name) }

#define AMDGV_LIST_HEAD(name) struct amdgv_list_head name = AMDGV_LIST_HEAD_INIT(name)

INLINE void AMDGV_INIT_LIST_HEAD(struct amdgv_list_head *list)
{
	list->next = list->prev = list;
}

INLINE void list_add(struct amdgv_list_head *entry, struct amdgv_list_head *prev,
		     struct amdgv_list_head *next)
{
	next->prev = entry;
	entry->next = next;
	entry->prev = prev;
	prev->next = entry;
}

/*
 * Insert a new element after the given list head. The new element does not
 * need to be initialised as empty list.
 */
INLINE void amdgv_list_add(struct amdgv_list_head *entry, struct amdgv_list_head *head)
{
	list_add(entry, head, head->next);
}

/*
 * Append a new element to the end of the list given with this list head.
 */
INLINE void amdgv_list_add_tail(struct amdgv_list_head *entry, struct amdgv_list_head *head)
{
	list_add(entry, head->prev, head);
}

INLINE void list_del(struct amdgv_list_head *prev, struct amdgv_list_head *next)
{
	next->prev = prev;
	prev->next = next;
}

/*
 * Remove the element from the list it is in.
 */
INLINE void amdgv_list_del(struct amdgv_list_head *entry)
{
	list_del(entry->prev, entry->next);
}

INLINE void amdgv_list_del_init(struct amdgv_list_head *entry)
{
	list_del(entry->prev, entry->next);
	AMDGV_INIT_LIST_HEAD(entry);
}

INLINE void amdgv_list_move_tail(struct amdgv_list_head *list, struct amdgv_list_head *head)
{
	list_del(list->prev, list->next);
	amdgv_list_add_tail(list, head);
}

/*
 * Check if the list is empty.
 */
INLINE bool amdgv_list_empty(struct amdgv_list_head *head)
{
	if (head != OSS_INVALID_HANDLE && head->next != OSS_INVALID_HANDLE)
		return head->next == head;
	//list not initialized return empty
	return true;
}

/*
 * Returns a pointer to the container of this list element.
 */
#ifndef container_of
#define container_of(ptr, type, member)                                                       \
	((type *)((char *)(ptr) - (char *)&((type *)0)->member))
#endif

/*
 * Alias of container_of
 */
#define amdgv_list_entry(ptr, type, member) container_of(ptr, type, member)

/*
 * Retrieve the first list entry for the given list pointer.
 */
#define amdgv_list_first_entry(ptr, type, member) amdgv_list_entry((ptr)->next, type, member)

/*
 * Retrieve the last list entry for the given list pointer.
 */
#define amdgv_list_last_entry(ptr, type, member) amdgv_list_entry((ptr)->prev, type, member)

/*
 * Loop through the list given by head and set pos to struct in the list.
 */
#define amdgv_list_for_each_entry(pos, head, type, member)                                    \
	for (pos = container_of((head)->next, type, member); &pos->member != (head);          \
	     pos = container_of(pos->member.next, type, member))

/*
 * Loop through the list, keeping a backup pointer to the element. This
 * macro allows for the deletion of a list element while looping through the
 * list.
 */
#define amdgv_list_for_each_entry_safe(pos, tmp, head, type, member)                          \
	for (pos = container_of((head)->next, type, member),                                  \
	    tmp = container_of(pos->member.next, type, member);                               \
	     &pos->member != (head);                                                          \
	     pos = tmp, tmp = container_of(pos->member.next, type, member))

#endif
