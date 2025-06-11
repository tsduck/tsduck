//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsLibTSDuckVersion.h"
#include "tsCerrReport.h"

TS_CERR_DEBUG(u"libtsduck loaded");

// Exported version of the TSDuck library.
// The names of these symbols are constant, their values are not.
const int tsduckLibraryVersionMajor = TS_VERSION_MAJOR;
const int tsduckLibraryVersionMinor = TS_VERSION_MINOR;
const int tsduckLibraryVersionCommit = TS_COMMIT;

// Exported symbol, the name of which depends on the TSDuck version.
// When an executable or shared library references these symbols, it is guaranteed that a
// compatible TSDuck library is activated. Otherwise, the dynamic references would have failed.
// Only the symbol names matter, the value is just unimportant.
const int LIBTSDUCK_VERSION_SYMBOL = TS_VERSION_INTEGER;
