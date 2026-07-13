//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Interface class for reactive worker handlers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsObject.h"

namespace ts {

    class ReactiveWorkerPool;

    //!
    //! Interface class for reactive worker handlers.
    //! This interface shall be implemented by classes which delegate lengthy tasks to worker threads.
    //! @ingroup libtscore reactor
    //!
    class TSCOREDLL ReactiveWorkerHandlerInterface
    {
        TS_INTERFACE(ReactiveWorkerHandlerInterface);
    public:
        //!
        //! Handle the end of a lengthy task which was delegated to a worker thread.
        //! @param [in,out] pool Worker pool in the context of which the handler is invoked.
        //! @param [in] error_code Application-specific error code, as returned by executeWork(), SYS_SUCCESS on success.
        //! @param [in] user_data Application-specific shared pointer which was passed to the worker.
        //! @see ReactiveWorkerInterface.
        //!
        virtual void handleWorkerCompletion(ReactiveWorkerPool& pool, int error_code, const ObjectPtr& user_data) = 0;
    };
}
