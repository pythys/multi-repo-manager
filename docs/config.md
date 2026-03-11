# config

`mrm` reads a YAML config with top-level `trees`.

If `--config` is omitted, `mrm.yml` in the current directory is used.

## recommended workflow

Use repo-first changes, then regenerate config with `mrm find`.

```sh
mrm find <path> --save # defaults to mrm.yml
mrm find <path> --save <config.yml>
```

Prefer not to hand-write or hand-edit YAML. Regeneration keeps config aligned
with actual repositories.

## schema

```yaml
trees:
  - root: <path>
    repos:
      - name: <relative/repo/path>
        type: git | svn | hg
        remotes:
          - name: <remote-name>
            url: <remote-url>
        branches:
          - name: <branch-name>
            remote: <remote-name>
            is_current: true | false
```

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
