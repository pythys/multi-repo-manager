# mrm

MRM (multi-repo-manager) is a tool to help in managing a large number of
repositories in a sane way utilizing bulk operations and yaml configurations

## installation

Install minimum requirements:

- `cmake`
- `make`
- `vcpkg`

Build and install

`make build`
`sudo make install`

## usage

Example Usage:

- create a config file

```
mkdir myrepos && cd myrepos
git clone https://github.com/bootandy/dust
git clone https://github.com/sharkdp/fd
git clone https://github.com/junegunn/fzf
cd ..
mrm find myrepos > myrepos.yml
```

- sync repos from config file

`mrm sync --config myrepos.yml myrepos`

- get help

`mrm --help`
`mrm sync --help`
`mrm find --help`

### SSH Credentials

If you have authenticated repos, then SSH installation is required along with
adding the SSH keys as sampled in below command:

`ssh-add ~/.ssh/id_rsa`

## Development Requirements

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
- Introduce a documentation tool like Doxygen and expand docs
- Dynamically link dependencies to allow packaging to deb, rpm, etc ...
