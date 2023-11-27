//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPollFiles.h"
#include "tsFileUtils.h"
#include "tsSysUtils.h"
#include "tsNullReport.h"
#include "tsErrCodeReport.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::PollFiles::PollFiles(const UString& wildcard,
                         PollFilesListener* listener,
                         MilliSecond poll_interval,
                         MilliSecond min_stable_delay,
                         Report& report) :
    _report(report),
    _files_wildcard(wildcard),
    _poll_interval(poll_interval),
    _min_stable_delay(min_stable_delay),
    _listener(listener)
{
}


//----------------------------------------------------------------------------
// Poll files continuously until the listener asks to terminate.
//----------------------------------------------------------------------------

void ts::PollFiles::pollRepeatedly()
{
    _report.debug(u"Starting PollFiles on %s, poll interval = %d ms, min stable delay = %d ms", {_files_wildcard, _poll_interval, _min_stable_delay});

    // Loop on poll for files.
    while (pollOnce()) {
        // Wait until next poll
        SleepThread(_poll_interval);
    }
}


//----------------------------------------------------------------------------
// Perform one poll operation, notify listener if necessary, and return immediately.
//----------------------------------------------------------------------------

bool ts::PollFiles::pollOnce()
{
    // Initially update the search criteria from the listener (it there is one).
    if (_listener != nullptr) {
        try {
            if (!_listener->updatePollFiles(_files_wildcard, _poll_interval, _min_stable_delay)) {
                // The listener asks to stop.
                return false;
            }
        }
        catch (const std::exception& e) {
            const char* msg = e.what();
            _report.error(u"Exception in PollFiles listener: %s", {msg == nullptr ? "unknown" : msg});
        }
    }

    // List files, sort according to name
    const Time now(Time::CurrentUTC());
    UStringVector found_files;
    ExpandWildcard(found_files, _files_wildcard);
    std::sort(found_files.begin(), found_files.end());

    // Compare currently found files with last polled state.
    PolledFileList::iterator polled = _polled_files.begin();
    for (const auto& name : found_files) {

        // Get characteristics of next found file
        const std::uintmax_t size = fs::file_size(name, &ErrCodeReport());
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
        if (pf->_pending && now >= pf->_found_date + _min_stable_delay) {
            pf->_pending = false;
            _notified_files.push_back(pf);
            _report.debug(u"PolledFiles: %s %s", {PolledFile::StatusEnumeration.name(pf->_status), name});
        }

        // Next polled file
        ++polled;
    }

    // Remove all remaining polled files
    while (polled != _polled_files.end()) {
        deleteFile(polled);
    }

    // Notify the listener
    if (!_notified_files.empty() && _listener != nullptr) {
        try {
            if (!_listener->handlePolledFiles(_notified_files)) {
                // The listener asks to stop.
                return false;
            }
        }
        catch (const std::exception& e) {
            const char* msg = e.what();
            _report.error(u"Exception in PollFiles listener: %s", {msg == nullptr ? "unknown" : msg});
        }
    }

    // Clear notification list, will be a new one at next poll
    _notified_files.clear();
    return true;
}


//----------------------------------------------------------------------------
// Mark a file as deleted, move from polled to notified files.
//----------------------------------------------------------------------------

void ts::PollFiles::deleteFile(PolledFileList::iterator& polled)
{
    _report.debug(u"PolledFiles: deleted %s", {(*polled)->_name});
    (*polled)->_status = PolledFile::DELETED;
    _notified_files.push_back(*polled);
    polled = _polled_files.erase(polled);
}
