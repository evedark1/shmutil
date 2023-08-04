#include "utest.h"
#include "shm_container.h"

UTEST(shared_memory_pool, malloc_free)
{
    size_t elemsize = 128;
    size_t count = 100;
    void *data = malloc(shared_memory_pool_size(elemsize, count));
    shared_memory_pool_t *pool = shared_memory_pool_create(data, elemsize, count);

    void **ptrs = malloc(sizeof(void *) * count);
    for (size_t i = 0; i < count; i++) {
        void *ptr = shared_memory_pool_malloc(pool);
        EXPECT_TRUE(ptr != NULL);
        memset(ptr, 0, elemsize);
        ptrs[i] = ptr;
    }
    EXPECT_TRUE(shared_memory_pool_malloc(pool) == NULL);

    for (size_t i = 0; i < count; i++) {
        void *ptr = ptrs[i];
        shared_memory_pool_free(pool, ptr);
    }
    EXPECT_TRUE(shared_memory_pool_malloc(pool) != NULL);

    free(ptrs);
    free(data);
}
