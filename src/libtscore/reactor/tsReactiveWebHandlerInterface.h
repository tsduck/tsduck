//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  All interface classes which are used as WebRequest handlers in a Reactor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsByteBlock.h"
#include "tsObject.h"

namespace ts {

    class ReactiveWebRequest;

    //!
    //! Interface class for WebRequest Reactor handlers.
    //! All methods are empty by default. An application may implement the required ones only.
    //! @ingroup libtscore reactor
    //!
    class TSCOREDLL ReactiveWebHandlerInterface
    {
        TS_INTERFACE(ReactiveWebHandlerInterface);
    public:
        //!
        //! Handle the completion of a Web request open operation.
        //! @param [in,out] request Web request for which the handler is invoked.
        //! @param [in] error_code System-specific error code, SYS_SUCCESS on success.
        //! @param [in] user_data The user-data shared pointer which was passed to startOpen().
        //!
        virtual void handleWebOpen(ReactiveWebRequest& request, int error_code, const ObjectPtr& user_data);

        //!
        //! Handle the reception of data from a Web request.
        //! @param [in,out] request Web request for which the handler is invoked.
        //! @param [in] data Safe pointer to the received data. Can be null in case of error.
        //! The handler may safely copy the shared pointer, the pointed data will not be modified by the ReactiveWebRequest.
        //! @param [in] error_code System-specific error code, including:
        //! - SYS_SUCCESS on success.
        //! - SYS_CANCELED when the transfer was aborted by the application.
        //! - SYS_EOF at end of file or if the other end of a communication link has disconnected,
        //! - SYS_ERROR in case of unknown error.
        //! @param [in] user_data The user-data shared pointer which was passed to startOpen().
        //!
        virtual void handleWebReceive(ReactiveWebRequest& request, const ByteBlockPtr& data, int error_code, const ObjectPtr& user_data);
    };
}
