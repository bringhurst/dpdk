/*-
 *   BSD LICENSE
 * 
 *   Copyright(c) 2010-2013 Intel Corporation. All rights reserved.
 *   All rights reserved.
 * 
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 * 
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _RTE_SPINLOCK_H_
#define _RTE_SPINLOCK_H_

/**
 * @file
 *
 * RTE Spinlocks
 *
 * This file defines an API for read-write locks, which are implemented
 * in an architecture-specific way. This kind of lock simply waits in
 * a loop repeatedly checking until the lock becomes available.
 *
 * All locks must be initialised before use, and only initialised once.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <rte_lcore.h>
#ifdef RTE_FORCE_INTRINSICS
#include <rte_common.h>
#endif

/**
 * The rte_spinlock_t type.
 */
typedef struct {
	volatile int locked; /**< lock status 0 = unlocked, 1 = locked */
} rte_spinlock_t;

/**
 * A static spinlock initializer.
 */
#define RTE_SPINLOCK_INITIALIZER { 0 }

/**
 * Initialize the spinlock to an unlocked state.
 *
 * @param sl
 *   A pointer to the spinlock.
 */
static inline void
rte_spinlock_init(rte_spinlock_t *sl)
{
	sl->locked = 0;
}

/**
 * Take the spinlock.
 *
 * @param sl
 *   A pointer to the spinlock.
 */
static inline void
rte_spinlock_lock(rte_spinlock_t *sl)
{
#ifndef RTE_FORCE_INTRINSICS
	int lock_val = 1;
	asm volatile (
			"1:\n"
			"xchg %[locked], %[lv]\n"
			"test %[lv], %[lv]\n"
			"jz 3f\n"
			"2:\n"
			"pause\n"
			"cmp $0, %[locked]\n"
			"jnz 2b\n"
			"jmp 1b\n"
			"3:\n"
			: [locked] "=m" (sl->locked), [lv] "=q" (lock_val)
			: "[lv]" (lock_val)
			: "memory");
#else
	while (__sync_lock_test_and_set(&sl->locked, 1))
		while(sl->locked)
			rte_pause();
#endif
}

/**
 * Release the spinlock.
 *
 * @param sl
 *   A pointer to the spinlock.
 */
static inline void
rte_spinlock_unlock (rte_spinlock_t *sl)
{
#ifndef RTE_FORCE_INTRINSICS
	int unlock_val = 0;
	asm volatile (
			"xchg %[locked], %[ulv]\n"
			: [locked] "=m" (sl->locked), [ulv] "=q" (unlock_val)
			: "[ulv]" (unlock_val)
			: "memory");
#else
	__sync_lock_release(&sl->locked);
#endif
}

/**
 * Try to take the lock.
 *
 * @param sl
 *   A pointer to the spinlock.
 * @return
 *   1 if the lock is successfully taken; 0 otherwise.
 */
static inline int
rte_spinlock_trylock (rte_spinlock_t *sl)
{
#ifndef RTE_FORCE_INTRINSICS
	int lockval = 1;

	asm volatile (
			"xchg %[locked], %[lockval]"
			: [locked] "=m" (sl->locked), [lockval] "=q" (lockval)
			: "[lockval]" (lockval)
			: "memory");

	return (lockval == 0);
#else
	return (__sync_lock_test_and_set(&sl->locked,1) == 0);
#endif
}

/**
 * Test if the lock is taken.
 *
 * @param sl
 *   A pointer to the spinlock.
 * @return
 *   1 if the lock is currently taken; 0 otherwise.
 */
static inline int rte_spinlock_is_locked (rte_spinlock_t *sl)
{
	return sl->locked;
}

/**
 * The rte_spinlock_recursive_t type.
 */
typedef struct {
	rte_spinlock_t sl; /**< the actual spinlock */
	volatile int user; /**< core id using lock, -1 for unused */
	volatile int count; /**< count of time this lock has been called */
} rte_spinlock_recursive_t;

/**
 * A static recursive spinlock initializer.
 */
#define RTE_SPINLOCK_RECURSIVE_INITIALIZER {RTE_SPINLOCK_INITIALIZER, -1, 0}

/**
 * Initialize the recursive spinlock to an unlocked state.
 *
 * @param slr
 *   A pointer to the recursive spinlock.
 */
static inline void rte_spinlock_recursive_init(rte_spinlock_recursive_t *slr)
{
	rte_spinlock_init(&slr->sl);
	slr->user = -1;
	slr->count = 0;
}

/**
 * Take the recursive spinlock.
 *
 * @param slr
 *   A pointer to the recursive spinlock.
 */
static inline void rte_spinlock_recursive_lock(rte_spinlock_recursive_t *slr)
{
	int id = rte_lcore_id();

	if (slr->user != id) {
		rte_spinlock_lock(&slr->sl);
		slr->user = id;
	}
	slr->count++;
}
/**
 * Release the recursive spinlock.
 *
 * @param slr
 *   A pointer to the recursive spinlock.
 */
static inline void rte_spinlock_recursive_unlock(rte_spinlock_recursive_t *slr)
{
	if (--(slr->count) == 0) {
		slr->user = -1;
		rte_spinlock_unlock(&slr->sl);
	}

}

/**
 * Try to take the recursive lock.
 *
 * @param slr
 *   A pointer to the recursive spinlock.
 * @return
 *   1 if the lock is successfully taken; 0 otherwise.
 */
static inline int rte_spinlock_recursive_trylock(rte_spinlock_recursive_t *slr)
{
	int id = rte_lcore_id();

	if (slr->user != id) {
		if (rte_spinlock_trylock(&slr->sl) == 0)
			return 0;
		slr->user = id;
	}
	slr->count++;
	return 1;
}

#ifdef __cplusplus
}
#endif

#endif /* _RTE_SPINLOCK_H_ */
