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
