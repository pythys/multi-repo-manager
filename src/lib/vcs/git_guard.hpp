#ifndef SRC_LIB_VCS_GIT_GUARD_HPP_
#define SRC_LIB_VCS_GIT_GUARD_HPP_

#include <git2.h>

class GitGuard {
  public:
    GitGuard() {
        git_libgit2_init();
    }

    ~GitGuard() {
        git_libgit2_shutdown();
    }
};

#endif // SRC_LIB_VCS_GIT_GUARD_HPP_
