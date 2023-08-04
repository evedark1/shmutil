#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "shmutil.h"

#define SHARED_MEMORY_SIZE (1024 * 1024 * 6)

void fill_shared_memory(shm_info_t *info)
{
    uint64_t *data = info->ptr;
    for (uint64_t i = 0; i < info->size / sizeof(uint64_t); i++) {
        data[i] = i;
    }
}

void test_shared_memory(shm_info_t *info)
{
    if (info->size != SHARED_MEMORY_SIZE) {
        printf("size error\n");
        return;
    }
    uint64_t *data = info->ptr;
    for (uint64_t i = 0; i < info->size / sizeof(uint64_t); i++) {
        if (data[i] != i) {
            printf("data error\n");
            return;
        }
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
    if (master) {
        shm = shared_memory_create(name, SHARED_MEMORY_SIZE);
        if (shm == NULL) {
            printf("create error\n");
            return -1;
        }
        fill_shared_memory(shm);
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
        test_shared_memory(shm);
        shared_memory_close(shm);
    }
    return 0;
}
