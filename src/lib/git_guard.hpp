#ifndef SRC_LIB_GIT_GUARD_HPP_
#define SRC_LIB_GIT_GUARD_HPP_

/**
 * @file git_guard.hpp
 * @brief RAII guard that manages the global libgit2 lifecycle.
 */

#include <git2.h>

/** Initializes libgit2 on construction and shuts it down on destruction. */
class GitGuard {
  public:
    GitGuard() {
        git_libgit2_init();
    }

    ~GitGuard() {
        git_libgit2_shutdown();
    }
};

#endif // SRC_LIB_GIT_GUARD_HPP_
