# Development

## tools

- [clang-format](https://clang.llvm.org/docs/ClangFormat.html) linter
- [clang-tidy](https://clang.llvm.org/extra/clang-tidy/) static analyzer
- [clang](https://clang.llvm.org/) compiler
- [cmake](https://cmake.org) build generator
- [conan](https://conan.io) package manager
- [docker](https://docs.docker.com/engine/install/) containerization
- [entr](https://github.com/eradman/entr) file watcher
- [gcc](https://gcc.gnu.org/git/?p=gcc.git) compiler
- [gdb](git://sourceware.org/git/binutils-gdb.git) debugger (gcc)
- [lldb](https://lldb.llvm.org) debugger (clang)
- [make](https://www.gnu.org/software/make) orchestrator and builder
- [zensical](https://zensical.org/) documentation generator
- [ninja](https://github.com/ninja-build/ninja) builder

`Dockerfile` can be used a reference for steps to build the project

## installing tools

Most tools are available through standard package managers (`apt`, `brew`, `dnf`,
`pacman`). Some tools may not be packaged or require specific installation:

```sh
# conan
mise use -g conan@latest

# zensical
pipx install zensical
```

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

## compile options

```sh
make COMPILER=gcc GENERATOR=Ninja build
make COMPILER=clang GENERATOR="Unix Makefiles" build
# etc ...
```

## test options

- all tests: `make test`
- unit tests: `make TESTTYPE=unit test`
- integration tests: `make TESTTYPE=integration test`

## static analysis

- all files:    `make scan`
- single file:  `make SCANMATCH=src/main.cpp scan`
- pattern:      `make SCANMATCH=src/**/*.cpp scan`

## debugging

- Using gdb: `gdb build/mrm/mrm`
- Using lldb: `lldb build/mrm/mrm`

## versioning

Single source of truth is the `VERSION` file. To propagate the version to all
other files:

```sh
make version
```

To update the version and propagate it:

```sh
make VERSION=1.2.3 version
```

This updates all versioned files including:

- `VERSION`
- `CMakeLists.txt`
- `mkdocs.yml`
- `conanfile.py`
- `docs/install.md`
- `README.md`

## libraries

All libraries fetched using Conan.

- [boost](https://github.com/boostorg/boost)
- [cli11](https://github.com/CLIUtils/CLI11)
- [ftxui](https://github.com/ArthurSonzogni/FTXUI)
- [googletest](https://github.com/google/googletest)
- [libgit2](https://github.com/libgit2/libgit2)
- [yaml-cpp](https://github.com/jbeder/yaml-cpp)
