# Roadmap

- Terminal based logic to decide between FTXUI and cout for all commands.
- Auto delete incomplete clones
- Sync command to provide --delete-untracked
- Make a more dynamic ssh key fallback (e.g. libssh2)
- Implement remotesync command.
- Implement exec command.
- Introduce branch tracking
- Flag recursion to .gitmodule and .gitignore for improved performance.
- Finalize [complgen](https://github.com/adaszko/complgen) (mrm.usage).
  - Linux bash: /etc/bash_completion.d
  - Linux zsh: /usr/share/zsh/site-functions/
  - Macos bash: $(brew --prefix)/etc/bash_completion.d
  - Macos zsh: $(brew --prefix)/share/zsh/site-functions
- Introduce dynamic and static packaging of software using cpack.
- Publish to Windows, MacOS and Linux distros.
- Switch from make to [just](https://github.com/casey/just)
