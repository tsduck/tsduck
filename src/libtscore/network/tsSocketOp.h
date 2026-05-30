//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Identification of various socket operations.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsNames.h"

namespace ts {
    //!
    //! Identification of various socket operations.
    //! These enumeration values can be used for debugging.
    //! @ingroup net
    //!
    enum class SocketOp {
        NONE,          //!< No operation.
        OPEN,          //!< Open socket.
        SET_OPTION,    //!< Set socket option.
        GET_OPTION,    //!< Get socket option.
        CONNECT,       //!< TCP client connection.
        LISTEN,        //!< TCP server listening for client.
        ACCEPT,        //!< TCP server accepting client connection.
        SEND,          //!< Send data.
        RECEIVE,       //!< Receive data.
        CLOSE_WRITER,  //!< TCP close write side (aka shutdown).
        DISCONNECT,    //!< TCP disconnect.
        CLOSE,         //!< Close socket.
    };

    //!
    //! Enumeration description of ts::SocketOp with meaningful names.
    //! @return A constant reference to the enumeration description.
    //!
    TSDUCKDLL const Names& SocketOpNames();
}
