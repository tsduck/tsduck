# TSDuck build scripts and associated configuration files

This directory contains all build scripts, for various purposes, on various
operating systems. Configuration files for binary installers are also included.
The result is an heterogeneous set of files which are described here.

On Unix systems (Linux, macOS), all build tasks are triggered from `make`.

On Windows systems, all build tasks are implemented using PowerShell scripts in
this directory. Visual Studio is required for development and debug only.
Building TSDuck and its binary installers does not use Visual Studio; the
scripts directly call MSBuild (a Microsoft equivalent of make, using project
files in XML format).

## Build scripts on Windows

- `msvc` : A subdirectory containing all project files for Visual Studio. All
  project files were manually carfted to be generic for all versions of Visual
  Studio to avoid multiple versions of the same project.
  These files are also valid MSBuild project files. The software may be built
  either using MSBuild (see `build.ps1`) or using the `tsduck.sln` solution file
  in the Visual Studio IDE.

- `tsduck.rc` : The Microsoft resource file which is used to build the various
  TSDuck executables.

- `build-common.psm1` : A common code module for all PowerShell scripts here.

- `build.ps1` : This script builds all TSDuck code, executables and DLL's.

- `build-java.ps1` : This script builds the Java bindings for TSDuck. It is
  automatically invoked by `build.ps1` and there is no need to explicitly
  run it.

- `build-installer.ps1` : This script builds the binary installers for TSDuck.
  It automatically invokes `build.ps1`. So, to build TSDuck installers from a
  freshly cloned repository, just run this script.

- `build-project-files.ps1` : Build the project files for "Qt Creator" and
  "Visual Studio". See `build-project-files.py`.

- `build-tsduck-header.ps1` : This script builds the `tsduck.h` header file which
  includes all public headers of the library. See `build-tsduck-header.py`.

- `get-version-from-sources.ps1` : Extract the TSDuck version number from the
  source files.

- `cleanup.ps1` : This script does a complete cleanup of the directory tree,
  removing all generated or temporary files. See `cleanup.py`.

- `tsduck.nsi` : This file is an NSIS script to build the binary installers of
  TSDuck. It is used by `build-installer.ps1`.

- `tsduck.props` : This Visual Studio property file is installed with the TSDuck
  development environment. It is referenced by third-party applications using
  the TSDuck library.

- `install-prerequisites.ps1` : This script downloads and installs all
  pre-requisite packages to build TSDuck on Windows.

- `install-wintools` : This directory contains scripts to install individual
  prerequisites. They are all invoked by `install-prerequisites.ps1`.

- `WindowsPowerShell.reg` : A registry file which add definitions to run a
  PowerShell script by double-clicking on it (the default action is to edit
  script files with notepad).

- `WindowsCompileLowPriority.reg` : A registry file which add definitions to
  force compilation and link processes to run with a lower priority to avoid
  killing the system while compiling using Visual Studio.

## Project files for Linux and macOS

- `build-project-files.py` : Build the project files for "Qt Creator" and
  "Visual Studio" (also used by MSBuild) for all TSDuck commands and plugins.
  See more details in the leading comments in the script.

- `build-remote.sh` : Build the TSDuck installers on a remote system, either a
  running physical system or a local VM to start.

- `build-tsduck-header.py` : This script builds the `tsduck.h` header file which
  includes all public headers of the TSDuck library. Adding new source files
  in the TSDuck library is a common task. Each time a new public file is added,
  `tsduck.h` must be updated. This script is automatically invoked by the
  makefile in `libtsduck`.

- `get-version-from-sources.sh` : Extract the TSDuck version number from the
  source files.

- `install-prerequisites.sh` : This script downloads and installs all
  pre-requisite packages to build TSDuck on Linux or macOS.

- `setenv.sh` : This script builds the path of the binary directory and sets the
  PATH (or simply displays the binary directory).

- `qtcreator` : This subdirectory contains all project files for Qt Creator.
  TSDuck does not use Qt. But Qt Creator is a superior C++ IDE which can be
  extremely useful to develop TSDuck or any C++ project. Note that Qt Creator
  is used only to develop and debug TSDuck. Building an official and complete
  version of TSDuck shall be done using make outside Qt Creator.

## Configuration files for Linux

- `tsduck.spec` : RPM specification file to create TSDuck `.rpm` packages on
  Fedora, RedHat, CentOS, AlmaLinux and other clones.

- `tsduck.control` : Template for Debian control file, used to create `.deb`
  packages for TSDuck on Ubuntu, Debian, Rapsbian and other derivatives.

- `tsduck-dev.control` : Template for Debian control file, used to create `.deb`
  packages for TSDuck development environment.
