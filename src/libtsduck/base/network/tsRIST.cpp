//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsRIST.h"
#include "tsLibRIST.h"


//----------------------------------------------------------------------------
// Get the version of the RIST library.
//----------------------------------------------------------------------------

ts::UString ts::GetRISTLibraryVersion()
{
#if defined(TS_NO_RIST)
    return u"This version of TSDuck was compiled without RIST support";
#else
    return UString::Format(u"librist version %s, API version %s", {::librist_version(), ::librist_api_version()});
#endif
}
