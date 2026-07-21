//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Interface class for line-oriented Telnet-like connection Reactor handlers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {

    class ReactiveTextStream;

    //!
    //! Interface class for line-oriented Telnet-like connection Reactor handlers.
    //! With a ReactiveTCPConnection, an application shall use ReactiveTCPConnectionHandlerInterface for
    //! the non-line-oriented parts of the connection (connection, close, etc).
    //! @ingroup libtscore reactor
    //!
    class TSCOREDLL ReactiveTextStreamHandlerInterface
    {
        TS_INTERFACE(ReactiveTextStreamHandlerInterface);
    public:
        //!
        //! Handle the reception of one text line.
        //! @param [in,out] stream Reactive stream for which the handler is invoked.
        //! @param [in] line Received text line, without end-of-line markers.
        //! @param [in] error_code System-specific error code, including:
        //! - SYS_SUCCESS on success.
        //! - SYS_EOF on end of input or if the peer has disconnected,
        //! - SYS_ERROR in case of unknown error.
        //!
        virtual void handleTextLine(ReactiveTextStream& stream, const UString& line, int error_code) = 0;
    };
}
