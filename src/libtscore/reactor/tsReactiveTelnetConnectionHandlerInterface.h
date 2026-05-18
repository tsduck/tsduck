//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Interface class for Telnet connection Reactor handlers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {

    class ReactiveTelnetConnection;

    //!
    //! Interface class for Telnet connection Reactor handlers.
    //! An application shall use ReactiveTCPConnectionHandlerInterface for the non-Telnet parts of the connection.
    //!
    class TSCOREDLL ReactiveTelnetConnectionHandlerInterface
    {
        TS_INTERFACE(ReactiveTelnetConnectionHandlerInterface);
    public:
        //!
        //! Handle the reception of one Telnet line.
        //! @param [in,out] sock TCP socket for which the handler is invoked.
        //! @param [in] line Received text line, without end-of-line markers.
        //! @param [in] error_code System-specific error code, SYS_SUCCESS on success, SYS_EOF if the peer has disconnected,
        //! SYS_ERROR in case of unknown error.
        //!
        virtual void handleTelnetLine(ReactiveTelnetConnection& sock, const UString& line, int error_code) = 0;
    };
}
