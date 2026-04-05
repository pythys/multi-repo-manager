# usage

## help

```sh
mrm --help
mrm <command> --help
```

## sync

Clone and synchronize repositories from config.

```sh
mrm sync
mrm sync --config myrepos.yml
mrm sync --config myrepos.yml --jobs 12
mrm sync --config myrepos.yml --prune
mrm sync --config myrepos.yml --root "client*"
```

Behavior:
- clone missing repos
- reconcile remotes
- reconcile branches
- align current branch when configured
- process nested dependencies

Options:
- `--config <file>` (`-c`): config file (default: `mrm.yml`), see [source
  options](#source-options)
- `--root <pattern>` (`-r`): filter by tree root, see [filtering](#filtering)
- `--name <pattern>` (`-n`): filter by repo name, see [filtering](#filtering)
- `--jobs <n>` (`-j`): max concurrent operations, see
  [concurrency](#concurrency)
- `--prune-remotes` (`-R`): remove remotes not declared in config
- `--prune-branches` (`-B`): remove local tracked branches not declared in
  config
- `--prune-repos` (`-P`): remove repository directories not declared in config
- `--prune` (`-p`): enable `--prune-remotes`, `--prune-branches`, and
  `--prune-repos`

Notes:
- all prune options are opt-in
- when pruning branches, the current branch is never deleted
- when pruning repos, entire repository directories are deleted from disk
- does not support `--find` (requires parent/child dependencies from config)

See [output modes](#output-modes) for terminal-specific formatting.

## list

Display repositories in table format.

```sh
mrm list
mrm list --config myrepos.yml
mrm list --config myrepos.yml --root "fork*"
mrm list --config myrepos.yml --name "*frontend*"
mrm list --find myrepos
```

Output columns:
- ROOT: tree root path
- NAME: repository name
- TYPE: repository type (git, svn, hg)
- REMOTES: count of configured remotes
- BRANCHES: count of local branches
- CURRENT: currently checked-out branch name

Options:
- `--config <file>` (`-c`): config file, see [source options](#source-options)
- `--find <path>` (`-f`): discover repos, see [source options](#source-options)
- `--root <pattern>` (`-r`): filter by tree root, see [filtering](#filtering)
- `--name <pattern>` (`-n`): filter by repo name, see [filtering](#filtering)

See [output modes](#output-modes) for terminal-specific formatting.

## find

Scan directories for repositories and generate config.

```sh
mrm find myrepos
mrm find myrepos --save
mrm find myrepos --save myrepos.yml
# or
mrm find myrepos > myrepos.yml
# multiple roots in one config
mrm find client fork personal --save myrepos.yml
```

Behavior:
- scans specified paths for repositories (`.git`, `.hg`, etc directories)
- outputs YAML config to stdout or saves to file
- each path becomes one `tree.root` in the generated config

Options:
- `--save` (`-s`): save to file instead of stdout (default: `mrm.yml`)

Recommended workflow:
- make changes in repositories first (remotes, branches, layout)
- regenerate config with `mrm find -s`

## status

Show working tree status for repositories.

```sh
mrm status
mrm status --config myrepos.yml
mrm status --config myrepos.yml --root "fork*"
mrm status --config myrepos.yml --name "*frontend*"
mrm status --find ~/projects
```

Behavior:
- reports uncommitted changes in each repository
- shows staged changes ready for commit
- shows unstaged changes in working tree
- highlights conflicted files

Output includes:
- staged: new, modified, deleted, renamed, type-changed
- unstaged: new, modified, deleted, renamed, type-changed
- conflicted

Options:
- `--config <file>` (`-c`): config file, see [source options](#source-options)
- `--find <path>` (`-f`): discover repos, see [source options](#source-options)
- `--root <pattern>` (`-r`): filter by tree root, see [filtering](#filtering)
- `--name <pattern>` (`-n`): filter by repo name, see [filtering](#filtering)

See [output modes](#output-modes) for terminal-specific formatting.

## update

Pull latest changes for repositories.

```sh
mrm update
mrm update --config myrepos.yml
mrm update --config myrepos.yml --jobs 12
mrm update --config myrepos.yml --root "client*"
mrm update --config myrepos.yml --name "*backend*"
mrm update --find ~/projects
```

Behavior:
- pulls latest changes from tracking branch
- operates on current branch in each repository

Options:
- `--config <file>` (`-c`): config file, see [source options](#source-options)
- `--find <path>` (`-f`): discover repos, see [source options](#source-options)
- `--root <pattern>` (`-r`): filter by tree root, see [filtering](#filtering)
- `--name <pattern>` (`-n`): filter by repo name, see [filtering](#filtering)
- `--jobs <n>` (`-j`): max concurrent operations, see
  [concurrency](#concurrency)

See [output modes](#output-modes) for terminal-specific formatting.

## remotesync

Synchronize branches between remotes.

```sh
mrm remotesync --source upstream --target origin --branch master
mrm remotesync --config myrepos.yml --source upstream --target origin --branch master
mrm remotesync --config myrepos.yml --source upstream --target origin --branch master --jobs 12
mrm remotesync --config myrepos.yml --source upstream --target origin --branch master --branch develop --dry-run
mrm remotesync --find ~/forks --source upstream --target origin --branch main
```

Behavior:
- syncs only explicitly selected `--branch` values
- pulls the source remote branch into the local branch before pushing
- if source branch is missing, tries local branch fallback; otherwise skips
- pushes the local branch to the target remote (creates if missing)

Options:
- `--config <file>` (`-c`): config file, see [source options](#source-options)
- `--find <path>` (`-f`): discover repos, see [source options](#source-options)
- `--root <pattern>` (`-r`): filter by tree root, see [filtering](#filtering)
- `--name <pattern>` (`-n`): filter by repo name, see [filtering](#filtering)
- `--jobs <n>` (`-j`): max concurrent operations, see
  [concurrency](#concurrency)
- `--source <remote>` (`-s`): source remote name (required)
- `--target <remote>` (`-t`): target remote name (required)
- `--branch <name>` (`-b`): branch to sync (repeatable, required)
- `--dry-run` (`-d`): report what would be pushed without changing remotes

See [output modes](#output-modes) for terminal-specific formatting.

## exec

Execute arbitrary commands in repositories with context substitution.

```sh
# VCS commands (must specify full command)
mrm exec --command "git remote get-url origin"
mrm exec --config myrepos.yml --type git --command "git status -sb"
mrm exec --find ~/projects --name "*api*" --command "git log --oneline -5"

# Arbitrary commands with placeholders
mrm exec --command "echo {name}: {type} repo at {path}"
mrm exec --command "du -sh {path}"
mrm exec --command "find {path} -name '*.cpp' -type f"
mrm exec --command "wc -l {path}/src/**/*.cpp"
mrm exec --command "tar -czf /backup/{name}.tar.gz -C {root} {name}"
```

Behavior:
- runs command in each targeted repository path
- substitutes placeholders with repository context before execution
- displays command output to stdout/stderr

Placeholders:
- `{path}`: full absolute path to repository (e.g., `/home/user/projects/my-app`)
- `{name}`: repository name only (e.g., `my-app`)
- `{root}`: tree root path (e.g., `/home/user/projects`)
- `{type}`: repository type (e.g., `git`, `svn`, `hg`)

Options:
- `--config <file>` (`-c`): config file, see [source options](#source-options)
- `--find <path>` (`-f`): discover repos, see [source options](#source-options)
- `--root <pattern>` (`-r`): filter by tree root, see [filtering](#filtering)
- `--name <pattern>` (`-n`): filter by repo name, see [filtering](#filtering)
- `--command <cmd>`: command to execute (required)
- `--type <type>`: repo type filter (`git`, `svn`, `hg`, or `all` - default:
  `all`)

See [output modes](#output-modes) for terminal-specific formatting.

## init

Initialize a new mrm workspace with recommended structure.

```sh
mkdir my-repos
cd my-repos
mrm init
```

Behavior:
- creates `README.md` with workspace documentation
- creates empty `mrm.yml` with usage instructions
- creates repositories directory (default: `r/`)
- initializes git repository
- adds repositories directory to `.gitignore`
- requires empty directory (no existing `mrm.yml` or `.git`)

Options:
- `--repos-path <path>`: repositories directory name (default: `r`)

Errors:
- fails if `mrm.yml` already exists
- fails if `.git` already exists
- fails if git command is not available

After initialization:
1. Clone repositories into the repos directory
2. Run `mrm find <repos-path> --save` to generate config
3. Commit `mrm.yml` to track your workspace

See [quick start](guides/quickstart.md) for a complete workflow example.

## source options

Commands that operate on repositories support two mutually exclusive source
options:

### --config (default)

```sh
mrm status --config myrepos.yml
mrm update  # uses mrm.yml by default
```

Load repository configuration from YAML file:
- default: `mrm.yml` in current directory
- short form: `-c`
- supports [filtering](#filtering) to target subset of repos

### --find (adhoc)

```sh
mrm status --find ~/projects
mrm update --find ~/work ~/personal --jobs 8
```

Discover repositories by scanning directories:
- scans specified paths for repositories (`.git`, `.hg`, etc directories)
- creates in-memory structure equivalent to config file
- each path becomes one tree root
- repeatable: `--find ~/work --find ~/personal`
- short form: `-f`
- supports [filtering](#filtering) to target subset of repos
- `sync` command does not support it as it requires a yaml file

## filtering

Most commands support filtering to target specific repositories:

```sh
# Filter by tree root
mrm status --config myrepos.yml --root "client*"

# Filter by repo name
mrm update --config myrepos.yml --name "*frontend*"

# Combine filters
mrm status --find ~/work ~/personal --root "*work" --name "*api*"

# Multiple patterns (OR logic)
mrm exec --config myrepos.yml --name "*react*" --name "*vue*" --command "fetch --all"
```

Filter options:
- `--root <pattern>` (`-r`): filter by tree root (top-level directories)
- `--name <pattern>` (`-n`): filter by repository name

Pattern syntax:
- `*`: matches any characters
- `?`: matches single character
- case-sensitive matching
- both options are repeatable and combine with OR logic

Filter behavior:
- filters apply sequentially: `--root` first, then `--name`
- empty trees are pruned after filtering
- no match results in empty operation (not an error)

## concurrency

Commands that perform network operations support the `--jobs` flag:

```sh
mrm sync --jobs 12
mrm update --config myrepos.yml --jobs 8
mrm remotesync --find ~/forks --source upstream --target origin --branch main --jobs 4
```

Options:
- `--jobs <n>` (`-j`): max concurrent repository operations
- `--jobs 0` (default): uses built-in fallback value

Notes:
- larger values can speed up network-bound operations
- may increase load on disk/network resources
- applies to: `sync`, `update`, `remotesync`

## output modes

Commands auto-select output mode based on terminal state:

Interactive terminal (TTY):
- `sync`, `update`, `remotesync`: live TUI with progress tracking (FTXUI)
- `status`: colored phase and message output
- `list`: colored table output
- `exec`: colored command output

Non-interactive (piped/redirected):
- `sync`, `update`, `remotesync`: plain text summary report at completion
- `status`: plain text phase and message output
- `list`: plain text table output
- `exec`: plain text command output

TUI controls:
- arrow keys: scroll up/down
- mouse wheel: scroll up/down
- after completion, final plain text report is printed

## SSH authentication

SSH auth fallback order:
1. ssh-agent
2. key files under `~/.ssh` (all private keys, tried in filename order)

If using agent:

```sh
ssh-add ~/.ssh/id_ed25519
```

## shell completion

Shell completion is a one-time setup. Add to your shell config:

zsh:
```sh
echo 'source <(mrm completion zsh)' >> ~/.zshrc
```

bash:
```sh
echo 'source <(mrm completion bash)' >> ~/.bashrc
```

## guides

- [Quick Start](guides/quickstart.md)
- [Fork Sync Guide](guides/remotesync-forks.md)
- [Nested Repos Guide](guides/nested-repos.md)
- [Best Practices](guides/best-practices.md)

## schema reference

- [YAML Schema](yaml-schema.md)
