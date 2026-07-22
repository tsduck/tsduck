//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Interface class for TLV-messages connection Reactor handlers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tstlvMessage.h"

namespace ts {

    class ReactiveTLVStream;

    //!
    //! Interface class for TLV-messages connection Reactor handlers.
    //! An application shall use ReactiveTCPConnectionHandlerInterface for the non-TLV parts of the connection.
    //! @ingroup libtscore reactor
    //!
    class TSCOREDLL ReactiveTLVStreamHandlerInterface
    {
        TS_INTERFACE(ReactiveTLVStreamHandlerInterface);
    public:
        //!
        //! Handle the reception of one valid message.
        //! @param [in,out] sock TCP socket for which the handler is invoked.
        //! @param [in] msg Pointer to a received decoded message. Can be null in case of error.
        //! @param [in] error_code System-specific error code, SYS_SUCCESS on success, SYS_EOF if the peer has disconnected,
        //! SYS_ERROR in case of unknown error.
        //!
        virtual void handleReceivedMessage(ReactiveTLVStream& sock, const tlv::MessagePtr& msg, int error_code) = 0;
    };
}
