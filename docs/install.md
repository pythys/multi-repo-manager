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

`docker build --no-cache --tag mrm .`
