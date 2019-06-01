This directory contains all source code for the TSDuck library.

All header files at this level are part of the public API of the library.
They are installed with the TSDuck development environment.

Subdirectories:

  - private : Internal to the library, not part of the public interface.
  - windows : Windows-specific.
  - unix    : Unix-specific, all Unix platforms including Linux and macOS.
  - linux   : Linux-specific.
  - mac     : macOS-specific.

If you add or remove source files in this directory, a few project files shall
be updated. Do not update them manually, use one of the follwing scripts:

  - Windows: build\Build-Project-Files.ps1
  - Unix:    build/build-project-files.sh

The following files are rebuilt:

  - build/msvc/libtsduck-files.props
  - build/msvc/libtsduck-filters.props
  - build/qtcreator/libtsduck/libtsduck-files.pri
  - src/libtsduck/tsduck.h
