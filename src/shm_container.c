#include <string.h>

#include "shmutil.h"
#include "shm_container.h"

#define POOL_FLAG_END -1
#define POOL_FLAG_USING -2

static int32_t align_size(int32_t size, int32_t align)
{
    // align must be 2^x
    if ((align & (align - 1)) != 0)
        return -1;
    return align * ((size + (align - 1)) / align);
}

static int32_t shared_memory_datapos(int32_t count, int32_t align)
{
    return align_size(sizeof(shared_memory_pool_t) + sizeof(int32_t) * count, align);
}

size_t shared_memory_pool_size(int32_t elemsize, int32_t count, int32_t align)
{
    if (align > 1)
        elemsize = align_size(elemsize, align);
    return shared_memory_datapos(count, align) + elemsize * count;
}

shared_memory_pool_t *shared_memory_pool_create(void *ptr, int32_t elemsize, int32_t count, int32_t align)
{
    if (align > 1)
        elemsize = align_size(elemsize, align);
    if (elemsize < 0)
        return NULL;

    shared_memory_pool_t *pool = ptr;
    pool->elemsize = elemsize;
    pool->count = count;
    pool->use_count = 0;

    if (pthread_spin_init(&pool->mutex, PTHREAD_PROCESS_SHARED) != 0)
        return NULL;
    pool->flag = 0xa1a20304;
    pool->datapos = shared_memory_datapos(count, align) - sizeof(shared_memory_pool_t);
    shared_memory_pool_clear(pool);
    return pool;
}

shared_memory_pool_t *shared_memory_pool_open(void *ptr)
{
    shared_memory_pool_t *pool = ptr;
    if (pool->flag != 0xa1a20304)
        return NULL;
    return pool;
}

void shared_memory_pool_clear(shared_memory_pool_t *pool)
{
    pthread_spin_lock(&pool->mutex);
    int32_t *meta = (int32_t *)pool->data;
    pool->first = 0;
    for (int32_t i = 0; i < pool->count - 1; i++) {
        meta[i] = i + 1;
    }
    meta[pool->count - 1] = POOL_FLAG_END;
    pool->use_count = 0;
    pthread_spin_unlock(&pool->mutex);
}

void *shared_memory_pool_malloc(shared_memory_pool_t *pool)
{
    void *p = NULL;
    pthread_spin_lock(&pool->mutex);
    int32_t *meta = (int32_t *)pool->data;
    if (pool->first >= 0) {
        int offset = pool->first;
        pool->first = meta[offset];
        meta[offset] = POOL_FLAG_USING;
        p = pool->data + pool->datapos + offset * pool->elemsize;
        pool->use_count++;
    }
    pthread_spin_unlock(&pool->mutex);
    return p;
}

void shared_memory_pool_free(shared_memory_pool_t *pool, void *ptr)
{
    int offset = shared_memory_pool_offset(pool, ptr);
    if (offset < 0)
        return;

    pthread_spin_lock(&pool->mutex);
    int32_t *meta = (int32_t *)pool->data;
    meta[offset]= pool->first;
    pool->first = offset;
    pool->use_count--;
    pthread_spin_unlock(&pool->mutex);
}

int32_t shared_memory_pool_offset(shared_memory_pool_t *pool, void *ptr)
{
    int32_t df = (uint8_t *)ptr - pool->data - pool->datapos;
    if (df % pool->elemsize != 0)
        return -1;
    int32_t offset = df / pool->elemsize;
    if (offset < 0 || offset >= pool->count)
        return -1;
    return offset;
}

void *shared_memory_pool_pointer(shared_memory_pool_t *pool, int32_t offset)
{
    void *ptr = NULL;
    if (offset < 0 || offset >= pool->count)
        return ptr;

    pthread_spin_lock(&pool->mutex);
    int32_t *meta = (int32_t *)pool->data;
    if (meta[offset] == POOL_FLAG_USING)
        ptr = pool->data + pool->datapos + offset * pool->elemsize;
    pthread_spin_unlock(&pool->mutex);
    return ptr;
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
