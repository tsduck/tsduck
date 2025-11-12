//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNIP.h"


//----------------------------------------------------------------------------
// Get the DVB-NIP signalling IPv4 address.
//----------------------------------------------------------------------------

const ts::IPSocketAddress& ts::NIPSignallingAddress4()
{
    // IPv4: 224.0.23.14, UDP port: 3937, see ETSI TS 103 876, section 8.2.2
    static const IPSocketAddress data {224, 0, 23, 14, NIP_SIGNALLING_PORT};
    return data;
}

const ts::IPSocketAddress& ts::NIPSignallingAddress6()
{
    // IPv6: FF0X:0:0:0:0:0:0:12D, UDP port: 3937, see ETSI TS 103 876, section 8.2.2
    // TODO: "FF0X" is incorrect, using FF00 for now.
    static const IPSocketAddress data {0xFF00, 0, 0, 0, 0, 0, 0, 0x012D, NIP_SIGNALLING_PORT};
    return data;
}
