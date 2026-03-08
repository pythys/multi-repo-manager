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
6. Run `make scan` last (slow, up to 3 minutes)

If any step fails, repeat the full cycle.

### Core Targets

- `make clean` — remove generated artifacts (`.cache`, `build`,
  `compile_commands.json`)
- `make build` — configure/build with CMake and generate `compile_commands.json`
- `make test` — run all tests (depends on `build`)
- `make lint` — checks for issues with `clang-format` and `cmake-format`
- `make lint-fix` — auto-format C++ and CMake files
- `make scan` — run `clang-tidy` (requires `build`)
- `make up-vcpkg` - update vcpkg baseline in vcpkg.json
- `make watch` — continuously run `clean test lint` when files change
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
- Do not run `make clean build` as it leads to a race condition in directory
  deletion and creation. Run the clean target in isolation first then combine.
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
  behavior or interfaces change (docs, usage text, and relevant comments).
- Keep command changes (names and flags) in sync with mrm.usage.
- For CLI/API naming, prefer user-facing terms (for example `--jobs`) over
  implementation-specific names (for example `--pool-size`).
- If `clang-tidy` is available, prefer running `make scan`, but do it last
  after compile, test, and formatting issues are resolved. Scan takes a long
  time and warnings that are not suppressed should be addressed. Do not timeout
  on `make scan` for at least 3 minutes.
- Prefer terse, efficient code over verbose implementations.
- Prefer modern C++ constructs where they improve clarity and correctness.
- Prefer functional style (free functions, algorithms, lambdas) when practical.
- Avoid unnecessary OO layering; use polymorphism and class hierarchies only
  when they add clear value.
