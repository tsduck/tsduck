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

ts::UString ts::WebRequest::_defaultProxyHost;
uint16_t    ts::WebRequest::_defaultProxyPort = 0;
ts::UString ts::WebRequest::_defaultProxyUser;
ts::UString ts::WebRequest::_defaultProxyPassword;


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::WebRequest::WebRequest(Report& report) :
    _report(report),
    _userAgent(u"tsduck"),
    _autoRedirect(true),
    _originalURL(),
    _finalURL(),
    _connectionTimeout(0),
    _receiveTimeout(0),
    _proxyHost(),
    _proxyPort(0),
    _proxyUser(),
    _proxyPassword(),
    _requestHeaders(),
    _responseHeaders(),
    _httpStatus(0),
    _contentSize(0),
    _headerContentSize(0),
    _dlData(nullptr),
    _dlFile(),
    _dlHandler(nullptr),
    _interrupted(false),
    _guts(nullptr)
{
    allocateGuts();
    CheckNonNull(_guts);
}


//----------------------------------------------------------------------------
// Destructor.
//----------------------------------------------------------------------------

ts::WebRequest::~WebRequest()
{
    if (_guts != nullptr) {
        deleteGuts();
        _guts = nullptr;
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
// Set other options.
//----------------------------------------------------------------------------

void ts::WebRequest::setConnectionTimeout(MilliSecond timeout)
{
    _connectionTimeout = timeout;
}

void ts::WebRequest::setReceiveTimeout(MilliSecond timeout)
{
    _receiveTimeout = timeout;
}

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

void ts::WebRequest::SetDefaultProxyHost(const UString& host, uint16_t port)
{
    _defaultProxyHost = host;
    _defaultProxyPort = port;
}


void ts::WebRequest::SetDefaultProxyUser(const UString& user, const UString& password)
{
    _defaultProxyUser = user;
    _defaultProxyPassword = password;
}


//----------------------------------------------------------------------------
// Set various arguments from command line.
//----------------------------------------------------------------------------

void ts::WebRequest::setArgs(const ts::WebRequestArgs& args)
{
    if (!args.proxyHost.empty()) {
        setProxyHost(args.proxyHost, args.proxyPort);
    }
    if (!args.proxyUser.empty()) {
        setProxyUser(args.proxyUser, args.proxyPassword);
    }
    if (args.connectionTimeout > 0) {
        setConnectionTimeout(args.connectionTimeout);
    }
    if (args.receiveTimeout > 0) {
        setReceiveTimeout(args.receiveTimeout);
    }
}


//----------------------------------------------------------------------------
// Set request headers.
//----------------------------------------------------------------------------

void ts::WebRequest::setRequestHeader(const UString& name, const UString& value)
{
    _requestHeaders.insert(std::make_pair(name, value));
}

void ts::WebRequest::clearRequestHeaders()
{
    _requestHeaders.clear();
}


//----------------------------------------------------------------------------
// Get the value of one or all headers.
//----------------------------------------------------------------------------

void ts::WebRequest::getResponseHeaders(HeadersMap& headers) const
{
    headers = _responseHeaders;
}

ts::UString ts::WebRequest::reponseHeader(const UString& name) const
{
    const HeadersMap::const_iterator it = _responseHeaders.find(name);
    return it == _responseHeaders.end() ? UString() : it->second;
}


//----------------------------------------------------------------------------
// Get the MIME type in the response headers.
//----------------------------------------------------------------------------

ts::UString ts::WebRequest::mimeType(bool simple, bool lowercase) const
{
    // Get complete MIME type.
    UString mime(reponseHeader(u"Content-Type"));

    // Get initial type, before ';', in simple form.
    if (simple) {
        const size_t semi = mime.find(u';');
        if (semi != NPOS) {
            mime.erase(semi);
        }
        mime.trim();
    }

    // Force case.
    if (lowercase) {
        mime.convertToLower();
    }

    return mime;
}


//----------------------------------------------------------------------------
// Process a list of headers. Header lines are terminated by LF or CRLF.
//----------------------------------------------------------------------------

void ts::WebRequest::processReponseHeaders(const UString& text)
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
            _responseHeaders.clear();
            _headerContentSize = 0;
            _httpStatus = 0;

            // The HTTP status is in the second field, as in "HTTP/1.1 200 OK".
            UStringVector fields;
            it->split(fields, u' ', true, true);
            if (fields.size() < 2 || !fields[1].toInteger(_httpStatus)) {
                _report.warning(u"no HTTP status found in header: %s", {*it});
            }
        }
        else if (colon != NPOS) {
            // Found a real header.
            UString name(*it, 0, colon);
            UString value(*it, colon + 1, it->size() - colon - 1);
            name.trim();
            value.trim();

            // Insert header.
            _responseHeaders.insert(std::make_pair(name, value));

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
    if (_dlData != nullptr) {
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

    // Pass data to application if a handler is defined.
    if (_dlHandler != nullptr && !_dlHandler->handleWebData(*this, addr, size)) {
        _report.debug(u"Web transfer is interrupted by application");
        _interrupted = true;
        return false;
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
        if (_dlData != nullptr && totalSize > _dlData->capacity()) {
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
    _dlData = nullptr;
    _dlHandler = nullptr;

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
    _interrupted = false;

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
        _dlData = nullptr;
        downloadClose();
    }

    return ok;
}


//----------------------------------------------------------------------------
// Download the content of the URL in a file.
//----------------------------------------------------------------------------

bool ts::WebRequest::downloadFile(const UString& fileName)
{
    _interrupted = false;

    // Transfer initialization.
    if (!clearTransferResults() || !downloadInitialize()) {
        return false;
    }

    // Create the output file.
    _dlFile.open(fileName.toUTF8().c_str(), std::ios::out | std::ios::binary);
    if (!_dlFile) {
        _report.error(u"error creating file %s", {fileName});
        downloadClose();
        return false;
    }

    // Actual transfer.
    const bool ok = download();
    _dlFile.close();
    downloadClose();
    return ok;
}


//----------------------------------------------------------------------------
// Download the content of the URL and pass data to the application.
//----------------------------------------------------------------------------

bool ts::WebRequest::downloadToApplication(WebRequestHandlerInterface* handler)
{
    _interrupted = false;

    // Transfer initialization.
    bool ok = handler != nullptr && clearTransferResults() && downloadInitialize();

    // Actual transfer.
    if (ok) {
        try {
            _dlHandler = handler;
            ok = handler->handleWebStart(*this, _headerContentSize);
            if (ok) {
                ok = download();
            }
            else {
                _report.debug(u"Web request is aborted by application before transfer");
            }
        }
        catch (...) {
            ok = false;
        }
        _dlHandler = nullptr;
        downloadClose();
    }

    return ok;
}
