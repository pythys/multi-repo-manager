# nested repos guide

Nested repositories are supported. Each repo is still defined as a separate
entry in `mrm.yml` with a nested `name`, its remotes, and its tracked branches.

## example layout

```
work/
  app/
  app/plugins/auth/
  app/plugins/billing/
  app/vendor/widgets/
```

## create a nested layout (example)

```sh
mkdir -p work/app
git clone git@github.com:org/app.git work/app

mkdir -p work/app/plugins
git clone git@github.com:org/auth-plugin.git work/app/plugins/auth
git clone git@github.com:org/billing-plugin.git work/app/plugins/billing

mkdir -p work/app/vendor
git clone git@github.com:org/widgets.git work/app/vendor/widgets
```

## generate config

```sh
mrm find work --save
```

This will generate nested repo entries like:

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
      - name: app/plugins/auth
        remotes:
          - name: origin
            url: git@github.com:org/auth-plugin.git
        branches:
          - name: master
            remote: origin
            is_current: true
      - name: app/plugins/billing
        remotes:
          - name: origin
            url: git@github.com:org/billing-plugin.git
        branches:
          - name: master
            remote: origin
            is_current: true
      - name: app/vendor/widgets
        remotes:
          - name: origin
            url: git@github.com:org/widgets.git
        branches:
          - name: master
            remote: origin
            is_current: true
```

## when the root is itself a repo

If the path you scan is itself a repository, it is recorded as a sibling entry
with `name: .` alongside its nested repos. Use `mrm find <path> --mindepth 1` to
exclude the root repository and keep only the nested ones.

## operating safely

- Use `mrm status` to verify each nested repo is clean.
- Use `--root` patterns to isolate a subtree when needed.
- Avoid aggressive pruning unless your config is fully accurate.

Example: target just the plugins subtree

```sh
mrm status --root "work*"
mrm update --root "work*"
```
