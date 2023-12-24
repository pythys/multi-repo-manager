# mrm

MRM (multi-repo-manager) is a tool to help in managing a large number of
repositories in a sane way utilizing bulk transactions and yaml configurations

## requirements

### installed on host

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

to install all vcpkg packages in one shot (defined in vcpkg.json):

`vcpkg install --feature-flags=manifests`

### automatically installed

- [googletest](https://github.com/google/googletest)

### candidates (not installed)

- [fmt](https://github.com/fmtlib/fmt)
  string formatting library
- [spdlog](https://github.com/gabime/spdlog)
  logging library
- [notcurses](https://github.com/dankamongmen/notcurses)
  TUI library
- [FTXUI](https://github.com/ArthurSonzogni/FTXUI)
  TUI library

## development setup

```sh
./build.py clean
./build.py build
./build.py test
./build.py lint
./build.py watch
```

To get help on available commands:

``` sh
./build.py --help
```

## debugging

To start a debug session with gdb run the below command

`gdb build/src/mrm`
