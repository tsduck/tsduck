//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        std::uintmax_t getSize() const { return _file_size; }

    private:
        friend class PollFiles;

        UString        _name {};          // File name
        Status         _status {ADDED};   // Status since last report
        std::uintmax_t _file_size = 0;    // File size in bytes
        Time           _file_date {};     // Last file modification date (UTC)
        bool           _pending = true;   // Not yet notified, waiting for stable state
        Time           _found_date {};    // First time (UTC) this size/date state was reported

        // Constructor
        PolledFile(const UString& name, const int64_t& size, const Time& date, const Time& now);

        // Check if file has changed size or date. If yes, return to pending state.
        void trackChange(const std::uintmax_t& size, const Time& date, const Time& now);
    };

    //!
    //! Safe pointer to a PolledFile (not thread-safe).
    //!
    typedef SafePtr<PolledFile, ts::null_mutex> PolledFilePtr;

    //!
    //! List of safe pointers to PolledFile (not thread-safe).
    //!
    typedef std::list<PolledFilePtr> PolledFileList;
}
