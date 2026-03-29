# quick start

This guide gets you started with mrm in minutes.

## 1. initialize workspace

```sh
mkdir my-repos
cd my-repos
mrm init
```

This creates a ready-to-use mrm workspace with a `r/` directory for repositories.

## 2. clone repositories

```sh
git clone https://github.com/sharkdp/fd r/fd
git clone https://github.com/BurntSushi/ripgrep r/ripgrep
```

## 3. discover and track repos

```sh
mrm find r --save
```

This scans `r/` and updates `mrm.yml`.

## 4. commit the config

```sh
git add mrm.yml
git commit -m "Add initial repos"
```

## 5. operate on all repos

```sh
mrm list
mrm status
mrm update
mrm exec -m "echo repo is: {name}"
```

## 6. add more repos later

```sh
git clone https://github.com/cli/cli r/cli
mrm find r --save
git add mrm.yml
git commit -m "Add cli repo"
```

Regenerate the config whenever you add, remove, or move repos.
