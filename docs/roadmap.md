# Roadmap

## Next

- Create a one-line curl installer

## Backlog

### Safety & Recovery
- Add `--dry-run` flag to all destructive commands (`sync`, `update`, `exec`)

### Configuration Management
- Support global config file at `~/.config/mrm/config` for user defaults (jobs,
  roots, exclude patterns)
- Add `mrm config validate <file>` command to check YAML correctness
- Support config composition with includes: `includes: [shared.yml, local.yml]`
- Allow environment variable expansion in YAML paths (`${HOME}`, `${USER}`)

### Authentication
- Support per-repo SSH key specification in YAML schema
- Ability to pass SSH keys to allow for CI integration

### Workspace Management
- Support git worktrees

### Discovery & Filtering
- Add `--max-depth` / `--min-depth` to `find` command
- Add `--exclude <pattern>` for discovery (skip `.archived/*`, `**/backup/*`)
- Add `mrm query` command with advanced filtering capabilities

### Observability & Reporting
- Add `--log-file <path>` flag to persist operation logs
- Show operation summaries: success/failure counts, timing statistics
- Add `--verbose` / `--debug` flags for detailed logging
- Add progress estimation based on average operation time

### Team Collaboration
- Add `mrm init --from-url <config-url>` to bootstrap from shared config
- Support config fetching: `mrm sync --config https://team.com/repos.yml`

### Git Advanced Features
- Add capability to interact properly with submodules in git
- Shallow or partial clones for very large repos
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
