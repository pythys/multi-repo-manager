# mrm

MRM (multi-repo-manager) is a tool to help in managing a large number of
repositories in a sane way utilizing bulk transactions and yaml configurations

## requirements

The following software is required for building and developing the project:

- [autoconf-archive](https://www.gnu.org/software/autoconf-archive/)
- [autoconf](https://www.gnu.org/software/autoconf/)
- [clang](https://clang.llvm.org/) clangd & clang-tidy
- [cmake](https://cmake.org)
- [cpplint](https://github.com/cpplint/cpplint)
- [entr](https://github.com/eradman/entr) live reload
- [gcc](https://gcc.gnu.org/git/?p=gcc.git)
- [gdb](git://sourceware.org/git/binutils-gdb.git)
- [make](https://www.gnu.org/software/make)
- [ninja](https://github.com/ninja-build/ninja) generator
- [vcpkg](https://github.com/microsoft/vcpkg) $VCPKG_ROOT must be defined

`Dockerfile` can be used a reference for steps to build the project

## development setup

```sh
make clean
make build
make lint
make test
# live reload
make watch
```

To get help on tasks `make help`

## debugging

To start a debug session with gdb run the below command

`gdb build/mrm/mrm`

## libraries

through vcpkg:

- [boost](https://github.com/boostorg/boost)
- [cli11](https://github.com/CLIUtils/CLI11)
- [ftxui](https://github.com/ArthurSonzogni/FTXUI)
- [libgit2](https://github.com/libgit2/libgit2)
- [tbb](https://github.com/oneapi-src/oneTBB)
- [yaml-cpp](https://github.com/jbeder/yaml-cpp)

through cmake:

- [googletest](https://github.com/google/googletest)
