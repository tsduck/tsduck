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
//  Perform a simple Web request - UNIX specific parts with libcurl.
//
//----------------------------------------------------------------------------

#include "tsWebRequest.h"
#include "tsSingletonManager.h"
#include <curl/curl.h>
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Global libcurl initialization using a singleton.
//----------------------------------------------------------------------------

namespace {

    // This singleton initialized libcurl in its constructor.
    class LibCurlInit
    {
        TS_DECLARE_SINGLETON(LibCurlInit);
    public:
        // Status code of libcurl initialization.
        const ::CURLcode initStatus;
    };

    TS_DEFINE_SINGLETON(LibCurlInit);

    // Constructor of the libcurl initialization.
    LibCurlInit::LibCurlInit() :
        initStatus(::curl_global_init(CURL_GLOBAL_ALL))
    {
    }
}


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

    // Initialize and clear CURL Easy transfer.
    bool init();
    void clear();

    // Build an error message from libcurl.
    UString message(const UString& title, ::CURLcode = ::CURLE_OK);

private:
    WebRequest& _request;
    ::CURL*     _curl;
    char        _error[CURL_ERROR_SIZE];  // Error message buffer for libcurl.

    // Libcurl callbacks for response headers and response data.
    // The userdata points to this object.
    static size_t headerCallback(char *ptr, size_t size, size_t nmemb, void *userdata);
    static size_t writeCallback(char *ptr, size_t size, size_t nmemb, void *userdata);

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
    _curl(0),
    _error{0}
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
// Initialize CURL Easy transfer.
//----------------------------------------------------------------------------

bool ts::WebRequest::SystemGuts::init()
{
    // Make sure we start from a clean state.
    clear();

    // Make sure we have an URL.
    if (_request._originalURL.empty()) {
        _request._report.error(u"no URL specified");
        return false;
    }

    // Initialize CURL Easy
    if ((_curl = ::curl_easy_init()) == 0) {
        _request._report.error(u"libcurl 'curl easy' initialization error");
        return false;
    }

    // Setup the error message buffer.
    ::CURLcode status = ::curl_easy_setopt(_curl, CURLOPT_ERRORBUFFER, _error);

    // Set the starting URL.
    if (status == ::CURLE_OK) {
        status = ::curl_easy_setopt(_curl, CURLOPT_URL, _request._originalURL.toUTF8().c_str());
    }

    // Set the response callbacks.
    if (status == ::CURLE_OK) {
        status = ::curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, &SystemGuts::writeCallback);
    }
    if (status == ::CURLE_OK) {
        status = ::curl_easy_setopt(_curl, CURLOPT_WRITEDATA, this);
    }
    if (status == ::CURLE_OK) {
        status = ::curl_easy_setopt(_curl, CURLOPT_HEADERFUNCTION, &SystemGuts::headerCallback);
    }
    if (status == ::CURLE_OK) {
        status = ::curl_easy_setopt(_curl, CURLOPT_HEADERDATA, this);
    }

    // Always follow redirections.
    if (status == ::CURLE_OK) {
        status = ::curl_easy_setopt(_curl, CURLOPT_FOLLOWLOCATION, 1L);
    }

    // Set the proxy settings.
    if (status == ::CURLE_OK && !_request._proxyHost.empty()) {
        status = ::curl_easy_setopt(_curl, CURLOPT_PROXY, _request._proxyHost.toUTF8().c_str());
        if (status == ::CURLE_OK && _request._proxyPort != 0) {
            status = ::curl_easy_setopt(_curl, CURLOPT_PROXYPORT, long(_request._proxyPort));
        }
        if (status == ::CURLE_OK && !_request._proxyUser.empty()) {
            status = ::curl_easy_setopt(_curl, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
            if (status == ::CURLE_OK) {
                status = ::curl_easy_setopt(_curl, CURLOPT_PROXYUSERNAME, _request._proxyUser.toUTF8().c_str());
            }
            if (status == ::CURLE_OK && !_request._proxyPassword.empty()) {
                status = ::curl_easy_setopt(_curl, CURLOPT_PROXYPASSWORD, _request._proxyPassword.toUTF8().c_str());
            }
        }
    }

    // Now process setopt error.
    if (status != ::CURLE_OK) {
        _request._report.error(message(u"libcurl setopt error", status));
        clear();
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// System-specific cleanup.
//----------------------------------------------------------------------------

void ts::WebRequest::SystemGuts::clear()
{
    // Make sure the CURL Easy is clean.
    if (_curl != 0) {
        ::curl_easy_cleanup(_curl);
        _curl = 0;
    }

    // Erase nul-terminated error message.
    _error[0] = 0;
}


//----------------------------------------------------------------------------
// Build an error message from libcurl.
//----------------------------------------------------------------------------

ts::UString ts::WebRequest::SystemGuts::message(const UString& title, ::CURLcode code)
{
    UString msg(title);
    if (code != ::CURLE_OK) {
        msg.append(u", ");
        const char* err = ::curl_easy_strerror(code);
        if (err != 0 && err[0] != 0) {
            msg.append(UString::FromUTF8(err));
        }
        else {
            msg.append(u"error code ");
            msg.append(UString::Decimal(int(code)));
        }
    }
    if (_error[0] != 0) {
        msg.append(u", ");
        msg.append(UString::FromUTF8(_error));
    }
    return msg;
}


//----------------------------------------------------------------------------
// Perform initialization before any download.
//----------------------------------------------------------------------------

bool ts::WebRequest::downloadInitialize()
{
    // Check that libcul was correctly initialized.
    if (LibCurlInit::Instance()->initStatus != ::CURLE_OK) {
        _report.error(_guts->message(u"libcurl initialization error", LibCurlInit::Instance()->initStatus));
        return false;
    }

    // Initialize "CURL Easy".
    return _guts->init();
}


//----------------------------------------------------------------------------
// Libcurl callback for response headers.
//----------------------------------------------------------------------------

size_t ts::WebRequest::SystemGuts::headerCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    // The userdata points to the guts object.
    SystemGuts* guts = reinterpret_cast<SystemGuts*>(userdata);
    if (guts == 0) {
        return 0; // error
    }

    // Header size.
    const size_t headerSize = size * nmemb;

    // Store headers in the request.
    guts->_request.processHeaders(UString::FromUTF8(ptr, headerSize));

    return headerSize;
}


//----------------------------------------------------------------------------
// Libcurl callback for response data.
//----------------------------------------------------------------------------

size_t ts::WebRequest::SystemGuts::writeCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    // The userdata points to the guts object.
    SystemGuts* guts = reinterpret_cast<SystemGuts*>(userdata);
    if (guts == 0) {
        return 0; // error
    }

    // Data size.
    const size_t dataSize = size * nmemb;

    //@@@@@

    return dataSize;
}
