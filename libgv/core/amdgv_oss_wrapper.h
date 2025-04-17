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

#ifndef AMDGV_OSS_WRAPPER_H
#define AMDGV_OSS_WRAPPER_H


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
#include "amdgv.h"
#include "amdgv_oss.h"

#ifndef INLINE
#define INLINE static inline
#endif

#define OSS_FUNCTION_NOT_IMPLEMENTED (0xFFFFFFFF)

typedef void *spin_lock_t;
typedef void *mutex_t;
typedef void *rwlock_t;
typedef void *rwsema_t;
typedef void *event_t;
typedef void *atomic_t;
typedef void *thread_t;
typedef void *timer_t;

typedef struct {
	int64_t counter;
} atomic64_t, oss_atomic64_t;

struct amdgv_event {
	void *event;
	bool  enabled;
};

extern struct oss_interface *amdgv_oss_funcs;

enum {
	AMDGV_SPIN_LOCK_LOWEST_RANK  = 0,
	AMDGV_SPIN_LOCK_LOW_RANK     = 1,
	AMDGV_SPIN_LOCK_MEDIUM_RANK  = 2,
	AMDGV_SPIN_LOCK_HIGH_RANK    = 3,
	AMDGV_SPIN_LOCK_HIGHEST_RANK = 4,
};

INLINE oss_dev_t oss_get_dev_from_bdf(uint32_t bdf)
{
	return amdgv_oss_funcs->get_vf_dev_from_bdf(bdf);
}

INLINE void oss_put_dev(oss_dev_t dev)
{
	if (amdgv_oss_funcs->put_vf_dev)
		amdgv_oss_funcs->put_vf_dev(dev);
}

INLINE int oss_map_vf_dev_res(oss_dev_t dev, struct oss_dev_res *res)
{
	return amdgv_oss_funcs->map_vf_dev_res(dev, res);
}

INLINE void oss_unmap_vf_dev_res(oss_dev_t dev, struct oss_dev_res *res)
{
	amdgv_oss_funcs->unmap_vf_dev_res(dev, res);
}

INLINE int oss_map_framebuffer(oss_dev_t dev, uint64_t phyAddress, uint32_t size, void **mappedAddress)
{
	if (amdgv_oss_funcs->map_framebuffer) {
		return amdgv_oss_funcs->map_framebuffer(dev, phyAddress, size, mappedAddress);
	} else
		return -1;
}

INLINE int oss_unmap_framebuffer(oss_dev_t dev, void *mappedAddress)
{
	if (amdgv_oss_funcs->unmap_framebuffer) {
		return amdgv_oss_funcs->unmap_framebuffer(dev, mappedAddress);
	} else
		return -1;
}

INLINE uint8_t oss_mm_read8(void *addr)
{
	return amdgv_oss_funcs->mm_readb(addr);
}

INLINE void oss_mm_write8(void *addr, uint8_t val)
{
	amdgv_oss_funcs->mm_writeb(addr, val);
}

INLINE uint8_t oss_io_read8(void *addr)
{
	return amdgv_oss_funcs->io_readb(addr);
}

INLINE void oss_io_write8(void *addr, uint8_t val)
{
	amdgv_oss_funcs->io_writeb(addr, val);
}

INLINE uint16_t oss_mm_read16(void *addr)
{
	return amdgv_oss_funcs->mm_readw(addr);
}

INLINE void oss_mm_write16(void *addr, uint16_t val)
{
	amdgv_oss_funcs->mm_writew(addr, val);
}

INLINE uint16_t oss_io_read16(void *addr)
{
	return amdgv_oss_funcs->io_readw(addr);
}

INLINE void oss_io_write16(void *addr, uint16_t val)
{
	amdgv_oss_funcs->io_writew(addr, val);
}

INLINE uint32_t oss_mm_read32(void *addr)
{
	return amdgv_oss_funcs->mm_readl(addr);
}

INLINE void oss_mm_write32(void *addr, uint32_t val)
{
	amdgv_oss_funcs->mm_writel(addr, val);
}

INLINE void oss_mm_write64(void *addr, uint64_t val)
{
	if (amdgv_oss_funcs->mm_writeq)
		amdgv_oss_funcs->mm_writeq(addr, val);
	else
		*((uint64_t *)addr) = val;
}

INLINE uint32_t oss_io_read32(void *addr)
{
	return amdgv_oss_funcs->io_readl(addr);
}

INLINE void oss_io_write32(void *addr, uint32_t val)
{
	amdgv_oss_funcs->io_writel(addr, val);
}

INLINE int oss_pci_read_config_byte(oss_dev_t dev, int where, uint8_t *val)
{
	return amdgv_oss_funcs->pci_read_config_byte(dev, where, val);
}

INLINE int oss_pci_read_config_word(oss_dev_t dev, int where, uint16_t *val)
{
	return amdgv_oss_funcs->pci_read_config_word(dev, where, val);
}

INLINE int oss_pci_read_config_dword(oss_dev_t dev, int where, uint32_t *val)
{
	return amdgv_oss_funcs->pci_read_config_dword(dev, where, val);
}

INLINE int oss_pci_write_config_byte(oss_dev_t dev, int where, uint8_t val)
{
	return amdgv_oss_funcs->pci_write_config_byte(dev, where, val);
}

INLINE int oss_pci_write_config_word(oss_dev_t dev, int where, uint16_t val)
{
	return amdgv_oss_funcs->pci_write_config_word(dev, where, val);
}

INLINE int oss_pci_write_config_dword(oss_dev_t dev, int where, uint32_t val)
{
	return amdgv_oss_funcs->pci_write_config_dword(dev, where, val);
}

INLINE int oss_pci_find_capability(oss_dev_t dev, int cap)
{
	return amdgv_oss_funcs->pci_find_cap(dev, cap);
}

INLINE int oss_pci_find_ext_cap(oss_dev_t dev, int cap)
{
	return amdgv_oss_funcs->pci_find_ext_cap(dev, cap);
}

INLINE int oss_pci_find_next_ext_cap(oss_dev_t dev, int start_pos, int cap)
{
	return amdgv_oss_funcs->pci_find_next_ext_cap(dev, start_pos, cap);
}

INLINE int oss_pci_restore_vf_rebar(oss_dev_t dev, int bar_idx)
{
	if (amdgv_oss_funcs->pci_restore_vf_rebar)
		return amdgv_oss_funcs->pci_restore_vf_rebar(dev, bar_idx);
	return 0;
}

INLINE int oss_pci_enable_sriov(oss_dev_t dev, uint32_t num_vf)
{
	return amdgv_oss_funcs->pci_enable_sriov(dev, num_vf);
}

INLINE int oss_pci_disable_sriov(oss_dev_t dev)
{
	return amdgv_oss_funcs->pci_disable_sriov(dev);
}

INLINE void *oss_pci_map_rom(oss_dev_t dev, unsigned long *size)
{
	return amdgv_oss_funcs->pci_map_rom(dev, size);
}

INLINE void oss_pci_unmap_rom(oss_dev_t dev, void *rom)
{
	amdgv_oss_funcs->pci_unmap_rom(dev, rom);
}

INLINE bool oss_pci_read_rom(oss_dev_t dev, unsigned char *dest, unsigned long *bytes_copied, unsigned long max_size)
{
	return amdgv_oss_funcs->pci_read_rom(dev, dest, bytes_copied, max_size);
}

INLINE int oss_register_interrupt(oss_dev_t dev, struct oss_intr_regrt_info *intr_regrt_info)
{
	return amdgv_oss_funcs->register_interrupt(dev, intr_regrt_info);
}

INLINE int oss_unregister_interrupt(oss_dev_t dev, struct oss_intr_regrt_info *intr_regrt_info)
{
	return amdgv_oss_funcs->unregister_interrupt(dev, intr_regrt_info);
}

INLINE int oss_pci_bus_reset(oss_dev_t dev)
{
	return amdgv_oss_funcs->pci_bus_reset(dev);
}

/* used to allocate a small buffer */
INLINE void *oss_malloc(uint32_t size)
{
	return amdgv_oss_funcs->alloc_small_memory(size);
}

/* used to allocate a small buffer where no context switch should be made*/
INLINE void *oss_malloc_atomic(uint32_t size)
{
	if (amdgv_oss_funcs->alloc_small_memory_atomic)
		return amdgv_oss_funcs->alloc_small_memory_atomic(size);
	else
		return amdgv_oss_funcs->alloc_small_memory(size);
}

INLINE void *oss_zalloc(uint32_t size)
{
	return amdgv_oss_funcs->alloc_small_zero_memory(size);
}

INLINE void oss_free(void *ptr)
{
	amdgv_oss_funcs->free_small_memory(ptr);
}

INLINE void *oss_get_physical_addr(void *addr)
{
	return amdgv_oss_funcs->get_physical_addr(addr);
}

/* used to allocate a large buffer */
INLINE void *oss_alloc_memory(uint32_t size)
{
	return amdgv_oss_funcs->alloc_memory(size);
}

INLINE void oss_free_memory(void *ptr)
{
	amdgv_oss_funcs->free_memory(ptr);
}

INLINE void *oss_memremap(uint64_t offset, uint32_t size, enum oss_memremap_type type)
{
	return amdgv_oss_funcs->memremap(offset, size, type);
}

INLINE void oss_memunmap(void *addr)
{
	amdgv_oss_funcs->memunmap(addr);
}

INLINE void *oss_memset(void *src, int c, uint64_t n)
{
	return amdgv_oss_funcs->memset(src, c, n);
}

INLINE void *oss_memcpy(void *dest, const void *src, uint64_t n)
{
	return amdgv_oss_funcs->memcpy(dest, src, n);
}

INLINE int oss_memcmp(const void *s1, const void *s2, uint64_t n)
{
	return amdgv_oss_funcs->memcmp(s1, s2, n);
}

INLINE int oss_alloc_dma_mem(oss_dev_t dev, uint32_t size, enum oss_dma_mem_type type,
			     struct oss_dma_mem_info *dma_mem_info)
{
	return amdgv_oss_funcs->alloc_dma_mem(dev, size, type, dma_mem_info);
}

INLINE void oss_free_dma_mem(void *handle)
{
	amdgv_oss_funcs->free_dma_mem(handle);
}

INLINE spin_lock_t oss_spin_lock_init(int rank)
{
	return amdgv_oss_funcs->spin_lock_init(rank);
}

INLINE void oss_spin_lock(spin_lock_t lock)
{
	amdgv_oss_funcs->spin_lock(lock);
}

INLINE void oss_spin_unlock(spin_lock_t lock)
{
	amdgv_oss_funcs->spin_unlock(lock);
}

INLINE void oss_spin_lock_irq(spin_lock_t lock)
{
	amdgv_oss_funcs->spin_lock_irq(lock);
}

INLINE void oss_spin_unlock_irq(spin_lock_t lock)
{
	amdgv_oss_funcs->spin_unlock_irq(lock);
}

INLINE void oss_spin_lock_fini(spin_lock_t lock)
{
	amdgv_oss_funcs->spin_lock_fini(lock);
}

INLINE mutex_t oss_mutex_init(void)
{
	return amdgv_oss_funcs->mutex_init();
}

INLINE void oss_mutex_lock(mutex_t mutex)
{
	amdgv_oss_funcs->mutex_lock(mutex);
}

INLINE void oss_mutex_unlock(mutex_t mutex)
{
	amdgv_oss_funcs->mutex_unlock(mutex);
}

INLINE void oss_mutex_fini(mutex_t mutex)
{
	amdgv_oss_funcs->mutex_fini(mutex);
}

INLINE rwlock_t oss_rwlock_init(void)
{
	return amdgv_oss_funcs->rwlock_init();
}

INLINE void oss_rwlock_read_lock(rwsema_t lock)
{
	amdgv_oss_funcs->rwlock_read_lock(lock);
}

INLINE int oss_rwlock_read_trylock(rwsema_t lock)
{
	int ret = 0;
	ret = amdgv_oss_funcs->rwlock_read_trylock(lock);
	if (ret)
		return -1;
	return 0;
}

INLINE void oss_rwlock_read_unlock(rwsema_t lock)
{
	amdgv_oss_funcs->rwlock_read_unlock(lock);
}

INLINE void oss_rwlock_write_lock(rwsema_t lock)
{
	amdgv_oss_funcs->rwlock_write_lock(lock);
}

INLINE int oss_rwlock_write_trylock(rwsema_t lock)
{
	int ret = 0;
	ret = amdgv_oss_funcs->rwlock_write_trylock(lock);
	if (ret)
		return -1;
	return 0;
}

INLINE void oss_rwlock_write_unlock(rwsema_t lock)
{
	amdgv_oss_funcs->rwlock_write_unlock(lock);
}

INLINE void oss_rwlock_fini(rwsema_t lock)
{
	amdgv_oss_funcs->rwlock_fini(lock);
}

INLINE rwlock_t oss_rwsema_init(void)
{
	return amdgv_oss_funcs->rwsema_init();
}

INLINE void oss_rwsema_read_lock(rwsema_t lock)
{
	amdgv_oss_funcs->rwsema_read_lock(lock);
}

INLINE int oss_rwsema_read_trylock(rwsema_t lock)
{
	int ret;
	ret = amdgv_oss_funcs->rwsema_read_trylock(lock);
	if (!ret)
		return -1;
	return 0;
}

INLINE void oss_rwsema_read_unlock(rwsema_t lock)
{
	amdgv_oss_funcs->rwsema_read_unlock(lock);
}

INLINE void oss_rwsema_write_lock(rwsema_t lock)
{
	amdgv_oss_funcs->rwsema_write_lock(lock);
}

INLINE int oss_rwsema_write_trylock(rwsema_t lock)
{
	int ret;
	ret = amdgv_oss_funcs->rwsema_write_trylock(lock);
	if (!ret)
		return -1;
	return 0;
}

INLINE void oss_rwsema_write_unlock(rwsema_t lock)
{
	amdgv_oss_funcs->rwsema_write_unlock(lock);
}

INLINE void oss_rwsema_fini(rwsema_t lock)
{
	amdgv_oss_funcs->rwsema_fini(lock);
}

INLINE void oss_copy_to_user(void *to, const void *from, uint32_t size)
{
	amdgv_oss_funcs->copy_to_user(to, from, size);
}

INLINE void oss_copy_from_user(void *to, const void *from, uint32_t size)
{
	amdgv_oss_funcs->copy_from_user(to, from, size);
}

INLINE event_t oss_event_init(void)
{
	if (!amdgv_oss_funcs->signal_event_forever) {
		struct amdgv_event *ev = (struct amdgv_event *)oss_malloc(sizeof(*ev));

		if (!ev)
			return NULL;
		ev->event = amdgv_oss_funcs->event_init();
		if (!ev->event) {
			oss_free(ev);
			return NULL;
		}
		ev->enabled = true;
		return (event_t)ev;
	}

	return amdgv_oss_funcs->event_init();
}

INLINE void oss_signal_event(event_t event)
{
	if (!amdgv_oss_funcs->signal_event_forever) {
		struct amdgv_event *ev = (struct amdgv_event *)event;

		if (ev->enabled)
			amdgv_oss_funcs->signal_event(ev->event);
		return;
	}

	amdgv_oss_funcs->signal_event(event);
}

INLINE void oss_signal_event_with_flag(event_t event, uint64_t flag)
{
	event_t evnt = event;
	if (!amdgv_oss_funcs->signal_event_forever) {
		struct amdgv_event *ev = (struct amdgv_event *)event;

		if (ev->enabled)
			evnt = ev->event;
		else
			return;
	}

	if (amdgv_oss_funcs->signal_event_with_flag)
		amdgv_oss_funcs->signal_event_with_flag(evnt, flag);
	else
		amdgv_oss_funcs->signal_event(evnt);
}

INLINE void oss_signal_event_forever(event_t event)
{
	if (!amdgv_oss_funcs->signal_event_forever) {
		struct amdgv_event *ev = (struct amdgv_event *)event;

		if (ev->enabled) {
			ev->enabled = false;
			amdgv_oss_funcs->signal_event(ev->event);
		}
		return;
	}

	amdgv_oss_funcs->signal_event_forever(event);
}

INLINE void oss_signal_event_forever_with_flag(event_t event, uint64_t flag)
{
	event_t evnt = event;
	if (!amdgv_oss_funcs->signal_event_forever) {
		struct amdgv_event *ev = (struct amdgv_event *)event;

		if (ev->enabled) {
			ev->enabled = false;
			evnt = ev->event;
		} else {
			return;
		}
	}

	if (amdgv_oss_funcs->signal_event_forever_with_flag)
		amdgv_oss_funcs->signal_event_forever_with_flag(evnt, flag);
	else
		amdgv_oss_funcs->signal_event_forever(evnt);
}

INLINE enum oss_event_state oss_wait_event(event_t event, uint32_t timeout)
{
	if (!amdgv_oss_funcs->signal_event_forever) {
		struct amdgv_event *ev = (struct amdgv_event *)event;

		if (!ev->enabled)
			return OSS_EVENT_STATE_WAKE_UP;
		return amdgv_oss_funcs->wait_event(ev->event, timeout);
	}

	return amdgv_oss_funcs->wait_event(event, timeout);
}

INLINE void oss_event_fini(event_t event)
{
	if (!amdgv_oss_funcs->signal_event_forever) {
		struct amdgv_event *ev = (struct amdgv_event *)event;

		amdgv_oss_funcs->event_fini(ev->event);
		oss_free(ev);
		return;
	}

	amdgv_oss_funcs->event_fini(event);
}

INLINE void oss_notifier_wakeup(event_t event, uint64_t count)
{
	if (amdgv_oss_funcs->notifier_wakeup)
		amdgv_oss_funcs->notifier_wakeup(event, count);
}

INLINE atomic_t oss_atomic_init(void)
{
	return amdgv_oss_funcs->atomic_init();
}

INLINE uint64_t oss_atomic_read(atomic_t atomic)
{
	return amdgv_oss_funcs->atomic_read(atomic);
}

INLINE void oss_atomic_set(atomic_t atomic, uint64_t val)
{
	amdgv_oss_funcs->atomic_set(atomic, val);
}

INLINE void oss_atomic_inc(atomic_t atomic)
{
	amdgv_oss_funcs->atomic_inc(atomic);
}

INLINE void oss_atomic_dec(atomic_t atomic)
{
	amdgv_oss_funcs->atomic_dec(atomic);
}

INLINE uint64_t oss_atomic_inc_return(atomic_t atomic)
{
	return amdgv_oss_funcs->atomic_inc_return(atomic);
}

INLINE uint64_t oss_atomic_dec_return(atomic_t atomic)
{
	return amdgv_oss_funcs->atomic_dec_return(atomic);
}

INLINE uint64_t oss_atomic_cmpxchg(atomic_t atomic, int64_t comperand, int64_t exchange)
{
	if (amdgv_oss_funcs->atomic_cmpxchg)
		return amdgv_oss_funcs->atomic_cmpxchg(atomic, comperand, exchange);
	return 0;
}

INLINE void oss_atomic_fini(atomic_t atomic)
{
	amdgv_oss_funcs->atomic_fini(atomic);
}

INLINE thread_t oss_create_thread(oss_callback_t threadfn, void *context, const char *name)
{
	return amdgv_oss_funcs->create_thread(threadfn, context, name);
}

/*
 * Instead of using this function, we suggest developers to use
 * oss_is_current_running_thread to detect current running thread
 */
INLINE thread_t oss_get_current_thread(void)
{
	if (!amdgv_oss_funcs->get_current_thread)
		return NULL;

	return amdgv_oss_funcs->get_current_thread();
}

INLINE bool oss_is_current_running_thread(thread_t thread)
{
	if (!amdgv_oss_funcs->is_current_running_thread)
		return false;

	return amdgv_oss_funcs->is_current_running_thread(thread);
}

INLINE void oss_close_thread(thread_t thread)
{
	amdgv_oss_funcs->close_thread(thread);
}

INLINE bool oss_thread_should_stop(thread_t thread)
{
	return amdgv_oss_funcs->thread_should_stop(thread);
}

INLINE timer_t oss_timer_init(oss_callback_t timer_cb, void *context)
{
	return amdgv_oss_funcs->timer_init(timer_cb, context);
}

INLINE timer_t oss_timer_init_ex(oss_callback_t timer_cb, void *context, oss_dev_t dev)
{
	if (amdgv_oss_funcs->timer_init_ex)
		return amdgv_oss_funcs->timer_init_ex(timer_cb, context, dev);
	else if (amdgv_oss_funcs->timer_init)
		return amdgv_oss_funcs->timer_init(timer_cb, context);
	else
		return NULL;
}

INLINE int oss_timer_start(timer_t timer, uint64_t interval_us, enum oss_timer_type type)
{
	return amdgv_oss_funcs->start_timer(timer, interval_us, type);
}

INLINE int oss_timer_pause(timer_t timer)
{
	return amdgv_oss_funcs->pause_timer(timer);
}

INLINE int oss_timer_try_pause(timer_t timer)
{
	int ret;

	if (amdgv_oss_funcs->try_pause_timer) {
		/*
		 * Linux kernel's try_pause_timer will return -1 if the timer is on the fly
		 * but oss function also return -1 when the function is not supported
		 * thus we do some changes here:
		 * return -1 --> function not supported
		 * return  0 --> timer is not active
		 * return  1 --> timer is active
		 * return  2 --> timer is busy with executing callback
		 *
		 */
		ret = amdgv_oss_funcs->try_pause_timer(timer);
		if (ret == -1)
			ret = 2;
	} else {
		ret = -1;
	}

	return ret;
}

INLINE void oss_timer_close(timer_t timer)
{
	amdgv_oss_funcs->close_timer(timer);
}

INLINE void oss_udelay(uint32_t usecs)
{
	amdgv_oss_funcs->udelay(usecs);
}

INLINE void oss_msleep(uint32_t msecs)
{
	amdgv_oss_funcs->msleep(msecs);
}

INLINE bool oss_ari_supported(void)
{
	return amdgv_oss_funcs->ari_supported();
}

INLINE void oss_print(int level, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);

	amdgv_oss_funcs->print(level, fmt, args);

	va_end(args);
}

INLINE int oss_vsnprintf(char *buf, uint32_t size, const char *fmt, ...)
{
	int	num;
	va_list args;

	va_start(args, fmt);

	num = amdgv_oss_funcs->vsnprintf(buf, size, fmt, args);

	va_end(args);

	return num;
}

INLINE void oss_memory_fence(void)
{
	amdgv_oss_funcs->memory_fence();
}

/* get current time in microseconds */
INLINE uint64_t oss_get_time_stamp(void)
{
	return amdgv_oss_funcs->get_time_stamp();
}

INLINE uint64_t oss_get_utc_time_stamp(void)
{
	return amdgv_oss_funcs->get_utc_time_stamp();
}

INLINE void oss_get_utc_time_stamp_str(char *buf, uint32_t buf_size)
{
	if (amdgv_oss_funcs->get_utc_time_stamp_str)
		amdgv_oss_funcs->get_utc_time_stamp_str(buf, buf_size);
}

INLINE int oss_strncmp(const char *cs, const char *ct, uint32_t count)
{
	return amdgv_oss_funcs->strncmp(cs, ct, count);
}

INLINE uint32_t oss_strlen(const char *s)
{
	return amdgv_oss_funcs->strlen(s);
}

INLINE uint32_t oss_strnlen(const char *s, uint32_t len)
{
	return amdgv_oss_funcs->strnlen(s, len);
}

INLINE uint32_t oss_get_assigned_vf_count(oss_dev_t dev, bool all_gpus)
{
	if (amdgv_oss_funcs->get_assigned_vf_count)
		return amdgv_oss_funcs->get_assigned_vf_count(dev, all_gpus);

	return OSS_FUNCTION_NOT_IMPLEMENTED;
}

INLINE uint32_t oss_do_div(uint64_t *n, uint32_t base)
{
	return amdgv_oss_funcs->do_div(n, base);
}

INLINE int oss_send_msg(oss_dev_t dev, uint32_t *msg_data, int msg_len, bool need_valid)
{
	return amdgv_oss_funcs->send_msg(dev, msg_data, msg_len, need_valid);
}

INLINE int oss_store_dump(const char *buf, uint32_t bdf)
{
	return amdgv_oss_funcs->store_dump && amdgv_oss_funcs->store_dump(buf, bdf);
}

INLINE void oss_get_random_bytes(void *buf, int nbytes)
{
	if (amdgv_oss_funcs->get_random_bytes)
		amdgv_oss_funcs->get_random_bytes(buf, nbytes);
}

INLINE void oss_yield(void)
{
	if (amdgv_oss_funcs->yield)
		amdgv_oss_funcs->yield();
}

INLINE void oss_sema_up(void *sem)
{
	if (amdgv_oss_funcs->sema_up)
		amdgv_oss_funcs->sema_up(sem);
}

INLINE void oss_sema_down(void *sem)
{
	if (amdgv_oss_funcs->sema_down)
		amdgv_oss_funcs->sema_down(sem);
}

INLINE void *oss_sema_init(int32_t val)
{
	if (amdgv_oss_funcs->sema_init)
		return amdgv_oss_funcs->sema_init(val);
	else
		return NULL;
}

INLINE void oss_sema_fini(void *sem)
{
	if (amdgv_oss_funcs->sema_fini)
		amdgv_oss_funcs->sema_fini(sem);
}

INLINE int oss_strnstr(const char *str, const char *substr, uint32_t max_size)
{
	if (amdgv_oss_funcs->strnstr)
		return amdgv_oss_funcs->strnstr(str, substr, max_size);

	return -1;
}

INLINE int oss_detect_fw(oss_dev_t dev, enum amdgv_firmware_id fw_id, enum amd_asic_type asic_type)
{
	if (amdgv_oss_funcs->detect_fw)
		return amdgv_oss_funcs->detect_fw(dev, fw_id, asic_type);

	return -1;
}

INLINE int oss_get_fw(oss_dev_t dev, enum amdgv_firmware_id fw_id,
		      enum amd_asic_type asic_type, unsigned char *pfw_image,
		      uint32_t *pfw_size, uint32_t fw_size_max)
{
	if (amdgv_oss_funcs->get_fw)
		return amdgv_oss_funcs->get_fw(dev, fw_id, asic_type, pfw_image, pfw_size, fw_size_max);

	return -1;
}

INLINE int oss_get_discovery_binary(oss_dev_t dev, enum amd_asic_type asic_type,
			  uint32_t *binary, uint32_t binary_size_max)
{
	if (amdgv_oss_funcs->get_discovery_binary)
		return amdgv_oss_funcs->get_discovery_binary(dev, asic_type, binary, binary_size_max);

	return -1;
}

INLINE uint64_t oss_create_hash_64(uint64_t val, unsigned int bits)
{
	if (amdgv_oss_funcs->create_hash_64)
		return amdgv_oss_funcs->create_hash_64(val, bits);

	return -1;
}

INLINE void oss_dump_stack(void)
{
	if (amdgv_oss_funcs->dump_stack)
		amdgv_oss_funcs->dump_stack();
}

INLINE void oss_usleep(uint32_t usecs)
{
	if (amdgv_oss_funcs->usleep)
		amdgv_oss_funcs->usleep(usecs);
	else {
		oss_udelay(usecs);
		oss_yield();
	}
}

INLINE int oss_copy_call_trace_buffer(void *call_trace_buf, uint32_t call_trace_buf_len)
{
	if (amdgv_oss_funcs->copy_call_trace_buffer)
		return amdgv_oss_funcs->copy_call_trace_buffer(call_trace_buf, call_trace_buf_len);
	return 0;
}

#ifdef WS_RECORD
INLINE int oss_store_record(const char *buf, uint32_t bdf, bool auto_sched)
{
	return amdgv_oss_funcs->store_record && amdgv_oss_funcs->store_record(buf, bdf, auto_sched);
}
#endif

INLINE int oss_store_rlcv_timestamp(const char *buf, uint32_t size, uint32_t bdf)
{
	return amdgv_oss_funcs->store_rlcv_timestamp && amdgv_oss_funcs->store_rlcv_timestamp(buf, size, bdf);
}

INLINE int oss_get_ih_rb_info(oss_dev_t dev, uint32_t ih_index, struct oss_ih_rb_info *p_ih_rb_info)
{
	if (amdgv_oss_funcs->get_ih_rb_info)
		return amdgv_oss_funcs->get_ih_rb_info(dev, ih_index, p_ih_rb_info);
	return 0;
}

INLINE void oss_signal_reset_happened(oss_dev_t dev, uint32_t idx_vf)
{
	if (amdgv_oss_funcs->signal_reset_happened)
		amdgv_oss_funcs->signal_reset_happened(dev, idx_vf);
}

INLINE void oss_signal_diag_data_ready(oss_dev_t dev)
{
	if (amdgv_oss_funcs->signal_diag_data_ready)
		amdgv_oss_funcs->signal_diag_data_ready(dev);
}

INLINE bool oss_diag_data_collect_disabled(oss_dev_t dev, uint32_t bdf)
{
	if (amdgv_oss_funcs->diag_data_collect_disabled)
		return amdgv_oss_funcs->diag_data_collect_disabled(dev, bdf);
	return false;
}

INLINE int oss_get_device_numa_node(oss_dev_t dev)
{
	if (amdgv_oss_funcs->get_device_numa_node)
		return amdgv_oss_funcs->get_device_numa_node(dev);

	return 0;
}

INLINE int oss_save_fb_sharing_mode(oss_dev_t dev, uint32_t mode)
{
	if (amdgv_oss_funcs->save_fb_sharing_mode)
		return amdgv_oss_funcs->save_fb_sharing_mode(dev, mode);

	return 0;
}

INLINE int
oss_save_accelerator_partition_mode(oss_dev_t dev,
				    uint32_t accelerator_partition_mode)
{
	if (amdgv_oss_funcs->save_accelerator_partition_mode)
		return amdgv_oss_funcs->save_accelerator_partition_mode(
			dev, accelerator_partition_mode);

	return 0;
}

INLINE int oss_save_memory_partition_mode(oss_dev_t dev,
					  uint32_t memory_partition_mode)
{
	if (amdgv_oss_funcs->save_memory_partition_mode)
		return amdgv_oss_funcs->save_memory_partition_mode(
			dev, memory_partition_mode);

	return 0;
}

INLINE int oss_clear_conf_file(oss_dev_t dev)
{
	if (amdgv_oss_funcs->clear_conf_file)
		return amdgv_oss_funcs->clear_conf_file(dev);

	return 0;
}

INLINE int oss_schedule_work(oss_dev_t dev, oss_callback_t fn, void *context)
{
	if (amdgv_oss_funcs->schedule_work)
		return amdgv_oss_funcs->schedule_work(dev, fn, context);

	return 0;
}

INLINE void oss_notify_shim_ext(oss_dev_t dev, uint32_t error_code,
	uint32_t severity, const char *fmt, ...)
{
	va_list args;

	if (amdgv_oss_funcs->notify_shim_ext) {
		va_start(args, fmt);
		amdgv_oss_funcs->notify_shim_ext(dev, error_code, severity, fmt, args);
		va_end(args);
	}
}

INLINE int oss_store_gfx_dump_data(const char *buf, uint32_t size, char *filename)
{
	return amdgv_oss_funcs->store_gfx_dump_data && amdgv_oss_funcs->store_gfx_dump_data(buf, size, filename);
}

INLINE bool oss_map_queue(oss_dev_t dev, bool start, enum oss_hw_queue_type queue_type,
							struct oss_aql_comp_rb_info *p_aql_comp_rb_info)
{
	return amdgv_oss_funcs->map_queue && amdgv_oss_funcs->map_queue(dev, start, queue_type, p_aql_comp_rb_info);
}

INLINE bool oss_bh_init(struct oss_bh_info *bh)
{
	return amdgv_oss_funcs->bh_init && amdgv_oss_funcs->bh_init(bh);
}

INLINE bool  oss_bh_queue(struct oss_bh_info *bh)
{
	return amdgv_oss_funcs->bh_queue && amdgv_oss_funcs->bh_queue(bh);
}

INLINE bool  oss_bh_fini(struct oss_bh_info *bh)
{
	return amdgv_oss_funcs->bh_fini && amdgv_oss_funcs->bh_fini(bh);
}



#endif
