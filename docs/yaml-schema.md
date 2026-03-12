# yaml schema

This page describes the schema of `mrm.yml`. It is a reference for how `mrm
find` writes the file, not a primary configuration guide.

If `--config` is omitted, `mrm.yml` in the current directory is used.

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
        type: git
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
- Each repo has a `name` (relative to the tree root), a `type` (like git), a
  list of `remotes`, and a list of `branches`.
- Each remote has a `name` and a `URL`.
- Each branch has a `name`, a `remote` it tracks, and a boolean `is_current`
  indicating if it is the currently selected branch.

## minimal example

```yaml
trees:
  - root: myrepos
    repos:
      - name: fd
        type: git
        remotes:
          - name: origin
            url: https://github.com/sharkdp/fd
```

## example with branches

```yaml
trees:
  - root: work
    repos:
      - name: app
        type: git
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
        type: git
        remotes:
          - name: origin
            url: https://git.mycompany.com/org/app.git
  - root: personal
    repos:
      - name: blog
        type: git
        remotes:
          - name: origin
            url: https://git.example.com/user/blog.git
```

## notes

- `name` is relative to `root`.
- Nested repo paths are allowed (for example `parent/child`).
- During `sync`, missing repos are cloned and existing repos are reconciled.
- Other SCM types are planned; see the roadmap.

## guides

- [Best Practices](guides/best-practices.md)
