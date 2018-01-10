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
//  Perform a simple Web request - Windows specific parts.
//
//  IMPLEMENTATION ISSUE:
//  If we allow redirection, we need to get notified of the final redirected
//  URL. To do this, we must use InternetSetStatusCallback and specify a
//  callback which will be notified of various events, including redirection.
//  This works fine with Win64. However, this crashes on Win32. To be honest,
//  the code does not even compile in Win32 even though the profile of the
//  callback is directly copied/pasted from INTERNET_STATUS_CALLBACK in
//  wininet.h (and it compiles in Win64). Using a type cast, the compilation
//  works but the execution crashes. The reason for this is a complete
//  mystery. As a workaround, we disable the automatic redirection and
//  we handle the redirection manually. Thus, we do not need a callback.
//
//----------------------------------------------------------------------------

#include "tsWebRequest.h"
#include "tsSysUtils.h"
#include "tsWinUtils.h"
#include <WinInet.h>
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// System-specific parts are stored in a private structure.
//----------------------------------------------------------------------------

class ts::WebRequest::SystemGuts
{
public:
    // Constructor with a reference to parent WebRequest.
    SystemGuts(WebRequest& request);

    // Destructor.
    ~SystemGuts();

    // Initialize, clear, start Web transfer.
    bool init();
    void clear();
    bool start();

    // Report an error message.
    void error(const UChar* message, ::DWORD code = ::GetLastError());

private:
    WebRequest& _request;        // Parent request.
    ::HINTERNET _inet;           // Handle to all Internet operations.
    ::HINTERNET _url;            // Handle to URL operations.
    int         _redirectCount;  // Current number of redirections.
    UString     _previousURL;    // Previous URL, before getting a redirection.

    // Transmit response headers to the WebRequest.
    void transmitResponseHeaders();

    // Inaccessible operations.
    SystemGuts() = delete;
    SystemGuts(const SystemGuts&) = delete;
    SystemGuts& operator=(const SystemGuts&) = delete;
};


//----------------------------------------------------------------------------
// System-specific constructor and destructor.
//----------------------------------------------------------------------------

ts::WebRequest::SystemGuts::SystemGuts(WebRequest& request) :
    _request(request),
    _inet(0),
    _url(0),
    _redirectCount(0)
{
}

ts::WebRequest::SystemGuts::~SystemGuts()
{
    clear();
}

void ts::WebRequest::allocateGuts()
{
    _guts = new SystemGuts(*this);
}

void ts::WebRequest::deleteGuts()
{
    delete _guts;
}


//----------------------------------------------------------------------------
// Download operations from the WebRequest class.
//----------------------------------------------------------------------------

bool ts::WebRequest::downloadInitialize()
{
    return _guts->init();
}

void ts::WebRequest::downloadAbort()
{
    _guts->clear();
}

bool ts::WebRequest::download()
{
    return _guts->start();
}


//----------------------------------------------------------------------------
// Report an error message.
//----------------------------------------------------------------------------

void ts::WebRequest::SystemGuts::error(const UChar* message, ::DWORD code)
{
    if (code == ERROR_SUCCESS) {
        _request._report.error(u"Web error: %s", {message});
    }
    else {
        _request._report.error(u"Web error: %s (%s)", {message, WinErrorMessage(code, u"Wininet.dll", INTERNET_ERROR_BASE, INTERNET_ERROR_LAST)});
    }
}


//----------------------------------------------------------------------------
// Initialize Web transfer.
//----------------------------------------------------------------------------

bool ts::WebRequest::SystemGuts::init()
{
    // Make sure we start from a clean state.
    clear();

    // Prepare proxy name.
    const bool useProxy = !_request.proxyHost().empty();
    ::DWORD access = INTERNET_OPEN_TYPE_PRECONFIG;
    const ::WCHAR* proxy = 0;
    UString proxyName(_request.proxyHost());

    if (useProxy) {
        access = INTERNET_OPEN_TYPE_PROXY;
        if (_request.proxyPort() != 0) {
            proxyName += UString::Format(u":%d", {_request.proxyPort()});
        }
        proxy = proxyName.wc_str();
    }

    // Open the main Internet handle.
    _inet = ::InternetOpenW(_request._userAgent.wc_str(), access, proxy, 0, 0);
    if (_inet == 0) {
        error(u"error accessing Internet handle");
        return false;
    }

    // Specify the proxy authentication, if provided.
    if (useProxy) {
        UString user(_request.proxyUser());
        UString pass(_request.proxyPassword());
        if (!user.empty() && !::InternetSetOptionW(_inet, INTERNET_OPTION_PROXY_USERNAME, &user[0], ::DWORD(user.size()))) {
            error(u"error setting proxy username");
            clear();
            return false;
        }
        if (!pass.empty() && !::InternetSetOptionW(_inet, INTERNET_OPTION_PROXY_PASSWORD, &pass[0], ::DWORD(pass.size()))) {
            error(u"error setting proxy password");
            clear();
            return false;
        }
    }

    // URL connection flags. Always disable redirections (see comment on top of file).
    const ::DWORD urlFlags =
        INTERNET_FLAG_KEEP_CONNECTION |
        INTERNET_FLAG_NO_UI |
        INTERNET_FLAG_NO_COOKIES |
        INTERNET_FLAG_PASSIVE |
        INTERNET_FLAG_NO_AUTO_REDIRECT;

    // Loop on redirections.
    for (;;) {
        // Keep track of current URL to fetch.
        _previousURL = _request._finalURL;

        // Now open the URL.
        _url = ::InternetOpenUrlW(_inet, _previousURL.wc_str(), 0, 0, urlFlags, 0);
        if (_url == 0) {
            error(u"error opening URL");
            clear();
            return false;
        }

        // Send the response headers to the WebRequest object.
        transmitResponseHeaders();

        // If redirections are not allowed or no redirection occured, stop now.
        // Redirection codes are 3xx (eg. "HTTP/1.1 301 Moved Permanently").
        if (!_request._autoRedirect || _request._httpStatus / 100 != 3 || _request._finalURL == _previousURL) {
            break;
        }

        // Close this URL, we need to redirect to _finalURL.
        ::InternetCloseHandle(_url);
        _url = 0;

        // Limit the number of redirections to avoid "looping sites".
        if (++_redirectCount > 16) {
            error(u"too many HTTP redirections");
            clear();
            return false;
        }
    };

    return true;
}


//----------------------------------------------------------------------------
// Abort / clear the Web transfer.
//----------------------------------------------------------------------------

void ts::WebRequest::SystemGuts::clear()
{
    // Close Internet handles.
    if (_url != 0 && !::InternetCloseHandle(_url)) {
        error(u"error closing URL handle");
    }
    if (_inet != 0 && !::InternetCloseHandle(_inet)) {
        error(u"error closing main Internet handle");
    }
    _url = _inet = 0;
    _redirectCount = 0;
}


//----------------------------------------------------------------------------
// Perform the Web transfer.
// The URL is open, the response headers have been received, now receive data.
//----------------------------------------------------------------------------

bool ts::WebRequest::SystemGuts::start()
{
    bool success = true;
    std::array<uint8_t, 1024> data;

    while (success) {
        ::DWORD gotSize = 0;
        if (!::InternetReadFile(_url, data.data(), ::DWORD(data.size()), &gotSize)) {
            error(u"download error");
            success = false;
        }
        else if (gotSize == 0) {
            // Successfully reading zero bytes means end of file.
            break;
        }
        else {
            // Get real data, transmit them to the WebRequest object.
            success = _request.copyData(data.data(), size_t(gotSize));
        }
    }

    return success;
}


//----------------------------------------------------------------------------
// Transmit response headers to the WebRequest.
//----------------------------------------------------------------------------

void ts::WebRequest::SystemGuts::transmitResponseHeaders()
{
    // Query the response headers from the URL handle.
    // First try with an arbitrary buffer size.
    UString headers(1024, CHAR_NULL);
    ::DWORD headersSize = ::DWORD(headers.size());
    ::DWORD index = 0;
    if (!::HttpQueryInfoW(_url, HTTP_QUERY_RAW_HEADERS_CRLF, &headers[0], &headersSize, &index)) {

        // Process actual error.
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            error(u"error getting HTTP response headers");
            return;
        }

        // The buffer was too small, reallocate one.
        headers.resize(size_t(headersSize));
        headersSize = ::DWORD(headers.size());
        index = 0;
        if (!::HttpQueryInfoW(_url, HTTP_QUERY_RAW_HEADERS_CRLF, &headers[0], &headersSize, &index)) {
            error(u"error getting HTTP response headers");
            return;
        }
    }

    // Adjust actual string length.
    headers.resize(std::min(std::max<::DWORD>(0, headersSize), ::DWORD(headers.size() - 1)));

    // Pass the headers to the WebRequest.
    _request.processHeaders(headers);
}
