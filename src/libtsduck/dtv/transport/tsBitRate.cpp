//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsBitRate.h"
#include "tsFeatures.h"

// Exported symbol, the name of which depends on the BitRate implementation.
// When an executable or shared library references these symbols, it is guaranteed that a
// compatible TSDuck library is activated. Otherwise, the dynamic references would have failed.
// Only the symbol names matter, the value is just unimportant.
const int TSDUCK_LIBRARY_BITRATE_SYMBOL = 0;


//----------------------------------------------------------------------------
// Register a description of bitrates for the --version options
//----------------------------------------------------------------------------

TS_REGISTER_FEATURE(u"bitrate", u"Bitrate", ts::Features::ALWAYS, ts::GetBitRateDescription);

ts::UString ts::GetBitRateDescription()
{
    return ts::BitRate().description();
}
