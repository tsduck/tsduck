//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Interface class for StreamInterface Reactor handlers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsByteBlock.h"
#include "tsObject.h"

namespace ts {

    class ReactiveStream;
    class ReactiveInputControl;

    //!
    //! Interface class for StreamInterface Reactor handlers.
    //! All methods are empty by default. An application may implement the required ones only.
    //! @ingroup libtscore reactor
    //!
    class TSCOREDLL ReactiveStreamHandlerInterface
    {
        TS_INTERFACE(ReactiveStreamHandlerInterface);
    public:
        //!
        //! Handle the completion of a write operation on a stream.
        //! @param [in,out] stream Stream device for which the handler is invoked.
        //! @param [in] error_code System-specific error code, including:
        //! - SYS_SUCCESS on success.
        //! - SYS_EOF if the handler is called as a completion of "close write" or "send EOF", whatever it means for the stream device.
        //! - SYS_CANCELED if the I/O was canceled before completion.
        //! - SYS_ERROR in case of unknown error.
        //! @param [in] user_data The user-data shared pointer which was passed to startWriteStream().
        //!
        virtual void handleWriteStream(ReactiveStream& stream, int error_code, const ObjectPtr& user_data);

        //!
        //! Handle the reception of data from a read operation on a stream.
        //! @param [in,out] stream Stream device for which the handler is invoked.
        //! @param [in] data Received data.
        //! @param [in,out] control Input control data that the handler may update according to the usage of @a data.
        //! @param [in] error_code System-specific error code, including:
        //! - SYS_SUCCESS on success.
        //! - SYS_EOF at end of file or if the other end of a communication link has disconnected,
        //! - SYS_ERROR in case of unknown error.
        //! @param [in] user_data The user-data shared pointer which was passed to startReadStream().
        //!
        virtual void handleReadStream(ReactiveStream& stream, const ByteBlock& data, ReactiveInputControl& control, int error_code, const ObjectPtr& user_data);
    };
}
