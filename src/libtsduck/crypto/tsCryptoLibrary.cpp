//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCryptoLibrary.h"


//----------------------------------------------------------------------------
// Get the name and version of the underlying cryptographic library.
//----------------------------------------------------------------------------

ts::UString ts::GetCryptographicLibraryVersion()
{
#if defined(TS_WINDOWS)
    // Don't know how to get the version of BCrypt library.
    return u"Microsoft BCrypt";
#else
    return u"@@@@@ TBC";
#endif
}
