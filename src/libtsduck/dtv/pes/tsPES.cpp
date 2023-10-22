//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPES.h"


//----------------------------------------------------------------------------
// Check if a SID value indicates a PES packet with long header
//----------------------------------------------------------------------------

bool ts::IsLongHeaderSID(uint8_t sid)
{
    return
        sid != SID_PSMAP &&    // Program stream map
        sid != SID_PAD &&      // Padding stream
        sid != SID_PRIV2 &&    // Private stream 2
        sid != SID_ECM &&      // ECM stream
        sid != SID_EMM &&      // EMM stream
        sid != SID_PSDIR &&    // Program stream directory
        sid != SID_DSMCC &&    // DSM-CC data
        sid != SID_H222_1_E;   // H.222.1 type E
}
