find_package(Boost REQUIRED COMPONENTS system)
find_package(CLI11 CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)
find_package(ftxui CONFIG REQUIRED)
find_package(libgit2 CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
add_library(mrm_core STATIC)
target_sources(
    mrm_core
    PRIVATE cli.cpp
            output_view.cpp
            config.cpp
            exec.cpp
            find.cpp
            repo_factory.cpp
            runtime.cpp
            status.cpp
            sync.cpp
            tracker.cpp
            update.cpp)
target_include_directories(mrm_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mrm_core PRIVATE Boost::system)
target_link_libraries(mrm_core PRIVATE CLI11::CLI11)
target_link_libraries(mrm_core PRIVATE TBB::tbb TBB::tbbmalloc)
target_link_libraries(mrm_core PRIVATE ftxui::screen ftxui::dom
                                       ftxui::component)
target_link_libraries(mrm_core PUBLIC libgit2::libgit2package)
target_link_libraries(mrm_core PRIVATE yaml-cpp::yaml-cpp)
