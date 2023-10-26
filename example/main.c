#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>

#include "shmutil.h"
#include "shm_container.h"

void rand_data(void *data, size_t len)
{
    uint8_t *d = data;
    for (size_t i = 0; i < len; i++)
        d[i] = rand();
}

int hash(void *data, size_t len)
{
    uint8_t *d = data;
    unsigned int r = 0;
    for (size_t i = 0; i < len; i++)
        r += d[i];
    return r & 0x00ffffff;
}

const size_t SHARE_ELEMSIZE = 1024;

struct shared_context {
    char pool_data[1024 * 300];
    char queue_data[1200];
};

struct local_context {
    shared_memory_pool_t *pool;
    shared_queue_t *queue;
};

void shared_context_create(void *ptr, struct local_context *local)
{
    struct shared_context *ctx = ptr;

    assert(shared_memory_pool_size(SHARE_ELEMSIZE, 256, 8) < sizeof(ctx->pool_data));
    local->pool = shared_memory_pool_create(ctx->pool_data, SHARE_ELEMSIZE, 256, 8);

    assert(shared_queue_size(1024) < sizeof(ctx->queue_data));
    local->queue = shared_queue_create(ctx->queue_data, 1024);
}

void shared_context_open(void *ptr, struct local_context *local)
{
    struct shared_context *ctx = ptr;

    local->pool = shared_memory_pool_open(ctx->pool_data);
    local->queue = shared_queue_open(ctx->queue_data);
}

void server(void *shared_ptr)
{
    struct local_context local;
    shared_context_create(shared_ptr, &local);

    int sid;
    int offset;
    int hs;
    char buffer[32];
    for (int i = 0; i < 1000; i++) {
        void *in = shared_memory_pool_malloc(local.pool);
        rand_data(in, SHARE_ELEMSIZE);

        sid = i;
        offset = shared_memory_pool_offset(local.pool, in);
        hs = hash(in, SHARE_ELEMSIZE);
        snprintf(buffer, sizeof(buffer), "%d,%d,%d", sid, offset, hs);

        while (shared_queue_put(local.queue, buffer, strlen(buffer)) <= 0) {
            usleep(1000);
        }
        // printf("item: %s\n", buffer);
        fflush(stdout);
    }

    snprintf(buffer, sizeof(buffer), "%d,%d,%d", -1, 0, 0);
    shared_queue_put(local.queue, buffer, strlen(buffer));
}

void client(void *shared_ptr)
{
    struct local_context local;
    shared_context_open(shared_ptr, &local);

    int sid;
    int offset;
    int hs;
    char buffer[32];
    while (1) {
        int len = shared_queue_get(local.queue, buffer, sizeof(buffer));
        if (len < 0) {
            printf("get queue fail\n");
            break;
        }
        else if (len == 0) {
            usleep(1000);
            continue;
        }
        buffer[len] = '\0';

        // printf("item: %s\n", buffer);
        sscanf(buffer, "%d,%d,%d", &sid, &offset, &hs);
        if (sid < 0)
            break;

        void *in = shared_memory_pool_pointer(local.pool, offset);
        if (hs != hash(in, SHARE_ELEMSIZE)) {
            printf("check fail\n");
            return;
        }
        shared_memory_pool_free(local.pool, in);
    }
    printf("check ok\n");
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        printf("usage: %s [name] [master]\n", argv[0]);
        return -1;
    }
    char *name = argv[1];
    int master = atoi(argv[2]);

    shm_info_t *shm;
    srand(123456);
    if (master) {
        shm = shared_memory_create(name, sizeof(struct shared_context));
        if (shm == NULL) {
            printf("create error\n");
            return -1;
        }

        server(shm->ptr);
        printf("input key to exit\n");
        getchar();

        shared_memory_close(shm);
        if (shared_memory_remove(name) != 0) {
            printf("remove error\n");
            return -1;
        }
    } else {
        shm = shared_memory_open(name);
        if (shm == NULL) {
            printf("open error\n");
            return -1;
        }
        if (shm->size != sizeof(struct shared_context)) {
            printf("size error\n");
            return -1;
        }

        client(shm->ptr);

        shared_memory_close(shm);
    }
    return 0;
}
