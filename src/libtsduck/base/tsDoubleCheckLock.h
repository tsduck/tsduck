//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Definition of a "double check lock" as defined in ACE.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsMutex.h"
#include "tsGuard.h"

namespace ts {

    //!
    //! The class DoubleCheckLock implements the <i>double check locking</i> design pattern.
    //! @ingroup thread
    //!
    //! The <i>double check locking</i> design pattern is used to protect shared data with
    //! the following characteristics:
    //!
    //! - One single reader thread.
    //! - The data are frequently read.
    //! - One (or more) writer thread.
    //! - The data are much less frequently written that read.
    //!
    //! The idea is that the reader thread uses its own private copy of
    //! the protected data and uses it without locking. At predefined
    //! points, the reader checks if the data have changed. When this is
    //! the case, a new copy is fetched under the protection of a mutex.
    //!
    //! The writer threads always update the data under the protection
    //! of the mutex.
    //!
    //! This method is slightly more complicated than using a mutex for
    //! each shared data access sequence but it significantly reduces
    //! the synchronization overhead, provided that the above conditions
    //! are met.
    //!
    //! The following is a usage template.
    //!
    //! <h3>Shared data</h3>
    //!
    //! @code
    //! SomeDataType data;           // the data to protect
    //! ts::DoubleCheckLock lock;  // associated lock
    //! @endcode
    //!
    //! <h3>Writer threads (not so often)</h3>
    //! @code
    //! {
    //!     // Acquire lock as writer
    //!     ts::DoubleCheckLock::Writer guard(lock);
    //!
    //!     // Modify data
    //!     data = ....
    //! }
    //! @endcode
    //!
    //! <h3>Reader thread</h3>
    //! @code
    //! SomeDataType copy;   // Private copy of data (or part of it)
    //!
    //! if (lock.changed()) {
    //!     // Acquire lock as reader
    //!     ts::DoubleCheckLock::Reader guard(lock);
    //!
    //!     // Get a copy of data
    //!     copy = data;
    //! }
    //!
    //! // Then, use "copy" (private to reader thread) instead of "data"
    //! @endcode
    //!
    class TSDUCKDLL DoubleCheckLock
    {
        TS_NOCOPY(DoubleCheckLock);
    public:
        //!
        //! Default constructor.
        //!
        DoubleCheckLock();

        //!
        //! Check if the shared data have been marked as "changed".
        //!
        //! This method is invoked in the reader thread to check if the
        //! data have been modified by a writer thread.
        //!
        //! @return True if a writer thread has modified the data since
        //! the last time a DoubleCheckLock::Reader has been used, false
        //! otherwise.
        //!
        inline bool changed() const {return _changed;}

        //!
        //! Guard class for writer threads.
        //!
        class TSDUCKDLL Writer : private Guard
        {
            TS_NOBUILD_NOCOPY(Writer);
        public:
            //!
            //! Default constructor, acquire the mutex and mark data as "changed".
            //!
            //! @param [in,out] lock Associated DoubleCheckLock.
            //!
            Writer(DoubleCheckLock& lock);

            //!
            //! Virtual destructor
            //!
            virtual ~Writer();
        };

        //!
        //! Guard class for the reader thread.
        //!
        class TSDUCKDLL Reader : private Guard
        {
            TS_NOBUILD_NOCOPY(Reader);
        public:
            //!
            //! Default constructor, acquire the mutex and clear "changed" state for data.
            //!
            //! @param [in,out] lock Associated DoubleCheckLock.
            //!
            Reader(DoubleCheckLock& lock);

            //!
            //! Virtual destructor
            //!
            virtual ~Reader();
        };

    private:
        Mutex _mutex;
        volatile bool _changed;
    };
}
