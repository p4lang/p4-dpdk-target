#ifndef _P4_SDE_OSDEP_H_
#define _P4_SDE_OSDEP_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <endian.h>

#include <target-sys/bf_sal/bf_sys_intf.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

#ifndef CPU_TO_LE16
#define CPU_TO_LE16(a) htole16(a)
#define CPU_TO_LE32(a) htole32(a)
#define CPU_TO_LE64(a) htole64(a)
#define LE16_TO_CPU(a) le16toh(a)
#define LE32_TO_CPU(a) le32toh(a)
#define LE64_TO_CPU(a) le64toh(a)
#define CPU_TO_BE16(a) htobe16(a)
#define CPU_TO_BE32(a) htobe32(a)
#define CPU_TO_BE64(a) htobe64(a)
#define BE16_TO_CPU(a) be16toh(a)
#define BE32_TO_CPU(a) be32toh(a)
#define BE64_TO_CPU(a) be64toh(a)
#endif

#ifndef BIT
#define BIT(a) (1UL << (a))
#endif /* BIT */
#ifndef BIT_ULL
#define BIT_ULL(a) (1ULL << (a))
#endif /* BIT_ULL */
//#define __le64 uint64_t


#define P4_SDE_MALLOC(size) bf_sys_malloc(size)
#define P4_SDE_CALLOC(num, size) bf_sys_calloc((num), (size))
#define P4_SDE_FREE(ptr) bf_sys_free(ptr)
#define P4_SDE_MEMSET(ptr, val, size) memset((ptr), (val), (size))

/* MUTEX */

typedef bf_sys_mutex_t p4_sde_mutex;

#define P4_SDE_MUTEX_INIT(mtx) bf_sys_mutex_init(mtx)
#define P4_SDE_MUTEX_LOCK(mtx) bf_sys_mutex_lock(mtx)
#define P4_SDE_MUTEX_TRY_LOCK(mtx) bf_sys_mutex_trylock(mtx)
#define P4_SDE_MUTEX_UNLOCK(mtx) bf_sys_mutex_unlock(mtx)
#define P4_SDE_MUTEX_DESTROY(mtx) bf_sys_mutex_del(mtx)

/* READ/WRITE LOCK */
typedef bf_sys_rwlock_t p4_sde_rwlock;

#define P4_SDE_RWLOCK_INIT(lock, lock_attr) \
		bf_sys_rwlock_init((lock), (lock_attr))
#define P4_SDE_RWLOCK_RDLOCK(lock) \
		bf_sys_rwlock_rdlock(lock)
#define P4_SDE_RWLOCK_TRY_RDLOCK(lock) \
		bf_sys_rwlock_tryrdlock(lock)
#define P4_SDE_RWLOCK_WRLOCK(lock) \
		bf_sys_rwlock_wrlock(lock)
#define P4_SDE_RWLOCK_TRY_WRLOCK(lock) \
		bf_sys_rwlock_trywrlock(lock)
#define P4_SDE_RWLOCK_UNLOCK(lock) \
		bf_sys_rwlock_unlock(lock)
#define P4_SDE_RWLOCK_DESTROY(lock) \
		bf_sys_rwlock_del(lock)

/* LOG */
#define P4_SDE_LOG(module, level, ...) \
		bf_sys_log_and_trace(module, level, __VA_ARGS__)

/* Timer */
#define P4_SDE_USLEEP(usecs) bf_sys_usleep(usecs)
#endif
