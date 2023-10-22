//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDVBCISSA.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::DVBCISSA::DVBCISSA() :
    CBC<AES>()
{
    // The IV is defined by the standard and not modifiable.
    static const uint8_t ivs[16] = {0x44, 0x56, 0x42, 0x54, 0x4d, 0x43, 0x50, 0x54, 0x41, 0x45, 0x53, 0x43, 0x49, 0x53, 0x53, 0x41};
    setIV(ivs, sizeof(ivs));
}


//----------------------------------------------------------------------------
// Simple virtual methods.
//----------------------------------------------------------------------------

ts::UString ts::DVBCISSA::name() const
{
    return u"DVB-CISSA";
}

bool ts::DVBCISSA::setIV(const void* iv_, size_t iv_length)
{
    // The IV is defined by the standard and not modifiable.
    // This method is hidden (private) but redirected to its super class.
    return CBC<AES>::setIV(iv_, iv_length);
}
