# NSIS packaging for Windows

This directory contains scripts to build the TSDuck installer on Windows
using NSIS, a free open-source packager for Windows.

- `build-installer.ps1` : This script builds the binary installers for TSDuck.
  It automatically invokes `scripts\build.ps1`. So, to build TSDuck installers
  from a freshly cloned repository, just run this script.

- `tsduck.nsi` : Windows NSIS script to build the binary installers of TSDuck.
   It is used by `build-installer.ps1`.

- `tsduck.props` : This Visual Studio property file is installed with the TSDuck
  development environment. It is referenced by third-party applications using
  the TSDuck library.
