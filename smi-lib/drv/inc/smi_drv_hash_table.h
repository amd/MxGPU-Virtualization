/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SMI_DRV_HASH_TABLE_H__
#define __SMI_DRV_HASH_TABLE_H__

#include "smi_drv_types.h"

enum SMI_HASH_TABLE_RET_CODES {
	SMI_HASH_TABLE_SUCCESS, // Successful Hash Table operation
	SMI_HASH_TABLE_KEY_ERROR, // Invalid Hash Table key passed
	SMI_HASH_TABLE_INVALID_PARAM, // Invalid input parameter
	SMI_HASH_TABLE_MEMORY_ERROR // Unable to allocate memory
};

struct hash_table_element {
	uint64_t key;
	void *data;
	struct hash_table_element *next_element;
};

struct hash_table {
	unsigned int num_of_elements;
	unsigned int (*hash_function)(unsigned int num_of_elements,
					  uint64_t key);
	struct hash_table_element **table;
	struct oss_interface *oss_funcs;
	void *mutex;
};

/**
 *  \brief          Default function for hashing, if no function is provided.
 *                  Implements a simple modulus operation.
 *
 *  \param [in]     num_of_elements - total number of entries to the hash table
 *
 *  \param [in]     key - key to compute table index for
 *
 *  \return         Index of key in hash table
 */
unsigned int default_hash_function(unsigned int num_of_elements, uint64_t key);

/**
 *  \brief          Allocates space for the hash table and returns pointer to
 * it.
 *
 *  \param [in]     num_of_rows - Number of rows in the hash table. Does not
 * represent the maximum number of entries. Greater number means more space
 * allocated in exchange for better performance.
 *
 *  \param [in]     function - Hash function to be used. NULL defaults to
 * default_hash_function()
 *
 *  \param [in]     oss_funcs - Pointer to OS independent interface functions
 *
 *  \return         Pointer to the allocated hash table
 */
struct hash_table *create_hash_table(unsigned int num_of_rows,
					 unsigned int (*function)(unsigned int,
								  uint64_t), struct oss_interface *oss_funcs);

/**
 *  \brief          Frees memory allocated for the hash table.
 *
 *  \param [in]     table - Hash Table to destroy
 *
 *  \return         SMI_HASH_TABLE_SUCCESS - Successful
 *                  SMI_HASH_TABLE_INVALID_PARAM - Parameters are not valid or
 * NULL
 */
enum SMI_HASH_TABLE_RET_CODES destroy_hash_table(struct hash_table *table);

/**
 *  \brief          Insert given data for the given key to the table.
 *
 *  \param [in]     table - Hash table to add data to.
 *
 *  \param [in]     key - Key to store data for.
 *
 *  \param [in]     data - Pointer to data to be added to the table for the
 * given key value.
 *
 *  \return         SMI_HASH_TABLE_SUCCESS - Successful
 *                  SMI_HASH_TABLE_KEY_ERROR - Key already present in the table
 *                  SMI_HASH_TABLE_INVALID_PARAM - Parameters are not valid or
 * NULL
 */
enum SMI_HASH_TABLE_RET_CODES add_to_hash_table(struct hash_table *table,
						uint64_t key, void *data);

/**
 *  \brief          Deletes data from the given hash table for the given key
 * value.
 *
 *  \param [in]     table - Hash Table to remove data from.
 *
 *  \param [in]     key - Key to remove the data for.
 *
 *  \return         SMI_HASH_TABLE_SUCCESS - Successful
 *                  SMI_HASH_TABLE_KEY_ERROR - Key not present in the table
 *                  SMI_HASH_TABLE_INVALID_PARAM - Parameters are not valid or
 * NULL
 */
enum SMI_HASH_TABLE_RET_CODES remove_from_hash_table(struct hash_table *table,
							 uint64_t key);

/**
 *  \brief          Pops data from the given hash table for the given key
 * value.
 *
 *  \param [in]     table - Hash Table to remove data from.
 *
 *  \param [in]     key - Key to remove the data for.
 *
 *  \param [in]     data - Pointer where the data should be returned.
 *
 *  \return         SMI_HASH_TABLE_SUCCESS - Successful
 *                  SMI_HASH_TABLE_KEY_ERROR - Key not present in the table
 *                  SMI_HASH_TABLE_INVALID_PARAM - Parameters are not valid or
 * NULL
 */
enum SMI_HASH_TABLE_RET_CODES pop_from_hash_table(struct hash_table *table,
						  uint64_t key, void *data);

/**
 *  \brief          For the given hash table and key returns data.
 *
 *  \param [in]     table - Hash Table to query for the given key.
 *
 *  \param [in]     key - Key to query the data for.
 *
 *  \param [in]     data - Pointer to data for the given key to be returned
 *
 *  \return         SMI_HASH_TABLE_SUCCESS - Successful
 *                  SMI_HASH_TABLE_KEY_ERROR - Key not present in the table
 *                  SMI_HASH_TABLE_INVALID_PARAM - Parameters are not valid or
 * NULL
 */
enum SMI_HASH_TABLE_RET_CODES get_from_hash_table(struct hash_table *table,
						  uint64_t key, void **data);
/**
 *  \brief          Clears the data from the table.
 *
 *  \param [in]     table - Hash Table to clear.
 *
 *  \return         SMI_HASH_TABLE_SUCCESS - Successful
 *                  SMI_HASH_TABLE_KEY_ERROR - Key not present in the table
 *                  SMI_HASH_TABLE_INVALID_PARAM - Parameters are not valid or
 * NULL
 */
enum SMI_HASH_TABLE_RET_CODES clear_table(struct hash_table *table);

#endif // __SMI_DRV_HASH_TABLE_H__
