//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsLibTSCoreVersion.h"

// Exported version of the TSCore library.
// The names of these symbols are constant, their values are not.
const int tscoreLibraryVersionMajor = TS_VERSION_MAJOR;
const int tscoreLibraryVersionMinor = TS_VERSION_MINOR;
const int tscoreLibraryVersionCommit = TS_COMMIT;

// Exported symbol, the name of which depends on the TSDuck version.
// When an executable or shared library references these symbols, it is guaranteed that a
// compatible TSDuck library is activated. Otherwise, the dynamic references would have failed.
// Only the symbol names matter, the value is just unimportant.
const int LIBTSCORE_VERSION_SYMBOL = TS_VERSION_INTEGER;
