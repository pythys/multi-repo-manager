# fork sync guide

Keep your fork (`origin`) synced with an upstream remote.

## 1. ensure remotes exist

Your fork should have both `origin` and `upstream`.

```sh
git remote -v
git remote add upstream git@github.com:ORG/REPO.git
```

Regenerate config so `mrm.yml` captures the remotes:

```sh
mrm find <roots> --save
```

## 2. run remotesync

```sh
mrm remotesync --source upstream --target origin --branch master
```

For multiple branches:

```sh
mrm remotesync --source upstream --target origin \
  --branch master --branch develop
```

## 3. dry run first (recommended)

```sh
mrm remotesync --source upstream --target origin \
  --branch master --dry-run
```

## 4. target a subset of roots

```sh
mrm remotesync --source upstream --target origin \
  --branch master --root "forks*"
```

Notes:
- `remotesync` pulls the source remote into the local branch before pushing.
- If the source branch is missing, it falls back to a local branch when present.
- Repositories with uncommitted changes are skipped and reported as failed.
  Commit or stash local work before running `remotesync`.
