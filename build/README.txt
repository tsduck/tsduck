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
  It is automatically invoked by Build.ps1, the main build script for TSDuck.

- git-hook.sh : This script implements the various git hooks, on Unix and
  Windows. The actual hook scripts, in .git/hooks, simply call this script.
  Updating this script is sufficient to update the behavious of the hooks.

Build scripts on Windows
------------------------
- msvc2017 : A subdirectory containing all Visual Studio 2017 and 2015 project
  files. These files are also valid MSBuild project files. The software may be
  built either using MSBuild (see Build.ps1) or using the tsduck.sln solution
  file in the Visual Studio GUI.

- tsduck.rc : The Microsoft resource file which is used to build the various
  TSDuck executables.

- Build-Common.psm1 : A common code module for all PowerShell scripts here.

- Build.ps1 : This script builds all TSDuck code, executables and DLL's.

- Build-Installer.ps1 : This script builds the binary installers for TSDuck.
  It automatically invoke Build.ps1. So, to build TSDuck installers from a
  freshly cloned repository, just run this script.

- Build-Project-Files.ps1 : This script builds all project files which list the
  source files in the TSDuck DLL. Adding new source files in the TSDuck library
  is a common task. Each time a new file is added, many different files must be
  updated: Visual Studio project files, Qt Creator project files, tsduck.h main
  header file. This script recreates all these files from the current set of
  source files in src/libtsduck. It is automatically invoked by Build.ps1 to
  ensure that the project files are always up to date.

- Build-User-Guide-PDF.ps1 : This script updates the Microsoft Word document
  for the TSDuck User's Guide. The version, dates, tables of contents and all
  other fields are updated. The PDF file is generated. So, when updating the
  documentation for new features, just add the description of the new features,
  save and close the Word file and then run this script.

- Cleanup.ps1 : This scripts does a complete cleanup of the directory tree,
  removing all generated or temporary files. This is a Windows equivalent of
  "make distclean" on Unix.

- tsduck.nsi : This file is an NSIS script to build the binary installers of
  TSDuck. It is used by Build-Installer.ps1.

- tsduck.props : This Visual Studio property file is installed with the TSDuck
  development environment. It is referenced by third-party applications using
  the TSDuck library.

- WindowsPowerShell.reg : A registry file which add definitions to run a
  PowerShell script by double-clicking on it (the default action is to edit
  script files with notepad).

Project files for Linux and macOS
---------------------------------
- build-project-files.sh : This shell script is the Unix equivalent of
  Build-Project-Files.ps1. It is automatically invoked by the makefiles of
  the directory of each project file.

- create-release-text.sh : This shell script recreates the MarkDown file which
  describes the latest TSDuck release on GitHub.

- build-remote.sh : Build the TSDuck installers on a remote system, either a
  running physical system or a local VM to start.

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

Doxygen documentation
---------------------
- Doxyfile : Project file to configure Doxygen.

- doxy-defaults.conf : Default values for all Doxygen configuration parameters.

- doxy-header.html : Template file for header of Doxygen-generated HTML pages.

- doxy-footer.html : Template file for footer of Doxygen-generated HTML pages.

- doxy-style.css : CSS style sheet for Doxygen-generated HTML pages.

- Build-Doxygen.ps1 : Windows PowerShell script to create the Doxygen
  documentation. On Unix systems, use "make doxygen" at project root level.
