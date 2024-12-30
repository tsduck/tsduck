//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIDSA.h"

namespace {
    // The IV is defined by the standard and not modifiable.
    const uint8_t iv_zero[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
}

const ts::BlockCipherProperties& ts::IDSA::Properties()
{
    // Thread-safe init-safe static data pattern:
    static const BlockCipherProperties props(DVS042<AES128>::Properties(), u"ATIS-IDSA", iv_zero, sizeof(iv_zero));
    return props;
}

ts::IDSA::IDSA() : DVS042<AES128>(IDSA::Properties(), true)
{
}

ts::IDSA::~IDSA()
{
}
