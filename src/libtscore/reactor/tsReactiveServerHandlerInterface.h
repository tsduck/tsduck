//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Interface class for ReactiveServer handlers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsObject.h"

namespace ts {

    class ReactiveServer;

    //!
    //! Interface class for ReactiveServer handlers.
    //! All methods are empty by default. An application may implement the required ones only.
    //!
    class TSCOREDLL ReactiveServerHandlerInterface
    {
        TS_INTERFACE(ReactiveServerHandlerInterface);
    public:
        //!
        //! Handle the termination of the reactive server.
        //! @param [in,out] server Reactive server for which the handler is invoked.
        //! @param [in] user_data The user-data shared pointer which was passed to startConnect().
        //!
        virtual void handleServerExited(ReactiveServer& server, const ObjectPtr& user_data);
    };
}
