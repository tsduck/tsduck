=======================================================
TSDuck build scripts and associated configuration files
=======================================================

This directory contains all build scripts, for various purposes, on various
operating systems. Configuration files for binary installers are also included.
The result is an heterogeneous set of files which are described here.

On Unix systems (Linux, macOS), all build tasks are triggered from the various
makefiles in the hierarchy of directory. For some specific tasks, makefiles
invoke scripts in this directory.

On Windows systems, all build tasks are implemented using PowerShell scripts in
this directory. Visual Studio is required for development and debug only.
Building TSDuck and its binary installers does not use Visual Studio; the
scripts directly call MSBuild (a Microsoft equivalent of make, using project
files in XML format).

Git hook files
--------------
TSDuck uses git hooks to automatically update the commit number in tsVersion.h.
The git hooks are automatically installed when building TSDuck.

- Makefile : The makefile in this directory installs the git hooks as default
  target. Whenever you build TSDuck, recursively from the root directory, the
  git hooks are installed or updated.

- git-hook-update.ps1 : This Windows PowerShell script installs the git hooks.
  It is automatically invoked by build.ps1, the main build script for TSDuck.

- git-hook.sh : This script implements the various git hooks, on Unix and
  Windows. The actual hook scripts, in .git/hooks, simply call this script.
  Updating this script is sufficient to update the behavious of the hooks.

Build scripts on Windows
------------------------
- msvc : A subdirectory containing all project files for Visual Studio 2015,
  2017 and 2019. All project files were manually carfted to be generic for
  all versions of Visual Studio to avoid multiple versions of the same project.
  These files are also valid MSBuild project files. The software may be built
  either using MSBuild (see build.ps1) or using the tsduck.sln solution file
  in the Visual Studio GUI.

- tsduck.rc : The Microsoft resource file which is used to build the various
  TSDuck executables.

- build-common.psm1 : A common code module for all PowerShell scripts here.

- build.ps1 : This script builds all TSDuck code, executables and DLL's.

- build-installer.ps1 : This script builds the binary installers for TSDuck.
  It automatically invoke build.ps1. So, to build TSDuck installers from a
  freshly cloned repository, just run this script.

- build-project-files.ps1 : This script builds all project files which list the
  source files in the TSDuck DLL. Adding new source files in the TSDuck library
  is a common task. Each time a new file is added, some header files must be
  updated, such as tsduck.h, the main header file. This script recreates all
  these files from the current set of source files in src/libtsduck. It is
  automatically invoked by build.ps1 to ensure that the project files are
  always up to date.

- build-user-guide-pdf.ps1 : This script updates the Microsoft Word document
  for the TSDuck User's Guide. The version, dates, tables of contents and all
  other fields are updated. The PDF file is generated. So, when updating the
  documentation for new features, just add the description of the new features,
  save and close the Word file and then run this script.

- get-version-from-sources.ps1 : Extract the TSDuck version number from the
  source files.

- cleanup.ps1 : This script does a complete cleanup of the directory tree,
  removing all generated or temporary files. This is a Windows equivalent of
  "make distclean" on Unix.

- tsduck.nsi : This file is an NSIS script to build the binary installers of
  TSDuck. It is used by build-installer.ps1.

- tsduck.props : This Visual Studio property file is installed with the TSDuck
  development environment. It is referenced by third-party applications using
  the TSDuck library.

- install-prerequisites.ps1 : This script downloads and installs all
  pre-requisite packages to build TSDuck on Windows. Alternatively,
  the following individual install-*.ps1 scripts can be used.

- install-libsrt.ps1 : This script downloads and installs libsrt, a pre-compiled
  binary static library for SRT on Windows.

- install-doxygen.ps1 : This script downloads and installs Doxygen.

- install-graphviz.ps1 : This script downloads and installs Graphviz, which is
  required by Doxygen to build class diagrams.

- install-nsis.ps1 : This script downloads and installs NSIS, the NullSoft
  Installer Scripting system which is required to build TSDuck installer.

- WindowsPowerShell.reg : A registry file which add definitions to run a
  PowerShell script by double-clicking on it (the default action is to edit
  script files with notepad).

- WindowsCompileLowPriority.reg : A registry file which add definitions to
  force compilation and link processes to run with a lower priority to avoid
  killing the system while compiling using Visual Studio.

Project files for Linux and macOS
---------------------------------
- build-project-files.sh : This shell script is the Unix equivalent of
  build-project-files.ps1. It is automatically invoked by the makefiles of
  the directory of each project file.

- create-release-text.sh : This shell script recreates the MarkDown file which
  describes the latest TSDuck release on GitHub.

- build-remote.sh : Build the TSDuck installers on a remote system, either a
  running physical system or a local VM to start.

- get-version-from-sources.sh : Extract the TSDuck version number from the
  source files.

- install-prerequisites.sh : This script downloads and installs all
  pre-requisite packages to build TSDuck on Linux or macOS.

- setenv.sh : This script builds the path of the binary directory and sets the
  PATH (or simply displays the binary directory).

- check-libtsduck-dependencies.sh : This script verifies that the source files
  are correctly organized in src/libtsduck. Specifically, it verifies that all
  included headers are strictly contained in a subdirectory or its dependencies.
  This script is useful to run only when there is a major reorganization of the
  source code tree.

- qtcreator : This subdirectory contains all project files for Qt Creator.
  TSDuck does not use Qt. But Qt Creator is a superior C++ IDE which can be
  extremely useful to develop TSDuck or any C++ project. Note that Qt Creator
  is used only to develop and debug TSDuck. Building an official and complete
  version of TSDuck shall be done using make outside Qt Creator.

Configuration files for Linux
-----------------------------
- 80-tsduck.perms : Security settings for DVB adapters. Installed with the
  TSDuck binary packages.

- 80-tsduck.rules : Udev rules for DVB adapters. Installed with the TSDuck
  binary packages.

- tsduck.spec : RPM specification file to create TSDuck .rpm packages on
  Fedora, Red Hat, CentOS and other clones.

- tsduck.control : Template for Debian control file, used to create .deb
  packages for TSDuck on Ubuntu, Debian, Rapsbian and other derivatives.

- tsduck.postinst : Template for post-installation script in .deb packages
  for TSDuck.

- tsduck-dev.control : Template for Debian control file, used to create .deb
  packages for TSDuck development environment.

- tsduck-dev.postinst : Template for post-installation script in .deb packages
  for TSDuck development environment.

- install-openelec-dvb-firmware.sh : Utility to download and install DVB device
  firmware from the OpenELEC project.

Doxygen documentation
---------------------
- Doxyfile : Project file to configure Doxygen.

- doxy-defaults.conf : Default values for all Doxygen configuration parameters.

- doxy-header.html : Template file for header of Doxygen-generated HTML pages.

- doxy-footer.html : Template file for footer of Doxygen-generated HTML pages.

- doxy-style.css : CSS style sheet for Doxygen-generated HTML pages.

- build-doxygen.ps1 : Windows PowerShell script to create the Doxygen
  documentation. On Unix systems, use "make doxygen" at project root level.
