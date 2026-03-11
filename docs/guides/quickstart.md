# quick start

This guide sets up a simple workspace, generates `mrm.yml`, and keeps it in a
manager repo so you can share it across machines.

## 1. create a workspace and manager repo

```sh
mkdir -p ~/repos
cd ~/repos

mkdir mrm-config
cd mrm-config
git init
```

## 2. add a couple repos

```sh
cd ~/repos

mkdir -p work personal
git clone https://github.com/sharkdp/fd work/fd
git clone https://github.com/BurntSushi/ripgrep personal/ripgrep
```

## 3. generate config with multiple trees

```sh
cd ~/repos/mrm-config
mrm find ../work ../personal --save
```

This creates two `trees` in `mrm.yml` and writes to the default file.

## 4. commit the config

```sh
git add mrm.yml
git commit -m "Add initial mrm config"
```

## 5. operate on all repos

```sh
mrm status
mrm update
```

Target a subset using `--root` patterns:

```sh
mrm status --root "work*"
mrm update --root "personal*"
```

## 6. add more repos later

```sh
cd ~/repos
git clone https://github.com/cli/cli work/cli

cd ~/repos/mrm-config
mrm find ../work ../personal --save
git add mrm.yml
git commit -m "Update mrm config"
```

Regenerate the config whenever you add, remove, or move repos.
