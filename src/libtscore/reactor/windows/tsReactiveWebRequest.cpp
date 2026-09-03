//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Perform a simple Web request (HTTP, HTTPS, FTP) in a Reactor environment.
//  Windows specific parts with WinInet.
//
//  IMPLEMENTATION NOTES:
//  We use WinInet in asynchronous mode. All results and progresses are
//  notified through the "WinInet status callback". This callback can be
//  synchronously called from a WinInet call or asynchronously called from
//  an internal worker thread of WinInet. Therefore, all data which are
//  manipulated by the callback shall be protected by a mutex.
//
//----------------------------------------------------------------------------

#include "tsReactiveWebRequest.h"
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

ts::UString ts::ReactiveWebRequest::GetLibraryVersion()
{
    static const WinModuleInfo info(::InternetOpenW, nullptr);
    return info.summary();
}


//----------------------------------------------------------------------------
// System-specific parts are stored in a private structure.
//----------------------------------------------------------------------------

class ts::ReactiveWebRequest::Guts: private ReactorHandlerInterface
{
    TS_NOBUILD_NOCOPY(Guts);
public:
    // Constructor with a reference to parent WebRequest.
    Guts(ReactiveWebRequest& request);
    virtual ~Guts() override;

    // Close and cleanup everything.
    void reset();

    // Start the transfer operation.
    bool start(HandlerType* handler, const UString& url, const ObjectPtr& user_data);

    // Abort the current transfer.
    bool abort(bool silent);

private:
    // Type of asynchronous operation in progress.
    enum AsyncOp {AsyncNone, AsyncInternetOpenUrl, AsyncInternetConnect, AsyncHttpOpenRequest, AsyncHttpSendRequest, AsyncInternetReadFile};

    // Guts private fields.
    ReactiveWebRequest&  _request;                 // Reference to parent instance.
    HandlerType*         _handler = nullptr;       // Application-defined handler.
    ObjectPtr            _handler_data {};         // User-data for _handler.
    EventId              _event {};                // Some event occurred, maybe call a handler.
    volatile ::HINTERNET _inet = nullptr;          // Handle to all Internet operations.
    volatile ::HINTERNET _inet_connect = nullptr;  // Handle to connection operations (POST request only).
    volatile ::HINTERNET _inet_request = nullptr;  // Handle to URL request operations.
    bool                 _use_http = false;
    bool                 _use_https = false;
    bool                 _use_insecure = false;
    bool                 _use_post = false;
    void*                _post_data = nullptr;
    ::DWORD              _post_size = 0;
    ::DWORD              _url_flags = 0;
    AsyncOp              _async_op = AsyncNone;    // Current asynchronous operation.
    UString              _host {};                 // String buffer for host name (InternetConnectW).
    UString              _user {};                 // String buffer for user name (InternetConnectW).
    UString              _pass {};                 // String buffer for password (InternetConnectW).
    UString              _request_headers {};      // Buffer for all request headers.
    ::WCHAR*             _headers_addr = nullptr;  // Request headers address, for WinInet calls.
    ::DWORD              _headers_len = 0;         // Request headers length, for WinInet calls.

    // Guts private fields which can be used in the status callback, from a WinInet internal thread.
    std::mutex                             _mutex {};         // Protect access to all following fields.
    ByteBlockPtr                           _received_data {}; // Received data, filled by WinInetStatusCallback(), emptied by receive callback.
    std::optional<UString>                 _last_url {};      // Last redirected URL as set in the callback.
    std::optional<::INTERNET_ASYNC_RESULT> _async_result {};  // Last completion as set in the callback.

    // Transmit response headers to the WebRequest.
    void transmitResponseHeaders();

    // WinInet status callback. Can be called from another thread (internal WinInet thread).
    static void CALLBACK WinInetStatusCallback(::HINTERNET handle, ::DWORD_PTR context, ::DWORD status, void* info, ::DWORD info_size);

    // Implementation of ReactorHandlerInterface.
    virtual void handleUserEvent(Reactor& reactor, EventId id) override;
};


//----------------------------------------------------------------------------
// System-specific constructors and destructor.
//----------------------------------------------------------------------------

ts::ReactiveWebRequest::Guts::Guts(ReactiveWebRequest& request) :
    _request(request)
{
}

ts::ReactiveWebRequest::Guts::~Guts()
{
    reset();
}

void ts::ReactiveWebRequest::Guts::reset()
{
    if (_inet_request != nullptr) {
        ::InternetCloseHandle(_inet_request);
        _inet_request = nullptr;
    }
    if (_inet_connect != nullptr) {
        ::InternetCloseHandle(_inet_connect);
        _inet_connect = nullptr;
    }
    if (_inet != nullptr) {
        ::InternetCloseHandle(_inet);
        _inet = nullptr;
    }
    if (_event.isValid()) {
        _request.reactor().deleteEvent(_event, true);
        _event.invalidate();
    }

    _handler = nullptr;
    _handler_data.reset();
    _use_http = _use_https = _use_post = _use_insecure = false;
    _post_data = nullptr;
    _post_size = _url_flags = 0;
    _async_op = AsyncNone;
    _host.clear();
    _user.clear();
    _pass.clear();
    _request_headers.clear();
    _headers_addr = nullptr;
    _headers_len = 0;

    std::lock_guard<std::mutex> lock(_mutex);
    _received_data.reset();
    _last_url.reset();
    _async_result.reset();
}


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::ReactiveWebRequest::ReactiveWebRequest(Reactor& reactor) :
    ReactiveBase(reactor),
    _guts(new Guts(*this))
{
}

ts::ReactiveWebRequest::~ReactiveWebRequest()
{
    if (_guts != nullptr) {
        delete _guts;
        _guts = nullptr;
    }
    _args.deleteTemporaryCookiesFile(report());
}


//----------------------------------------------------------------------------
// Macro to set an option on the Internet handle.
//----------------------------------------------------------------------------

#define TS_INET_SETOPT(name, value_addr, value_size)                                                    \
    if (!::InternetSetOptionW(_inet, name, value_addr, ::DWORD(value_size))) {                          \
        _request.report().error(u"error setting WinInet option " #name u": %s", SysErrorCodeMessage()); \
        reset();                                                                                        \
        return false;                                                                                   \
    }                                                                                                   \
    using TS_UNIQUE_NAME(for_trailing_semicolon) [[maybe_unused]] = int


//----------------------------------------------------------------------------
// Start the operation of opening an URL.
//----------------------------------------------------------------------------

bool ts::ReactiveWebRequest::start(ReactiveWebHandlerInterface* handler, const UString& url, const ObjectPtr& user_data)
{
    return _guts->start(handler, url, user_data);
}

bool ts::ReactiveWebRequest::Guts::start(ReactiveWebHandlerInterface* handler, const UString& url, const ObjectPtr& user_data)
{
    if (url.empty()) {
        _request.report().error(u"no URL specified");
        return false;
    }
    if (_inet != nullptr) {
        _request.report().error(u"internal error, transfer already started, cannot download %s", url);
        return false;
    }

    // Make sure we start from a clean state.
    reset();
    _received_data = std::make_shared<ByteBlock>();

    // URL characteristics.
    _request._status.reset(url);
    _use_http = url.starts_with(u"http:");
    _use_https = url.starts_with(u"https:");
    _use_insecure = _request.args().isInsecure();
    _use_post = _request.args().isPost();
    _post_data = _use_post ? const_cast<uint8_t*>(_request.args().postData().data()) : nullptr;
    _post_size = _use_post ? ::DWORD(_request.args().postData().size()) : 0;

    // POST requests are supported in http: and https: schemes only.
    if (_use_post && !_use_http && !_use_https) {
        _request.report().error(u"POST requests are only allowed on HTTP URL: " + url);
        reset();
        return false;
    }

    // URL connection flags.
    _url_flags =
        INTERNET_FLAG_KEEP_CONNECTION |                                          // Use keep-alive.
        INTERNET_FLAG_NO_UI |                                                    // Disable popup windows.
        (_request.args().cookiesEnabled() ? 0 : INTERNET_FLAG_NO_COOKIES) |      // Don't store cookies, don't send stored cookies.
        INTERNET_FLAG_PASSIVE |                                                  // Use passive mode with FTP (less NAT issues).
        (_request.args().autoRedirect() ? 0 : INTERNET_FLAG_NO_AUTO_REDIRECT) |  // Don't follow HTTP redirections.
        INTERNET_FLAG_NO_CACHE_WRITE |                                           // Don't save downloaded data to local disk cache.
        (_use_https ? INTERNET_FLAG_SECURE : 0);

    // Prepare proxy name.
    UString proxy_name(_request.args().proxyHost());
    const bool use_proxy = !proxy_name.empty();
    ::DWORD access_type = INTERNET_OPEN_TYPE_PRECONFIG;
    const ::WCHAR* proxy = nullptr;

    if (use_proxy) {
        access_type = INTERNET_OPEN_TYPE_PROXY;
        if (_request.args().proxyPort() != 0) {
            proxy_name.format(u":%d", _request.args().proxyPort());
        }
        proxy = proxy_name.wc_str();
    }

    // Open the main Internet handle. All operations will be asynchronous on all derived handles.
    _inet = ::InternetOpenW(_request.args().userAgent().wc_str(), access_type, proxy, nullptr, INTERNET_FLAG_ASYNC);
    if (_inet == nullptr) {
        _request.report().error(u"error accessing Internet handle: %s", SysErrorCodeMessage());
        return false;
    }

    // Specify the callback for all derived handles and its dwContext parameter.
    ::InternetSetStatusCallbackW(_inet, WinInetStatusCallback);
    ::DWORD_PTR cb_param = ::DWORD_PTR(this);
    TS_INET_SETOPT(INTERNET_OPTION_CONTEXT_VALUE, &cb_param, sizeof(cb_param));

    // Specify the proxy authentication, if provided.
    if (use_proxy) {
        UString user(_request.args().proxyUser());
        UString pass(_request.args().proxyPassword());
        if (!user.empty()) {
            TS_INET_SETOPT(INTERNET_OPTION_PROXY_USERNAME, user.data(), user.size());
        }
        if (!pass.empty()) {
            TS_INET_SETOPT(INTERNET_OPTION_PROXY_PASSWORD, pass.data(), pass.size());
        }
    }

    // Set compression.
    if (_request.args().compressionEnabled()) {
        // We must separately set the Accept-Encoding header and configure automatic decompression.
        _request_headers = u"Accept-Encoding: deflate, gzip";
        ::BOOL mode = true;
        TS_INET_SETOPT(INTERNET_OPTION_HTTP_DECODING, &mode, sizeof(mode));
    }

    // Specify the various timeouts.
    ::DWORD timeout = ::DWORD(_request.args().connectionTimeout().count());
    if (timeout > 0) {
        TS_INET_SETOPT(INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    }
    timeout = ::DWORD(_request.args().receiveTimeout().count());
    if (timeout > 0) {
        TS_INET_SETOPT(INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
        TS_INET_SETOPT(INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
    }

    // Build the list of request headers.
    _request.args().appendRequestHeaders(_request_headers);
    if (!_request_headers.empty()) {
        _headers_addr = _request_headers.wc_str();
        _headers_len = ::DWORD(_request_headers.size());
    }

    // Now open the URL. We have the choice between:
    // - InternetOpenUrl()
    // - InternetConnect() + HttpOpenRequest() + HttpSendRequest()
    // InternetOpenUrl() is easier, more general, and can handle all types of URL. However, in the case of
    // HTTP(S), it can handle GET requests only (not POST) and cannot disable all security options. Therefore,
    // we use InternetOpenUrl() when possible and fallback to the complex scenario otherwise.

    if (!_use_post && !_use_insecure) {
        // This can be handled by InternetOpenUrl() in one call.
        _async_op = AsyncInternetOpenUrl;
        _inet_request = ::InternetOpenUrlW(_inet, _request._status.originalURL().wc_str(), _headers_addr, _headers_len, _url_flags, ::DWORD_PTR(this));
        ::DWORD err = ::GetLastError();
        if (_inet_request != nullptr) {
            // Synchronous completion, simulate an asynchronous one.
            _async_result.emplace();
            _async_result->dwResult = true; // success
            _async_result->dwError = ERROR_SUCCESS;
            _request.reactor().signalEvent(_event);
        }
        else if (err != ERROR_IO_PENDING) {
            // Actual error, not an asynchronous completion.
            _request.report().error(u"error opening URL %s: %s", url, WinErrorMessage(err));
            return false;
        }
        // Else, asynchronous completion. The status callback INTERNET_STATUS_HANDLE_CREATED will provide
        // the handle value and the callback INTERNET_STATUS_REQUEST_COMPLETE will signal the final status.
    }
    else {
        // This is an HTTP(S) case that InternetOpenUrl() cannot handle.
        // We need to split the URL.
        const URL u(url);
        _host = u.getHost();
        _user = u.getUserName();
        _pass = u.getPassword();
        uint16_t port = u.getPort();
        if (port == 0) {
            // Use default port.
            port = _use_https ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
        }
        if (_use_https && _use_insecure) {
            // In secure mode, only some flags can be set to InternetConnect(). Others must be added later.
            _url_flags |= INTERNET_FLAG_IGNORE_CERT_CN_INVALID;
        }

        // Connect to the host.
        _async_op = AsyncInternetConnect;
        _inet_connect = ::InternetConnectW(_inet, _host.wc_str(), port,
                                           _user.empty() ? nullptr : _user.wc_str(),
                                           _pass.empty() ? nullptr : _pass.wc_str(),
                                           INTERNET_SERVICE_HTTP, _url_flags, ::DWORD_PTR(this));
        ::DWORD err = ::GetLastError();
        if (_inet_connect != nullptr) {
            // Synchronous completion, simulate an asynchronous one.
            _async_result.emplace();
            _async_result->dwResult = true;  // success
            _async_result->dwError = ERROR_SUCCESS;
            _request.reactor().signalEvent(_event);
        }
        else if (err != ERROR_IO_PENDING) {
            // Actual error, not an asynchronous completion.
            _request.report().error(u"error connecting to host %s: %s", _host, WinErrorMessage(err));
            return false;
        }
        // Else, asynchronous completion. The status callback INTERNET_STATUS_HANDLE_CREATED will provide
        // the handle value and the callback INTERNET_STATUS_REQUEST_COMPLETE will signal the final status.
    }

    /*@@@@


            // Build the request.
            const wchar_t* accept_types[] = {L"*SLASH*", nullptr};
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
            if (use_https && use_insecure) {
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

            // POST data to send. HttpSendRequestW needs a non-const pointer but the data are unmodified.
            void* post_data = nullptr;
            ::DWORD post_size = 0;
            if (use_post) {
                post_data = const_cast<uint8_t*>(_request.args().postData().data());
                post_size = ::DWORD(_request.args().postData().size());
            }

            // Send the request.
            if (!::HttpSendRequestW(_inet_request, header_address, header_length, post_data, post_size)) {
                error(u"error sending request to " + _previous_url);
                clear();
                return false;
            }
        }

        // Send the response headers to the WebRequest object.
        // Do not expect any response header from "file:" URL.
        if (!_previous_url.starts_with(u"file:")) {
            transmitResponseHeaders();
        }

    @@@*/

    return false; //@@@
}


//----------------------------------------------------------------------------
// Transmit response headers to the WebRequest.
//----------------------------------------------------------------------------

void ts::ReactiveWebRequest::Guts::transmitResponseHeaders()
{
    // Query the response headers from the URL handle.
    // First try with an arbitrary buffer size.
    UString headers(1024, CHAR_NULL);
    ::DWORD headers_size = ::DWORD(headers.size());
    ::DWORD index = 0;
    if (!::HttpQueryInfoW(_inet_request, HTTP_QUERY_RAW_HEADERS_CRLF, headers.data(), &headers_size, &index)) {

        // Process actual error.
        ::DWORD err = ::GetLastError();
        if (err != ERROR_INSUFFICIENT_BUFFER) {
            _request.report().error(u"error getting HTTP response headers: %s", WinErrorMessage(err));
            return;
        }

        // The buffer was too small, reallocate one.
        headers.resize(size_t(headers_size));
        headers_size = ::DWORD(headers.size());
        index = 0;
        if (!::HttpQueryInfoW(_inet_request, HTTP_QUERY_RAW_HEADERS_CRLF, headers.data(), &headers_size, &index)) {
            err = ::GetLastError();
            _request.report().error(u"error getting HTTP response headers: %s", WinErrorMessage(err));
            return;
        }
    }

    // Adjust actual string length.
    headers.resize(std::min(std::max<::DWORD>(0, headers_size), ::DWORD(headers.size() - 1)));

    // Pass the headers to the WebRequest.
    _request._status.processReponseHeaders(headers, _request.report());
}


//----------------------------------------------------------------------------
// WinInet status callback.
//----------------------------------------------------------------------------

void ts::ReactiveWebRequest::Guts::WinInetStatusCallback(::HINTERNET handle, ::DWORD_PTR context, ::DWORD status, void* info, ::DWORD info_size)
{
    // The context of the WinInet handle is the address of the request's guts.
    const auto guts = reinterpret_cast<Guts*>(context);
    if (guts != nullptr && info != nullptr) {
        // Warning: this callback can be called in the context of a WinInet internal thread.
        // Make sure that all accesses in guts are properly synchronized. Do NOT use the report()
        // of the reactor since most reactors are single-threaded and use a non-thread-safe report.
        if (status == INTERNET_STATUS_REDIRECT) {
            // About to redirect to a new URL.
            std::lock_guard<std::mutex> lock(guts->_mutex);
            guts->_last_url.emplace(static_cast<::WCHAR*>(info), size_t(info_size));
            guts->_request.reactor().signalEvent(guts->_event);
        }
        else if (status == INTERNET_STATUS_HANDLE_CREATED && info_size >= sizeof(::INTERNET_ASYNC_RESULT)) {
            // Intermediate phase where a handle is created. We can directly update the handle in the Guts
            // because the handles are volatile and this callback is invoked when the corresponding handle
            // in Guts is unused (because it is being created).
            const ::HINTERNET h = ::HINTERNET(static_cast<::INTERNET_ASYNC_RESULT*>(info)->dwResult);
            if (guts->_async_op == AsyncInternetOpenUrl || guts->_async_op == AsyncHttpOpenRequest) {
                guts->_inet_request = h;
            }
            else if (guts->_async_op == AsyncInternetConnect) {
                guts->_inet_connect = h;
            }
            else {
                // Shouldn't get there.
                assert(false);
            }
        }
        else if (status == INTERNET_STATUS_REQUEST_COMPLETE && info_size >= sizeof(::INTERNET_ASYNC_RESULT)) {
            // An asynchronous operation is complete.
            std::lock_guard<std::mutex> lock(guts->_mutex);
            guts->_async_result = *static_cast<::INTERNET_ASYNC_RESULT*>(info);
            guts->_request.reactor().signalEvent(guts->_event);
        }
    }
}


//----------------------------------------------------------------------------
// Handle a user-defined event in a Reactor.
//----------------------------------------------------------------------------

void ts::ReactiveWebRequest::Guts::handleUserEvent(Reactor& reactor, EventId id)
{
    // Filter events.
    if (id == _event) {
        // Local copy of a completion status, outside lock.
        std::optional<::INTERNET_ASYNC_RESULT> result;

        // Process data from the WinInet status callback under lock.
        {
            std::lock_guard<std::mutex> lock(_mutex);
            result.swap(_async_result);
            if (!_last_url.has_value()) {
                if (!_last_url->empty()) {
                    _request._status.setFinalURL(*_last_url);
                }
                _last_url.reset();
            }
        }

        // Process asynchronous request completion.
        if (result.has_value()) {
            //@@@
        }
    }
}


//----------------------------------------------------------------------------
// Abort the operation of receiving data from the web request.
//----------------------------------------------------------------------------

bool ts::ReactiveWebRequest::abort(bool silent)
{
    return _guts->abort(silent);
}

bool ts::ReactiveWebRequest::Guts::abort(bool silent)
{
    return false; //@@@
}
