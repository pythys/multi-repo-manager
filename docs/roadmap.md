# Roadmap

## Next

- Create a one-line curl installer

## Backlog

- mrm find can have filters and an exec applied on found repos.
- Similar to --root, filter repos by --name.
- mrm list to show which repos are managed.
- Ability to pass SSH keys to allow for CI integration.
- Add `--recurse` to `find`, and default to not recursing for performance or
  possibly an inverse flag to disable recursing.
- Add powershell completion.
- Add capability to interact properly with submodules in git.
- Shallow or partial clones for very large repos.
- Implement subversion logic.
- Implement mercurial logic.
- Package for major OSes and distros:
  - brew
  - deb
  - rpm
  - pacman / AUR
  - windows (mingw, msi)
