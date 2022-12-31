//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//!  Description of a polled file.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsEnumeration.h"
#include "tsSafePtr.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Description of a polled file.
    //! @ingroup system
    //!
    class TSDUCKDLL PolledFile
    {
    public:
        //!
        //! Status of a polled file.
        //!
        enum Status {
            MODIFIED,  //!< The file was modified since last time.
            ADDED,     //!< The file was added since last time.
            DELETED    //!< The file was deleted since last time.
        };

        //!
        //! Enumeration names for Status.
        //!
        static const ts::Enumeration StatusEnumeration;

        //!
        //! Get the file name.
        //! @return The file name.
        //!
        const UString& getFileName() const { return _name; }

        //!
        //! Get file status since last notification.
        //! @return The file status since last notification.
        //!
        Status getStatus() const { return _status; }

        //!
        //! Check if the file has been updated (created or modified) since last notification.
        //! @return True if the file has been updated.
        //!
        bool updated() const { return _status == MODIFIED || _status == ADDED; }

        //!
        //! Check if the file has been deleted since last notification.
        //! @return True if the file has been deleted.
        //!
        bool deleted() const { return _status == DELETED; }

        //!
        //! Get file size in bytes.
        //! @return The file size in bytes.
        //!
        int64_t getSize() const { return _file_size; }

    private:
        friend class PollFiles;

        UString _name;           // File name
        Status  _status;         // Status since last report
        int64_t _file_size;      // File size in bytes
        Time    _file_date;      // Last file modification date (UTC)
        bool    _pending;        // Not yet notified, waiting for stable state
        Time    _found_date;     // First time (UTC) this size/date state was reported

        // Constructor
        PolledFile(const UString& name, const int64_t& size, const Time& date, const Time& now);

        // Check if file has changed size or date. If yes, return to pending state.
        void trackChange(const int64_t& size, const Time& date, const Time& now);
    };

    //!
    //! Safe pointer to a PolledFile (not thread-safe).
    //!
    typedef SafePtr<PolledFile, NullMutex> PolledFilePtr;

    //!
    //! List of safe pointers to PolledFile (not thread-safe).
    //!
    typedef std::list<PolledFilePtr> PolledFileList;
}
