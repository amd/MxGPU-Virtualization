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

#ifndef AMDGV_OSS_H
#define AMDGV_OSS_H

/* because linux kernel and common toolchain use different macro for stdarg.h
 * use this logic to avoid re-define issue */
#if !defined(_LINUX_STDARG_H) && !defined(_STDARG_H)
#include <stdarg.h>
/* no matter which stdarg.h is included, define the other one */
#ifndef _LINUX_STDARG_H
#define _LINUX_STDARG_H
#endif
#ifndef _STDARG_H
#define _STDARG_H
#endif

#endif
#include "amdgv_asic.h"

#define OSS_INVALID_HANDLE NULL
#define EVENT_FLAGS_SKIPPED (1 << 0)

typedef void *oss_dev_t;
typedef int (*oss_callback_t)(void *context);
typedef void (*oss_bh_callback_t)(void *handle, void *context, void *arg1, void *arg2);

enum oss_data_type {
	OSS_DATA_TYPE_U8  = 0,
	OSS_DATA_TYPE_U16 = 1,
	OSS_DATA_TYPE_U32 = 2,
};

enum oss_dma_mem_type {
	OSS_DMA_MEM_CACHEABLE = (1 << 0),
	OSS_DMA_PA_CONTIGUOUS = (1 << 1),
};

enum oss_memremap_type {
	OSS_MEMREMAP_WB = 1 << 0,
	OSS_MEMREMAP_WT = 1 << 1,
	OSS_MEMREMAP_WC = 1 << 2,
};

struct oss_dma_mem_info {
	/* the bus address of allocated dma memory */
	uint64_t bus_addr;
	/* the mapped pointer of dma memory */
	void	*va_ptr;
	/* the physical address of dma memory */
	uint64_t phys_addr;
	/* this dma memory handle */
	void	*handle;
};

#define AMDGV_IH_SRC_DATA_MAX_SIZE_DW 4

/* the structure contains decoded IV ring information */
struct oss_interrupt_cb_info {
	unsigned client_id;
	unsigned src_id;
	unsigned ring_id;
	unsigned vm_id;
	unsigned vm_id_src;
	uint64_t timestamp;
	unsigned timestamp_src;
	unsigned pas_id;
	unsigned pasid_src;
	unsigned src_data[AMDGV_IH_SRC_DATA_MAX_SIZE_DW];
};

/* Interrupt callback function return the below value
 * to indicate the process state.
 */
enum oss_irq_return {
	OSS_IRQ_NONE	= 0,
	OSS_IRQ_HANDLED = 1,
};

/* callback function get called when interrupt triggered */
typedef enum oss_irq_return (*oss_interrupt_cb_t)(void *context);

/* shim drv pass down decoded iv ring entry */
typedef enum oss_irq_return (*oss_interrupt_cb2_t)(void *context,
						   struct oss_interrupt_cb_info *cb_info);

enum oss_intr_cb_type {
	/* libgv get notified when interrupt triggered */
	OSS_INTR_CB_REGULAR = 0,
	/* shim drv needs pass down decoded iv ring entry */
	OSS_INTR_CB_DECODED = 1,
};

enum oss_intr_type {
	OSS_INTR_TYPE_MSI  = 0,
	OSS_INTR_TYPE_MSIX = 1,
};

struct oss_intr_regrt_entry {
	/* the msi vector to bind */
	unsigned int idx_msi_vector;

	/* the interrupt callback type */
	enum oss_intr_cb_type intr_cb_type;

	/* interrupt callback function */
	union {
		oss_interrupt_cb_t  interrupt_cb;
		oss_interrupt_cb2_t interrupt_cb2;
	} int_cb;

	/* interrupt callback context */
	void *context;
};

struct oss_intr_regrt_info {
	/* the type of interrupt to register */
	enum oss_intr_type intr_type;

	/* the number of msi vectors to enable */
	int num_msi_vectors;

	/* the number of intr entry in intr_entries */
	int num_intr_entry;

	/* the array of intr entry */
	struct oss_intr_regrt_entry *intr_entries;

	/* It is used by GVShim to store private data. */
	void *priv;
};

enum oss_event_state {
	OSS_EVENT_STATE_WAKE_UP	    = 0,
	OSS_EVENT_STATE_TIMEOUT	    = 1,
	OSS_EVENT_STATE_INTERRUPTED = 2,
	OSS_EVENT_FAKE_SIGNAL = 3,
};

enum oss_timer_type {
	OSS_TIMER_TYPE_PERIODIC = 0,
	OSS_TIMER_TYPE_ONE_TIME = 1,
};

enum oss_timer_call_back_return {
	OSS_TIMER_RETURN_NORESTART = 0,
	OSS_TIMER_RETURN_RESTART   = 1,
};

enum oss_hw_queue_type {
	OSS_SDMA_QUEUE        = 0,
	OSS_GFX_QUEUE         = 1,
	OSS_COMPUTE_QUEUE     = 2,
	OSS_COMPUTE_AQL_QUEUE = 3,
};

struct oss_dev_res {
	/* the mapped framebuffer pointer */
	void	*fb;
	/* the size of framebuffer */
	uint64_t fb_size;
	/* the mapped mmio pointer */
	void	*mmio;
	/* the size of mmio */
	uint64_t mmio_size;
};

struct oss_ih_rb_info {
	void *rb;
	uint32_t rb_bytes_size;
	uint32_t rb_rptr;
	uint32_t rb_wptr;
	uint32_t rb_in_system;
	uint64_t rb_address;
	uint32_t doorbell_index;
	uint32_t overflow;
	uint32_t overflow_count;
};

struct oss_aql_comp_rb_info {
	uint32_t    ring_dw_size;
	uint32_t    *ring_base;
	uint32_t    doorbell_offset_in_dword;
	uint64_t    *wptr_poll_memory;
	uint32_t    reserve[2];
};

struct oss_bh_info {
	void                *handle;
	oss_bh_callback_t   fn;
	void                *context;
	void                *arg1;
	void                *arg2;
};

struct oss_interface {
	/* get the vf device handle of shim drv */
	oss_dev_t (*get_vf_dev_from_bdf)(uint32_t bdf);

	/* put the vf device to decrease the ref count */
	void (*put_vf_dev)(oss_dev_t dev);
	/* map or unmap vf device resources, device resources contains
	 * mapped framebuffer, mmio and doorbell.
	 */
	int (*map_vf_dev_res)(oss_dev_t dev, struct oss_dev_res *res);
	void (*unmap_vf_dev_res)(oss_dev_t dev, struct oss_dev_res *res);
	int (*map_framebuffer)(oss_dev_t dev, uint64_t phyAddress,
						uint32_t size, void **mappedAddress);
	int (*unmap_framebuffer)(oss_dev_t dev, void *mappedAddress);

	/* READ mmio memory */
	uint8_t (*mm_readb)(void *addr);
	uint16_t (*mm_readw)(void *addr);
	uint32_t (*mm_readl)(void *addr);

	/* WRITE mmio memory */
	void (*mm_writeb)(void *addr, uint8_t val);
	void (*mm_writew)(void *addr, uint16_t val);
	void (*mm_writel)(void *addr, uint32_t val);
	void (*mm_writeq)(void *addr, uint64_t val);

	/* READ io port */
	uint8_t (*io_readb)(void *addr);
	uint16_t (*io_readw)(void *addr);
	uint32_t (*io_readl)(void *addr);

	/* WRITE io port */
	void (*io_writeb)(void *addr, uint8_t val);
	void (*io_writew)(void *addr, uint16_t val);
	void (*io_writel)(void *addr, uint32_t val);

	/* pci config space read operations */
	int (*pci_read_config_byte)(oss_dev_t dev, int where, uint8_t *val);
	int (*pci_read_config_word)(oss_dev_t dev, int where, uint16_t *val);
	int (*pci_read_config_dword)(oss_dev_t dev, int where, uint32_t *val);

	/* pci config space write operations */
	int (*pci_write_config_byte)(oss_dev_t dev, int where, uint8_t val);
	int (*pci_write_config_word)(oss_dev_t dev, int where, uint16_t val);
	int (*pci_write_config_dword)(oss_dev_t dev, int where, uint32_t val);

	/* found the pos of pci (ext) cap */
	int (*pci_find_cap)(oss_dev_t dev, int cap);
	int (*pci_find_ext_cap)(oss_dev_t dev, int cap);
	int (*pci_find_next_ext_cap)(oss_dev_t dev, int start_pos, int cap);

	/* restore VF resizable BAR */
	int (*pci_restore_vf_rebar)(oss_dev_t dev, int bar_idx);

	/* enable or disable SRIOV */
	int (*pci_enable_sriov)(oss_dev_t dev, uint32_t num_vf);
	int (*pci_disable_sriov)(oss_dev_t dev);

	/* map or unmap rom bar */
	void *(*pci_map_rom)(oss_dev_t dev, unsigned long *size);
	void (*pci_unmap_rom)(oss_dev_t dev, void *rom);
	bool (*pci_read_rom)(oss_dev_t dev, unsigned char *dest, unsigned long *bytes_copied, unsigned long max_size);

	/* register and unregister interrupts */
	int (*register_interrupt)(oss_dev_t dev, struct oss_intr_regrt_info *intr_regrt_info);
	int (*unregister_interrupt)(oss_dev_t dev,
				    struct oss_intr_regrt_info *intr_regrt_info);

	/* bus reset */
	int (*pci_bus_reset)(oss_dev_t dev);

	/* allocate or free a small memory */
	void *(*alloc_small_memory)(uint32_t size);
	void *(*alloc_small_memory_atomic)(uint32_t size);
	void *(*alloc_small_zero_memory)(uint32_t size);
	void (*free_small_memory)(void *ptr);
	void *(*get_physical_addr)(void *addr);

	/* allocate or free a large memory, it may sleep */
	void *(*alloc_memory)(uint32_t size);
	void (*free_memory)(void *ptr);

	/* allocate and map physical system memory */
	int (*alloc_dma_mem)(oss_dev_t dev, uint32_t size, enum oss_dma_mem_type type,
			     struct oss_dma_mem_info *dma_mem_info);
	void (*free_dma_mem)(void *handle);

	void *(*memremap)(uint64_t offset, uint32_t size, uint32_t flags);
	void (*memunmap)(void *addr);

	/* memory operations */
	void *(*memset)(void *src, int c, uint64_t n);
	void *(*memcpy)(void *dest, const void *src, uint64_t n);
	int (*memcmp)(const void *s1, const void *s2, uint64_t n);

	/* string operations */
	int (*strncmp)(const char *s1, const char *s2, uint64_t n);
	uint32_t (*strlen)(const char *s);
	uint32_t (*strnlen)(const char *s, uint32_t maxlen);

	/* divide operation */
	uint32_t (*do_div)(uint64_t *n, uint32_t base);

	/* spin lock operations */

	/* the lower rank lock can't preempt the higher rank lock */
	void *(*spin_lock_init)(int rank);
	void (*spin_lock)(void *lock);
	void (*spin_unlock)(void *lock);
	/* disable irq when lock */
	void (*spin_lock_irq)(void *lock);
	/* restore irq when unlock */
	void (*spin_unlock_irq)(void *lock);
	void (*spin_lock_fini)(void *lock);

	/* mutext operations */
	void *(*mutex_init)(void);
	void (*mutex_lock)(void *mutex);
	void (*mutex_unlock)(void *mutex);
	void (*mutex_fini)(void *mutex);

	/* rwlock operations */
	void *(*rwlock_init)(void);
	void (*rwlock_read_lock)(void *lock);
	int (*rwlock_read_trylock)(void *lock);
	void (*rwlock_read_unlock)(void *lock);
	void (*rwlock_write_lock)(void *lock);
	int (*rwlock_write_trylock)(void *lock);
	void (*rwlock_write_unlock)(void *lock);
	void (*rwlock_fini)(void *lock);

	/* rwsema operations */
	void *(*rwsema_init)(void);
	void (*rwsema_read_lock)(void *lock);
	int (*rwsema_read_trylock)(void *lock);
	void (*rwsema_read_unlock)(void *lock);
	void (*rwsema_write_lock)(void *lock);
	int (*rwsema_write_trylock)(void *lock);
	void (*rwsema_write_unlock)(void *lock);
	void (*rwsema_fini)(void *lock);

	/* event operations */
	void *(*event_init)(void);
	void (*signal_event)(void *event);
	void (*signal_event_with_flag)(void *event, uint64_t);
	void (*signal_event_forever)(void *event);
	void (*signal_event_forever_with_flag)(void *event, uint64_t);
	enum oss_event_state (*wait_event)(void *event, uint32_t timeout);
	void (*event_fini)(void *event);
	/* notifier wakeup */
	int (*notifier_wakeup)(void *notifier, uint64_t count);
	/* atomic operations */
	void *(*atomic_init)(void);
	uint64_t (*atomic_read)(void *atomic);
	void (*atomic_set)(void *atomic, uint64_t val);
	void (*atomic_inc)(void *atomic);
	void (*atomic_dec)(void *atomic);
	uint64_t (*atomic_inc_return)(void *atomic);
	uint64_t (*atomic_dec_return)(void *atomic);
	int64_t (*atomic_cmpxchg)(void *atomic, int64_t comperand, int64_t exchange);
	void (*atomic_fini)(void *atomic);

	/* create a kernel thread */
	void *(*create_thread)(oss_callback_t threadfn, void *context, const char *name);
	void *(*get_current_thread)(void);
	bool (*is_current_running_thread)(void *thread);
	void (*close_thread)(void *thread);
	bool (*thread_should_stop)(void *thread);

	/* timer operations */
	void *(*timer_init)(oss_callback_t timer_cb, void *context);
	void *(*timer_init_ex)(oss_callback_t timer_cb, void *context, oss_dev_t dev);
	int (*start_timer)(void *timer, uint64_t interval_us, enum oss_timer_type type);
	int (*pause_timer)(void *timer);
	int (*try_pause_timer)(void *timer);
	void (*close_timer)(void *timer);

	uint32_t (*get_assigned_vf_count)(oss_dev_t dev, bool all_gpus);
	/* delay several microseconds */
	void (*udelay)(uint32_t usecs);

	/* sleep several milliseconds */
	void (*msleep)(uint32_t msecs);

	/* sleep several microseconds */
	void (*usleep)(uint32_t usecs);

	/* optional yield to allow the system to execute on long tasks */
	void (*yield)(void);

	/* memory fence */
	void (*memory_fence)(void);

	/* return the machine's up time
	 * (return value is in microseconds)
	 */
	uint64_t (*get_time_stamp)(void);

	uint64_t (*get_utc_time_stamp)(void);

	/* get human readable UTC time */
	void (*get_utc_time_stamp_str)(char *buf, uint32_t buf_size);

	/* if os support ari or not */
	bool (*ari_supported)(void);

	/* print string */
	void (*print)(int level, const char *fmt, va_list args);

	/* vsnprintf string */
	int (*vsnprintf)(char *buf, uint32_t size, const char *fmt, va_list args);

	/* send message to PF KMD driver */
	int (*send_msg)(oss_dev_t dev, uint32_t *msg_data, int msg_len, bool need_valid);

	int (*store_dump)(const char *buf, uint32_t bdf);

	void (*get_random_bytes)(void *buf, int nbytes);

	void (*sema_up)(void *sema);
	void (*sema_down)(void *sema);
	void *(*sema_init)(int32_t val);
	void (*sema_fini)(void *sema);

	int (*copy_from_user)(void *to, const void *from, uint32_t size);
	int (*copy_to_user)(void *to, const void *from, uint32_t size);

	/* search substring in string, all string have max size constraint
	 * if found, return the index of substring in string
	 * else return -1 in case of error or not found
	 */
	int (*strnstr)(const char *str, const char *substr, uint32_t max_size);

	int (*detect_fw)(oss_dev_t dev, enum amdgv_firmware_id fw_id, enum amd_asic_type asic_type);

	int (*get_fw)(oss_dev_t dev, enum amdgv_firmware_id fw_id,
		      enum amd_asic_type asic_type, unsigned char *pfw_image,
		      uint32_t *fw_size, uint32_t fw_size_max);

	int (*get_discovery_binary)(oss_dev_t dev, enum amd_asic_type asic_type,
			  uint32_t *binary, uint32_t binary_size_max);

	uint64_t (*create_hash_64)(uint64_t val, unsigned int bits);

	void (*dump_stack)(void);

	int (*copy_call_trace_buffer)(void *trace_buff, uint32_t trace_buf_len);
#ifdef WS_RECORD
	int (*store_record)(const char *buf, uint32_t bdf, bool auto_sched);
#endif

	int (*store_rlcv_timestamp)(const char *buf, uint32_t size, uint32_t bdf);
	bool (*get_ih_rb_info)(oss_dev_t dev, uint32_t ih_index, struct oss_ih_rb_info *p_ih_rb_info);

	void (*signal_reset_happened)(oss_dev_t dev, uint32_t idx_vf);
	void (*signal_diag_data_ready)(oss_dev_t dev);
	bool (*diag_data_collect_disabled)(oss_dev_t dev, uint32_t bdf);
	void (*signal_manual_dump_happened)(oss_dev_t dev, uint32_t idx_vf);
	int (*get_device_numa_node)(oss_dev_t dev);
	int (*save_fb_sharing_mode)(oss_dev_t dev, uint32_t mode);

	int (*save_accelerator_partition_mode)(
		oss_dev_t dev, uint32_t accelerator_partition_mode);
	int (*save_memory_partition_mode)(oss_dev_t dev,
					  uint32_t memory_partition_mode);
	int (*clear_conf_file)(oss_dev_t dev);

	/* Run fn as a passive level work item */
	int (*schedule_work)(oss_dev_t dev, oss_callback_t fn, void *context);
	void (*notify_shim_ext)(oss_dev_t dev, uint32_t error_code, uint32_t severity,
			const char *fmt, va_list args);

	int (*store_gfx_dump_data)(const char *buf, uint32_t size, char *filename);

	/* Map different types of hw queue */
	bool (*map_queue)(oss_dev_t dev, bool start, enum oss_hw_queue_type queue_type,
					struct oss_aql_comp_rb_info *p_aql_comp_rb_info);

	bool  (*bh_init)(struct oss_bh_info *bh);
	bool  (*bh_queue)(struct oss_bh_info *bh);
	bool  (*bh_fini)(struct oss_bh_info *bh);
};

#endif
