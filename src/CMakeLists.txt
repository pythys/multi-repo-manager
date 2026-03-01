add_subdirectory(lib)
add_executable(mrm main.cpp)
target_link_libraries(mrm PRIVATE mrm_core)

install(TARGETS mrm RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(
    DIRECTORY ${CMAKE_BINARY_DIR}/completions/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/mrm/completions
    OPTIONAL)
