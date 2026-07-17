# yaml schema

This page describes the schema of `mrm.yml`. It is a reference for the YAML
that `mrm find` writes and `mrm sync` reads, not a primary configuration guide.

If `--config` is omitted, `mrm.yml` in the current directory is used. `--config`
also accepts a git repository reference (`<repo-url>//<path>[?ref=<ref>]`) so the
config can be fetched from a repo over SSH or HTTPS; see
[remote config](usage.md#remote-config).

## recommended workflow

Use repo-first changes, then regenerate config with `mrm find`.

```sh
mrm find <path> --save # defaults to mrm.yml
mrm find <path> --save <config.yml>
```

Prefer not to hand-write or hand-edit YAML. Regeneration keeps the schema
aligned with actual repositories.

## schema

```yaml
trees:
  - root: <path>
    repos:
      - name: <relative/repo/path>
        remotes:
          - name: <remote-name>
            url: <remote-url>
        branches:
          - name: <branch-name>
            remote: <remote-name>
            is_current: true | false
```

- Each file contains a list of trees.
- A `tree` is a set of repositories under a common root directory.
- A `root` is a path relative to the config file.
- The `repos` are a collection of repositories under the tree.
- Each repo has a `name` (relative to the tree root), a list of `remotes`, and a
  list of `branches`. When the tree root is itself a repository, its `name` is
  `.`.
- Each remote has a `name` and a `url`.
- Each branch has a `name`, a `remote` it tracks, and a boolean `is_current`
  indicating if it is the currently selected branch.
- `branches` contains local branches that track a remote branch. Local branches
  without upstream tracking are not written by `mrm find`.
- `mrm sync` creates missing configured branches from `<remote>/<name>`, updates
  branch upstream tracking when needed, and switches to the branch with
  `is_current: true`.

## minimal example

```yaml
trees:
  - root: myrepos
    repos:
      - name: fd
        remotes:
          - name: origin
            url: https://github.com/sharkdp/fd
        branches:
          - name: master
            remote: origin
            is_current: true
```

## example with branches

```yaml
trees:
  - root: work
    repos:
      - name: app
        remotes:
          - name: origin
            url: git@github.com:org/app.git
        branches:
          - name: master
            remote: origin
            is_current: true
```

## example with multiple trees

```yaml
trees:
  - root: work
    repos:
      - name: app
        remotes:
          - name: origin
            url: https://git.mycompany.com/org/app.git
        branches:
          - name: master
            remote: origin
            is_current: true
  - root: personal
    repos:
      - name: blog
        remotes:
          - name: origin
            url: https://git.example.com/user/blog.git
        branches:
          - name: main
            remote: origin
            is_current: true
```

## notes

- `name` is relative to `root`.
- Nested repo paths are allowed (for example `parent/child`).
- `remotes` is required for each repo; `origin` is required when `sync` needs to
  clone a missing repo.
- `branches` may be empty, but configured branch entries require `name`,
  `remote`, and `is_current`.
- During `sync`, missing repos are cloned and existing repos are reconciled.

## guides

- [Best Practices](guides/best-practices.md)
