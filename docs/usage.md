# usage

Example Usage:

- create a config file

```
mkdir myrepos && cd myrepos
git clone https://github.com/bootandy/dust
git clone https://github.com/sharkdp/fd
git clone https://github.com/junegunn/fzf
git clone https://github.com/siduck/st
cd ..
mrm find myrepos --save myrepos.yml
```

- sync repos from config file

`mrm sync --config myrepos.yml myrepos`

- get help

`mrm --help` or `mrm -h`
`mrm <command> --help` e.g.:
  - `mrm sync --help`
  - `mrm find -h`

## SSH Credentials

If you have authenticated repos, then SSH installation is required along with
adding the SSH keys as sampled in below command:

`ssh-add ~/.ssh/id_rsa`
