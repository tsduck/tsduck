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

TS_BLOCK_CIPHER_DEFINE_PROPERTIES(ts::IDSA, IDSA, (DVS042<AES128>::PROPERTIES(), u"ATIS-IDSA", iv_zero, sizeof(iv_zero)));

ts::IDSA::IDSA() : DVS042<AES128>(IDSA::PROPERTIES(), true)
{
}

ts::IDSA::~IDSA()
{
}
