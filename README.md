# mrm

MRM (multi-repo-manager) is a tool to help in managing a large number of
repositories in a sane way utilizing bulk transactions and yaml configurations

## requirements

The following software is required for building and developing the project:

- [autoconf](https://www.gnu.org/software/autoconf/)
  needed in various dependencies
- [autoconf-archive](https://www.gnu.org/software/autoconf-archive/)
  needed in installing boost with vcpkg
- [clang](https://clang.llvm.org/)
  needed for clangd and clang-tidy
- [cmake](https://cmake.org)
  main build system for the project
- [cpplint](https://github.com/cpplint/cpplint)
  linting tool for C++
- [entr](https://github.com/eradman/entr)
  watching files and recompiling
- [gdb](git://sourceware.org/git/binutils-gdb.git)
  debugging the system
- [make](https://www.gnu.org/software/make)
  running make commands and as cmake generator
- [ninja](https://github.com/ninja-build/ninja)
  alternative generator to make
- [vcpkg](https://github.com/microsoft/vcpkg)
  installs other dependencies

## development setup

```sh
make clean
make build
make test
make lint
# to repeat above cycle
make watch
```

To get help on tasks `make help`

## debugging

To start a debug session with gdb run the below command

`gdb build/mrm/mrm`

## libraries

installed automatically with vcpkg:

- [boost](https://github.com/boostorg/boost)
- [cli11](https://github.com/CLIUtils/CLI11)
- [ftxui](https://github.com/ArthurSonzogni/FTXUI)
- [libgit2](https://github.com/libgit2/libgit2)
- [tbb](https://github.com/oneapi-src/oneTBB)
- [yaml-cpp](https://github.com/jbeder/yaml-cpp)

installed automatically with cmake:

- [googletest](https://github.com/google/googletest)

## potential libraries

- [fmt](https://github.com/fmtlib/fmt)
  string formatting library
- [spdlog](https://github.com/gabime/spdlog)
  logging library
