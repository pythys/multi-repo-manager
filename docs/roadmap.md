# Roadmap

## Next

- Create a one-line curl installer

## Backlog

### Safety & Recovery
- Add `--dry-run` flag to all destructive commands (`sync`, `update`, `exec`)
- Add `--interactive` / `--confirm` flag for operations with `--prune`
- Add persistent operation log (e.g., `~/.mrm/history.log`)
- Add `mrm undo` command to rollback last operation using git reflog

### Configuration Management
- Support global config file at `~/.config/mrm/config` for user defaults (jobs,
  roots, exclude patterns)
- Add `mrm config validate <file>` command to check YAML correctness
- Support config composition with includes: `includes: [shared.yml, local.yml]`
- Allow environment variable expansion in YAML paths (`${HOME}`, `${USER}`)

### Error Handling
- Add `mrm retry` command to re-run only failed operations from last run
- Add `--continue-on-error` vs `--fail-fast` modes
- Improve error messages with actionable suggestions
- Add exponential backoff (increase wait) retry for network failures
- Add `mrm conflicts` command to list all repos with merge conflicts

### Authentication
- Support HTTPS authentication with credential helpers (Git Credential Manager)
- Add `--credential-file` or `--token` flags for CI/CD usage
- Support per-repo SSH key specification in YAML schema
- Ability to pass SSH keys to allow for CI integration
- Add `GIT_ASKPASS` integration for password prompts

### Workspace Management
- Add `mrm snapshot save <name>` / `mrm snapshot restore <name>` commands
- Add `mrm health` command to verify workspace state (dirty repos, unpushed
  commits, diverged branches)
- Add `--skip-dirty` flag to skip repos with uncommitted changes
- Support dependency ordering in YAML for cross-repo builds
- Support git worktrees

### Discovery & Filtering
- Add `--max-depth` / `--min-depth` to `find` command
- Add `--exclude <pattern>` for discovery (skip `.archived/*`, `**/backup/*`)
- Add state-based filtering: `--filter "status=dirty"`, `--filter "ahead>0"`
- Add `mrm query` command with advanced filtering capabilities
- Add `--recurse` to `find`, and default to not recursing for performance or
  possibly an inverse flag to disable recursing

### Observability & Reporting
- Add `--log-file <path>` flag to persist operation logs
- Add structured output: `--format json|csv|yaml` for all commands
- Show operation summaries: success/failure counts, timing statistics
- Add `--verbose` / `--debug` flags for detailed logging
- Add progress estimation based on average operation time

### Performance & Scalability
- Add caching layer for remote metadata (with expiration)
- Add `--since <timestamp>` to process only recently-modified repos
- Parallelize `exec` command execution with `--jobs` support
- Support sparse-checkout in YAML schema

### Team Collaboration
- Add `mrm init --from-url <config-url>` to bootstrap from shared config
- Add `mrm diff-workspace <config.yml>` to compare workspace setups
- Support config fetching: `mrm sync --config https://team.com/repos.yml`
- Add workspace verification: `mrm verify --against <canonical-config>`

### Git Advanced Features
- Add capability to interact properly with submodules in git
- Shallow or partial clones for very large repos
- Add Git LFS awareness (opt-in LFS object pulling)
- Add `mrm hooks sync --from <template-dir>` to distribute git hooks

### VCS Support
- Implement subversion logic
- Implement mercurial logic

### Distribution
- Add powershell completion
- Package for major OSes and distros:
  - brew
  - deb
  - rpm
  - pacman / AUR
  - windows (mingw, msi)
