# mrm

**mrm (multi-repo-manager)** helps you manage many git repositories as one using
a YAML repository list.

Define your repositories once in a YAML file, then run a single command across
all of them to turn hours of repetitive maintenance into minutes.

Track one YAML file across multiple machines to ensure your repositories,
remotes, and branches stay in sync, so you can switch devices or collaborate
without setting up everything from scratch.

mrm is fast by default with parallel execution, and it ships with a beautiful
interactive FTXUI interface backed by a native C++ runtime.

![mrm screenshot](assets/screenshot.png)

Quick install:

```sh
curl -fsSL https://git.pythys.com/taher/multi-repo-manager/raw/branch/master/scripts/install.sh | sh -s -- --version 0.1.0
```

Example Command:

```sh
mrm update --config myrepos.yml --jobs 15
```

- 📦 [Installation](install.md)
- 🏁 [Quick Start](guides/quickstart.md)
- 🚀 [Usage](usage.md)
- 📚 [Guides](guides/README.md)
- ⚙️ [Development](development.md)
- 🧾 [YAML Schema](yaml-schema.md)
- 🗺️ [Roadmap](roadmap.md)

### Author

Taher Alkhateeb
https://github.com/pythys

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
