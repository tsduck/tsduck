==========================================
TSDuck - The MPEG Transport Stream Toolkit
==========================================

Abstract:

  TSDuck provides some simple utilities to process MPEG Transport Streams (TS),
  either as recorded files or live streams. The structure of an MPEG TS is
  defined in ISO 13818-1.

  Unless specified otherwise, the TS files are binary files containing adjacent
  188-byte TS packets. Use the utility tsresync if a captured file is corrupted
  or contains 204-byte packets.

  Through tsp, the Transport Stream Processor, many types of analysis and
  transformation can be applied on live or recorded transport streams.
  This utility can be extended through plugins. Existing plugins can be
  enhanced and new plugins can be developed using a library of C++ classes.

Usage:

  All utilities accept the options --help and --version.
  Using --help, each utility displays its syntax and exits.
  Using --version, it displays the TSDuck version and exits.

Building:

  TSDuck can be built on Windows and Linux systems.
  See file BUILDING.txt for details.

License:

  TSDuck is distributed under the terms of the Simplified BSD License.
  See file LICENSE.txt for details.

Copyright:

  Copyright (c) 2005-2017, Thierry Lelegard
  All rights reserved.
