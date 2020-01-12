//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Stubs when libcurl is not available.
//----------------------------------------------------------------------------

#if defined(TS_NO_CURL)

#define TS_NO_CURL_MESSAGE u"This version of TSDuck was compiled without Web support"

class ts::WebRequest::SystemGuts {};
void ts::WebRequest::allocateGuts() { _guts = new SystemGuts; }
void ts::WebRequest::deleteGuts() { delete _guts; }
bool ts::WebRequest::downloadInitialize() { _report.error(TS_NO_CURL_MESSAGE); return false; }
void ts::WebRequest::downloadClose() {}
bool ts::WebRequest::download() { _report.error(TS_NO_CURL_MESSAGE); return false; }
ts::UString ts::WebRequest::GetLibraryVersion() { return UString(); }

#else

//----------------------------------------------------------------------------
// Normal libcurl support
//----------------------------------------------------------------------------

// Some curl macros contains "old style" casts.
TS_LLVM_NOWARNING(old-style-cast)

#include <curl/curl.h>
//
// The callback CURLOPT_XFERINFOFUNCTION was introduced with libcurl 7.32.0.
// Before this version, CURLOPT_PROGRESSFUNCTION shall be used.
//
// The function is equivalent, the callbacks are equivalent, except
// that the old callback used "double" parameters while the new one
// uses "curl_off_t" values.
//
#if LIBCURL_VERSION_NUM < 0x072000
    #define TS_CALLBACK_PARAM     double
    #define TS_CALLBACK_FUNCTION  CURLOPT_PROGRESSFUNCTION
    #define TS_CALLBACK_DATA      CURLOPT_PROGRESSDATA
#else
    #define TS_CALLBACK_PARAM     ::curl_off_t
    #define TS_CALLBACK_FUNCTION  CURLOPT_XFERINFOFUNCTION
    #define TS_CALLBACK_DATA      CURLOPT_XFERINFODATA
#endif


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
     TS_NOBUILD_NOCOPY(SystemGuts);
public:
    // Constructor with a reference to parent WebRequest.
    SystemGuts(WebRequest& request);

    // Destructor.
    ~SystemGuts();

    // Initialize, clear, start CURL Easy transfer.
    bool init();
    void clear();
    bool start();

    // Build an error message from libcurl.
    UString message(const UString& title, ::CURLcode = ::CURLE_OK);

private:
    WebRequest&   _request;
    ::CURL*       _curl;
    ::curl_slist* _headers;
    char          _error[CURL_ERROR_SIZE];  // Error message buffer for libcurl.

    // Libcurl callbacks for response headers and response data.
    // The userdata points to this object.
    static size_t headerCallback(char *ptr, size_t size, size_t nmemb, void *userdata);
    static size_t writeCallback(char *ptr, size_t size, size_t nmemb, void *userdata);
    static int progressCallback(void *clientp, TS_CALLBACK_PARAM dltotal, TS_CALLBACK_PARAM dlnow, TS_CALLBACK_PARAM ultotal, TS_CALLBACK_PARAM ulnow);
};


//----------------------------------------------------------------------------
// System-specific constructor and destructor.
//----------------------------------------------------------------------------

ts::WebRequest::SystemGuts::SystemGuts(WebRequest& request) :
    _request(request),
    _curl(nullptr),
    _headers(nullptr),
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

    // Initialize CURL Easy
    if ((_curl = ::curl_easy_init()) == nullptr) {
        _request._report.error(u"libcurl 'curl easy' initialization error");
        return false;
    }

    // The curl_easy_setopt() function is a strange macro which triggers warnings.
    TS_PUSH_WARNING()
    TS_LLVM_NOWARNING(disabled-macro-expansion)

    // Setup the error message buffer.
    ::CURLcode status = ::curl_easy_setopt(_curl, CURLOPT_ERRORBUFFER, _error);

    // Set the user agent.
    if (status == ::CURLE_OK && !_request._userAgent.empty()) {
        status = ::curl_easy_setopt(_curl, CURLOPT_USERAGENT, _request._userAgent.toUTF8().c_str());
    }

    // Set the starting URL.
    if (status == ::CURLE_OK) {
        status = ::curl_easy_setopt(_curl, CURLOPT_URL, _request._originalURL.toUTF8().c_str());
    }

    // Set the connection timeout.
    if (status == ::CURLE_OK && _request._connectionTimeout > 0) {
        status = ::curl_easy_setopt(_curl, CURLOPT_CONNECTTIMEOUT_MS, long(_request._connectionTimeout));
    }

    // Set the receive timeout. There is no such parameter in libcurl.
    // We set this timeout to the max duration of low speed = 1 B/s.
    if (status == ::CURLE_OK && _request._receiveTimeout > 0) {
        // The LOW_SPEED_TIME option is in seconds. Round to higher.
        const long timeout = long((_request._receiveTimeout + 999) / 1000);
        status = ::curl_easy_setopt(_curl, CURLOPT_LOW_SPEED_TIME, timeout);
        if (status == ::CURLE_OK) {
            status = ::curl_easy_setopt(_curl, CURLOPT_LOW_SPEED_LIMIT, long(1)); // bytes/second
        }
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
    if (status == ::CURLE_OK) {
        status = ::curl_easy_setopt(_curl, TS_CALLBACK_FUNCTION, &SystemGuts::progressCallback);
    }
    if (status == ::CURLE_OK) {
        status = ::curl_easy_setopt(_curl, TS_CALLBACK_DATA, this);
    }
    if (status == ::CURLE_OK) {
        // Enable progress meter.
        status = ::curl_easy_setopt(_curl, CURLOPT_NOPROGRESS, 0L);
    }

    // Always follow redirections.
    if (status == ::CURLE_OK) {
        status = ::curl_easy_setopt(_curl, CURLOPT_FOLLOWLOCATION, _request._autoRedirect ? 1L : 0L);
    }

    // Set the proxy settings.
    if (status == ::CURLE_OK && !_request.proxyHost().empty()) {
        status = ::curl_easy_setopt(_curl, CURLOPT_PROXY, _request.proxyHost().toUTF8().c_str());
        if (status == ::CURLE_OK && _request.proxyPort() != 0) {
            status = ::curl_easy_setopt(_curl, CURLOPT_PROXYPORT, long(_request.proxyPort()));
        }
        if (status == ::CURLE_OK && !_request.proxyUser().empty()) {
            status = ::curl_easy_setopt(_curl, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
            if (status == ::CURLE_OK) {
                status = ::curl_easy_setopt(_curl, CURLOPT_PROXYUSERNAME, _request.proxyUser().toUTF8().c_str());
            }
            if (status == ::CURLE_OK && !_request.proxyPassword().empty()) {
                status = ::curl_easy_setopt(_curl, CURLOPT_PROXYPASSWORD, _request.proxyPassword().toUTF8().c_str());
            }
        }
    }

    // Set the cookie file.
    if (status == ::CURLE_OK && _request._useCookies) {
        // COOKIEFILE can be empty.
        status = ::curl_easy_setopt(_curl, CURLOPT_COOKIEFILE, _request._cookiesFileName.toUTF8().c_str());
    }
    if (status == ::CURLE_OK && _request._useCookies && !_request._cookiesFileName.empty()) {
        // COOKIEJAR cannot be empty.
        status = ::curl_easy_setopt(_curl, CURLOPT_COOKIEJAR, _request._cookiesFileName.toUTF8().c_str());
    }

    // Set the request headers.
    if (status == ::CURLE_OK && !_request._requestHeaders.empty()) {
        for (HeadersMap::const_iterator it = _request._requestHeaders.begin(); it != _request._requestHeaders.end(); ++it) {
            const UString header(it->first + u": " + it->second);
            _headers = ::curl_slist_append(_headers, header.toUTF8().c_str());
        }
        status = ::curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, _headers);
    }

    // End of curl_easy_setopt() sequence.
    TS_POP_WARNING()

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
    // Deallocate list of headers.
    if (_headers != nullptr) {
        ::curl_slist_free_all(_headers);
        _headers = nullptr;
    }

    // Make sure the CURL Easy is clean.
    if (_curl != nullptr) {
        ::curl_easy_cleanup(_curl);
        _curl = nullptr;
    }

    // Erase nul-terminated error message.
    _error[0] = 0;
}


//----------------------------------------------------------------------------
// Perform transfer.
//----------------------------------------------------------------------------

bool ts::WebRequest::SystemGuts::start()
{
    assert(_curl != nullptr);

    const ::CURLcode status = ::curl_easy_perform(_curl);
    const bool ok = status == ::CURLE_OK;

    if (!ok && !_request._interrupted) {
        _request._report.error(message(u"download error", status));
    }

    clear();
    return ok;
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
        if (err != nullptr && err[0] != 0) {
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
// Abort initialized download.
//----------------------------------------------------------------------------

void ts::WebRequest::downloadClose()
{
    _guts->clear();
}


//----------------------------------------------------------------------------
// Perform actual download.
//----------------------------------------------------------------------------

bool ts::WebRequest::download()
{
    return _guts->start();
}


//----------------------------------------------------------------------------
// Libcurl callback for response headers.
//----------------------------------------------------------------------------

size_t ts::WebRequest::SystemGuts::headerCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    // The userdata points to the guts object.
    SystemGuts* guts = reinterpret_cast<SystemGuts*>(userdata);
    if (guts == nullptr) {
        return 0; // error
    }
    else {
        // Store headers in the request.
        const size_t headerSize = size * nmemb;
        guts->_request.processReponseHeaders(UString::FromUTF8(ptr, headerSize));
        return headerSize;
    }
}


//----------------------------------------------------------------------------
// Libcurl callback for response data.
//----------------------------------------------------------------------------

size_t ts::WebRequest::SystemGuts::writeCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    const size_t dataSize = size * nmemb;

    // The userdata points to the guts object.
    SystemGuts* guts = reinterpret_cast<SystemGuts*>(userdata);

    // Process downloaded data. Return 0 on error.
    return guts != nullptr && guts->_request.copyData(ptr, dataSize) ? dataSize : 0;
}


//----------------------------------------------------------------------------
// Libcurl progress callback for response data.
//----------------------------------------------------------------------------

int ts::WebRequest::SystemGuts::progressCallback(void *clientp, TS_CALLBACK_PARAM dltotal, TS_CALLBACK_PARAM dlnow, TS_CALLBACK_PARAM ultotal, TS_CALLBACK_PARAM ulnow)
{
    // The clientp points to the guts object.
    SystemGuts* guts = reinterpret_cast<SystemGuts*>(clientp);

    // We only use dltotal to reserve the buffer size. Return 0 on success.
    return guts != nullptr && guts->_request.setPossibleContentSize(size_t(dltotal)) ? 0 : 1;
}


//----------------------------------------------------------------------------
// Get the version of the underlying HTTP library.
//----------------------------------------------------------------------------

ts::UString ts::WebRequest::GetLibraryVersion()
{
    UString result(u"libcurl");

    // Get version from libcurl.
    const ::curl_version_info_data* info = ::curl_version_info(CURLVERSION_NOW);
    if (info != nullptr) {
        if (info->version != nullptr) {
            result += u": " + UString::FromUTF8(info->version);
        }
        if (info->ssl_version != nullptr) {
            result += u", ssl: " + UString::FromUTF8(info->ssl_version);
        }
        if (info->libz_version != nullptr) {
            result += u", libz: " + UString::FromUTF8(info->libz_version);
        }
    }

    return result;
}

#endif // TS_NO_CURL
