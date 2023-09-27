shmutil is tools for process shared memory

- shm_info_t: shared memory info
- shared_memory_pool_t: fixed size memory pool
- shared_queue_t: memory queue

### build

```bash
mkdir -p build
cd build
cmake ..
make
```

### example

```bash
./shm_example shm_ex 1 &&
./shm_example shm_ex 0 &&
```