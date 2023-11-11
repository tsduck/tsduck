# GitHub repository maintenance

This directory contains scripts to maintain the TSDuck repository on GitHub.

All scripts accept the following common options:

- `--repo owner/name` : Alternate GitHub repo. Default: `tsduck/tsduck`.
- `--token string` : GitHub access token. This is required to update the repo.
  By default, use environment variables `GITHUB_TOKEN` or, if not defined, `HOMEBREW_GITHUB_API_TOKEN`.
- `--branch name` : Alternate Git branch. Default: `master`.
- `-n` or `--dry-run` : Display what would be done, but don't do.
- `-v` or `--verbose` : Verbose display.

## Python modules prerequisites

### PyGitHub

- Install: `pip install PyGithub`
- On Ubuntu: `sudo apt install python3-github`
- Documentation: https://pygithub.readthedocs.io/
- Minimum required version: 1.57

Warning: There is a PyPI module named `github` which is different and
incompatible with `PyGitHub`. The two declare the module `github` with
different contents and classes. You cannot install the two at the same
time. If you accidentally installed `github` instead of `PyGitHub`,
run `pip uninstall github` before `pip install PyGithub`.

### requests

- Install: `pip install requests`
- Documentation: https://requests.readthedocs.io/

The module `requests` is a dependency for `PyGithub` and should be
automatically installed with `PyGithub`.
