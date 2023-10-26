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
    void *data = malloc(shared_memory_pool_size(elemsize, count, 8));
    shared_memory_pool_t *pool = shared_memory_pool_create(data, elemsize, count, 8);

    int32_t *offset = malloc(sizeof(int32_t) * count);
    for (size_t i = 0; i < count; i++) {
        if(i == 50)
            EXPECT_EQ(pool->use_count, 50);

        void *ptr = shared_memory_pool_malloc(pool);
        EXPECT_TRUE(ptr != NULL);
        memory_set_value(ptr, elemsize, i);
        offset[i] = shared_memory_pool_offset(pool, ptr);
    }
    EXPECT_TRUE(shared_memory_pool_malloc(pool) == NULL);
    EXPECT_EQ(pool->use_count, 100);

    for (size_t i = 0; i < count; i++) {
        if(i == 50)
            EXPECT_EQ(pool->use_count, 50);

        void *ptr = shared_memory_pool_pointer(pool, offset[i]);
        EXPECT_EQ(memory_check_value(ptr, elemsize, i), 1);
        shared_memory_pool_free(pool, ptr);
    }
    EXPECT_EQ(pool->use_count, 0);

    free(offset);
    free(data);
}

UTEST(shared_memory_pool, error_value)
{
    size_t elemsize = 1200;
    size_t count = 100;
    void *data = malloc(shared_memory_pool_size(elemsize, count, 8));
    shared_memory_pool_t *pool = shared_memory_pool_create(data, elemsize, count, 8);

    uint8_t *ptrs[4];
    int32_t offset[4];
    for (int i = 0; i < 4; i++) {
        ptrs[i] = shared_memory_pool_malloc(pool);
        EXPECT_TRUE(ptrs[i] != NULL);
        offset[i] = shared_memory_pool_offset(pool, ptrs[i]);
        EXPECT_TRUE(offset[i] >= 0);
    }

    // test invalid pointer
    for (int i = 0; i < 4; i++) {
        int p = shared_memory_pool_offset(pool, ptrs[i] + 10);
        EXPECT_TRUE(p < 0);
    }

    // test invalid pointer
    for (int i = 0; i < 20; i++) {
        int found = 0;
        for (int j = 0; j < 4; j++) {
            if (offset[j] == i) {
                found = 1;
                break;
            }
        }

        void *ptr = shared_memory_pool_pointer(pool, i);
        if (found)
            EXPECT_TRUE(ptr != NULL);
        else
            EXPECT_TRUE(ptr == NULL);
    }

    free(data);
}

UTEST(shared_memory_pool, align)
{
    int32_t elemsize = 1200;
    int32_t count = 100;
    void *data = aligned_alloc(32, shared_memory_pool_size(elemsize, count, 32));
    shared_memory_pool_t *pool = shared_memory_pool_create(data, elemsize, count, 32);

    for (int i = 0; i < count; i++) {
        void *p = shared_memory_pool_malloc(pool);
        EXPECT_TRUE(p != NULL);
        EXPECT_TRUE((intptr_t)p % 32 == 0);
    }

    free(data);
}