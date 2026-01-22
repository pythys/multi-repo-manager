# Install

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

`make dockerize`

Once completed an image called "mrm" will be available on the machine. To use:

`docker run -it --rm -v $PWD:/tree mrm --help`

To alias the docker command

`alias mrm='docker run -it --rm --user $(id -u):$(id -g) -v $PWD:/tree'`

Example usage:

`mrm find /tree`

To build docker partially (skip base rebuild):

`docker build --platform=linux/amd64 --no-cache --tag mrm .`

### troubleshooting

On macos you might get a docker build crash due to [illegal
instruction](https://github.com/docker/for-mac/issues/7255) in ca-certificates.
To solve this problem:

Docker Desktop -> Settings -> General -> Virtual Machine Options ->
  - Uncheck "Use Rosetta for x86_64/amd64 emulation on Apple Silicon"
  - Optionally, Select Docker VMM as Virtual Machine Manager
