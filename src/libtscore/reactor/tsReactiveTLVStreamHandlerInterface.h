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
#include "tsObject.h"
#include "tstlvMessage.h"

namespace ts {

    class ReactiveTLVStream;

    //!
    //! Interface class for TLV-messages connection Reactor handlers.
    //! An application shall use ReactiveTCPConnectionHandlerInterface for the non-TLV parts of the connection.
    //! All methods are empty by default. An application may implement the required ones only.
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
        //! @param [in] error_code System-specific error code, including:
        //! - SYS_SUCCESS on success.
        //! - SYS_EOF on end of input or if the peer has disconnected,
        //! - SYS_ERROR in case of unknown error.
        //! @param [in] user_data The user-data shared pointer which was passed to startReceive().
        //!
        virtual void handleReceiveMessage(ReactiveTLVStream& sock, const tlv::MessagePtr& msg, int error_code, const ObjectPtr& user_data);

        //!
        //! Handle the termination of message send operation.
        //! @param [in,out] sock TCP socket for which the handler is invoked.
        //! @param [in] msg Pointer the serialized content of the message. Can be null in case of error.
        //! @param [in] error_code System-specific error code, including:
        //! - SYS_SUCCESS on success.
        //! - SYS_EOF if the handler is called as a completion of "close write" or "send EOF", whatever it means for the stream device.
        //! - SYS_CANCELED if the I/O was canceled before completion.
        //! - SYS_ERROR in case of unknown error.
        //! @param [in] user_data The user-data shared pointer which was passed to startSendMessage().
        //!
        virtual void handleSendMessage(ReactiveTLVStream& sock, const ByteBlockPtr& msg, int error_code, const ObjectPtr& user_data);
    };
}
