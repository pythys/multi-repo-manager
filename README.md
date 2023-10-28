# mrm

MRM (multi-repo-manager) is a tool to help in managing a large number of
repositories in a sane way utilizing bulk transactions and yaml configurations

## requirements

### installed on host

- [autoconf](https://www.gnu.org/software/autoconf/)
  needed in various dependencies
- [autoconf-archive](https://www.gnu.org/software/autoconf-archive/)
  needed in installing boost with vcpkg
- [cmake](https://cmake.org)
  main build system for the project
- [ctags](https://github.com/universal-ctags/ctags)
  generating tags for development with emacs
- [entr](https://github.com/eradman/entr)
  watching files and recompiling
- [make](https://www.gnu.org/software/make)
  currently default generator of cmake
- [python](https://www.python.org)
  to run the build.py build script
- [vcpkg](https://github.com/microsoft/vcpkg)
  installs other dependencies

### installed via vcpkg

- [boost](https://github.com/boostorg/boost) `vcpkg install boost`
- [cli11](https://github.com/CLIUtils/CLI11) `vcpkg install cli11`
- [libgit2](https://github.com/libgit2/libgit2) `vcpkg install libgit2`
- [tbb](https://github.com/oneapi-src/oneTBB) `vcpkg install tbb`
- [yaml-cpp](https://github.com/jbeder/yaml-cpp) `vcpkg install yaml-cpp`

### automatically installed

- [googletest](https://github.com/google/googletest)

### candidates (not installed)

- [fmt](https://github.com/fmtlib/fmt)
  string formatting library
- [PDCurses](https://github.com/wmcbrine/PDCurses)
  multi-platform curses library
- [spdlog](https://github.com/gabime/spdlog)
  logging library
- [thread-pool](https://github.com/bshoshany/thread-pool)
  thread pooling library

## build

```sh
./build.py clean
./build.py build
```

To get help on available commands:

``` sh
./build.py --help
```
