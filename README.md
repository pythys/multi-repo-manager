# mrm

MRM (multi-repo-manager) is a tool to help in managing a large number of
repositories in a sane way utilizing bulk operations and yaml configurations

## source install

Install minimum requirements:

- `cmake`
- `make`
- `vcpkg`

Build and install

`make build`
`sudo make install`

## docker install

Warning: dockerizing the project might take a long time depending on hardware,
OS and docker settings.

`make dockerize`

Once completed an image called "mrm" will be available on the machine. To use:

`docker run --platform linux/amd64 -it --rm -v $PWD:/opt/repos mrm --help`

To alias the above command

`alias mrm='docker run --platform linux/amd64 -it --rm -v $PWD:/opt/repos mrm'`

To build docker partially (skip base rebuild):

`docker build --target builder -t mrm .`

### troubleshooting

On macos you might get a crash due to [illegal
instruction](https://github.com/docker/for-mac/issues/7255) in ca-certificates.
To solve this problem:

Docker Desktop -> Settings -> General -> Virtual Machine Options ->
  - Uncheck "Use Rosetta for x86_64/amd64 emulation on Apple Silicon"
  - Optionally, Select Docker VMM as Virtual Machine Manager

## usage

Example Usage:

- create a config file

```
mkdir myrepos && cd myrepos
git clone https://github.com/bootandy/dust
git clone https://github.com/sharkdp/fd
git clone https://github.com/junegunn/fzf
cd ..
mrm find myrepos --save myrepos.yml
```

- sync repos from config file

`mrm sync --config myrepos.yml myrepos`

- get help

`mrm --help` or `mrm -h`
`mrm <command> --help` e.g.:
  - `mrm sync --help`
  - `mrm find -h`

### SSH Credentials

If you have authenticated repos, then SSH installation is required along with
adding the SSH keys as sampled in below command:

`ssh-add ~/.ssh/id_rsa`

## Development Requirements

- [clang-tidy](https://clang.llvm.org/)
- [clang](https://clang.llvm.org/)
- [cmake](https://cmake.org)
- [complgen](https://github.com/adaszko/complgen)
- [cpplint](https://github.com/cpplint/cpplint)
- [docker](https://docs.docker.com/engine/install/)
- [entr](https://github.com/eradman/entr) live reload
- [gcc](https://gcc.gnu.org/git/?p=gcc.git)
- [gdb](git://sourceware.org/git/binutils-gdb.git)
- [lldb](https://lldb.llvm.org)
- [make](https://www.gnu.org/software/make)
- [ninja](https://github.com/ninja-build/ninja)
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

## static analysis

- all files:    `make scan`
- single file:  `make scan SCANMATCH=src/main.cpp`
- pattern:      `make scan SCANMATCH=src/**/*.cpp`

## debugging

- Using gdb: `gdb build/mrm/mrm`
- Using lldb: `lldb build/mrm/mrm`

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

- Unify all commands (except find) under a generic interface.
- Graceful handling of errors per repo (continue)
- Switch all commands to be parallel.
- Implement callback functions in git_manager to report on progress.
- Implement all progress reporting through messages sent to observer.
- Implement the exec command.
- Implement dynamic FTXUI interface for all commands.
- Implement static interface when !is_terminal.
- Limit recursion to .gitmodule and .gitignore for improved performance.
- Introduce a documentation tool like Doxygen.
- Dynamically link dependencies for packaging.
- Provide a solution for shell completion. A
  [candidate](https://github.com/adaszko/complgen)
- Resolve all clang-tidy issues
