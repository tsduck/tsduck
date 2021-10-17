This directory contains all source code for the TSDuck library.

All header files are part of the public API of the library, with a few
exceptions (see below). They are installed with the TSDuck development
environment.

Main subdirectories:

  - base   : Base C++ library, data structures, system support.
  - crypto : Cryptographic code, mostly extracted from LibTomCrypt.
  - dtv    : Digital TV classes, transport streams, signalization, tuning.
  - plugin : Integration of plugins in TSDuck.

These directories may contain additional subdirectories, either for
functional classification or for system-specific purpose. In the latter
case, the standard subdirectory naming is the following:

  - private : Internal to the library, not part of the public interface.
  - windows : Windows-specific.
  - unix    : Unix-specific, all Unix platforms including Linux and macOS.
  - linux   : Linux-specific.
  - mac     : macOS-specific.

If you add or remove source files in this directory, a few project files shall
be updated. Do not update them manually, use one of the following scripts:

  - Windows: scripts/build-tsduck-header.ps1
  - Unix:    scripts/build-tsduck-header.sh

The following files are rebuilt:

  - src/libtsduck/tsduck.h
