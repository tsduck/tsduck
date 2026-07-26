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
#include "tsObject.h"
#include "tsUString.h"

namespace ts {

    class ReactiveTextStream;

    //!
    //! Interface class for line-oriented Telnet-like connection Reactor handlers.
    //! With a ReactiveTCPConnection, an application shall use ReactiveTCPConnectionHandlerInterface for
    //! the non-line-oriented parts of the connection (connection, close, etc).
    //! All methods are empty by default. An application may implement the required ones only.
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
        //! @param [in] user_data The user-data shared pointer which was passed to startReadText().
        //!
        virtual void handleTextLine(ReactiveTextStream& stream, const UString& line, int error_code, const ObjectPtr& user_data);

        //!
        //! Handle the termination of a write operation.
        //! @param [in,out] stream Reactive stream for which the handler is invoked.
        //! @param [in] text Sent text. This is the raw sent text, with embedded end-of-lines and UTF-8 translation of UString lines.
        //! When @a flush was set to false in some start-write operations, the @a text contains the concatenation of all previous
        //! unflushed operations.
        //! @param [in] error_code System-specific error code, including:
        //! - SYS_SUCCESS on success.
        //! - SYS_EOF if the handler is called as a completion of "close write" or "send EOF", whatever it means for the stream device.
        //! - SYS_CANCELED if the I/O was canceled before completion.
        //! - SYS_ERROR in case of unknown error.
        //! @param [in] user_data The user-data shared pointer which was passed to startWriteLine() or startWriteText().
        //!
        virtual void handleWriteText(ReactiveTextStream& stream, const std::string& text, int error_code, const ObjectPtr& user_data);
    };
}
