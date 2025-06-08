//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsRIST.h"
#include "tsLibRIST.h"
#include "tsFeatures.h"


//----------------------------------------------------------------------------
// Get the version of the RIST library.
//----------------------------------------------------------------------------

ts::UString ts::GetRISTLibraryVersion()
{
#if defined(TS_NO_RIST)
    return u"This version of TSDuck was compiled without RIST support";
#else
    return UString::Format(u"librist version %s, API version %s", ::librist_version(), ::librist_api_version());
#endif
}


//----------------------------------------------------------------------------
// Register for options --version and --support.
//----------------------------------------------------------------------------

#if defined(TS_NO_RIST)
    #define SUPPORT ts::Features::UNSUPPORTED
#else
    #define SUPPORT ts::Features::SUPPORTED
#endif

TS_REGISTER_FEATURE(u"rist", u"RIST library", SUPPORT, ts::GetRISTLibraryVersion);
