# Development

## tools

- [clang-tidy](https://clang.llvm.org/) static analysis
- [clang](https://clang.llvm.org/) compiler
- [cmake](https://cmake.org) builder
- [complgen](https://github.com/adaszko/complgen) shell completion
- [cpplint](https://github.com/cpplint/cpplint) linter
- [docker](https://docs.docker.com/engine/install/) make dockerize
- [entr](https://github.com/eradman/entr) watching files
- [gcc](https://gcc.gnu.org/git/?p=gcc.git) compiler
- [gdb](git://sourceware.org/git/binutils-gdb.git) debugger
- [lldb](https://lldb.llvm.org) debugger
- [make](https://www.gnu.org/software/make) builder & generator
- [ninja](https://github.com/ninja-build/ninja) generator
- [vcpkg](https://github.com/microsoft/vcpkg) $VCPKG_ROOT must be defined

`Dockerfile` can be used a reference for steps to build the project

## development setup

```sh
make clean build lint test
# live reload
make watch
```

To get help on tasks `make help`

## compile options

```sh
make build COMPILER=gcc GENERATOR=Ninja
make build COMPILER=clang GENERATOR="Unix Makefiles"
# etc ...
```

## test options

- all tests: `make test`
- unit tests: `make TESTTYPE=unit test`
- integration tests: `make TESTTYPE=integration test`

## static analysis

- all files:    `make scan`
- single file:  `make scan SCANMATCH=src/main.cpp`
- pattern:      `make scan SCANMATCH=src/**/*.cpp`

## debugging

- Using gdb: `gdb build/mrm/mrm`
- Using lldb: `lldb build/mrm/mrm`

## libraries

All libraries fetched using vcpkg.

- [boost](https://github.com/boostorg/boost)
- [cli11](https://github.com/CLIUtils/CLI11)
- [ftxui](https://github.com/ArthurSonzogni/FTXUI)
- [googletest](https://github.com/google/googletest)
- [libgit2](https://github.com/libgit2/libgit2)
- [tbb](https://github.com/oneapi-src/oneTBB)
- [yaml-cpp](https://github.com/jbeder/yaml-cpp)
