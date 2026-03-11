# Install

## source install

Minimum requirements:

- [clang](https://clang.llvm.org/)
- [cmake](https://cmake.org)
- [make](https://www.gnu.org/software/make)
- [ninja](https://github.com/ninja-build/ninja)

Optional:

- [vcpkg](https://github.com/microsoft/vcpkg) (only if you want vcpkg-managed dependencies)

Build and install:

```sh
make build
sudo make install
```

## docker install

`make dockerize`

Once completed an image called "mrm" will be available on the machine. To use:

`docker run -it --rm -v $PWD:/tree mrm --help`

To alias the docker command

`alias mrm='docker run -it --rm --user $(id -u):$(id -g) -v $PWD:/tree mrm'`

Example usage:

`mrm find /tree`

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
