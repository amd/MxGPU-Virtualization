/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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
 * THE SOFTWARE
 */

#include <linux/stacktrace.h>
#include <linux/kallsyms.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/mm.h>

#include "gim_debug.h"
#include "gim_config.h"
#include "gim.h"
#include "gim_memory_sentinel.h"

#define GIM_MEMORY_SENTINEL_SAVE_STACK_BUFFER_SIZE 16
#define GIM_MEMORY_SENTINEL_ALLOC_RETRY_TIME 1
#define GIM_MEMORY_SENTINEL_DEFAULT_TABLE_SIZE 4
#define GIM_MEMORY_SENTINEL_TABLE_FULL_WARNING_LOG_THRESHOLD 1
#ifndef KSYM_NAME_LEN
#define KSYM_NAME_LEN		512
#endif

struct gim_memory_sentinel_entry {
	void *pointer;
	void *alloc_func_symbol;
	struct {
		uint16_t status;
		uint16_t type;
		uint32_t size;
	};
};

struct gim_memory_sentinel_table {
	uint64_t row_bitmap;
	uint64_t col_bitmap[GIM_MEMORY_SENTINEL_TABLE_COL];
	struct gim_memory_sentinel_entry entries[GIM_MEMORY_SENTINEL_TABLE_ROW * GIM_MEMORY_SENTINEL_TABLE_COL];
};

/*
 * Memory Sentinel Manager could manage 64 tables at most.
 * each bit in table_bitmap represent a table.
 * 1 means the table is available, 0 means the table is full or not created.
 * tables_count is the number of tables that are currently created
*/
struct gim_memory_sentinel_manager {
	atomic64_t table_bitmap;
	struct gim_memory_sentinel_table *tables[64];
	uint32_t tables_count;
	uint32_t detect_mode;
	spinlock_t lock;
};

struct gim_memory_sentinel_manager *gim_memory_sentinel;

static inline void *_kmalloc(size_t size, gfp_t flags)
{
	return kmalloc(size, flags);
}

static inline void *_kzalloc(size_t size, gfp_t flags)
{
	return kzalloc(size, flags);
}

static inline void *_kmalloc_array(size_t n, size_t size, gfp_t flags)
{
	return kmalloc_array(n, size, flags);
}

static inline void *_vmalloc(size_t size)
{
	return vmalloc(size);
}

static inline void *_vzalloc(size_t size)
{
	return vzalloc(size);
}

void *(*gim_kmalloc)(size_t size, gfp_t flags) = _kmalloc;
void *(*gim_kzalloc)(size_t size, gfp_t flags) = _kzalloc;
void *(*gim_kmalloc_array)(size_t n, size_t size, gfp_t flags) = _kmalloc_array;
void *(*gim_vmalloc)(size_t size) = _vmalloc;
void *(*gim_vzalloc)(size_t size) = _vzalloc;

void (*gim_kfree)(const void *p) = kfree;
void (*gim_vfree)(const void *p) = vfree;

#if !defined(HAVE_KFREE_SENSITIVE)
	void (*gim_kzfree)(const void *p) = kzfree;
#else
	void (*gim_kfree_sensitive)(const void *p) = kfree_sensitive;
#endif

static void *gim_sentinel_kmalloc(size_t size, unsigned int flags);
static void *gim_sentinel_kzalloc(size_t size, unsigned int flags);
static void *gim_sentinel_kmalloc_array(size_t n, size_t size, gfp_t flags);
static void *gim_sentinel_vmalloc(size_t size);
static void *gim_sentinel_vzalloc(size_t size);

static void gim_sentinel_kfree(const void *p);
static void gim_sentinel_vfree(const void *p);
static void gim_sentinel_secure_kfree(const void *p);


static inline bool is_vmalloc_range(const void *p)
{
	return ((uint64_t)p >= VMALLOC_START && (uint64_t)p <= VMALLOC_END);
}

static struct gim_memory_sentinel_entry *gim_memory_sentinel_get_entry_by_index(struct gim_memory_sentinel_manager *msm, uint32_t index)
{
	uint32_t table_index;
	struct gim_memory_sentinel_table *table;

	if (unlikely(!msm || index >= GIM_MEMORY_SENTINEL_TABLE_ENTRY_INDEX_MAX))
		return NULL;

	table_index = (index >> (GIM_MEMORY_SENTINEL_BITMAP_ORDER * 2)) & 0x3F;
	if (unlikely(table_index >= msm->tables_count))
		return NULL;

	table = msm->tables[table_index];
	if (unlikely(!table))
		return NULL;

	return &table->entries[index & (GIM_MEMORY_SENTINEL_TABLE_ROW * GIM_MEMORY_SENTINEL_TABLE_COL - 1)];
}

static int gim_memory_sentinel_table_entry_check_overflow_normal(struct gim_memory_sentinel_entry *entry, uint32_t index)
{
	int ret = 0;
	void *ptr;
	uint64_t *rear;
	struct gim_memory_sentinel_malloc_header *header;

	ptr = entry->pointer;
	if (!ptr) {
		return ret;
	}

	header = (struct gim_memory_sentinel_malloc_header *)ptr;

	if (header->size != entry->size || header->index != index) {
		gim_warn("Sentinel Check: memory 0x%016llx header broken, which allocated by %pS, size: %d\n",
			(uint64_t)(header + sizeof(*header)), entry->alloc_func_symbol, entry->size);
		entry->status |= GIM_MEMORY_SENTINEL_ENTRY_HEADER_BROKEN;
		ret = GIM_MEMORY_SENTINEL_ERROR_OVERFLOW;
	}

	rear = ptr + entry->size - sizeof(uint64_t);

	if ((uint64_t)entry->alloc_func_symbol != *(uint64_t *)rear) {
		gim_warn("Sentinel Check: memory 0x%016llx write overflow, which allocated by %pS, size: %ld\n",
			(uint64_t)(header + sizeof(*header)), entry->alloc_func_symbol, header->size - sizeof(*header) - sizeof(*rear));
		entry->status |= GIM_MEMORY_SENTINEL_ENTRY_REAR_BROKEN;
		ret = GIM_MEMORY_SENTINEL_ERROR_OVERFLOW;
	}
	return ret;
}

static int gim_memory_sentinel_table_entry_check_overflow_page_align(struct gim_memory_sentinel_entry *entry, uint32_t index)
{
	int ret = 0;
	void *ptr;
	uint64_t *rear;
	struct gim_memory_sentinel_malloc_header *header;

	// there are two kinds of situation when alloc_func_symbol is not equal to rear:
	// 1. rear is broken
	// 2. header is broken and someone write a "valid" entry index into header. So we need to double check alloc_ptr to verify

	ptr = entry->pointer;
	if (!ptr) {
		return ret;
	}

	header = ptr + PAGE_SIZE - sizeof(struct gim_memory_sentinel_malloc_header);

	if (header->size != entry->size || header->index != index) {
		gim_warn("Sentinel Check: memory 0x%016llx header broken, which allocated by %pS, size: %d\n",
			(uint64_t)(header + sizeof(*header)), entry->alloc_func_symbol, entry->size);
		entry->status |= GIM_MEMORY_SENTINEL_ENTRY_REAR_BROKEN;
		ret = GIM_MEMORY_SENTINEL_ERROR_OVERFLOW;
	}

	rear = ptr + entry->size - PAGE_SIZE;

	if ((uint64_t)entry->alloc_func_symbol != *(uint64_t *)rear) {
		gim_warn("Sentinel Check: memory 0x%016llx write overflow, which allocated by %pS, size: %ld\n",
			(uint64_t)(header + sizeof(*header)), entry->alloc_func_symbol, header->size - (PAGE_SIZE << 1));
		entry->status |= GIM_MEMORY_SENTINEL_ENTRY_REAR_BROKEN;
		ret = GIM_MEMORY_SENTINEL_ERROR_OVERFLOW;
	}
	return ret;
}

static int gim_memory_sentinel_table_entry_check_overflow(struct gim_memory_sentinel_entry *entry, uint32_t index)
{
	void *ptr;
	int ret = 0;

	if (unlikely(!entry || index >= GIM_MEMORY_SENTINEL_TABLE_ENTRY_INDEX_MAX)) {
		return GIM_MEMORY_SENTINEL_ERROR_INVALID_INPUT;
	}

	ptr = entry->pointer;

	if (ptr) {
		if (entry->type == GIM_MEMORY_SENTINEL_ALLOC_TYPE_NORMAL) {
			ret = gim_memory_sentinel_table_entry_check_overflow_normal(entry, index);
		} else if (entry->type == GIM_MEMORY_SENTINEL_ALLOC_TYPE_PAGE_ALIGN) {
			ret = gim_memory_sentinel_table_entry_check_overflow_page_align(entry, index);
		}
	}

	return ret;
}

static int gim_memory_sentinel_table_entry_check_memleak(struct gim_memory_sentinel_entry *entry)
{
	void *ptr;
	int ret = 0;

	if (!entry) {
		return GIM_MEMORY_SENTINEL_ERROR_INVALID_INPUT;
	}

	ptr = entry->pointer;
	if (ptr) {
		gim_warn("Sentinel Check: memory leak detected, pointer: 0x%llx, size: %u, func: %pS\n", (uint64_t)ptr, entry->size, (char *)entry->alloc_func_symbol);
		if (entry->type != GIM_MEMORY_SENTINEL_ALLOC_TYPE_NORMAL && entry->type != GIM_MEMORY_SENTINEL_ALLOC_TYPE_PAGE_ALIGN)
			return GIM_MEMORY_SENTINEL_ERROR_MEMLEAK;

		if (virt_addr_valid(ptr))
			kfree(ptr);
		else if (is_vmalloc_range(ptr))
			vfree(ptr);
		ret = GIM_MEMORY_SENTINEL_ERROR_MEMLEAK;
	}
	return ret;
}

static struct gim_memory_sentinel_table *gim_memory_sentinel_table_new(void)
{
	struct gim_memory_sentinel_table *table = NULL;

	table = kmalloc(sizeof(*table), GFP_KERNEL);
	if (!table) {
		return NULL;
	}

	table->row_bitmap = GIM_MEMORY_SENTINEL_BITMAP_INIT;

	memset(table->col_bitmap, 0xFF, sizeof(uint64_t) * GIM_MEMORY_SENTINEL_TABLE_COL);

	// Initialize all entries to zero
	memset(table->entries, 0, sizeof(struct gim_memory_sentinel_entry) *
		   GIM_MEMORY_SENTINEL_TABLE_ROW * GIM_MEMORY_SENTINEL_TABLE_COL);

	return table;
}

static uint32_t __gim_sentinel_check_memory_overflow(struct gim_memory_sentinel_manager *msm)
{
	struct gim_memory_sentinel_table *table = NULL;
	uint32_t index;
	uint32_t table_index;
	uint32_t overflow_cnt = 0;
	int ret;
	int row = 0, col = 0;

	if (!msm) {
		gim_warn("sentinel manager is not initialized\n");
		return GIM_MEMORY_SENTINEL_ERROR_INVALID_INPUT;
	}

	if (msm->detect_mode != 1) {
		gim_info("sentinel manager is not in check mode\n");
		return GIM_MEMORY_SENTINEL_ERROR_INVALID_INPUT;
	}

	spin_lock(&msm->lock);

	for (table_index = 0; table_index < msm->tables_count; table_index++) {
		table = msm->tables[table_index];

		for (row = 0 ; row < GIM_MEMORY_SENTINEL_TABLE_ROW ; row++) {
			if (GIM_MEMORY_SENTINEL_BITMAP_INIT == table->col_bitmap[row]) {
				// this line is empty
				continue;
			}
			for (col = 0 ; col < GIM_MEMORY_SENTINEL_TABLE_COL ; col++) {
				if (1 == (table->col_bitmap[row] & ((uint64_t)1 << col))) {
					// this entry is freed, will not check it
					continue;
				}
				index = row * GIM_MEMORY_SENTINEL_TABLE_COL + col;
				ret = gim_memory_sentinel_table_entry_check_overflow(&table->entries[index], index);
				if (GIM_MEMORY_SENTINEL_ERROR_OVERFLOW == ret) {
					overflow_cnt++;
				}
			}
		}
	}

	spin_unlock(&msm->lock);

	return overflow_cnt;
}

uint32_t gim_sentinel_check_memory_overflow(void)
{
	return __gim_sentinel_check_memory_overflow(gim_memory_sentinel);
}

static void gim_memory_sentinel_table_free(struct gim_memory_sentinel_table *table)
{
	int index = 0;
	int row = 0, col = 0;

	if (!table) {
		return;
	}

	// check memleak before free table
	for (row = 0 ; row < GIM_MEMORY_SENTINEL_TABLE_ROW ; row++) {
		if (GIM_MEMORY_SENTINEL_BITMAP_INIT == table->col_bitmap[row]) {
			// this line is empty
			continue;
		}
		for (col = 0 ; col < GIM_MEMORY_SENTINEL_TABLE_COL ; col++) {
			if (1 == (table->col_bitmap[row] & ((uint64_t)1 << col))) {
				// this entry is freed, will not check it
				continue;
			}
			index = row * GIM_MEMORY_SENTINEL_TABLE_COL + col;
			gim_memory_sentinel_table_entry_check_overflow(&table->entries[index], index);
			gim_memory_sentinel_table_entry_check_memleak(&table->entries[index]);
		}
	}

	table->row_bitmap = 0;

	kfree(table);
}

static int gim_memroy_sentinel_extend_table(struct gim_memory_sentinel_manager *msm)
{
	struct gim_memory_sentinel_table *table = NULL;

	if (msm->tables_count >= GIM_MEMORY_SENTINEL_TABLE_MAX) {
		return GIM_MEMORY_SENTINEL_ERROR_INVALID_INPUT;
	}

	table = gim_memory_sentinel_table_new();
	if (table) {
		msm->tables[msm->tables_count] = table;
		atomic64_or((uint64_t)1 << msm->tables_count, &msm->table_bitmap);
		msm->tables_count++;
		gim_dbg("table size: %d, row_bitmap: 0x%016llx, col_bitmap: 0x%016llx, table: 0x%016llx, msm->table_bitmap: 0x%016llx\n",
			msm->tables_count, table->row_bitmap, table->col_bitmap[0], (uint64_t)table, atomic64_read(&msm->table_bitmap));
		return 0;
	}

	gim_warn("extend table %d failed\n", msm->tables_count);
	return GIM_MEMORY_SENTINEL_ERROR_NO_MEM;
}

static void __gim_memory_sentinel_init(struct gim_memory_sentinel_manager **msm, uint32_t detect_mode)
{
	int i = 0;
	if (unlikely(!msm)) {
		return;
	}

	if (NULL != (*msm)) {
		gim_warn("gim_memory_sentinel manager already initialized\n");
		return;
	}
	(*msm) = kzalloc(sizeof(struct gim_memory_sentinel_manager), GFP_KERNEL);

	if (!(*msm)) {
		gim_warn("gim_memory_sentinel manager malloc failed\n");
		return;
	}

	if (detect_mode < SENTINEL_MODE__START || detect_mode > SENTINEL_MODE__MAX) {
		gim_warn("detect mode %d is invalid\n", detect_mode);
		detect_mode = SENTINEL_MODE__DEFAULT;
	}

	(*msm)->detect_mode = detect_mode;

	// Set bitmap and size to 0
	atomic64_set(&(*msm)->table_bitmap, 0);
	(*msm)->tables_count = 0;

	if (detect_mode == 0) {
		// Do not allocate table if detect mode is 0.
		// Unless some other module need to use it.
		return;
	}
	spin_lock_init(&(*msm)->lock);

	if (detect_mode == 1) {
		while (i < GIM_MEMORY_SENTINEL_DEFAULT_TABLE_SIZE) {
			if (gim_memroy_sentinel_extend_table(*msm)) {
				gim_warn("gim_memory_sentinel_extend_table failed\n");
				kfree(*msm);
				(*msm) = NULL;
				return;
			}
			i++;
		}

		gim_kmalloc = gim_sentinel_kmalloc;
		gim_kzalloc = gim_sentinel_kzalloc;
		gim_kmalloc_array = gim_sentinel_kmalloc_array;
		gim_vmalloc = gim_sentinel_vmalloc;
		gim_vzalloc = gim_sentinel_vzalloc;
		gim_kfree = gim_sentinel_kfree;
		gim_vfree = gim_sentinel_vfree;
#if !defined(HAVE_KFREE_SENSITIVE)
		gim_kzfree = gim_sentinel_secure_kfree;
#else
		gim_kfree_sensitive = gim_sentinel_secure_kfree;
#endif

	} else {
		return;
	}

	gim_info("GIM memory sentinel enabled, detect mode: %d\n", detect_mode);
}

static void __gim_memory_sentinel_fini(struct gim_memory_sentinel_manager **msm)
{
	int i;

	if (!msm || !(*msm)) {
		gim_warn("gim_memory_sentinel manager is not initialized\n");
		return;
	}

	// set malloc/free function to original to avoid some other module alloc/free memory after gim_exit()
	gim_kmalloc = _kmalloc;
	gim_kzalloc = _kzalloc;
	gim_kmalloc_array = _kmalloc_array;
	gim_vmalloc = _vmalloc;
	gim_vzalloc = _vzalloc;
	gim_kfree = kfree;
	gim_vfree = vfree;
#if !defined(HAVE_KFREE_SENSITIVE)
	gim_kzfree = kzfree;
#else
	gim_kfree_sensitive = kfree_sensitive;
#endif

	// Free all allocated tables
	for (i = 0; i < (*msm)->tables_count; i++) {
		if ((*msm)->tables[i]) {
			gim_memory_sentinel_table_free((*msm)->tables[i]);
			(*msm)->tables[i] = NULL;
		}
	}

	// Clear bitmap and reset size
	atomic64_set(&(*msm)->table_bitmap, 0);
	(*msm)->tables_count = 0;

	kfree((*msm));

	(*msm) = NULL;
}

void gim_memory_sentinel_init(void)
{
	__gim_memory_sentinel_init(&gim_memory_sentinel, gim_conf_get_sentinel_mode_opt());
}

void gim_memory_sentinel_fini(void)
{
	__gim_memory_sentinel_fini(&gim_memory_sentinel);
}

bool gim_sentinel_is_enabled(void)
{
	if (gim_memory_sentinel && gim_memory_sentinel->detect_mode != 0) {
		return true;
	}
	return false;
}

static uint32_t __gim_memory_sentinel_insert_entry(struct gim_memory_sentinel_manager *msm, void *pointer, void *alloc_func_symbol, uint32_t size, uint16_t type)
{
	uint32_t i, j;
	struct gim_memory_sentinel_table *table;
	uint32_t table_index;
	uint32_t entry_index;
	static int warning_log_threshold = GIM_MEMORY_SENTINEL_TABLE_FULL_WARNING_LOG_THRESHOLD;

	// If table is full or extend table fail, return GIM_MEMORY_SENTINEL_TABLE_ENTRY_INDEX_MAX + 1 and do not insert any entry
	if (unlikely(!msm)) {
		gim_warn("msm is NULL\n");
		return GIM_MEMORY_SENTINEL_TABLE_ENTRY_INDEX_MAX + 1;
	}

	spin_lock(&msm->lock);

	if (unlikely(!atomic64_read(&msm->table_bitmap))) {
		spin_unlock(&msm->lock);
		goto table_full;
	}

	table_index = __ffs64(atomic64_read(&msm->table_bitmap));
	table = msm->tables[table_index];

	i = __ffs64(table->row_bitmap);
	j = __ffs64(table->col_bitmap[i]);
	entry_index = i * GIM_MEMORY_SENTINEL_TABLE_COL + j;
	table->entries[entry_index].alloc_func_symbol = alloc_func_symbol;
	table->entries[entry_index].size = size;
	table->entries[entry_index].status = 0;
	table->entries[entry_index].type = type;
	table->entries[entry_index].pointer = pointer;
	// clear correspond bit of entry j
	table->col_bitmap[i] &= (~((uint64_t)1 << j));
	// clear correspond bit of list i if list i is full
	table->row_bitmap &= (~((uint64_t)(!table->col_bitmap[i]) << i));
	atomic64_and(~((uint64_t)(!table->row_bitmap) << table_index), &msm->table_bitmap);

	gim_dbg("table %d, row bitmap: 0x%016llx, col_bitmap[%d]: 0x%016llx\n", table_index, table->row_bitmap, i, table->col_bitmap[i]);

	spin_unlock(&msm->lock);

	return entry_index + (table_index * (GIM_MEMORY_SENTINEL_TABLE_ROW * GIM_MEMORY_SENTINEL_TABLE_COL));

table_full:

	if (warning_log_threshold > 0) {
		gim_warn("gim_memory_sentinel table is full\n");
		warning_log_threshold--;
	}

	return GIM_MEMORY_SENTINEL_TABLE_ENTRY_INDEX_MAX + 1;
}

static void *gim_sentinel_malloc_helper(void *ptr, size_t size, uint64_t alloc_func_symbol, uint16_t type)
{
	struct gim_memory_sentinel_malloc_header *header;
	uint32_t index;
	uint64_t *rear;
	void *ret_ptr = NULL;
	size_t total_size;

	if (GIM_MEMORY_SENTINEL_ALLOC_TYPE_PAGE_ALIGN == type) {
		total_size = PAGE_SIZE + size + PAGE_SIZE;
		header = ptr + PAGE_SIZE - sizeof(struct gim_memory_sentinel_malloc_header);
		rear = ptr + PAGE_SIZE + size;
		ret_ptr = ptr + PAGE_SIZE;
	} else {
		total_size = sizeof(struct gim_memory_sentinel_malloc_header) + size + sizeof(uint64_t);
		header = ptr;
		rear = ptr + total_size - sizeof(uint64_t);
		ret_ptr = ptr + sizeof(struct gim_memory_sentinel_malloc_header);
	}

	header->size = (uint32_t)total_size;
	*rear = alloc_func_symbol;

	index = __gim_memory_sentinel_insert_entry(gim_memory_sentinel, ptr, (void *)alloc_func_symbol, total_size, type);

	if (unlikely(GIM_MEMORY_SENTINEL_TABLE_ENTRY_INDEX_MAX <= index)) {
		gim_dbg("sentinel table is full, ptr 0x%llx will not be monitor\n", (uint64_t)ptr);
		return ptr;
	}
	header->index = index;

	gim_dbg("function %pS malloc %ld, return ptr: 0x%016llx, malloc ptr: 0x%016llx, total size: %ld, index: %d, type: %d, alloc func: %pS\n",
		(void *)alloc_func_symbol, size, (uint64_t)ret_ptr, (uint64_t)ptr, total_size, index, type, (void *)*rear);

	return ret_ptr;
}

static inline unsigned long gim_sentinel_get_allocate_stack(void)
{
	unsigned long entries[GIM_MEMORY_SENTINEL_SAVE_STACK_BUFFER_SIZE];
	unsigned long record_entry = 0;
	int num_entries, i;

	num_entries = stack_trace_save(entries, ARRAY_SIZE(entries), 1);

	for (i = 0; i < num_entries; i++) {
		char sym_name[KSYM_NAME_LEN];

		sprint_symbol(sym_name, entries[i]);

		if (!strstr(sym_name, "alloc")) {
			record_entry = entries[i];
			break;
		}
	}
	return record_entry;
}

static void *gim_sentinel_kmalloc(size_t size, unsigned int flags)
{
	uint16_t type = GIM_MEMORY_SENTINEL_ALLOC_TYPE_NORMAL;
	size_t total_size;
	void *ptr;
	unsigned long record_entry;

	if (size > (PAGE_SIZE << GIM_MEMORY_SENTINEL_ALLOC_MAX_ORDER)) {
		gim_warn("alloc size %ld bigger than max order, return NULL\n", size);
		return NULL;
	}

	record_entry = gim_sentinel_get_allocate_stack();

	if (size & (PAGE_SIZE - 1) || !size) {
		total_size = sizeof(struct gim_memory_sentinel_malloc_header) + size + sizeof(uint64_t);
	} else {
		type = GIM_MEMORY_SENTINEL_ALLOC_TYPE_PAGE_ALIGN;
		total_size = PAGE_SIZE + size + PAGE_SIZE;
	}
	ptr = kmalloc(total_size, flags);
	if (!ptr) {
		gim_warn("function %pS malloc size 0x%lx fail\n", (void *)record_entry, size);
		return NULL;
	}

	return gim_sentinel_malloc_helper(ptr, size, (uint64_t)record_entry, type);
}

static void *gim_sentinel_kzalloc(size_t size, unsigned int flags)
{
	uint16_t type = GIM_MEMORY_SENTINEL_ALLOC_TYPE_NORMAL;
	size_t total_size;
	void *ptr;
	unsigned long record_entry;

	if (size > (PAGE_SIZE << GIM_MEMORY_SENTINEL_ALLOC_MAX_ORDER)) {
		gim_warn("alloc size %ld bigger than max order, return NULL\n", size);
		return NULL;
	}

	record_entry = gim_sentinel_get_allocate_stack();

	if (size & (PAGE_SIZE - 1) || !size) {
		total_size = sizeof(struct gim_memory_sentinel_malloc_header) + size + sizeof(uint64_t);
	} else {
		type = GIM_MEMORY_SENTINEL_ALLOC_TYPE_PAGE_ALIGN;
		total_size = PAGE_SIZE + size + PAGE_SIZE;
	}
	ptr = kzalloc(total_size, flags);
	if (!ptr) {
		gim_warn("function %pS malloc size 0x%lx fail\n", (void *)record_entry, size);
		return NULL;
	}

	return gim_sentinel_malloc_helper(ptr, size, (uint64_t)record_entry, type);
}

static void *gim_sentinel_kmalloc_array(size_t n, size_t size, gfp_t flags)
{
	return gim_sentinel_kmalloc(n * size, flags);
}

static void *gim_sentinel_vmalloc(size_t size)
{
	uint16_t type = GIM_MEMORY_SENTINEL_ALLOC_TYPE_NORMAL;
	size_t total_size;
	void *ptr;
	unsigned long record_entry;

	if (size > (totalram_pages() << PAGE_SHIFT)) {
		gim_warn("alloc size %ld bigger than total ram, return NULL\n", size);
		return NULL;
	}

	record_entry = gim_sentinel_get_allocate_stack();

	if (size & (PAGE_SIZE - 1) || !size) {
		total_size = sizeof(struct gim_memory_sentinel_malloc_header) + size + sizeof(uint64_t);
	} else {
		type = GIM_MEMORY_SENTINEL_ALLOC_TYPE_PAGE_ALIGN;
		total_size = PAGE_SIZE + size + PAGE_SIZE;
	}
	ptr = vmalloc(total_size);
	if (!ptr) {
		gim_warn("function %pS malloc size 0x%lx fail\n", (void *)record_entry, size);
		return NULL;
	}

	return gim_sentinel_malloc_helper(ptr, size, (uint64_t)record_entry, type);
}

static void *gim_sentinel_vzalloc(size_t size)
{
	uint16_t type = GIM_MEMORY_SENTINEL_ALLOC_TYPE_NORMAL;
	size_t total_size;
	void *ptr;
	unsigned long record_entry;

	if (size > (totalram_pages() << PAGE_SHIFT)) {
		gim_warn("alloc size %ld bigger than total ram, return NULL\n", size);
		return NULL;
	}

	record_entry = gim_sentinel_get_allocate_stack();

	if (size & (PAGE_SIZE - 1) || !size) {
		total_size = sizeof(struct gim_memory_sentinel_malloc_header) + size + sizeof(uint64_t);
	} else {
		type = GIM_MEMORY_SENTINEL_ALLOC_TYPE_PAGE_ALIGN;
		total_size = PAGE_SIZE + size + PAGE_SIZE;
	}
	ptr = vzalloc(total_size);
	if (!ptr) {
		gim_warn("function %pS malloc size 0x%lx fail\n", (void *)record_entry, size);
		return NULL;
	}

	return gim_sentinel_malloc_helper(ptr, size, (uint64_t)record_entry, type);
}

static int __gim_memory_sentinel_remove_entry(struct gim_memory_sentinel_manager *msm, uint32_t index, bool internal_remove)
{
	int i, j;
	struct gim_memory_sentinel_table *table;
	uint32_t table_index;
	uint32_t entry_index;
	uint16_t type;

	if (unlikely(!msm || index >= GIM_MEMORY_SENTINEL_TABLE_ENTRY_INDEX_MAX))
		return GIM_MEMORY_SENTINEL_ERROR_INVALID_INPUT;

	table_index = (index >> (GIM_MEMORY_SENTINEL_BITMAP_ORDER * 2)) & 0x3F;
	if (unlikely(table_index >= msm->tables_count))
		return GIM_MEMORY_SENTINEL_ERROR_INVALID_INPUT;

	table = msm->tables[table_index];
	if (unlikely(!table))
		return GIM_MEMORY_SENTINEL_ERROR_INVALID_INPUT;

	entry_index = index & (GIM_MEMORY_SENTINEL_TABLE_ROW * GIM_MEMORY_SENTINEL_TABLE_COL - 1);
	i = (entry_index >> GIM_MEMORY_SENTINEL_BITMAP_ORDER) & 0x3F;
	j = entry_index & GIM_MEMORY_SENTINEL_TABLE_ROW_MASK;
	type = table->entries[entry_index].type;

	if (!internal_remove && (GIM_MEMORY_SENTINEL_ALLOC_TYPE_INVALID == type
		|| GIM_MEMORY_SENTINEL_ALLOC_TYPE_NORMAL == type
		|| GIM_MEMORY_SENTINEL_ALLOC_TYPE_PAGE_ALIGN == type)) {
		gim_warn("cannot remove internal entry %d\n", index);
		return GIM_MEMORY_SENTINEL_ERROR_INVALID_INPUT;
	}

	spin_lock(&msm->lock);

	memset(&table->entries[entry_index], 0, sizeof(struct gim_memory_sentinel_entry));

	table->col_bitmap[i] |= ((uint64_t)1 << j);
	table->row_bitmap |= ((uint64_t)1 << i);
	atomic64_or((uint64_t)1 << table_index, &msm->table_bitmap);

	spin_unlock(&msm->lock);

	gim_dbg("table %d, row bitmap: 0x%016llx, col_bitmap[%d]: 0x%016llx\n", table_index, table->row_bitmap, i, table->col_bitmap[i]);

	return 0;
}

static void gim_memory_sentinel_free_by_checktable(struct gim_memory_sentinel_manager *msm, const void *p)
{
	struct gim_memory_sentinel_entry *entry = NULL;
	bool finished_free = false;
	int table_index;
	int row = 0, col = 0;

	// check the whole table;
	for (table_index = 0; table_index < msm->tables_count && !finished_free; table_index++) {
		struct gim_memory_sentinel_table *table = msm->tables[table_index];
		int entry_index = 0;

		for (row = 0 ; row < GIM_MEMORY_SENTINEL_TABLE_ROW ; row++) {
			if (GIM_MEMORY_SENTINEL_BITMAP_INIT == table->col_bitmap[row]) {
				// this line is empty
				continue;
			}
			for (col = 0 ; col < GIM_MEMORY_SENTINEL_TABLE_COL ; col++) {
				void *user_ptr = NULL;

				if (1 == (table->col_bitmap[row] & ((uint64_t)1 << col))) {
					// this entry is freed, will not check it
					continue;
				}
				entry_index = row * GIM_MEMORY_SENTINEL_TABLE_COL + col;

				entry = &table->entries[entry_index];

				if (GIM_MEMORY_SENTINEL_ALLOC_TYPE_NORMAL == entry->type)
					user_ptr = entry->pointer + sizeof(struct gim_memory_sentinel_malloc_header);
				else if (GIM_MEMORY_SENTINEL_ALLOC_TYPE_PAGE_ALIGN == entry->type)
					user_ptr = entry->pointer + PAGE_SIZE;

				if (p == user_ptr) {
					if (virt_addr_valid(entry->pointer)) {
						gim_warn("Sentinel Check: kfree pointer 0x%016llx, header 0x%016llx, header may broken\n", (uint64_t)p, (uint64_t)entry->pointer);
						kfree(entry->pointer);
						finished_free = true;
					} else if (is_vmalloc_range(entry->pointer)) {
						gim_warn("Sentinel Check: vfree pointer 0x%016llx, header 0x%016llx, header may broken\n", (uint64_t)p, (uint64_t)entry->pointer);
						vfree(entry->pointer);
						finished_free = true;
					}
					__gim_memory_sentinel_remove_entry(gim_memory_sentinel, entry_index, true);
				}
			}
		}
	}

	if (!finished_free) {
		if (virt_addr_valid(p)) {
			gim_dbg("kfree p 0x%016llx\n", (uint64_t)p);
			kfree(p);
			finished_free = true;
		} else if (is_vmalloc_range(p)) {
			gim_dbg("vfree p 0x%016llx\n", (uint64_t)p);
			vfree(p);
			finished_free = true;
		} else {
			gim_warn("cannot free invalid address 0x%016llx\n", (uint64_t)p);
		}
	}
}

static bool __gim_memory_sentinel_header_accessible(void *header)
{
	char buffer[sizeof(struct gim_memory_sentinel_malloc_header)];
	return !copy_from_kernel_nofault(buffer, header, sizeof(buffer));
}

static void *gim_sentinel_free_helper(const void *p)
{
	void *ptr;
	uint64_t *rear;
	struct gim_memory_sentinel_malloc_header *header;
	void *alloc_ptr;
	struct gim_memory_sentinel_entry *entry = NULL;
	uint16_t type;

	if (!p) {
		return NULL;
	}

	if (!virt_addr_valid(p) && !is_vmalloc_range(p)) {
		gim_warn("Try to free invalid address 0x%016llx\n", (uint64_t)p);
		return NULL;
	}

	ptr = (void *)(p - sizeof(struct gim_memory_sentinel_malloc_header));
	header = ptr;

	if (!__gim_memory_sentinel_header_accessible(header)) {
		gim_dbg("header 0x%016llx is not accessible\n", (uint64_t)header);
		goto check_table;
	}

	entry = gim_memory_sentinel_get_entry_by_index(gim_memory_sentinel, header->index);

	if (!entry) {
		gim_dbg("cannot get table entry from header, address 0x%016llx may be wrote overflow\n", (uint64_t)ptr);
		goto check_table;
	}

	// there are two kinds of situation when alloc_func_symbol is not equal to rear:
	// 1. rear is broken
	// 2. header is broken and someone write a "valid" entry index into header. So we need to double check alloc_ptr to verify

	if (header->size != entry->size) {
		gim_dbg("header size %d is not equal to entry size %d, header may broken, index: %d, try delay free\n",
			header->size, entry->size, header->index);
		goto check_table;
	}

	type = entry->type;
	if (GIM_MEMORY_SENTINEL_ALLOC_TYPE_NORMAL == type) {
		alloc_ptr = ptr;
		rear = alloc_ptr + entry->size - sizeof(uint64_t);
	} else if (GIM_MEMORY_SENTINEL_ALLOC_TYPE_PAGE_ALIGN == type) {
		alloc_ptr = (void *)(p - PAGE_SIZE);
		rear = alloc_ptr + entry->size - PAGE_SIZE;
	} else {
		gim_warn("%s could not process type %d\n", __func__, type);
		return NULL;
	}

	if (alloc_ptr != entry->pointer) {
		gim_dbg("alloc_ptr 0x%016llx is not equal to entry->pointer 0x%016llx, header may broken, try delay free\n",
			(uint64_t)alloc_ptr, (uint64_t)entry->pointer);
		goto check_table;
	}

	if ((uint64_t)entry->alloc_func_symbol != *(uint64_t *)rear) {
		gim_warn("Sentinel Check: memory 0x%016llx write overflow, which allocated by %pS, size: %ld\n",
			(uint64_t)p, entry->alloc_func_symbol,
			type == GIM_MEMORY_SENTINEL_ALLOC_TYPE_NORMAL ? header->size - sizeof(*header) - sizeof(*rear) : header->size - (PAGE_SIZE << 1));
	}

	__gim_memory_sentinel_remove_entry(gim_memory_sentinel, header->index, true);
	return alloc_ptr;

check_table:
	gim_memory_sentinel_free_by_checktable(gim_memory_sentinel, p);
	return NULL;
}

void gim_sentinel_kfree(const void *p)
{
	void *alloc_ptr = NULL;

	alloc_ptr = gim_sentinel_free_helper(p);

	if (likely(alloc_ptr)) {
		kfree(alloc_ptr);
		gim_dbg("[%s] free allc_ptr 0x%016llx successful, user ptr: 0x%016llx\n", __func__, (uint64_t)alloc_ptr, (uint64_t)p);
	}
}

void gim_sentinel_vfree(const void *p)
{
	void *alloc_ptr = NULL;

	alloc_ptr = gim_sentinel_free_helper(p);

	if (likely(alloc_ptr)) {
		vfree(alloc_ptr);
		gim_dbg("[%s] free allc_ptr 0x%016llx successful, user ptr: 0x%016llx\n", __func__, (uint64_t)alloc_ptr, (uint64_t)p);
	} else {
		gim_dbg("[%s] free allc_ptr 0x%016llx failed, user ptr: 0x%016llx\n", __func__, (uint64_t)alloc_ptr, (uint64_t)p);
	}
}

void gim_sentinel_secure_kfree(const void *p)
{
	void *alloc_ptr = NULL;

	alloc_ptr = gim_sentinel_free_helper(p);

	if (likely(alloc_ptr)) {
#if !defined(HAVE_KFREE_SENSITIVE)
		kzfree(alloc_ptr);
#else
		kfree_sensitive(alloc_ptr);
#endif
		gim_dbg("[%s] secure free allc_ptr 0x%016llx successful, user ptr: 0x%016llx\n", __func__, (uint64_t)alloc_ptr, (uint64_t)p);
	}
}