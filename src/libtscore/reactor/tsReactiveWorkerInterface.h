//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Interface class for reactive workers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsObject.h"

namespace ts {

    class ReactiveWorkerPool;

    //!
    //! Interface class for reactive workers.
    //!
    class TSCOREDLL ReactiveWorkerInterface
    {
        TS_INTERFACE(ReactiveWorkerInterface);
    public:
        //!
        //! Execute a lengthy task in a worker thread.
        //! @param [in,out] pool Worker pool in the context of which the handler is invoked.
        //! @param [in] user_data Application-specific shared pointer which is passed to the worker.
        //! @return Application-specific error code. By convention, return SYS_SUCCESS on success.
        //!
        virtual int executeWork(ReactiveWorkerPool& pool, const ObjectPtr& user_data) = 0;
    };
}
