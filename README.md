# mrm

MRM (multi-repo-manager) is a tool to help in managing a large number of
repositories in a sane way utilizing bulk transactions and yaml configurations

## requirements

direct on host:

- [cmake](https://cmake.org)
- [make](https://www.gnu.org/software/make)
- [vcpkg](https://github.com/microsoft/vcpkg)
- [python](https://www.python.org)

through vcpkg:

- [yaml-cpp](https://github.com/jbeder/yaml-cpp) `vcpkg install yaml-cpp`
- [cli11](https://github.com/CLIUtils/CLI11) `vcpkg install cli11`

candidates (not yet added):

- [boost](https://github.com/boostorg/boost)
- [fmt](https://github.com/fmtlib/fmt)
- [googletest](https://github.com/google/googletest)
- [spdlog](https://github.com/gabime/spdlog)

## build

```sh
./build.py clean
./build.py build
```
