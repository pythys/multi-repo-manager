# Roadmap

- Implement remotesync command.
- Add --recurse to find for improved performance.
- Make a more dynamic and robust ssh key fallback (e.g. libssh2)
- Finalize [complgen](https://github.com/adaszko/complgen) (mrm.usage).
  - Linux bash: /etc/bash_completion.d
  - Linux zsh: /usr/share/zsh/site-functions/
  - Macos bash: $(brew --prefix)/etc/bash_completion.d
  - Macos zsh: $(brew --prefix)/share/zsh/site-functions
- Introduce dynamic and static packaging of software using cpack.
- Publish to Windows, MacOS and Linux distros.
- Switch from make to [just](https://github.com/casey/just)
