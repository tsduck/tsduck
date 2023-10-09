//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Version identification of TSDuck as strings.
//!
//----------------------------------------------------------------------------

#pragma once

#include "tsPlatform.h"
#include "tsVersion.h"

//!
//! Define the TSDuck version as an 8-bit string literal.
//!
#define TS_VERSION_STRING TS_STRINGIFY(TS_VERSION_MAJOR) "." TS_STRINGIFY(TS_VERSION_MINOR) "-" TS_STRINGIFY(TS_COMMIT)

//!
//! Define the TSDuck version as a 16-bit string literal.
//!
#define TS_VERSION_USTRING TS_USTRINGIFY(TS_VERSION_MAJOR) u"." TS_USTRINGIFY(TS_VERSION_MINOR) u"-" TS_USTRINGIFY(TS_COMMIT)

//!
//! Define the TSDuck version as an integer, suitable for comparisons.
//!
#define TS_VERSION_INTEGER ((TS_VERSION_MAJOR * 10000000) + (TS_VERSION_MINOR * 100000) + TS_COMMIT)
