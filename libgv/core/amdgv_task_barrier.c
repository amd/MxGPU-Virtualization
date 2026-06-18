/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>

#include "amdgv_oss_wrapper.h"
#include "amdgv_task_barrier.h"

int shared_exclusion_init(struct amdgv_adapter *adapt,
						struct shared_exclusion_lock *sel)
{
	int i;

	if (!sel)
		return -1;

	/* Initialize main semaphore to 1 (available) */
	sel->sem = oss_sema_init(1);
	if (!sel->sem)
		goto fail;

	/* Initialize gate semaphores to 0 (blocked) */
	for (i = 0; i < SHARED_EXCLUSION_NUM_GROUPS; i++) {
		sel->gate[i] = oss_sema_init(0);
		if (!sel->gate[i])
			goto fail;

		sel->count[i] = oss_atomic_init();
		if (!sel->count[i])
			goto fail;
		oss_atomic_set(sel->count[i], 0);

		sel->active[i] = false;
	}

	sel->lock = oss_mutex_init();
	if (sel->lock == OSS_INVALID_HANDLE)
		goto fail;

	return 0;

fail:
	shared_exclusion_fini(adapt, sel);
	return -1;
}

void shared_exclusion_fini(struct amdgv_adapter *adapt,
						struct shared_exclusion_lock *sel)
{
	uint8_t i;

	if (!sel)
		return;

	if (sel->sem) {
		oss_sema_fini(sel->sem);
		sel->sem = NULL;
	}

	for (i = 0; i < SHARED_EXCLUSION_NUM_GROUPS; i++) {
		if (sel->gate[i]) {
			oss_sema_fini(sel->gate[i]);
			sel->gate[i] = NULL;
		}

		if (sel->count[i]) {
			oss_atomic_fini(sel->count[i]);
			sel->count[i] = NULL;
		}

		sel->active[i] = false;
	}

	if (sel->lock) {
		oss_mutex_fini(sel->lock);
		sel->lock = NULL;
	}
}

void shared_exclusion_enter(struct amdgv_adapter *adapt,
						struct shared_exclusion_lock *sel,
						uint8_t group)
{
	uint8_t count;
	uint8_t waiters;

	if (!sel || group >= SHARED_EXCLUSION_NUM_GROUPS)
		return;

	oss_mutex_lock(sel->lock);
	count = oss_atomic_inc_return(sel->count[group]);

	if (count == 1) {
		/* First thread of this group - acquire main semaphore */
		oss_mutex_unlock(sel->lock);
		oss_sema_down(sel->sem);

		/* Got the semaphore - mark group as active and wake up waiters */
		oss_mutex_lock(sel->lock);
		sel->active[group] = true;

		/* Signal all threads waiting at the gate */
		waiters = oss_atomic_read(sel->count[group]) - 1;
		while (waiters > 0) {
			oss_sema_up(sel->gate[group]);
			waiters--;
		}
		oss_mutex_unlock(sel->lock);
	} else if (!sel->active[group]) {
		/* Not first and group not yet active - wait at gate */
		oss_mutex_unlock(sel->lock);
		oss_sema_down(sel->gate[group]);
	} else {
		/* Group already active - proceed immediately */
		oss_mutex_unlock(sel->lock);
	}
}

void shared_exclusion_exit(struct amdgv_adapter *adapt,
						struct shared_exclusion_lock *sel,
						uint8_t group)
{
	uint8_t count;

	if (!sel || group >= SHARED_EXCLUSION_NUM_GROUPS)
		return;

	oss_mutex_lock(sel->lock);
	count = oss_atomic_dec_return(sel->count[group]);

	if (count == 0) {
		/* Last thread of this group - release main semaphore */
		sel->active[group] = false;
		oss_sema_up(sel->sem);
	}
	oss_mutex_unlock(sel->lock);
}
