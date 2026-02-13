add_subdirectory(lib)
add_executable(mrm main.cpp)
target_link_libraries(mrm PRIVATE mrm_core)
