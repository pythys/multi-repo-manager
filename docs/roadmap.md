# roadmap

- Auto delete incomplete clones
- Switch most commands to be parallel.
- Implement callback functions in git_manager to report on progress.
- Implement all progress reporting through messages sent to observer.
- Implement the remotesync command.
- Implement the exec command.
- Implement dynamic FTXUI interface for all commands.
- Implement static interface when !is_terminal for all commands.
- Limit recursion to .gitmodule and .gitignore for improved performance.
- Introduce dynamic and static packaging of software using cpack.
- Finalize [complgen](https://github.com/adaszko/complgen) (mrm.usage).
  - linux bash: /etc/bash_completion.d
  - linux zsh: /usr/share/zsh/site-functions/
  - macos bash: $(brew --prefix)/etc/bash_completion.d
  - macos zsh: $(brew --prefix)/share/zsh/site-functions
- Resolve clang-tidy scan issues
