//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//  Perform a simple Web request. Common parts. See specific parts in
//  unix/tsWebRequestGuts.cpp and windows/tsWebRequestGuts.cpp.
//
//----------------------------------------------------------------------------

#include "tsWebRequest.h"
#include "tsFatal.h"
#include "tsIntegerUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::WebRequest::WebRequest(Report& report) :
    _report(report),
    _userAgent(u"tsduck"),
    _autoRedirect(true),
    _originalURL(),
    _finalURL(),
    _proxyHost(),
    _proxyPort(0),
    _proxyUser(),
    _proxyPassword(),
    _headers(),
    _httpStatus(0),
    _contentSize(0),
    _headerContentSize(0),
    _dlData(0),
    _dlFile(),
    _guts(0)
{
    allocateGuts();
    CheckNonNull(_guts);
}


//----------------------------------------------------------------------------
// Destructor.
//----------------------------------------------------------------------------

ts::WebRequest::~WebRequest()
{
    if (_guts != 0) {
        deleteGuts();
        _guts = 0;
    }
}


//----------------------------------------------------------------------------
// Set the URL to get.
//----------------------------------------------------------------------------

void ts::WebRequest::setURL(const UString& url)
{
    _originalURL = url;
    _finalURL = url;
}


//----------------------------------------------------------------------------
// Set the optional proxy data.
//----------------------------------------------------------------------------

void ts::WebRequest::setProxyHost(const UString& host, uint16_t port)
{
    _proxyHost = host;
    _proxyPort = port;
}


void ts::WebRequest::setProxyUser(const UString& user, const UString& password)
{
    _proxyUser = user;
    _proxyPassword = password;
}


//----------------------------------------------------------------------------
// Get the value of one or all headers.
//----------------------------------------------------------------------------

void ts::WebRequest::getHeaders(HeadersMap& headers) const
{
    headers = _headers;
}

ts::UString ts::WebRequest::header(const UString& name) const
{
    const HeadersMap::const_iterator it = _headers.find(name);
    return it == _headers.end() ? UString() : it->second;
}


//----------------------------------------------------------------------------
// Process a list of headers. Header lines are terminated by LF or CRLF.
//----------------------------------------------------------------------------

void ts::WebRequest::processHeaders(const UString& text)
{
    // Split header lines.
    const UString CR(1, u'\r');
    UStringList lines;
    text.toRemoved(CR).split(lines, u'\n', true, true);

    // Process headers one by one.
    for (UStringList::const_iterator it = lines.begin(); it != lines.end(); ++it) {

        _report.debug(u"HTTP header: %s", {*it});
        const size_t colon = it->find(u':');
        size_t size = 0;

        if (it->startWith(u"HTTP/")) {
            // This is the initial header. When we receive this, this is either
            // the first time we are called for this request or we have been
            // redirected to another URL. In all cases, reset the context.
            _headers.clear();
            _headerContentSize = 0;
            _httpStatus = 0;

            // The HTTP status is in the second field, as in "HTTP/1.1 200 OK".
            UStringVector fields;
            it->split(fields, u' ', true, true);
            if (fields.size() < 2 || !fields[1].toInteger(_httpStatus)) {
                _report.warning(u"no HTTP status found in header: %s", {*it});
            }
        }
        else if (colon != UString::NPOS) {
            // Found a real header.
            UString name(*it, 0, colon);
            UString value(*it, colon + 1, it->size() - colon - 1);
            name.trim();
            value.trim();

            // Insert header.
            _headers.insert(std::make_pair(name, value));

            // Process specific headers.
            if (name.similar(u"Location")) {
                _finalURL = value;
                _report.debug(u"redirected to %s", {_finalURL});
            }
            else if (name.similar(u"Content-length") && value.toInteger(size)) {
                setPossibleContentSize(size);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Copy some downloaded data.
//----------------------------------------------------------------------------

bool ts::WebRequest::copyData(const void* addr, size_t size)
{
    // Copy data in memory buffer if there is one.
    if (_dlData != 0) {
        // Check maximum buffer size.
        const size_t newSize = BoundedAdd(_dlData->size(), size);
        if (newSize >= _dlData->max_size()) {
            return false; // too large (but unlikely)
        }

        // Enlarge the buffer capacity to avoid too frequent reallocations.
        // At least double the capacity of the buffer each time.
        if (newSize > _dlData->capacity()) {
            _dlData->reserve(std::max(newSize, 2 * _dlData->capacity()));
        }

        // Finally copy the data.
        _dlData->append(addr, size);
    }

    // Save data in file if there is one.
    if (_dlFile.is_open()) {
        _dlFile.write(reinterpret_cast<const char*>(addr), size);
        if (!_dlFile) {
            _report.error(u"error saving downloaded file");
            return false;
        }
    }

    _contentSize += size;
    return true;
}


//----------------------------------------------------------------------------
// Provide possible total download size.
//----------------------------------------------------------------------------

bool ts::WebRequest::setPossibleContentSize(size_t totalSize)
{
    if (totalSize > _headerContentSize) {
        // Keep this value.
        _headerContentSize = totalSize;
        _report.debug(u"announced content size: %d bytes", {_headerContentSize});

        // Enlarge memory buffer when necessary to avoid too frequent reallocations.
        if (_dlData != 0 && totalSize > _dlData->capacity()) {
            if (totalSize > _dlData->max_size()) {
                return false; // too large (but unlikely)
            }
            _dlData->reserve(totalSize);
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Download the content of the URL as text.
//----------------------------------------------------------------------------

bool ts::WebRequest::downloadTextContent(UString& text)
{
    // Download the content as raw binary data.
    ByteBlock data;
    if (downloadBinaryContent(data)) {
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
// Clear the transfer results, status, etc.
//----------------------------------------------------------------------------

bool ts::WebRequest::clearTransferResults()
{
    _httpStatus = 0;
    _contentSize = 0;
    _headerContentSize = 0;
    _finalURL = _originalURL;

    // Close spurious file (should not happen).
    if (_dlFile.is_open()) {
        _dlFile.close();
    }

    // Make sure we have an URL.
    if (_originalURL.empty()) {
        _report.error(u"no URL specified");
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Download the content of the URL as binary data.
//----------------------------------------------------------------------------

bool ts::WebRequest::downloadBinaryContent(ByteBlock& data)
{
    data.clear();

    // Transfer initialization.
    bool ok = clearTransferResults() && downloadInitialize();

    // Actual transfer.
    if (ok) {
        try {
            _dlData = &data;
            ok = download();
        }
        catch (...) {
            ok = false;
        }
        _dlData = 0;
    }

    return ok;
}


//----------------------------------------------------------------------------
// Download the content of the URL in a file.
//----------------------------------------------------------------------------

bool ts::WebRequest::downloadFile(const UString& fileName)
{
    // Transfer initialization.
    if (!clearTransferResults() || !downloadInitialize()) {
        return false;
    }

    // Create the output file.
    _dlFile.open(fileName.toUTF8().c_str(), std::ios::out | std::ios::binary);
    if (!_dlFile) {
        _report.error(u"error creating file %s", {fileName});
        downloadAbort();
        return false;
    }

    // Actual transfer.
    const bool ok = download();
    _dlFile.close();
    return ok;
}
