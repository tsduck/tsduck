//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSDuck Python bindings: information features.
//
//----------------------------------------------------------------------------

#include "tspy.h"
#include "tsVersionInfo.h"

TSDUCKPY uint32_t tspyVersionInteger()
{
    return TS_VERSION_INTEGER;
}

TSDUCKPY void tspyVersionString(uint8_t* buffer, size_t* size)
{
    ts::py::FromString(ts::VersionInfo::GetVersion(ts::VersionInfo::Format::SHORT), buffer, size);
}
