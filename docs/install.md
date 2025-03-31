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
