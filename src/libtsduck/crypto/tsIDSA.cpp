//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIDSA.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::IDSA::IDSA() :
    DVS042<AES>()
{
    // The IV are defined by the standard and not modifiable.
    static const uint8_t iv_zero[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    setIV(iv_zero, sizeof(iv_zero));
}


//----------------------------------------------------------------------------
// Simple virtual methods.
//----------------------------------------------------------------------------

ts::UString ts::IDSA::name() const
{
    return u"ATIS-IDSA";
}

bool ts::IDSA::setIV(const void* iv_, size_t iv_length)
{
    // The IV are defined by the standard and not modifiable.
    // This method is hidden (private) but redirected to its super class.
    return DVS042<AES>::setIV(iv_, iv_length);
}

bool ts::IDSA::setShortIV(const void* iv_, size_t iv_length)
{
    // The IV are defined by the standard and not modifiable.
    // This method is hidden (private) but redirected to its super class.
    return DVS042<AES>::setShortIV(iv_, iv_length);
}
