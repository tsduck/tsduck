## Scripts to automate the installation of various tools on Windows

Most scripts in this directory are directly copied from this repository:
https://github.com/lelegard/win-install-scripts

Unlike most Windows systems, the GitHub CI runners do not have winget installed.
This is due to some difficulties with Windows Server. However, it is still possible
to install winget on Windows Server using the script `winget-install.ps1` which is
copied from this repository: https://github.com/asheroto/winget-install
