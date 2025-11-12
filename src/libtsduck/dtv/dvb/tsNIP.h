//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup libtsduck mpeg
//!  Generic DVB-NIP (Native IP) definitions.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsIPSocketAddress.h"

namespace ts {
    //!
    //! DVB-NIP signalling TSI Transport Session Identifier (LCT) value.
    //! @see ETSI TS 103 876, section 8.2.2
    //!
    constexpr uint8_t NIP_SIGNALLING_TSI = 0;

    //!
    //! DVB-NIP signalling UDP port.
    //! @see ETSI TS 103 876, section 8.2.2
    //!
    constexpr uint16_t NIP_SIGNALLING_PORT = 3937;

    //!
    //! Get the DVB-NIP signalling IPv4 address and port (224.0.23.14, UDP port 3937).
    //! @return A constant reference to the DVB-NIP signalling IPv4address.
    //! @see ETSI TS 103 876, section 8.2.2
    //!
    TSDUCKDLL const IPSocketAddress& NIPSignallingAddress4();

    //!
    //! Get the DVB-NIP signalling IPv6 address and port (FF0X:0:0:0:0:0:0:12D, UDP port 3937).
    //! @return A constant reference to the DVB-NIP signalling IPv6 address.
    //! @see ETSI TS 103 876, section 8.2.2
    //!
    TSDUCKDLL const IPSocketAddress& NIPSignallingAddress6();
}
