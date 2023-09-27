#include "utest.h"
#include "shm_container.h"

static void memory_set_value(void *ptr, size_t sz, uint8_t value) {
    memset(ptr, value, sz);
}

static int memory_check_value(void *ptr, size_t sz, uint8_t value) {
    uint8_t *p = ptr;
    for (size_t i = 0; i < sz; i++) {
        if (*p != value)
            return 0;
    }
    return 1;
}

UTEST(shared_memory_pool, malloc_free)
{
    size_t elemsize = 1200;
    size_t count = 100;
    void *data = malloc(shared_memory_pool_size(elemsize, count));
    shared_memory_pool_t *pool = shared_memory_pool_create(data, elemsize, count);

    int32_t *offset = malloc(sizeof(int32_t) * count);
    for (size_t i = 0; i < count; i++) {
        void *ptr = shared_memory_pool_malloc(pool);
        EXPECT_TRUE(ptr != NULL);
        memory_set_value(ptr, elemsize, i);
        offset[i] = shared_memory_pool_offset(pool, ptr);
    }
    EXPECT_TRUE(shared_memory_pool_malloc(pool) == NULL);

    for (size_t i = 0; i < count; i++) {
        void *ptr = shared_memory_pool_pointer(pool, offset[i]);
        EXPECT_EQ(memory_check_value(ptr, elemsize, i), 1);
        shared_memory_pool_free(pool, ptr);
    }

    free(offset);
    free(data);
}
