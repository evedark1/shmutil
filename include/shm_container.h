#pragma once
#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief shared memory pool
 */
typedef struct {
    int32_t elemsize;
    int32_t count;
    int32_t use_count;

    // private field
    uint32_t flag;
    int32_t datapos;
    pthread_spinlock_t mutex;
    int32_t first;
    uint8_t data[0];
} shared_memory_pool_t;

/**
 * @brief get shared memory pool total size
 * @param elemsize element size
 * @param count element count
 * @return total size
 */
extern size_t shared_memory_pool_size(int32_t elemsize, int32_t count, int32_t align);

/**
 * @brief create shared memory pool
 * @param ptr shared memory pointer
 * @param elemsize element size
 * @param count element count
 * @return shared memory pool
 */
extern shared_memory_pool_t *shared_memory_pool_create(void *ptr, int32_t elemsize, int32_t count, int32_t align);

/**
 * @brief open exist shared memory pool
 * @param ptr shared memory pointer
 * @return shared memory pool
 */
extern shared_memory_pool_t *shared_memory_pool_open(void *ptr);

/**
 * @brief clear memory pool, thread safe
 * @param pool shared memory pool
 */
extern void shared_memory_pool_clear(shared_memory_pool_t *pool);

/**
 * @brief malloc from shared memory pool, thread safe
 * @param pool shared memory pool
 * @return NULL on fail
 */
extern void *shared_memory_pool_malloc(shared_memory_pool_t *pool);

/**
 * @brief free from shared memory pool, thread safe
 * @param pool shared memory pool
 * @param ptr pointer to free
 */
extern void shared_memory_pool_free(shared_memory_pool_t *pool, void *ptr);

/**
 * @brief get offset in shared memory pool, thread safe
 * @param pool shared memory pool
 * @param ptr pointer malloc by pool
 * @return offset in shared memory pool, -1 on fail
 */
extern int32_t shared_memory_pool_offset(shared_memory_pool_t *pool, void *ptr);

/**
 * @brief get pointer in shared memory pool, thread safe
 * @param pool shared memory pool
 * @param offset offset in shared memory pool
 * @return NULL on fail
 */
extern void *shared_memory_pool_pointer(shared_memory_pool_t *pool, int32_t offset);

/**
 * @brief shared memory queue
 */
typedef struct {
    size_t size;

    // private field
    pthread_mutex_t mutex;
    uint32_t flag;
    int32_t readpos;
    int32_t writepos;
    uint8_t data[0];
} shared_queue_t;

/**
 * @brief get shared queue total size
 * @param size buffer size
 * @return total size
 */
extern size_t shared_queue_size(size_t size);

/**
 * @brief create shared queue
 * @param ptr shared memory pointer
 * @param size buffer size
 * @return shared queue
 */
extern shared_queue_t *shared_queue_create(void *ptr, size_t size);

/**
 * @brief open exist shared queue
 * @param ptr shared memory pointer
 * @return shared queue
 */
extern shared_queue_t *shared_queue_open(void *ptr);

/**
 * @brief puts data into the queue, thread safe
 * @param queue shared queue
 * @param data the data to be added
 * @param len the length of the data
 * @return length add, <= 0 on fail
 */
extern int shared_queue_put(shared_queue_t *queue, void *data, int len);

/**
 * @brief gets data from the queue, thread safe
 * @param queue shared queue
 * @param buffer the buffer to be copied
 * @param len the length of the buffer
 * @return >0 length of the get data, =0 no data, < 0 fail
 */
extern int shared_queue_get(shared_queue_t *queue, void *buffer, int len);


#ifdef __cplusplus
}
#endif
