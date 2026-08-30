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
        //! Handle the completion of a Web request receive operation.
        //! @param [in,out] request Web request for which the handler is invoked.
        //! @param [out] buffer Address of the buffer for the received data.
        //! @param [in] size Size in bytes of the received data.
        //! @param [in] error_code System-specific error code, including:
        //! - SYS_SUCCESS on success.
        //! - SYS_CANCELED when the transfer was aborted by the application.
        //! - SYS_EOF at end of file or if the other end of a communication link has disconnected,
        //! - SYS_ERROR in case of unknown error.
        //! @param [in] user_data The user-data shared pointer which was passed to startOpen().
        //!
        virtual void handleWebReceive(ReactiveWebRequest& request, void* buffer, size_t size, int error_code, const ObjectPtr& user_data);
    };
}
