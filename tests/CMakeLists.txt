find_package(GTest REQUIRED)
include_directories(${CMAKE_SOURCE_DIR}/src/lib)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_definitions(-DTEST_RESOURCES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources")

set(UNIT_TESTS
    unit/completion_tests.cpp
    unit/find_tests.cpp
    unit/runtime_tests.cpp
    unit/tracker_tests.cpp
    unit/output_view_tests.cpp
    unit/config_tests.cpp
    unit/git_manager_tests.cpp)
foreach(test_src ${UNIT_TESTS})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE GTest::gtest_main)
    target_link_libraries(${test_name} PRIVATE mrm_core)
    include(GoogleTest)
    gtest_discover_tests(${test_name} DISCOVERY_TIMEOUT 30 PROPERTIES LABELS
                                                                      unit)
endforeach()

set(INTEGRATION_TESTS
    integration/sync_tests.cpp integration/remotesync_tests.cpp
    integration/status_tests.cpp integration/exec_tests.cpp
    integration/init_tests.cpp integration/update_tests.cpp)
foreach(test_src ${INTEGRATION_TESTS})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE GTest::gtest_main)
    target_link_libraries(${test_name} PRIVATE mrm_core)
    include(GoogleTest)
    gtest_discover_tests(${test_name} DISCOVERY_TIMEOUT 30
                         PROPERTIES LABELS integration)
endforeach()
