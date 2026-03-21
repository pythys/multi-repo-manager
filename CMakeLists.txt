cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0167 NEW) # FindBoost warning fix

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
project(mrm VERSION "${PROJECT_VERSION}")
include(GNUInstallDirs)

# compile options
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(MSVC)
    add_compile_options(/W4 /experimental:module)
else()
    add_compile_options(-Wall -Wextra -Wshadow -Wconversion -Wpedantic -Werror)
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
set(CPACK_PACKAGE_NAME "mrm")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Multi Repo Manager")
include(CPack)
