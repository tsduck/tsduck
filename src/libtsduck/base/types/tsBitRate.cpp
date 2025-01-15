//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsBitRate.h"
#include "tsVersionInfo.h"


//----------------------------------------------------------------------------
// Register a description of bitrates for the --version options
//----------------------------------------------------------------------------

TS_REGISTER_FEATURE(u"bitrate", u"Bitrate", ALWAYS, ts::GetBitRateDescription);

ts::UString ts::GetBitRateDescription()
{
    return ts::BitRate().description();
}
