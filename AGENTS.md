# AGENTS

This repository is **mrm (multi-repo-manager)**, a C++ project built with CMake
and orchestrated by a Makefile. The Makefile is the primary entry point for
build, test, lint, and tooling tasks.

## Makefile Orchestration

The Makefile is the single source of truth for day‑to‑day development tasks. Use
`make help` for a current list of targets.

## Development Flow

For most code changes follow this sequence:

1. `make clean`
2. `make build`
3. `make lint`
4. `make test`
5. Fix any failures
6. Run `make scan` last, it is slow.

- If any step fails, repeat the full cycle.
- If the environment does not have clang-tidy installed, then skip `make scan`
- `make scan` reports a massive warning count in external code. Ignore it and
  only react to reported warnings that affect project code.
- `make scan` take a long time. Do not time out before at least 5 minutes.

### Core Targets

- `make clean` — remove generated artifacts (`.cache`, `build`,
  `compile_commands.json`)
- `make deps` — install dependencies with Conan (auto-creates profile if needed)
- `make build` — configure/build with CMake and generate `compile_commands.json`
  - Depends on `make deps`
  - Auto-generates/updates `conanfile.lock` if missing or stale
  - Lock file is used for reproducible builds across all environments
- `make test` — run all tests (depends on `build`)
- `make lint` — checks for issues with `clang-format` and `cmake-format`
- `make lint-fix` — auto-format C++ and CMake files
- `make scan` — run `clang-tidy` (requires `build`)
- `make watch` — continuously run `clean test lint` when files change
- `make install` / `make uninstall` — copy/remove `mrm` to/from `/usr/local/bin`
- `make docs` — generate Doxygen docs
- `make completion` — generate shell completions into `build/completions`
- `make package` — build packages via CPack
- `make dockerize` — build Docker image `mrm`

### Makefile Options

These environment variables control orchestration:

- `BUILDTYPE`: `Debug` or `Release` (default)
- `COMPILER`: `clang` (default) or `gcc`
- `GENERATOR`: `Ninja` (default) or `Unix Makefiles`
- `TESTTYPE`: `unit`, `integration`, or empty for all tests
- `SCANMATCH`: glob pattern for `clang-tidy` (default: `src/**/*.cpp
  src/**/*.hpp`)

Example:

```sh
make COMPILER=gcc GENERATOR=Ninja build
make BUILDTYPE=Debug build
make TESTTYPE=unit test
make SCANMATCH=src/main.cpp scan
```

## Project Layout

- `build/` — CMake build output (generated)
- `docs/` — documentation
- `scripts/` - helper scripts for development and CI
- `src/` — application/library source code
- `tests/` — tests (driven by CTest)
- `CMakeLists.txt` — CMake entry point
- `Dockerfile`, `Doxyfile` — container and docs tooling
- `Makefile` — task orchestration (primary)

## Tooling and Dependencies

Primary tools used via Makefile:

- Build: `cmake`, `ninja` (default generator), `clang` or `gcc`
- Format: `clang-format`, `cmake-format`
- Analysis: `clang-tidy`
- Tests: `ctest`
- Docs: `doxygen`
- Other: `entr`, `docker`, `cpack`

Libraries are managed via `conan`. See `docs/development.md` for details.

## Notes for Agents

- The user might modify your code in between runs. Check user changes before
  applying patches.
- Prefer using Makefile targets over calling tools directly.
- Do not run `make clean build` as it leads to a race condition in directory
  deletion and creation. Run the clean target in isolation first then combine.
- `make build` depends on `make deps` which installs Conan dependencies. The
  deps target auto-creates a Conan profile on first run if none exists.
- `make build` creates `compile_commands.json` and symlinks it into build
  subdirectories.
- `make install` and `make uninstall` write to `/usr/local/bin` (may require
  elevated privileges).
- Multiple repository types are supported in the design like git, subversion
  etc, so always keep the abstraction and don't break it with improper names
  or designs that assume a git-only implementation.
- Write tests for features that require it, without abiding to 100% coverage.
- Keep changes consistent with CMake and Makefile conventions already in place.
- Keep code changes and documentation in sync in the same change whenever
  behavior or interfaces change. Documentation is located in docs/.
- For CLI/API naming, prefer user-facing terms (for example `--jobs`) over
  implementation-specific names (for example `--pool-size`).
- Prefer terse, efficient code over verbose implementations.
- Prefer modern C++ constructs where they improve clarity and correctness.
- Prefer functional style (free functions, algorithms, lambdas) when practical.
- Avoid useless comments in code that describe something which should be
  understood by reading the code. Limit comments to actual useful information
  that cannot be understood clearly or easily from the code.
- Avoid unnecessary OO layering; use polymorphism and class hierarchies only
  when they add clear value.
- The source code for libraries used in this project can be scanned to use them
  properly. The libraries can be found in Conan's cache at ~/.conan2/p/.
- Prefer portable code that works across most linux distros, macos and windows.

## Upgrade Testing

The project has library and tool dependencies that impact behavior. Testing
should be done against the following:

- Newer versions of libraries in conanfile.py.
- Newer versions of tools:
  - clang
  - clang-format
  - clang-tidy
  - cmake
  - cmake-format
  - conan
  - docker
  - doxygen
  - gcc
  - make
  - ninja
- Newer versions of above tools in Dockerfile.
