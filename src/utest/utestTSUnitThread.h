//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Thread wrapper for TSUnit.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsThread.h"

namespace utest {
    //!
    //! TSUnit wrapper for thread main code.
    //!
    //! TSUnit is not designed for multi-threading. Any assertion failure in a thread
    //! produces unspecified results, typically a crash of the application, and there
    //! is no error message about the failing display. This class is a wrapper
    //! around the main code of a thread. In case of assertion failure, a TSUnit
    //! error is displayed and the application properly exits.
    //!
    class TSUnitThread : public ts::Thread
    {
        TS_NOCOPY(TSUnitThread);
    public:
        //!
        //! Default constructor.
        //!
        TSUnitThread() = default;

        //!
        //! Destructor.
        //!
        virtual ~TSUnitThread() override;

        //!
        //! Constructor from specified attributes.
        //! @param [in] attributes The set of attributes.
        //!
        TSUnitThread(const ts::ThreadAttributes& attributes);

        //!
        //! Actual test code (thread main code).
        //!
        virtual void test() = 0;

    protected:
        // Implementation of thread interface.
        virtual void main() override;
    };
}
