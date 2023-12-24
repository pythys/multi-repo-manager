find_package(Boost REQUIRED COMPONENTS system)
find_package(TBB CONFIG REQUIRED)
find_package(unofficial-libgit2 CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
add_library(mrm_core STATIC)
target_sources(mrm_core PRIVATE
  config.cpp
  exec.cpp
  find.cpp
  git_manager.cpp
  repo_factory.cpp
  sync.cpp
)
target_link_libraries(mrm_core PRIVATE Boost::system)
target_link_libraries(mrm_core PRIVATE TBB::tbb TBB::tbbmalloc)
target_link_libraries(mrm_core PRIVATE unofficial::libgit2::libgit2)
target_link_libraries(mrm_core PRIVATE yaml-cpp)
