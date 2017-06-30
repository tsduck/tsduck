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
//
//  Poll for files
//
//----------------------------------------------------------------------------

#include "tsPollFiles.h"
#include "tsStringUtils.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Enumerations, names for values
//----------------------------------------------------------------------------

const ts::Enumeration ts::PolledFile::StatusEnumeration
    ("modified", ts::PolledFile::MODIFIED,
     "added",    ts::PolledFile::ADDED,
     "deleted",  ts::PolledFile::DELETED,
     TS_NULL);


//----------------------------------------------------------------------------
// Description of a polled file - Constructor
//----------------------------------------------------------------------------

ts::PolledFile::PolledFile(const std::string& name, const int64_t& size, const Time& date, const Time& now) :
    _name(name),
    _status(ADDED),
    _file_size(size),
    _file_date(date),
    _pending(true),
    _found_date(now)
{
}


//----------------------------------------------------------------------------
// Check if file has changed size or date.
//----------------------------------------------------------------------------

void ts::PolledFile::trackChange(const int64_t& size, const Time& date, const Time& now)
{
    if (_file_size != size || _file_date != date) {
        _status = MODIFIED;
        _file_size = size;
        _file_date = date;
        _pending = true;
        _found_date = now;
    }
}


//----------------------------------------------------------------------------
// This method polls files for modification.
//----------------------------------------------------------------------------

ts::PollFiles::PollFiles(const std::string& wildcard,
                         MilliSecond poll_interval,
                         MilliSecond min_stable_delay,
                         PollFilesListener& listener,
                         ReportInterface& report) :
    _files_wildcard(wildcard),
    _report(report),
    _listener(listener),
    _polled_files(),
    _notified_files()

{
    _report.debug("Starting PollFiles on %s, poll interval = %" FMT_INT64 "d ms, min stable delay = %" FMT_INT64 "d ms",
                  _files_wildcard.c_str(), poll_interval, min_stable_delay);

    StringVector found_files;  // Files that are found at each poll

    // Loop on poll
    while (listener.updatePollFiles(_files_wildcard, poll_interval, min_stable_delay)) {

        // List files, sort according to name
        const Time now(Time::CurrentUTC());
        ExpandWildcard(found_files, _files_wildcard);
        std::sort(found_files.begin(), found_files.end());

        // Compare currently found files with last polled state.
        PolledFileList::iterator polled = _polled_files.begin();
        for (StringVector::const_iterator found = found_files.begin(); found != found_files.end(); ++found) {

            // Get characteristics of next found file
            const std::string& name(*found);
            const int64_t size(GetFileSize(name));
            const Time date(GetFileModificationTimeUTC(name));

            // Remove polled files before the found file
            while (polled != _polled_files.end() && name > (*polled)->_name) {
                deleteFile(polled);
            }

            // Track change in current found file
            if (polled == _polled_files.end() || name < (*polled)->_name) {
                // The found file is new, must be added in polled list.
                polled = _polled_files.insert(polled, PolledFilePtr(new PolledFile(name, size, date, now)));
            }
            else {
                // The file was already there last time, track changes
                assert(polled != _polled_files.end());
                assert(name == (*polled)->_name);
                (*polled)->trackChange(size, date, now);
            }

            // Check if the file need to be notified
            PolledFilePtr& pf(*polled);
            if (pf->_pending && now >= pf->_found_date + min_stable_delay) {
                pf->_pending = false;
                _notified_files.push_back(pf);
                _report.debug("PolledFiles: " + PolledFile::StatusEnumeration.name(pf->_status) + " " + name);
            }

            // Next polled file
            ++polled;
        }

        // Remove all remaining polled files
        while (polled != _polled_files.end()) {
            deleteFile(polled);
        }

        // Notify the listener
        if (!_notified_files.empty() && !notifyListener()) {
            return;
        }

        // Clear notification list, will be a new one at next poll
        _notified_files.clear();

        // Wait until next poll
        SleepThread(poll_interval);
    }
}


//----------------------------------------------------------------------------
// Mark a file as deleted, move from polled to notified files.
//----------------------------------------------------------------------------

void ts::PollFiles::deleteFile(PolledFileList::iterator& polled)
{
    _report.debug("PolledFiles: deleted " + (*polled)->_name);
    (*polled)->_status = PolledFile::DELETED;
    _notified_files.push_back(*polled);
    polled = _polled_files.erase(polled);
}


//----------------------------------------------------------------------------
// Notify listener
//----------------------------------------------------------------------------

bool ts::PollFiles::notifyListener()
{
    try {
        return _listener.handlePolledFiles(_notified_files);
    }
    catch (const std::exception& e) {
        const char* msg = e.what();
        _report.error("Exception in PollFiles listener: %s", msg == 0 ? "unknown" : msg);
        return true;
    }
}
