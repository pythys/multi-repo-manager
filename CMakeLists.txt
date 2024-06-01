cmake_minimum_required(VERSION 3.27)

# vcpkg
if(NOT DEFINED ENV{VCPKG_ROOT})
    message(FATAL_ERROR "VCPKG_ROOT is not defined.")
endif()
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

project(mrm VERSION 1.0)

# compile options
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(TIDY_CHECKS "clang-tidy;--checks=-llvmlibc*")
set(TIDY_FILTER ";--header-filter=${CMAKE_SOURCE_DIR}/(src|tests)/.*")
set(CMAKE_CXX_CLANG_TIDY "${TIDY_CHECKS}${TIDY_FILTER}")
set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "ac01f462-0f5f-432a-86aa-acef252918a6")
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
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()
enable_testing()

# sub-projects
add_subdirectory(src)
add_subdirectory(tests)

# cpack
set(CPACK_GENERATOR "TGZ;ZIP")
include(CPack)
