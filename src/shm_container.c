#include <string.h>

#include "shm_container.h"

static inline size_t align_size(size_t size, size_t align)
{
    return align * ((size + (align - 1)) / align);
}

size_t shared_memory_pool_size(size_t elemsize, size_t count)
{
    elemsize = align_size(elemsize, sizeof(void *));
    return sizeof(shared_memory_pool_t) + elemsize * count;
}

shared_memory_pool_t *shared_memory_pool_create(void *ptr, size_t elemsize, size_t count)
{
    shared_memory_pool_t *pool = ptr;
    pool->elemsize = align_size(elemsize, sizeof(void *));
    pool->count = count;

    if (shm_lock_init(&pool->mutex) != 0)
        return NULL;
    pool->flag = 0xa1a20304;

    // init freelist link
    pool->first = 0;
    for (size_t i = 0; i < count - 1; i++) {
        *(int *)(pool->data + i * pool->elemsize) = (i + 1) * pool->elemsize;
    }
    *(int *)(pool->data + (count - 1) * pool->elemsize) = -1;

    return pool;
}

shared_memory_pool_t *shared_memory_pool_open(void *ptr)
{
    shared_memory_pool_t *pool = ptr;
    if (pool->flag != 0xa1a20304)
        return NULL;
    return pool;
}

void *shared_memory_pool_malloc(shared_memory_pool_t *pool)
{
    pthread_mutex_lock(&pool->mutex);
    void *p = NULL;
    if (pool->first >= 0) {
        p = pool->data + pool->first;
        pool->first = *(int *)p;
    }
    pthread_mutex_unlock(&pool->mutex);
    return p;
}

void shared_memory_pool_free(shared_memory_pool_t *pool, void *ptr)
{
    pthread_mutex_lock(&pool->mutex);
    *(int *)ptr = pool->first;
    pool->first = (uint8_t *)ptr - pool->data;
    pthread_mutex_unlock(&pool->mutex);
}

int32_t shared_memory_pool_offset(shared_memory_pool_t *pool, void *ptr)
{
    return (uint8_t *)ptr - pool->data;
}

void *shared_memory_pool_pointer(shared_memory_pool_t *pool, int32_t offset)
{
    if (offset % pool->elemsize != 0 || offset / pool->elemsize >= pool->count)
        return NULL;
    return pool->data + offset;
}

size_t shared_queue_size(size_t size)
{
    return sizeof(shared_queue_t) + size;
}

shared_queue_t *shared_queue_create(void *ptr, size_t size)
{
    shared_queue_t *queue = ptr;
    queue->size = size;

    if (shm_lock_init(&queue->mutex) != 0)
        return NULL;
    queue->flag = 0xa1a21314;
    queue->readpos = 0;
    queue->writepos = 0;

    return queue;
}

shared_queue_t *shared_queue_open(void *ptr)
{
    shared_queue_t *queue = ptr;
    if (queue->flag != 0xa1a21314)
        return NULL;
    return queue;
}

static int shared_queue_shrink(shared_queue_t *queue, int len)
{
    if (queue->readpos > 0) {
        if (queue->writepos > queue->readpos) {
            memmove(queue->data, queue->data + queue->readpos, queue->writepos - queue->readpos);
        }
        queue->writepos -= queue->readpos;
        queue->readpos = 0;
    }

    if (queue->writepos + len > queue->size)
        return -1;
    return 0;
}

int shared_queue_put(shared_queue_t *queue, void *data, int len)
{
    pthread_mutex_lock(&queue->mutex);

    int tlen = len + sizeof(int);
    if (queue->writepos + tlen > queue->size) {
        if (shared_queue_shrink(queue, tlen) != 0) {
            pthread_mutex_unlock(&queue->mutex);
            return 0;
        }
    }

    memcpy(queue->data + queue->writepos, &len, sizeof(int));
    queue->writepos += sizeof(int);
    memcpy(queue->data + queue->writepos, data, len);
    queue->writepos += len;

    pthread_mutex_unlock(&queue->mutex);
    return len;
}

int shared_queue_get(shared_queue_t *queue, void *buffer, int len)
{
    pthread_mutex_lock(&queue->mutex);

    int sz = queue->writepos - queue->readpos;
    if (sz <= 0) {
        pthread_mutex_unlock(&queue->mutex);
        return 0;
    }

    int hlen;
    memcpy(&hlen, queue->data + queue->readpos, sizeof(int));
    if (hlen > len) {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }
    queue->readpos += sizeof(int);

    memcpy(buffer, queue->data + queue->readpos, hlen);
    queue->readpos += hlen;

    pthread_mutex_unlock(&queue->mutex);
    return hlen;
}
