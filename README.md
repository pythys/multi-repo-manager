# mrm

MRM (multi-repo-manager) is a tool to help in managing a large number of
repositories in a sane way utilizing bulk operations and yaml configurations

## installation

After getting all requirements issue the commands:

`make build`
`sudo make install`

## usage

**TODO** document this section thoroughly

## requirements

- [clang](https://clang.llvm.org/)
- [cmake](https://cmake.org)
- [cpplint](https://github.com/cpplint/cpplint)
- [docker](https://docs.docker.com/engine/install/)
- [entr](https://github.com/eradman/entr) live reload
- [gcc](https://gcc.gnu.org/git/?p=gcc.git)
- [gdb](git://sourceware.org/git/binutils-gdb.git)
- [make](https://www.gnu.org/software/make)
- [ninja](https://github.com/ninja-build/ninja)
- [vcpkg](https://github.com/microsoft/vcpkg) $VCPKG_ROOT must be defined

`Dockerfile` can be used a reference for steps to build the project

## development setup

```sh
make clean build lint scan test
# live reload
make watch
```

To get help on tasks `make help`

## debugging

To start a debug session with gdb run the below command

`gdb build/mrm/mrm`

## libraries

Through vcpkg:

- [boost](https://github.com/boostorg/boost)
- [cli11](https://github.com/CLIUtils/CLI11)
- [ftxui](https://github.com/ArthurSonzogni/FTXUI)
- [libgit2](https://github.com/libgit2/libgit2)
- [tbb](https://github.com/oneapi-src/oneTBB)
- [yaml-cpp](https://github.com/jbeder/yaml-cpp)

Through cmake:

- [googletest](https://github.com/google/googletest)

## Roadmap

- Implement SyncScreen to show progress of syncing
- Find recurse only in .gitmodule and .gitignore (performance improvement)
- Introduce usage documentation in this file
- Introduce a documentation tool like Doxygen and expand docs
- Dynamically link dependencies to allow packaging to deb, rpm, etc ...
