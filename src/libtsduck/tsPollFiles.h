//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//!  Poll for files
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsCerrReport.h"
#include "tsEnumeration.h"
#include "tsSafePtr.h"
#include "tsTime.h"

namespace ts {

    // Description of a polled file
    class TSDUCKDLL PolledFile
    {
    public:
        // Status of a polled file
        enum Status {MODIFIED, ADDED, DELETED};

        // Analysis of enum values
        static const ts::Enumeration StatusEnumeration;

        // Get file name
        const std::string& getFileName() const {return _name;}
        
        // Get file status since last notification
        Status getStatus() const {return _status;}
        
    private:
        friend class PollFiles;

        const std::string _name; // File name
        Status _status;          // Status since last report
        int64_t  _file_size;       // File size in bytes
        Time   _file_date;       // Last file modification date (UTC)
        bool   _pending;         // Not yet notified, waiting for stable state
        Time   _found_date;      // First time (UTC) this size/date state was reported

        // Constructor
        PolledFile (const std::string& name, const int64_t& size, const Time& date, const Time& now);

        // Check if file has changed size or date. If yes, return to pending state.
        void trackChange (const int64_t& size, const Time& date, const Time& now);
    };

    typedef SafePtr<PolledFile, NullMutex> PolledFilePtr;
    typedef std::list<PolledFilePtr> PolledFileList;

    // Interface for classes listening for file modification
    class TSDUCKDLL PollFilesListener
    {
    public:
        // Invoked when files have changed.
        // The entries in the list are sorted by file names.
        // Must return true to continue polling, false to exit PollFiles().
        virtual bool handlePolledFiles (const PolledFileList&) = 0;

        // Invoked before each poll to give the opportunity to change
        // where and how the files are polled. This is an optional feature,
        // the default implementation does not change anything.
        // Must return true to continue polling, false to exit PollFiles().
        virtual bool updatePollFiles (std::string& wildcard, MilliSecond& poll_interval, MilliSecond& min_stable_delay) {return true;}

        // Virtual destructor
        virtual ~PollFilesListener() {}
    };
    
    // To poll files for modification, we need a method like PollFiles(...).
    // For technical reasons, we need a class. The method PollFiles(...) is
    // actually a constructor, but we do not need to know...
    class TSDUCKDLL PollFiles
    {
    public:
        // Constructor, acting as pseudo-method PollFiles(...).
        // Invoke the listener each time something has changed.
        // The first time, all files are reported as "added".
        PollFiles (const std::string& wildcard,  // Wildcard specification of files to poll
                   MilliSecond poll_interval,    // Between two poll operations
                   MilliSecond min_stable_delay, // File size needs to be stable during that
                   PollFilesListener&,           // Invoked on file modification
                   ReportInterface& = CERR);     // For debug messages

    private:
        std::string        _files_wildcard;
        ReportInterface&   _report;
        PollFilesListener& _listener;
        PolledFileList     _polled_files;   // Updated at each poll, sorted by file name
        PolledFileList     _notified_files; // Modifications to notify

        // Mark a file as deleted, move from polled to notified files.
        void deleteFile (PolledFileList::iterator&);

        // Notify listener. Return true to continue polling, false to exit PollFiles().
        bool notifyListener();

        // Unaccessible operations
        PollFiles() = delete;
        PollFiles(const PollFiles&) = delete;
    };
}
