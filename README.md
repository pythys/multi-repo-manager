# mrm

mrm (multi-repo-manager) is a tool to help in managing a large number of
repositories in a sane way utilizing bulk operations and yaml configurations

## source install

Minimum Requirements:

- [clang](https://clang.llvm.org/)
- [cmake](https://cmake.org)
- [make](https://www.gnu.org/software/make)
- [ninja](https://github.com/ninja-build/ninja)
- [vcpkg](https://github.com/microsoft/vcpkg)

Build and install

`make build`
`sudo make install`

## docker install

Warning: dockerizing the project might take a long time, extending up to an hour
depending on the following factors:

- hardware capabilities
- operating system
- docker settings (virtual machine options)
- bandwidth

`make dockerize`

Once completed an image called "mrm" will be available on the machine. To use:

`docker run --platform linux/amd64 -it --rm -v $PWD:/opt/repos mrm --help`

To alias the above command

`alias mrm='docker run --platform linux/amd64 -it --rm -v $PWD:/opt/repos mrm'`

To build docker partially (skip base rebuild):

`docker build --target builder -t mrm .`

### troubleshooting

On macos you might get a docker build crash due to [illegal
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
git clone https://github.com/siduck/st
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

## roadmap

- Graceful handling of errors per repo (continue)
- Switch most commands to be parallel.
- Implement callback functions in git_manager to report on progress.
- Implement all progress reporting through messages sent to observer.
- Implement the remotesync command.
- Implement the exec command.
- Implement dynamic FTXUI interface for all commands.
- Implement static interface when !is_terminal for all commands.
- Limit recursion to .gitmodule and .gitignore for improved performance.
- Introduce a documentation tool like Doxygen.
- Introduce dynamic and static packaging of software using cpack.
- Finalize [complgen](https://github.com/adaszko/complgen) (mrm.usage).
- Resolve clang-tidy scan issues
