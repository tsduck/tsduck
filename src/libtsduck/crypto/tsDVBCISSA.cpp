//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDVBCISSA.h"

namespace {
    // The IV is defined by the standard and not modifiable.
    const uint8_t ivs[16] = {0x44, 0x56, 0x42, 0x54, 0x4d, 0x43, 0x50, 0x54, 0x41, 0x45, 0x53, 0x43, 0x49, 0x53, 0x53, 0x41};
}

const ts::BlockCipherProperties& ts::DVBCISSA::Properties()
{
    // Thread-safe init-safe static data pattern:
    static const BlockCipherProperties props(CBC<AES128>::Properties(), u"DVB-CISSA", ivs, sizeof(ivs));
    return props;
}

ts::DVBCISSA::DVBCISSA() : CBC<AES128>(DVBCISSA::Properties())
{
}

ts::DVBCISSA::~DVBCISSA()
{
}
