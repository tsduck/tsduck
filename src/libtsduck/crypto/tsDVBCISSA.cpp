//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDVBCISSA.h"

namespace {
    // The IV is defined by the standard and not modifiable.
    const uint8_t ivs[16] = {0x44, 0x56, 0x42, 0x54, 0x4d, 0x43, 0x50, 0x54, 0x41, 0x45, 0x53, 0x43, 0x49, 0x53, 0x53, 0x41};
}

TS_BLOCK_CIPHER_DEFINE_PROPERTIES(ts::DVBCISSA, DVBCISSA, (CBC<AES128>::PROPERTIES(), u"DVB-CISSA", ivs, sizeof(ivs)));

ts::DVBCISSA::DVBCISSA() : CBC<AES128>(DVBCISSA::PROPERTIES())
{
}

ts::DVBCISSA::~DVBCISSA()
{
}
