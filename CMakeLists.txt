cmake_minimum_required(VERSION 3.20)

# Boost FindPackage compatibility for CMake >= 3.30
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Read version from VERSION file
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
project(mrm VERSION "${PROJECT_VERSION}")

include(GNUInstallDirs)

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler and linker flags
if(MSVC)
    add_compile_options(/W4 /MT)
elseif(APPLE)
    add_compile_options(-Wall -Wextra -Wshadow -Wconversion -Wpedantic -Werror)
    # macos ssl bugfix
    link_libraries("-framework Security" "-framework CoreFoundation")
elseif(UNIX)
    add_compile_options(-Wall -Wextra -Wshadow -Wconversion -Wpedantic -Werror)
    add_link_options(-static-libgcc -static-libstdc++)
else()
    add_compile_options(-Wall -Wextra -Wshadow -Wconversion -Wpedantic -Werror)
    add_link_options(-static-libgcc -static-libstdc++)
endif()

enable_testing()

# Sub-projects
add_subdirectory(src ${CMAKE_BINARY_DIR}/mrm)
add_subdirectory(tests ${CMAKE_BINARY_DIR}/tests)

# Packageing
if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
else()
    set(CPACK_GENERATOR "TGZ;ZIP")
endif()
set(CPACK_PACKAGE_NAME "mrm")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CPACK_SYSTEM_OS "darwin")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(CPACK_SYSTEM_OS "windows")
else()
    set(CPACK_SYSTEM_OS "linux")
endif()
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)$")
    set(CPACK_SYSTEM_ARCH "amd64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
    set(CPACK_SYSTEM_ARCH "arm64")
else()
    set(CPACK_SYSTEM_ARCH ${CMAKE_SYSTEM_PROCESSOR})
endif()
set(CPACK_PACKAGE_FILE_NAME
    "mrm_${PROJECT_VERSION}_${CPACK_SYSTEM_OS}_${CPACK_SYSTEM_ARCH}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Multi Repo Manager")
include(CPack)
