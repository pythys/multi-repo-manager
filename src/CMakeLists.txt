add_subdirectory(lib)
include_directories(lib)
add_executable(mrm main.cpp)
target_link_libraries(mrm PRIVATE mrm_core)
