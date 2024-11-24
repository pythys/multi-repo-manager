find_package(Boost REQUIRED COMPONENTS system)
find_package(CLI11 CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)
find_package(ftxui CONFIG REQUIRED)
find_package(unofficial-libgit2 CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
add_library(mrm_core STATIC)
target_sources(mrm_core PRIVATE
  cli.cpp
  config.cpp
  exec.cpp
  find.cpp
  repo_factory.cpp
  sync.cpp
)
target_link_libraries(mrm_core PRIVATE Boost::system)
target_link_libraries(mrm_core PRIVATE CLI11::CLI11)
target_link_libraries(mrm_core PRIVATE TBB::tbb TBB::tbbmalloc)
target_link_libraries(mrm_core PRIVATE ftxui::screen ftxui::dom ftxui::component)
target_link_libraries(mrm_core PRIVATE unofficial::libgit2::libgit2)
target_link_libraries(mrm_core PRIVATE yaml-cpp::yaml-cpp)
