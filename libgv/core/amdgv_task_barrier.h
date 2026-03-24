/*
 * Copyright (c) 2019-2026 Advanced Micro Devices, Inc. All rights reserved.
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
#ifndef AMDGV_TASK_BARRIER_H_
#define AMDGV_TASK_BARRIER_H_

/*
 * Represents an instance of a task barrier.
 */
struct task_barrier {
	atomic_t count;
	atomic_t thread_count;
	atomic_t thread_yield_count;
	uint32_t terminate;
};

static inline bool task_barrier_init(struct task_barrier *tb)
{
	oss_atomic_set(&tb->count, 0);
	oss_atomic_set(&tb->thread_count, 0);
	oss_atomic_set(&tb->thread_yield_count, 0);
	tb->terminate = 0;

	return true;
}

static inline void task_barrier_fini(struct task_barrier *tb)
{
	oss_atomic_set(&tb->count, 0);
	oss_atomic_set(&tb->thread_count, 0);
	oss_atomic_set(&tb->thread_yield_count, 0);
	tb->terminate = 0;
}

static inline void task_barrier_enter(struct task_barrier *tb, uint32_t count)
{
	oss_atomic_inc_return(&tb->thread_count);

	while (oss_atomic_read(&tb->thread_count) < count)
		oss_usleep(50);
}

static inline void task_barrier_exit(struct task_barrier *tb, uint32_t count)
{
	oss_atomic_inc_return(&tb->thread_yield_count);

	while (oss_atomic_read(&tb->thread_yield_count) < count)
		oss_usleep(50);

	oss_atomic_dec_return(&tb->thread_count);

	while (oss_atomic_read(&tb->thread_count) > 0)
		oss_usleep(50);

	oss_atomic_dec_return(&tb->thread_yield_count);
}

static inline int task_barrier_enter_timeout(struct task_barrier *tb, uint32_t count, uint32_t timeout)
{
	int usec_timeout = timeout;

	oss_atomic_inc_return(&tb->thread_count);

	while ((oss_atomic_read(&tb->thread_count) < count) && (tb->terminate == 0)) {
		oss_usleep(50);
		if (usec_timeout > 50)
			usec_timeout -= 50;
		else {
			tb->terminate = 1;
			oss_usleep(usec_timeout);
		}
	}

	return (oss_atomic_read(&tb->thread_count) != count);
}

/* Convenience function when nothing to be done in between entry and exit */
static inline void task_barrier_full(struct task_barrier *tb, uint32_t count)
{
	task_barrier_enter(tb, count);
	task_barrier_exit(tb, count);
}

/*
 * Shared Exclusion Lock
 *
 * Usage:
 *   // Define group IDs
 *   #define GROUP_A  0
 *   #define GROUP_B  1
 *
 *   // In group A code:
 *   shared_exclusion_enter(adapt, &lock, GROUP_A);
 *   // ... do group A work ...
 *   shared_exclusion_exit(adapt, &lock, GROUP_A);
 *
 *   // In group B code:
 *   shared_exclusion_enter(adapt, &lock, GROUP_B);
 *   // ... do group B work ...
 *   shared_exclusion_exit(adapt, &lock, GROUP_B);
 */

#define SHARED_EXCLUSION_NUM_GROUPS 2

struct shared_exclusion_lock {
	void *sem;                                  /* Main semaphore for mutual exclusion */
	void *gate[SHARED_EXCLUSION_NUM_GROUPS];    /* Gate semaphores for each group */
	atomic_t count[SHARED_EXCLUSION_NUM_GROUPS]; /* Number of threads in each group */
	bool active[SHARED_EXCLUSION_NUM_GROUPS];   /* True when group owns the semaphore */
	mutex_t lock;                               /* Mutex to protect all fields */
};

int shared_exclusion_init(struct amdgv_adapter *adapt, struct shared_exclusion_lock *sel);
void shared_exclusion_fini(struct amdgv_adapter *adapt, struct shared_exclusion_lock *sel);
void shared_exclusion_enter(struct amdgv_adapter *adapt, struct shared_exclusion_lock *sel, uint8_t group);
void shared_exclusion_exit(struct amdgv_adapter *adapt, struct shared_exclusion_lock *sel, uint8_t group);

#endif
