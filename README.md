# mrm

MRM (multi-repo-manager) is a tool to help in managing a large number of
repositories in a sane way utilizing bulk transactions and yaml configurations

## requirements

direct on host:

- [cmake](https://cmake.org)
- [make](https://www.gnu.org/software/make)
- [vcpkg](https://github.com/microsoft/vcpkg)
- [python](https://www.python.org)
- [entr](https://github.com/eradman/entr)

through vcpkg:

- [cli11](https://github.com/CLIUtils/CLI11) `vcpkg install cli11`
- [yaml-cpp](https://github.com/jbeder/yaml-cpp) `vcpkg install yaml-cpp`

candidates (not yet added):

- [boost](https://github.com/boostorg/boost)
- [fmt](https://github.com/fmtlib/fmt)
- [googletest](https://github.com/google/googletest)
- [libgit2cpp](https://github.com/AndreyG/libgit2cpp)
- [spdlog](https://github.com/gabime/spdlog)

## build

```sh
./build.py clean
./build.py build
```
