add_executable(shm_example main.c)
target_link_libraries(shm_example shmutil pthread rt)
