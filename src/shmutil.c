#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "shmutil.h"

static shm_info_t *shm_info_malloc()
{
    shm_info_t *info = malloc(sizeof(shm_info_t));
    if (info == NULL)
        return NULL;
    info->fd = -1;
    info->size = 0;
    info->ptr = NULL;
    return info;
}

shm_info_t *shared_memory_create(const char *name, size_t size)
{
    shm_info_t *info = shm_info_malloc();
    if (info == NULL)
        return NULL;

    info->fd = shm_open(name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (info->fd < 0)
        goto err;
    if (ftruncate(info->fd, size) < 0)
        goto err;
    info->size = size;
    info->ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, info->fd, 0);
    if (MAP_FAILED == info->ptr) {
        info->ptr = NULL;
        goto err;
    }

    return info;

err:
    shared_memory_close(info);
    return NULL;
}

shm_info_t *shared_memory_open(const char *name)
{
    struct stat st;
    shm_info_t *info = shm_info_malloc();
    if (info == NULL)
        return NULL;

    info->fd = shm_open(name, O_RDWR, S_IRUSR | S_IWUSR);
    if (info->fd < 0)
        goto err;
    if (fstat(info->fd, &st) < 0)
        goto err;
    info->size = st.st_size;
    info->ptr = mmap(NULL, info->size, PROT_READ | PROT_WRITE, MAP_SHARED, info->fd, 0);
    if (MAP_FAILED == info->ptr) {
        info->ptr = NULL;
        goto err;
    }

    return info;

err:
    shared_memory_close(info);
    return NULL;
}

void shared_memory_close(shm_info_t *info)
{
    if (info->ptr != NULL)
        munmap(info->ptr, info->size);
    if (info->fd >= 0)
        close(info->fd);
    free(info);
}

int shared_memory_remove(const char *name)
{
    return shm_unlink(name);
}

int shm_lock_init(pthread_mutex_t *mutex)
{
    pthread_mutexattr_t mat;
    int r;
    if ((r = pthread_mutexattr_init(&mat)) != 0)
        return r;
    if ((r = pthread_mutexattr_setpshared(&mat, PTHREAD_PROCESS_SHARED)) != 0)
        return r;
    if ((r = pthread_mutex_init(mutex, &mat)) != 0)
        return r;
    return 0;
}