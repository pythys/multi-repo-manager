find_package(TBB CONFIG REQUIRED)
find_package(unofficial-libgit2 CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS graph)
add_library(mrm_core STATIC)
target_sources(mrm_core PRIVATE
  config.cpp
  exec.cpp
  find.cpp
  sync.cpp
)
target_link_libraries(mrm_core PRIVATE TBB::tbb TBB::tbbmalloc)
target_link_libraries(mrm_core PRIVATE unofficial::libgit2::libgit2)
target_link_libraries(mrm_core PRIVATE yaml-cpp)
