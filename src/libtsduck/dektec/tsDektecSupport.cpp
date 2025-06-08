//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------

#include "tsDektecSupport.h"
#include "tsFeatures.h"

// Register the Dektec feature as living in shared library libtsdektec.so / tsdektec.dll.
TS_REGISTER_FEATURE(u"dektec", u"tsdektec");

// Check if this version of TSDuck was built with Dektec support.
bool ts::HasDektecSupport()
{
    return Features::Instance().isSupported(u"dektec");
}
