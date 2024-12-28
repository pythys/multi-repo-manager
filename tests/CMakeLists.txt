include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
include_directories(${CMAKE_SOURCE_DIR}/src/lib)
add_definitions(-DTEST_RESOURCES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources")
set(TEST_SRCS config_tests.cpp)
foreach(test_src ${TEST_SRCS})
  get_filename_component(test_name ${test_src} NAME_WE)
  add_executable(${test_name} ${test_src})
  target_link_libraries(${test_name} PRIVATE GTest::gtest_main)
  target_link_libraries(${test_name} PRIVATE mrm_core)
  include(GoogleTest)
  gtest_discover_tests(${test_name})
endforeach()
