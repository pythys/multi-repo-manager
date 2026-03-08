# usage

## help

```sh
mrm --help
mrm <command> --help
```

## create config from existing repos

```sh
mrm find myrepos --save myrepos.yml
# or
mrm find myrepos > myrepos.yml
# multiple roots in one config
mrm find client fork personal --save myrepos.yml
```

Each `find` path becomes one `tree.root` in the generated config.
Without `--save`, output is printed to stdout.

Recommended workflow:
- make changes in repositories first (remotes, branches, layout)
- regenerate config with `mrm find`

## sync repos from config

```sh
mrm sync --config myrepos.yml
mrm sync --config myrepos.yml --jobs 12
mrm sync --config myrepos.yml --prune
mrm sync --config myrepos.yml --root "client*"
```

`sync` behavior:
- clone missing repos
- reconcile remotes
- reconcile branches
- align current branch when configured
- process nested dependencies

Prune options (all opt-in):
- `--prune-remotes` (`-R`): remove remotes not declared in config
- `--prune-branches` (`-B`): remove local tracked branches not declared in config
- `--prune` (`-p`): enable both `--prune-remotes` and `--prune-branches`
- `--root <pattern>` (`-r`): apply command only to matching tree roots (supports `*`, `?`, repeatable)
- short equivalents exist (for example `-c` for `--config`, `-j` for `--jobs`)

When pruning branches, the current branch is never deleted.

## status of repos from config

```sh
mrm status --config myrepos.yml
mrm status --config myrepos.yml --root "fork*"
```

Status output is based on libgit2 `git_status_list` and includes:
- staged: new, modified, deleted, renamed, type-changed
- worktree: new, modified, deleted, renamed, type-changed
- conflicted

## update repos from config

```sh
mrm update --config myrepos.yml
mrm update --config myrepos.yml --jobs 12
mrm update --config myrepos.yml --root "client*"
```

## sync branches between remotes

```sh
mrm remotesync --config myrepos.yml --source upstream --target origin --branch master
mrm remotesync --config myrepos.yml --source upstream --target origin --branch master --jobs 12
mrm remotesync --config myrepos.yml --source upstream --target origin --branch master --branch develop --dry-run
mrm remotesync --config myrepos.yml --source upstream --target origin --branch master --root "client*"
```

`remotesync` behavior:
- syncs only explicitly selected `--branch` values
- pulls the source remote branch into the local branch before pushing
- if source branch is missing, tries local branch fallback; otherwise skips
- pushes the local branch to the target remote (creates if missing)
- `--dry-run` (`-n`) reports what would be pushed without changing remotes

## exec custom command in repos

```sh
mrm exec --config myrepos.yml --type git --command "remote get-url origin"
mrm exec --config myrepos.yml --root "client*" --command "status -sb"
```

`exec` runs the command in each targeted repository path from config.

- `--type all` (default) targets all repo types in config. Only works for
  shared commands.
- when the repo CLI exists (for example `git`), mrm prefixes it for you unless
  your command already starts with it.

## concurrency

Use `--jobs` (or `-j`) on `sync`, `update`, and `remotesync` to control max concurrent repo operations.

- `--jobs 0` (default) uses the built-in fallback value.
- larger values can speed up network-bound runs but may increase load on disk/network.

## output modes

`sync` and `update` auto-select output mode:
- interactive terminal: FTXUI live TUI
- non-terminal (redirect/script): final plain-text summary report

TUI notes:
- scroll with arrow keys or mouse wheel
- after completion, a final plain-text report is printed

## SSH authentication

SSH auth fallback order:
1. ssh-agent
2. default key files:
   - `~/.ssh/id_ed25519`
   - `~/.ssh/id_ed25519.pub`

If using agent:

```sh
ssh-add ~/.ssh/id_ed25519
```
