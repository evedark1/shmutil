aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC)
add_executable(unittest ${SRC})
target_link_libraries(unittest shmutil pthread rt)
