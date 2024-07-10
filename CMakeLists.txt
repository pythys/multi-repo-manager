cmake_minimum_required(VERSION 3.28)

# vcpkg toolchain
if(NOT DEFINED ENV{VCPKG_ROOT})
    message(FATAL_ERROR "VCPKG_ROOT is not defined.")
endif()
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

project(mrm VERSION 1.0)

# vcpkg include
set(VCPKG_INCLUDE "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/include")
include_directories(${VCPKG_INCLUDE})

# compile options
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_program(CLANG_TIDY_BIN NAMES clang-tidy)
if(NOT CMAKE_GENERATOR STREQUAL "Ninja" AND CLANG_TIDY_BIN)
    set(TIDY_CHECKS "clang-tidy;--checks=-llvmlibc*")
    set(TIDY_FILTER ";--header-filter=${CMAKE_SOURCE_DIR}/(src|tests)/.*")
    set(CMAKE_CXX_CLANG_TIDY "${TIDY_CHECKS}${TIDY_FILTER}")
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
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
add_subdirectory(src ${CMAKE_BINARY_DIR}/mrm)
add_subdirectory(tests ${CMAKE_BINARY_DIR}/tests)

# cpack
set(CPACK_GENERATOR "TGZ;ZIP")
include(CPack)
