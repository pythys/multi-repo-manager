# Install

## one-line installer (macos, linux)

```sh
curl -fsSL https://github.com/pythys/multi-repo-manager/raw/master/scripts/install.sh | sh -s -- --version 0.1.0
```

The installer prints a copy-pastable command to add the install directory to your PATH.

## source install

Minimum requirements:

note: both clang and gcc are needed for mrm and conan respectively.

- [clang](https://clang.llvm.org/)
- [cmake](https://cmake.org)
- [conan](https://conan.io)
- [gcc](https://gcc.gnu.org/)
- [make](https://www.gnu.org/software/make)
- [ninja](https://github.com/ninja-build/ninja)

Build and install:

```sh
make build
sudo make install
```

## docker install

`make dockerize`

Once completed an image called "mrm" will be available on the machine. To use:

`docker run -it --rm -v $PWD:/repos mrm --help`

To alias the docker command

`alias mrm='docker run -it --rm --user $(id -u):$(id -g) -v $PWD:/repos mrm'`

Example usage:

`mrm find .`

To rebuild the Docker image without cache:

`docker build --no-cache --tag mrm .`

## shell completion

Identify your shell and configure:

zsh:
```sh
echo 'source <(mrm completion zsh)' >> ~/.zshrc
```

bash:
```sh
echo 'source <(mrm completion bash)' >> ~/.bashrc
```

Restart your shell after applying the above for the changes to take effect.

## next

- [Quick Start](guides/quickstart.md)
- [Fork Sync Guide](guides/remotesync-forks.md)
- [Nested Repos Guide](guides/nested-repos.md)
- [Best Practices](guides/best-practices.md)
- [YAML Schema](yaml-schema.md)
