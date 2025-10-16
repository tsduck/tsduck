//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
#include "tsWinModuleInfo.h"
#include "tsURL.h"

#include "tsBeforeStandardHeaders.h"
#include <wininet.h>
#include "tsAfterStandardHeaders.h"

// Required link libraries.
#if defined(TS_MSC)
    #pragma comment(lib, "Wininet.lib")
#endif


//----------------------------------------------------------------------------
// Get the version of the underlying HTTP library.
//----------------------------------------------------------------------------

ts::UString ts::WebRequest::GetLibraryVersion()
{
    return WinModuleInfo(::InternetOpenW, nullptr).summary();
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

    // Delegation of methods from WebRequest.
    bool init();
    bool receive(void* buffer, size_t maxSize, size_t& retSize);
    void clear();

private:
    WebRequest&          _request;                 // Parent request.
    volatile ::HINTERNET _inet = nullptr;          // Handle to all Internet operations.
    volatile ::HINTERNET _inet_connect = nullptr;  // Handle to connection operations (POST request only).
    volatile ::HINTERNET _inet_request = nullptr;  // Handle to URL request operations.
    int                  _redirect_count = 0;      // Current number of redirections.
    UString              _previous_url {};         // Previous URL, before getting a redirection.

    // Report an error message.
    void error(const UString& message, ::DWORD code = ::GetLastError());

    // Transmit response headers to the WebRequest.
    void transmitResponseHeaders();
};


//----------------------------------------------------------------------------
// System-specific constructor and destructor.
//----------------------------------------------------------------------------

ts::WebRequest::SystemGuts::SystemGuts(WebRequest& request) :
    _request(request)
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
    if (_is_open) {
        return _guts->receive(buffer, maxSize, retSize);
    }
    else {
        _report.error(u"transfer not started");
        return false;
    }
}

bool ts::WebRequest::close()
{
    bool success = _is_open;
    _guts->clear();
    _is_open = false;
    return success;
}

void ts::WebRequest::abort()
{
    _guts->clear();
}


//----------------------------------------------------------------------------
// Report an error message.
//----------------------------------------------------------------------------

void ts::WebRequest::SystemGuts::error(const UString& message, ::DWORD code)
{
    if (code == ERROR_SUCCESS) {
        _request._report.error(u"Web error: %s", message);
    }
    else {
        _request._report.error(u"Web error: %s (%s)", message, WinErrorMessage(code));
    }
}


//----------------------------------------------------------------------------
// Abort / clear the Web transfer.
//----------------------------------------------------------------------------

void ts::WebRequest::SystemGuts::clear()
{
    // Close Internet handles.
    if (_inet_request != nullptr && !::InternetCloseHandle(_inet_request)) {
        error(u"error closing URL request handle");
    }
    if (_inet_connect != nullptr && !::InternetCloseHandle(_inet_connect)) {
        error(u"error closing connection handle");
    }
    if (_inet != nullptr && !::InternetCloseHandle(_inet)) {
        error(u"error closing main Internet handle");
    }
    _inet_request = nullptr;
    _inet_connect = nullptr;
    _inet = nullptr;
}


//----------------------------------------------------------------------------
// Initialize Web transfer.
//----------------------------------------------------------------------------

bool ts::WebRequest::SystemGuts::init()
{
    // Make sure we start from a clean state.
    clear();
    _redirect_count = 0;

    // Prepare proxy name.
    const bool use_proxy = !_request.proxyHost().empty();
    ::DWORD access_type = INTERNET_OPEN_TYPE_PRECONFIG;
    const ::WCHAR* proxy = nullptr;
    UString proxy_name(_request.proxyHost());

    if (use_proxy) {
        access_type = INTERNET_OPEN_TYPE_PROXY;
        if (_request.proxyPort() != 0) {
            proxy_name += UString::Format(u":%d", _request.proxyPort());
        }
        proxy = proxy_name.wc_str();
    }

    // Open the main Internet handle.
    _inet = ::InternetOpenW(_request._user_agent.wc_str(), access_type, proxy, nullptr, 0);
    if (_inet == nullptr) {
        error(u"error accessing Internet handle");
        return false;
    }

    // Specify the proxy authentication, if provided.
    if (use_proxy) {
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

    // List of request headers as one string.
    UString headers;

    // Set compression.
    if (_request._use_compression) {
        // We must separately set the Accept-Encoding header and configure automatic decompression.
        headers = u"Accept-Encoding: deflate, gzip";
        ::BOOL mode = TRUE;
        if (!::InternetSetOptionW(_inet, INTERNET_OPTION_HTTP_DECODING, &mode, ::DWORD(sizeof(mode)))) {
            error(u"error setting compression mode");
            clear();
            return false;
        }
    }

    // Specify the various timeouts.
    if (_request._connection_timeout.count() > 0) {
        ::DWORD timeout = ::DWORD(_request._connection_timeout.count());
        if (!::InternetSetOptionW(_inet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, ::DWORD(sizeof(timeout)))) {
            error(u"error setting connection timeout");
            clear();
            return false;
        }
    }
    if (_request._receive_timeout.count() > 0) {
        ::DWORD timeout = ::DWORD(_request._receive_timeout.count());
        if (!::InternetSetOptionW(_inet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, ::DWORD(sizeof(timeout))) ||
            !::InternetSetOptionW(_inet, INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, &timeout, ::DWORD(sizeof(timeout))))
        {
            error(u"error setting receive timeout");
            clear();
            return false;
        }
    }

    // URL connection flags.
    const ::DWORD url_flags =
        INTERNET_FLAG_KEEP_CONNECTION |   // Use keep-alive.
        INTERNET_FLAG_NO_UI |             // Disable popup windows.
        (_request._use_cookies ? 0 : INTERNET_FLAG_NO_COOKIES) | // Don't store cookies, don't send stored cookies.
        INTERNET_FLAG_PASSIVE |           // Use passive mode with FTP (less NAT issues).
        INTERNET_FLAG_NO_AUTO_REDIRECT |  // Disable redirections (see comment on top of file).
        INTERNET_FLAG_NO_CACHE_WRITE;     // Don't save downloaded data to local disk cache.

    // Build the list of request headers.
    if (!_request._request_headers.empty()) {
        for (const auto& it : _request._request_headers) {
            if (!headers.empty()) {
                headers.append(u"\r\n");
            }
            headers.append(it.first);
            headers.append(u": ");
            headers.append(it.second);
        }
    }
    ::WCHAR* header_address = nullptr;
    ::DWORD  header_length = 0;
    if (!headers.empty()) {
        header_address = headers.wc_str();
        header_length = ::DWORD(headers.size());
    }

    // Loop on redirections.
    for (;;) {
        // Keep track of current URL to fetch.
        _previous_url = _request._final_url;
        const bool use_http = _previous_url.starts_with(u"http:");
        const bool use_https = _previous_url.starts_with(u"https:");
        const bool use_post = !_request._post_data.empty();

        // Flags for HTTPS.
        ::DWORD flags = url_flags;
        if (use_https) {
            flags |= INTERNET_FLAG_SECURE;
        }

        // POST requests are supported in http: and https: schemes only.
        if (use_post && !use_http && !use_https) {
            error(u"POST requests are only allowed on HTTP URL: " + _previous_url);
            clear();
            return false;
        }

        // Now open the URL. We have the choice between:
        // - InternetOpenUrl()
        // - InternetConnect() + HttpOpenRequest() + HttpSendRequest()
        // InternetOpenUrl() is easier, more general, and can handle all types of URL. However, in the case of
        // HTTP(S), it can handle GET requests only (not POST) and cannot disable all security options. Therefore,
        // we use InternetOpenUrl() when possible and fallback to the complex scenario otherwise.
        if (!use_post && !_request._insecure) {
            // This can be handled by InternetOpenUrl() in one call.
            _inet_request = ::InternetOpenUrlW(_inet, _previous_url.wc_str(), header_address, header_length, flags, 0);
            if (_inet_request == nullptr) {
                error(u"error opening URL: " + _previous_url);
                clear();
                return false;
            }
        }
        else {
            // This is an HTTP(S) case that InternetOpenUrl() cannot handle.
            // We need to split the URL.
            URL url(_previous_url);
            UString host(url.getHost());
            UString user(url.getUserName());
            UString pass(url.getPassword());
            uint16_t port = url.getPort();
            if (port == 0) {
                // Use default port.
                port = use_https ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
            }
            if (use_https && _request._insecure) {
                // In secure mode, only some flags can be set to InternetConnect(). Others must be added later.
                flags |= INTERNET_FLAG_IGNORE_CERT_CN_INVALID;
            }

            // Connect to the host.
            _inet_connect = ::InternetConnectW(_inet, host.wc_str(), port,
                                               user.empty() ? nullptr : user.wc_str(),
                                               pass.empty() ? nullptr : pass.wc_str(),
                                               INTERNET_SERVICE_HTTP, flags, 0);
            if (_inet_connect == nullptr) {
                error(u"error connecting to host " + host);
                clear();
                return false;
            }

            // Build the request.
            const wchar_t* accept_types[] = {L"*/*", nullptr};
            UString path(url.getPath());
            UString query(url.getQuery());
            if (!query.empty()) {
                path.append(u'?');
                path.append(query);
            }
            _inet_request = ::HttpOpenRequestW(_inet_connect, use_post ? L"POST" : L"GET", path.wc_str(), nullptr, nullptr, accept_types, flags | INTERNET_FLAG_RELOAD, 0);
            if (_inet_request == nullptr) {
                error(u"error opening request to " + _previous_url);
                clear();
                return false;
            }

            // Set additional insecure flags after HttpOpenRequest() and before HttpSendRequest().
            if (use_https && _request._insecure) {
                // Get current security flags.
                ::DWORD cur_flags = 0;
                ::DWORD ret_size = ::DWORD(sizeof(cur_flags));
                if (!::InternetQueryOptionW(_inet_request, INTERNET_OPTION_SECURITY_FLAGS, &cur_flags, &ret_size)) {
                    error(u"error getting security flags on HTTP request");
                    clear();
                    return false;
                }
                // Now add other insecure flags.
                cur_flags |= INTERNET_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID | SECURITY_FLAG_IGNORE_REVOCATION | SECURITY_FLAG_IGNORE_UNKNOWN_CA;
                if (!::InternetSetOptionW(_inet_request, INTERNET_OPTION_SECURITY_FLAGS, &cur_flags, ::DWORD(sizeof(cur_flags)))) {
                    error(u"error setting insecure mode");
                    clear();
                    return false;
                }
            }

            // Send the request.
            if (!::HttpSendRequestW(_inet_request, header_address, header_length, _request._post_data.data(), ::DWORD(_request._post_data.size()))) {
                error(u"error sending request to " + _previous_url);
                clear();
                return false;
            }
        }

        // Send the response headers to the WebRequest object.
        // Do not expect any response header from file: URL.
        if (_previous_url.starts_with(u"file:")) {
            // Pass empty headers to the WebRequest.
            _request.processReponseHeaders(u"");
        }
        else {
            // Get actual response headers and pass them to the WebRequest.
            transmitResponseHeaders();
        }

        // If redirections are not allowed or no redirection occured, stop now.
        // Redirection codes are 3xx (eg. "HTTP/1.1 301 Moved Permanently").
        if (!_request._auto_redirect || _request._http_status / 100 != 3 || _request._final_url == _previous_url) {
            break;
        }

        // Close this URL, we need to redirect to _final_url.
        if (_inet_connect != nullptr) {
            // This is a POST request using an intermediate connection.
            ::InternetCloseHandle(_inet_connect);
            _inet_connect = nullptr;
        }
        ::InternetCloseHandle(_inet_request);
        _inet_request = nullptr;

        // Limit the number of redirections to avoid "looping sites".
        if (++_redirect_count > 16) {
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
    const bool success = ::InternetReadFile(_inet_request, buffer, ::DWORD(maxSize), &thisSize);

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
    if (!::HttpQueryInfoW(_inet_request, HTTP_QUERY_RAW_HEADERS_CRLF, &headers[0], &headersSize, &index)) {

        // Process actual error.
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            error(u"error getting HTTP response headers");
            return;
        }

        // The buffer was too small, reallocate one.
        headers.resize(size_t(headersSize));
        headersSize = ::DWORD(headers.size());
        index = 0;
        if (!::HttpQueryInfoW(_inet_request, HTTP_QUERY_RAW_HEADERS_CRLF, &headers[0], &headersSize, &index)) {
            error(u"error getting HTTP response headers");
            return;
        }
    }

    // Adjust actual string length.
    headers.resize(std::min(std::max<::DWORD>(0, headersSize), ::DWORD(headers.size() - 1)));

    // Pass the headers to the WebRequest.
    _request.processReponseHeaders(headers);
}
