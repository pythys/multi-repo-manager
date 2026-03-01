# AGENTS

This repository is **mrm (multi-repo-manager)**, a C++ project built with CMake
and orchestrated by a Makefile. The Makefile is the primary entry point for
build, test, lint, and tooling tasks.

## Quick Start

```sh
make help
make clean build
make lint
make test
```

## Makefile Orchestration

The Makefile is the single source of truth for day‑to‑day development tasks. Use
`make help` for a current list of targets.

### Core Targets

- `make clean` — remove generated artifacts (`.cache`, `build`,
  `compile_commands.json`)
- `make build` — configure/build with CMake and generate `compile_commands.json`
- `make test` — run all tests (depends on `build`)
- `make lint` — run `clang-format` checks (no changes)
- `make lint-fix` — auto-format C++ and CMake files
- `make scan` — run `clang-tidy` (requires `build`)
- `make watch` — watch files and run `clean test lint`
- `make docs` — generate Doxygen docs
- `make completion` — generate shell completions into `build/completions`
- `make package` — build packages via CPack
- `make dockerize` — build Docker image `mrm`
- `make install` / `make uninstall` — copy/remove `mrm` to/from `/usr/local/bin`

### Makefile Options

These environment variables control orchestration:

- `COMPILER`: `clang` (default) or `gcc`
- `GENERATOR`: `Ninja` (default) or `Unix Makefiles`
- `TESTTYPE`: `unit`, `integration`, or empty for all tests
- `SCANMATCH`: glob pattern for `clang-tidy` (default: `src/**/*.cpp
  src/**/*.hpp`)
- `VCPKG_ROOT`: path to vcpkg; empty disables vcpkg toolchain

Example:

```sh
make build COMPILER=gcc GENERATOR=Ninja
make TESTTYPE=unit test
make scan SCANMATCH=src/main.cpp
```

## Project Layout

- `src/` — application/library source code
- `tests/` — tests (driven by CTest)
- `docs/` — documentation (see `docs/development.md`)
- `build/` — CMake build output (generated)
- `CMakeLists.txt` — CMake entry point
- `Makefile` — task orchestration (primary)
- `Dockerfile`, `Doxyfile` — container and docs tooling

## Tooling and Dependencies

Primary tools used via Makefile:

- Build: `cmake`, `ninja` (default generator), `clang` or `gcc`
- Format: `clang-format`, `cmake-format`
- Analysis: `clang-tidy`
- Tests: `ctest`
- Docs: `doxygen`
- Other: `complgen`, `entr`, `docker`, `cpack`

Libraries are managed via `vcpkg` (optional). See `docs/development.md` for
details.

## Notes for Agents

- Prefer using Makefile targets over calling tools directly.
- `make build` creates `compile_commands.json` and symlinks it into build
  subdirectories.
- `make install` and `make uninstall` write to `/usr/local/bin` (may require
  elevated privileges).
- Write tests for features that require it, without abiding to 100% coverage.
- Keep changes consistent with CMake and Makefile conventions already in place.
- Keep code changes and documentation in sync in the same change whenever
  behavior or interfaces change (docs, usage text, and relevant comments).
- Keep command changes (names and flags) in sync with mrm.usage.
- For CLI/API naming, prefer user-facing terms (for example `--jobs`) over
  implementation-specific names (for example `--pool-size`).
- If `clang-tidy` is available, prefer running `make scan`, but do it last
  after compile, test, and formatting issues are resolved.
- Prefer terse, efficient code over verbose implementations.
- Prefer modern C++ constructs where they improve clarity and correctness.
- Prefer functional style (free functions, algorithms, lambdas) when practical.
- Avoid unnecessary OO layering; use polymorphism and class hierarchies only
  when they add clear value.
- If any error occurs in make targets lint, test or scan, then repeat all of
  them, to avoid changes in a later target triggering more issues in a previous
  one. A clear full round of clean lint, test and scan is needed to accept work.
