#pragma once

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct shm_info_t_ {
    int fd;
    size_t size;
    void *ptr;
} shm_info_t;

/**
 * @brief create new shared memory
 * @param name name used by shm_open
 * @param size shared memory size
 * @return NULL for error
 */
extern shm_info_t *shared_memory_create(const char *name, size_t size);

/**
 * @brief open exist shared memory
 * @param name name used by shm_open
 * @return NULL for error
 */
extern shm_info_t *shared_memory_open(const char *name);

/**
 * @brief close shared memory, shared memory still exist
 * @param info create by shm_create/shm_open
 */
extern void shared_memory_close(shm_info_t *info);

/**
 * @brief remove shared memory
 * @param name name used by shm_open
 * @return 0 on success, -1 on error
 */
extern int shared_memory_remove(const char *name);

/**
 * @brief init phtread_mutex with PTHREAD_PROCESS_SHARED
 * @param mutex the mutex pointer
 * @return 0 on success, other fail
 */
extern int shm_lock_init(pthread_mutex_t *mutex);

#ifdef __cplusplus
}
#endif
