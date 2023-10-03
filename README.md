# mrm

MRM (multi-repo-manager) is a tool to help in managing a large number of
repositories in a sane way utilizing bulk transactions and yaml configurations

## requirements

### install on host

- [cmake](https://cmake.org)
- [entr](https://github.com/eradman/entr)
- [make](https://www.gnu.org/software/make)
- [python](https://www.python.org)
- [vcpkg](https://github.com/microsoft/vcpkg)

### install on vcpkg

- [cli11](https://github.com/CLIUtils/CLI11) `vcpkg install cli11`
- [libgit2](https://github.com/libgit2/libgit2) `vcpkg install libgit2`
- [yaml-cpp](https://github.com/jbeder/yaml-cpp) `vcpkg install yaml-cpp`

### automatically installed

- [googletest](https://github.com/google/googletest)

### candidates (not installed)

- [boost](https://github.com/boostorg/boost)
  general purpose library completing std
- [fmt](https://github.com/fmtlib/fmt)
  string formatting library
- [spdlog](https://github.com/gabime/spdlog)
  logging library

## build

```sh
./build.py clean
./build.py build
```
