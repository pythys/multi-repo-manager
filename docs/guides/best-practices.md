# best practices

## config strategy

- Prefer regenerating `mrm.yml` with `mrm find` instead of hand-editing.
- Regenerate when you add/remove repos, change remotes, move paths, or change the
  tracked branch.
- Store `mrm.yml` in a dedicated manager repo so it can be versioned and shared.
- Use a single config file per workspace unless teams need distinct scopes.

Example: adding a repo and regenerating

```sh
git clone https://github.com/cli/cli work/cli
mrm find work personal --save
git add mrm.yml
git commit -m "Add cli repo"
```

## naming and layout

- Keep top-level roots simple: `work`, `personal`, `client`, `forks`.
- Use consistent repo names under each root to avoid confusion across machines.
- Keep repo paths stable to minimize churn in `mrm.yml`.

Example: stable, predictable layout

```
~/repos/
  work/
    api
    web
  personal/
    blog
```

## day-to-day safety

- Use `mrm status` before `mrm update` to understand repo health.
- Use `--root` patterns to target subsets for risky operations.
- Enable prune flags only when you are confident the config is authoritative.
- Pruning deletes remotes/branches that are not in `mrm.yml` (current branch is
  never deleted).

Example: target a subset and dry-run branch syncs

```sh
mrm status --root "work*"
mrm update --root "work*"
mrm remotesync --source upstream --target origin --branch master --dry-run
```

## collaboration

- Commit config changes with short, descriptive messages.
- Use review in PRs if the config is shared by a team.
- Align on root names and remotes across the team to avoid drift.
- Use `remotesync` for forks to keep `origin` in line with `upstream`.

Example: keep forks aligned

```sh
mrm remotesync --source upstream --target origin --branch master
```

## performance

- Start with default jobs; increase `--jobs` only if your network can handle
  more parallel operations.
- Decrease `--jobs` if you see throttling, timeouts, or high disk pressure.
- Use `mrm exec` for one-off bulk commands instead of manual loops.

Example: bulk check remotes

```sh
mrm exec --command "git remote -v"
```
