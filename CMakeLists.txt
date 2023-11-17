cmake_minimum_required(VERSION 3.27)
project(mrm VERSION 1.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_CLANG_TIDY clang-tidy;--checks=-llvmlibc*)
set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "ac01f462-0f5f-432a-86aa-acef252918a6")
set(CPACK_GENERATOR "TGZ;ZIP")
include(CPack)
if(MSVC)
    add_compile_options(/W4 /experimental:module)
else()
    add_compile_options(
        -Wall
        -Wextra
        -Wshadow
        -Wconversion
        -Wpedantic
        -Werror
        -std=c++20
    )
endif()
add_subdirectory(src)
enable_testing()
add_subdirectory(tests)
