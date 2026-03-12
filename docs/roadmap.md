# Roadmap

## Planned

- Package for major OSes and distros:
  - one-line curl installer
  - brew
  - deb
  - rpm
  - pacman / AUR
  - windows (mingw, msi)

## Ideas

- Add `--recurse` to `find`, and default to not recursing for performance or
  possibly an inverse flag to disable recursing.
- Add powershell completion.
- Similar to --root, filter repos by --name.
- Add a --format for data outputs like json, xml, etc.
- Add capability to interact properly with submodules in git.
- Shallow or partial clones for very large repos.
- Ability to pass SSH keys to allow for CI integration.
- Implement subversion logic.
- Implement mercurial logic.
- mrm list to show which repos are managed.
- mrm find can have filters and an exec applied on found repos.
