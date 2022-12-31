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

#include "tsBeforeStandardHeaders.h"
#include <WinInet.h>
#include "tsAfterStandardHeaders.h"

// Required link libraries.
#if defined(TS_MSC)
    #pragma comment(lib, "Wininet.lib")
#endif


//----------------------------------------------------------------------------
// System-specific parts are stored in a private structure.
//----------------------------------------------------------------------------

class ts::WebRequest::SystemGuts
{
    TS_NOBUILD_NOCOPY(SystemGuts);
public:
    // Constructor with a reference to parent WebRequest.
    SystemGuts(WebRequest& request);

    // Destructor.
    ~SystemGuts();

    // Delegation of methods from WebRequest.
    bool init();
    bool receive(void* buffer, size_t maxSize, size_t& retSize);
    void clear();

private:
    WebRequest& _request;        // Parent request.
    volatile ::HINTERNET _inet;  // Handle to all Internet operations.
    volatile ::HINTERNET _url;   // Handle to URL operations.
    int         _redirectCount;  // Current number of redirections.
    UString     _previousURL;    // Previous URL, before getting a redirection.

    // Report an error message.
    void error(const UChar* message, ::DWORD code = ::GetLastError());

    // Transmit response headers to the WebRequest.
    void transmitResponseHeaders();
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

bool ts::WebRequest::startTransfer()
{
    return _guts->init();
}

bool ts::WebRequest::receive(void* buffer, size_t maxSize, size_t& retSize)
{
    if (_isOpen) {
        return _guts->receive(buffer, maxSize, retSize);
    }
    else {
        _report.error(u"transfer not started");
        return false;
    }
}

bool ts::WebRequest::close()
{
    bool success = _isOpen;
    _guts->clear();
    _isOpen = false;
    return success;
}

void ts::WebRequest::abort()
{
    _guts->clear();
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
    _url = nullptr;
    _inet = nullptr;
}


//----------------------------------------------------------------------------
// Initialize Web transfer.
//----------------------------------------------------------------------------

bool ts::WebRequest::SystemGuts::init()
{
    // Make sure we start from a clean state.
    clear();
    _redirectCount = 0;

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

    // Specify the various timeouts.
    if (_request._connectionTimeout > 0) {
        ::DWORD timeout = ::DWORD(_request._connectionTimeout);
        if (!::InternetSetOptionW(_inet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, ::DWORD(sizeof(timeout)))) {
            error(u"error setting connection timeout");
            clear();
            return false;
        }
    }
    if (_request._receiveTimeout > 0) {
        ::DWORD timeout = ::DWORD(_request._receiveTimeout);
        if (!::InternetSetOptionW(_inet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, ::DWORD(sizeof(timeout))) ||
            !::InternetSetOptionW(_inet, INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, &timeout, ::DWORD(sizeof(timeout))))
        {
            error(u"error setting receive timeout");
            clear();
            return false;
        }
    }

    // URL connection flags.
    const ::DWORD urlFlags =
        INTERNET_FLAG_KEEP_CONNECTION |   // Use keep-alive.
        INTERNET_FLAG_NO_UI |             // Disable popup windows.
        (_request._useCookies ? 0 : INTERNET_FLAG_NO_COOKIES) | // Don't store cookies, don't send stored cookies.
        INTERNET_FLAG_PASSIVE |           // Use passive mode with FTP (less NAT issues).
        INTERNET_FLAG_NO_AUTO_REDIRECT |  // Disable redirections (see comment on top of file).
        INTERNET_FLAG_NO_CACHE_WRITE;     // Don't save downloaded data to local disk cache.

    // Build the list of request headers.
    ::WCHAR* headerAddress = 0;
    ::DWORD headerLength = 0;
    UString headers;
    if (!_request._requestHeaders.empty()) {
        for (const auto& it : _request._requestHeaders) {
            if (!headers.empty()) {
                headers.append(u"\r\n");
            }
            headers.append(it.first);
            headers.append(u": ");
            headers.append(it.second);
        }
        headerAddress = headers.wc_str();
        headerLength = ::DWORD(headers.size());
    }

    // Loop on redirections.
    for (;;) {
        // Keep track of current URL to fetch.
        _previousURL = _request._finalURL;

        // Now open the URL.
        _url = ::InternetOpenUrlW(_inet, _previousURL.wc_str(), headerAddress, headerLength, urlFlags, 0);
        if (_url == 0) {
            error(u"error opening URL");
            clear();
            return false;
        }

        // Send the response headers to the WebRequest object.
        // Do not expect any response header from file: URL.
        if (_previousURL.startWith(u"file:")) {
            // Pass empty headers to the WebRequest.
            _request.processReponseHeaders(u"");
        }
        else {
            // Get actual response headers and pass them to the WebRequest.
            transmitResponseHeaders();
        }

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
// Perform Web transfer.
//----------------------------------------------------------------------------

bool ts::WebRequest::SystemGuts::receive(void* buffer, size_t maxSize, size_t& retSize)
{
    ::DWORD thisSize = 0;
    const bool success = ::InternetReadFile(_url, buffer, ::DWORD(maxSize), &thisSize);

    if (!success) {
        error(u"download error");
    }

    retSize = size_t(thisSize);
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
    _request.processReponseHeaders(headers);
}


//----------------------------------------------------------------------------
// Get the version of the underlying HTTP library.
//----------------------------------------------------------------------------

ts::UString ts::WebRequest::GetLibraryVersion()
{
    // Do not know which version...
    return u"WinInet";
}
