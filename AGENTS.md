# AGENTS

**mrm (multi-repo-manager)** is a C++ project built with CMake and orchestrated
by Make. List targets with `make help`

## Development Flow

Run in order:

1. `make clean`
2. `make build`
3. `make lint`
4. `make test`
5. Fix failures
6. `make scan` (slow ~ 10 minutes)
7. Repeat until complete

- Do not run `make clean build` as it leads to race conditions.

## Notes for Agents

- Maintain multi repository type abstraction (git, subversion, etc).
- Write tests for critical functionality without breaking public signatures.
- Keep /docs in sync with code.
- Prefer terse, functional and portable (linux, macos, windows) code.
- Avoid redundant comments that are better described with code.
- Library source code used in this project can be found at ~/.conan2/p.
