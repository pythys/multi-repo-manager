# AGENTS

This repository is **mrm (multi-repo-manager)**, a C++ project built with CMake
and orchestrated by a Makefile.

The Makefile is the source of truth for day‑to‑day development tasks. Use
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
- `make scan` take a long time. Do not time out before at least 10 minutes.

## Notes for Agents

- Do not run `make clean build` as it leads to a race condition in directory
  deletion and creation. Run the clean target in isolation first.
- Multiple repository types are supported in the design like git, subversion
  etc, so always keep the abstraction and don't break it with improper names
  or designs that assume a git-only implementation.
- Write tests for features that require it, without abiding to 100% coverage.
- Keep code changes and documentation in sync in the same change whenever
  behavior or interfaces change. Documentation is located in docs/.
- Prefer terse, efficient code over verbose implementations.
- Prefer modern C++ constructs where they improve clarity and correctness.
- Prefer functional style (free functions, algorithms, lambdas) when practical.
- Avoid useless comments in code that describe something which should be
  understood by reading the code.
- The source code for libraries used in this project can be scanned to use them
  properly. The libraries can be found in Conan's cache at ~/.conan2/p/.
- Prefer portable code that works across most linux distros, macos and windows.
