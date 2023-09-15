# multi-repo-manager

## requirements

on host:

- [cmake](https://cmake.org)
- [make](https://www.gnu.org/software/make)
- [vcpkg](https://github.com/microsoft/vcpkg)

through vcpkg:

- [boost](https://github.com/boostorg/boost)
- [cli11](https://github.com/CLIUtils/CLI11) `vcpkg install cli11`
- [fmt](https://github.com/fmtlib/fmt)
- [googletest](https://github.com/google/googletest)
- [spdlog](https://github.com/gabime/spdlog)

## build

```sh
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

