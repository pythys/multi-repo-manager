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
6. `make scan` (slow ~ 2-5 minutes)
7. Repeat until complete

- Do not run `make clean build` as it leads to race conditions.

## Notes for Agents

- Write tests for critical functionality without breaking public signatures.
- Keep documentation in "/docs" in sync with code changes.
- Library source code used in this project can be found at ~/.conan2/p.
- Prefer terse, functional and portable (linux, macos, windows) code.
- Avoid code comments.
- Trim trailing whitespace.
- Use "master" not "main" as the default branch.
