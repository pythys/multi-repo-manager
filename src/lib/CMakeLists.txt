find_package(Boost REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(ftxui CONFIG REQUIRED)
find_package(libgit2 CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

add_library(mrm_core STATIC)

target_sources(
    mrm_core
    PRIVATE cli.cpp
            # util/
            util/common.cpp
            util/runtime.cpp
            # core/
            core/tree.cpp
            core/repo_type.cpp
            core/tracker.cpp
            # vcs/
            vcs/repo_factory.cpp
            # persistence/
            persistence/config.cpp
            persistence/discovery.cpp
            # presentation/
            presentation/output_view.cpp
            presentation/tracked_operation.cpp
            # completion/
            completion/completion_bash.cpp
            completion/completion_spec.cpp
            completion/completion_zsh.cpp
            # command/
            command/completion.cpp
            command/exec.cpp
            command/find.cpp
            command/init.cpp
            command/list.cpp
            command/remotesync.cpp
            command/status.cpp
            command/sync.cpp
            command/update.cpp)

target_include_directories(mrm_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mrm_core PRIVATE Boost::headers Boost::filesystem)
target_link_libraries(mrm_core PUBLIC CLI11::CLI11)
target_link_libraries(mrm_core PRIVATE ftxui::screen ftxui::dom
                                       ftxui::component)
target_link_libraries(mrm_core PUBLIC libgit2::libgit2)
target_link_libraries(mrm_core PUBLIC yaml-cpp::yaml-cpp)
