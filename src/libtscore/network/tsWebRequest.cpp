//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Perform a simple Web request. Common parts. See specific parts in
//  unix/tsWebRequestGuts.cpp and windows/tsWebRequestGuts.cpp.
//
//----------------------------------------------------------------------------

#include "tsWebRequest.h"
#include "tsErrCodeReport.h"
#include "tsURL.h"
#include "tsFeatures.h"


//----------------------------------------------------------------------------
// Register for options --version and --support.
//----------------------------------------------------------------------------

#if defined(TS_NO_CURL) && !defined(TS_WINDOWS)
    #define SUPPORT ts::Features::UNSUPPORTED
#else
    #define SUPPORT ts::Features::SUPPORTED
#endif

TS_REGISTER_FEATURE(u"http", u"Web library", SUPPORT, ts::WebRequest::GetLibraryVersion);


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::WebRequest::WebRequest(Report* report, bool non_blocking) :
    Device(report, non_blocking)
{
    allocateGuts();
}

ts::WebRequest::WebRequest(ReporterBase* delegate, bool non_blocking) :
    Device(delegate, non_blocking)
{
    allocateGuts();
}

ts::WebRequest::~WebRequest()
{
    if (_guts != nullptr) {
        deleteGuts();
        _guts = nullptr;
    }
    _args.deleteTemporaryCookiesFile(report());
}


//----------------------------------------------------------------------------
// Open an URL and start the transfer.
//----------------------------------------------------------------------------

bool ts::WebRequest::open(const UString& url, IOSB* iosb)
{
    if (url.empty()) {
        report().error(u"no URL specified");
        return false;
    }

    if (_is_open) {
        report().error(u"internal error, transfer already started, cannot download %s", url);
        return false;
    }

    _status.reset(url);
    _interrupted = false;

    // System-specific transfer initialization.
    _is_open = startTransfer(iosb);
    return _is_open;
}


//----------------------------------------------------------------------------
// Download the content of the URL as binary data.
//----------------------------------------------------------------------------

bool ts::WebRequest::downloadBinaryContent(const UString& url, ByteBlock& data, size_t chunk_size)
{
    data.clear();

    // The request must be in blocking mode.
    if (!checkNonBlocking(false, u"WebRequest::downloadBinaryContent")) {
        return false;
    }

    // Transfer initialization.
    if (!open(url)) {
        return false;
    }

    // Initialize download buffers.
    size_t received_size = 0;
    data.reserve(_status.announcedContentSize());
    data.resize(chunk_size);
    bool success = true;

    for (;;) {
        // Transfer one chunk.
        size_t this_size = 0;
        success = receive(data.data() + received_size, data.size() - received_size, this_size);
        received_size += std::min(this_size, data.size() - received_size);

        // Error or end of transfer.
        if (!success || this_size == 0) {
            break;
        }

        // Enlarge the buffer for next chunk.
        // Don't do that too often in case of very short transfers.
        if (data.size() - received_size < chunk_size / 2) {
            data.resize(received_size + chunk_size);
        }
    }

    // Resize data buffer to actually transfered size.
    data.resize(received_size);
    return close() && success;
}


//----------------------------------------------------------------------------
// Download the content of the URL as text.
//----------------------------------------------------------------------------

bool ts::WebRequest::downloadTextContent(const UString& url, UString& text, size_t chunk_size)
{
    // Download the content as raw binary data.
    ByteBlock data;
    if (downloadBinaryContent(url, data, chunk_size)) {
        // Convert to UTF-8.
        text.assignFromUTF8(reinterpret_cast<const char*>(data.data()), data.size());
        // Remove all CR, just keep the LF.
        text.remove(u'\r');
        return true;
    }
    else {
        // Download error.
        text.clear();
        return false;
    }
}


//----------------------------------------------------------------------------
// Download the content of the URL in a file.
//----------------------------------------------------------------------------

bool ts::WebRequest::downloadFile(const UString& url, const fs::path& file_name, size_t chunk_size)
{
    // The request must be in blocking mode.
    if (!checkNonBlocking(false, u"WebRequest::downloadFile")) {
        return false;
    }

    // Transfer initialization.
    if (!open(url)) {
        return false;
    }

    // Create the output file.
    std::ofstream file(file_name, std::ios::out | std::ios::binary);
    if (!file) {
        report().error(u"error creating file %s", file_name);
        close();
        return false;
    }

    std::vector<char> buffer(chunk_size);
    bool success = true;

    for (;;) {
        // Transfer one chunk.
        size_t thisSize = 0;
        success = receive(buffer.data(), buffer.size(), thisSize);

        // Error or end of transfer.
        if (!success || thisSize == 0) {
            break;
        }

        file.write(buffer.data(), thisSize);
        if (!file) {
            report().error(u"error saving download to %s", file_name);
            success = false;
            break;
        }
    }

    // Resize data buffer to actually transfered size.
    file.close();
    return close() && success;
}
