/*
 * Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "smi_drv_hash_table.h"

#include "amdgv_oss.h"

unsigned int default_hash_function(unsigned int num_of_elements, uint64_t key)
{
	return key % num_of_elements;
}

struct hash_table *create_hash_table(unsigned int num_of_rows,
					 unsigned int (*function)(unsigned int,
								  uint64_t), struct oss_interface *oss_funcs)
{
	struct hash_table *hash = NULL;

	if (num_of_rows == 0)
		return NULL;

	hash = (struct hash_table *)oss_funcs->alloc_small_zero_memory(
		sizeof(struct hash_table));
	if (hash == NULL) {
		return NULL;
	}

	hash->num_of_elements = num_of_rows;
	if (function == NULL) {
		hash->hash_function = default_hash_function;
	} else {
		hash->hash_function = function;
	}

	hash->table = (struct hash_table_element **)oss_funcs->alloc_small_zero_memory(
		hash->num_of_elements * sizeof(struct hash_table_element *));
	if (hash->table == NULL) {
		oss_funcs->free_small_memory(hash);
		return NULL;
	}

	oss_funcs->memset(hash->table, 0,
		   hash->num_of_elements * sizeof(struct hash_table_element *));

	hash->oss_funcs = oss_funcs;

	hash->mutex = hash->oss_funcs->mutex_init();

	return hash;
}

enum SMI_HASH_TABLE_RET_CODES clear_table(struct hash_table *table)
{
	struct hash_table_element *element = NULL;
	if (table == NULL) {
		return SMI_HASH_TABLE_INVALID_PARAM;
	}

	table->oss_funcs->mutex_lock(table->mutex);

	for (unsigned int i = 0; i < table->num_of_elements; ++i) {
		while (table->table[i] != NULL) {
			element = table->table[i];
			table->table[i] = table->table[i]->next_element;
			table->oss_funcs->free_small_memory(element->data);
			table->oss_funcs->free_small_memory(element);
		}
	}

	table->oss_funcs->mutex_unlock(table->mutex);

	return SMI_HASH_TABLE_SUCCESS;
}

enum SMI_HASH_TABLE_RET_CODES destroy_hash_table(struct hash_table *table)
{
	if (table == NULL) {
		return SMI_HASH_TABLE_INVALID_PARAM;
	}

	const int ret_code = clear_table(table);
	if (ret_code != SMI_HASH_TABLE_SUCCESS) {
		return ret_code;
	}

	table->oss_funcs->free_small_memory(table->table);
	table->oss_funcs->mutex_fini(table->mutex);
	table->oss_funcs->free_small_memory(table);

	return SMI_HASH_TABLE_SUCCESS;
}

enum SMI_HASH_TABLE_RET_CODES add_to_hash_table(struct hash_table *table,
						uint64_t key, void *data)
{
	void *new_data = NULL;
	struct hash_table_element *new_element = NULL;
	struct hash_table_element *iterator = NULL;
	struct hash_table_element *prev_iterator = NULL;

	if (table == NULL) {
		return SMI_HASH_TABLE_INVALID_PARAM;
	}

	table->oss_funcs->mutex_lock(table->mutex);
	new_data = data;

	const unsigned int index =
		table->hash_function(table->num_of_elements, key);

	new_element = (struct hash_table_element *)table->oss_funcs->alloc_small_zero_memory(
		sizeof(struct hash_table_element));
	if (new_element == NULL) {
		table->oss_funcs->mutex_unlock(table->mutex);
		return SMI_HASH_TABLE_MEMORY_ERROR;
	}

	new_element->data = new_data;
	new_element->key = key;

	iterator = table->table[index];
	prev_iterator = NULL;
	while (iterator != NULL) {
		if (iterator->key == key) {
			table->oss_funcs->free_small_memory(new_element);
			table->oss_funcs->mutex_unlock(table->mutex);
			return SMI_HASH_TABLE_KEY_ERROR;
		}
		prev_iterator = iterator;
		iterator = iterator->next_element;
	}
	if (prev_iterator == NULL) {
		new_element->next_element = table->table[index];
		table->table[index] = new_element;
	} else {
		new_element->next_element = NULL;
		prev_iterator->next_element = new_element;
	}

	table->oss_funcs->mutex_unlock(table->mutex);
	return SMI_HASH_TABLE_SUCCESS;
}

enum SMI_HASH_TABLE_RET_CODES remove_from_hash_table(struct hash_table *table,
							 uint64_t key)
{
	struct hash_table_element *iterator = NULL;
	struct hash_table_element *prev_iterator = NULL;

	if (table == NULL) {
		return SMI_HASH_TABLE_INVALID_PARAM;
	}

	table->oss_funcs->mutex_lock(table->mutex);
	const unsigned int index =
		table->hash_function(table->num_of_elements, key);

	iterator = table->table[index];
	prev_iterator = NULL;
	while (iterator != NULL) {
		if (iterator->key == key) {
			if (prev_iterator == NULL) {
				table->table[index] = iterator->next_element;
			} else {
				prev_iterator->next_element =
					iterator->next_element;
			}
			table->oss_funcs->free_small_memory(iterator);
			table->oss_funcs->mutex_unlock(table->mutex);
			return SMI_HASH_TABLE_SUCCESS;
		}
		prev_iterator = iterator;
		iterator = iterator->next_element;
	}
	table->oss_funcs->mutex_unlock(table->mutex);
	return SMI_HASH_TABLE_KEY_ERROR;
}

enum SMI_HASH_TABLE_RET_CODES pop_from_hash_table(struct hash_table *table,
						  uint64_t key, void *data)
{
	struct hash_table_element *iterator = NULL;
	struct hash_table_element *prev_iterator = NULL;

	if (table == NULL) {
		return SMI_HASH_TABLE_INVALID_PARAM;
	}

	table->oss_funcs->mutex_lock(table->mutex);

	const unsigned int index =
		table->hash_function(table->num_of_elements, key);
	iterator = table->table[index];
	prev_iterator = NULL;
	while (iterator != NULL) {
		if (iterator->key == key) {
			if (prev_iterator == NULL) {
				table->table[index] = iterator->next_element;
			} else {
				prev_iterator->next_element =
					iterator->next_element;
			}
			data = iterator->data;
			table->oss_funcs->free_small_memory(iterator);
			table->oss_funcs->mutex_unlock(table->mutex);
			return SMI_HASH_TABLE_SUCCESS;
		}
		prev_iterator = iterator;
		iterator = iterator->next_element;
	}
	table->oss_funcs->mutex_unlock(table->mutex);
	return SMI_HASH_TABLE_KEY_ERROR;
}

enum SMI_HASH_TABLE_RET_CODES get_from_hash_table(struct hash_table *table,
						  uint64_t key, void **data)
{
	struct hash_table_element *iterator = NULL;

	if (data == NULL || table == NULL) {
		return SMI_HASH_TABLE_INVALID_PARAM;
	}

	table->oss_funcs->mutex_lock(table->mutex);

	const unsigned int index =
		table->hash_function(table->num_of_elements, key);

	iterator = table->table[index];
	while (iterator != NULL) {
		if (iterator->key == key) {
			*data = iterator->data;
			table->oss_funcs->mutex_unlock(table->mutex);
			return SMI_HASH_TABLE_SUCCESS;
		}
		iterator = iterator->next_element;
	}

	table->oss_funcs->mutex_unlock(table->mutex);
	return SMI_HASH_TABLE_KEY_ERROR;
}
