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
    bool start(HandlerType* handler, const UString& url, size_t buffer_size, const ObjectPtr& user_data);

    // Abort the current transfer.
    bool abort(bool silent);

private:
    // Type of asynchronous operation in progress.
    enum AsyncOp {AsyncNone, AsyncInternetOpenUrl, AsyncInternetConnect, AsyncHttpOpenRequest, AsyncHttpSendRequest, AsyncInternetReadFile};

    // Name of these operations.
    static const UChar* AsyncName(AsyncOp op);

    // Guts private fields.
    ReactiveWebRequest&  _request;                 // Reference to parent instance.
    HandlerType*         _handler = nullptr;       // Application-defined handler.
    ObjectPtr            _handler_data {};         // User-data for _handler.
    EventId              _event {};                // Some event occurred, maybe call a handler.
    volatile ::HINTERNET _inet = nullptr;          // Handle to all Internet operations.
    volatile ::HINTERNET _inet_connect = nullptr;  // Handle to connection operations (POST request only).
    volatile ::HINTERNET _inet_request = nullptr;  // Handle to URL request operations.
    bool                 _aborting = false;        // Abort operation in progress.
    bool                 _open_completed = false;  // Open operation completed, now receiving content.
    bool                 _use_http = false;
    bool                 _use_https = false;
    bool                 _use_insecure = false;
    bool                 _use_post = false;
    void*                _post_data = nullptr;
    ::DWORD              _post_size = 0;
    ::DWORD              _url_flags = 0;
    size_t               _buffer_size = 0;         // Default size for receive buffer.
    ::DWORD              _received_size = 0;       // Size of received data, asynchronously set by InternetReadFile().
    ByteBlockPtr         _received_data {};        // Received data.
    AsyncOp              _async_op = AsyncNone;    // Current asynchronous operation.
    UString              _host {};                 // String buffer for host name (InternetConnectW).
    UString              _user {};                 // String buffer for user name (InternetConnectW).
    UString              _pass {};                 // String buffer for password (InternetConnectW).
    UString              _path {};                 // String buffer for URL path+query (HttpOpenRequestW).
    UString              _request_headers {};      // Buffer for all request headers.
    ::WCHAR*             _headers_addr = nullptr;  // Request headers address, for WinInet calls.
    ::DWORD              _headers_len = 0;         // Request headers length, for WinInet calls.

    // Guts private fields which can be used in the status callback, from a WinInet internal thread.
    // Only one asynchronous operation is started at a time. So each value which is set from a callback
    // will be processed before the next asynchronous operation is started. Because these fields are
    // used from the WinInet internal callback thread and the reactor thread, a mutex must be used.
    std::mutex                             _mutex {};         // Protect access to all following fields.
    std::optional<UString>                 _last_url {};      // Last INTERNET_STATUS_REDIRECT callback.
    std::optional<::HINTERNET>             _last_handle {};   // Last INTERNET_STATUS_HANDLE_CREATED callback.
    std::optional<::INTERNET_ASYNC_RESULT> _async_result {};  // Last INTERNET_STATUS_REQUEST_COMPLETE callback.

    // Mark the start of an asynchronous operation.
    void calling(AsyncOp op);

    // Notify the successful synchronous completion of the current operation.
    void synchronousCompletion();

    // Complete/abort the operation, call the appropriate handler.
    void completeOperation(int error_code);

    // Transmit response headers to the WebRequest.
    void transmitResponseHeaders();

    // Start a receive operation. Complete the request on error.
    void startReceive();

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
    _aborting = _open_completed = _use_http = _use_https = _use_post = _use_insecure = false;
    _post_data = nullptr;
    _post_size = _url_flags = _received_size = 0;
    _buffer_size = 0;
    _received_data.reset();
    _async_op = AsyncNone;
    _host.clear();
    _user.clear();
    _pass.clear();
    _path.clear();
    _request_headers.clear();
    _headers_addr = nullptr;
    _headers_len = 0;

    std::lock_guard<std::mutex> lock(_mutex);
    _last_url.reset();
    _last_handle.reset();
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
// Name of asynchronous operations.
//----------------------------------------------------------------------------

const ts::UChar* ts::ReactiveWebRequest::Guts::AsyncName(AsyncOp op)
{
    switch (op) {
        case AsyncNone:             return u"no asynchronous function";
        case AsyncInternetOpenUrl:  return u"InternetOpenUrl";
        case AsyncInternetConnect:  return u"InternetConnect";
        case AsyncHttpOpenRequest:  return u"HttpOpenRequest";
        case AsyncHttpSendRequest:  return u"HttpSendRequest";
        case AsyncInternetReadFile: return u"InternetReadFile";
        default:                    return u"unknown asynchronous function";
    }
}


//----------------------------------------------------------------------------
// Mark the start of an asynchronous operation.
//----------------------------------------------------------------------------

void ts::ReactiveWebRequest::Guts::calling(AsyncOp op)
{
    _request.reactor().trace(u"calling %s", AsyncName(op));
    _async_op = op;
}


//----------------------------------------------------------------------------
// Notify the successful synchronous completion of an operation.
//----------------------------------------------------------------------------

void ts::ReactiveWebRequest::Guts::synchronousCompletion()
{
    _request.reactor().trace(u"%s synchronous completion", AsyncName(_async_op));
    _async_result.emplace();
    _async_result->dwResult = true;  // success
    _async_result->dwError = ERROR_SUCCESS;
    _request.reactor().signalEvent(_event);
}


//----------------------------------------------------------------------------
// Complete/abort the operation, call the appropriate handler.
//----------------------------------------------------------------------------

void ts::ReactiveWebRequest::Guts::completeOperation(int error_code)
{
    if (_handler != nullptr) {
        // Report the completion in open handler if not yet called, in receive handler otherwise.
        if (_open_completed) {
            _handler->handleWebReceive(_request, nullptr, error_code, _handler_data);
        }
        else {
            _handler->handleWebOpen(_request, error_code, _handler_data);
        }
    }
    reset();
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

bool ts::ReactiveWebRequest::start(ReactiveWebHandlerInterface* handler, const UString& url, size_t buffer_size, const ObjectPtr& user_data)
{
    return _guts->start(handler, url, buffer_size, user_data);
}

bool ts::ReactiveWebRequest::Guts::start(ReactiveWebHandlerInterface* handler, const UString& url, size_t buffer_size, const ObjectPtr& user_data)
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

    // WinInet does not user overlapped I/O and cannot be directly integrated into IOCP.
    // An asynchronous WinInet operation invokes an application-defined callback from a WinInet internal thread.
    // Our callback will signal the following event to get back into the reactor.
    _event = _request.reactor().newEvent(this);
    if (!_event.isValid()) {
        return false;
    }

    // Application-defined values.
    _handler = handler;
    _handler_data = user_data;
    _buffer_size = std::max<size_t>(buffer_size, 256);  // enforce a minimal buffer size

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
        calling(AsyncInternetOpenUrl);
        _inet_request = ::InternetOpenUrlW(_inet, _request._status.originalURL().wc_str(), _headers_addr, _headers_len, _url_flags, ::DWORD_PTR(this));
        if (_inet_request != nullptr) {
            synchronousCompletion();
        }
        else if (::GetLastError() != ERROR_IO_PENDING) {
            // Actual error, not an asynchronous completion.
            _request.report().error(u"error opening URL %s: %s", url, WinErrorMessage(::GetLastError()));
            return false;
        }
    }
    else {
        // This is an HTTP(S) case that InternetOpenUrl() cannot handle.
        // We need to split the URL.
        const URL u(url);
        uint16_t port = u.getPort();
        _host = u.getHost();
        _user = u.getUserName();
        _pass = u.getPassword();
        _path = u.getPath();
        const UString query (u.getQuery());
        if (!query.empty()) {
            _path.append(u'?');
            _path.append(query);
        }
        if (port == 0) {
            // Use default port.
            port = _use_https ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
        }
        if (_use_https && _use_insecure) {
            // In secure mode, only some flags can be set to InternetConnect(). Others must be added later.
            _url_flags |= INTERNET_FLAG_IGNORE_CERT_CN_INVALID;
        }

        // Connect to the host.
        calling(AsyncInternetConnect);
        _inet_connect = ::InternetConnectW(_inet, _host.wc_str(), port,
                                           _user.empty() ? nullptr : _user.wc_str(),
                                           _pass.empty() ? nullptr : _pass.wc_str(),
                                           INTERNET_SERVICE_HTTP, _url_flags, ::DWORD_PTR(this));
        if (_inet_connect != nullptr) {
            synchronousCompletion();
        }
        else if (::GetLastError() != ERROR_IO_PENDING) {
            // Actual error, not an asynchronous completion.
            _request.report().error(u"error connecting to host %s: %s", _host, WinErrorMessage(::GetLastError()));
            return false;
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Start a receive operation. Complete the request on error.
//----------------------------------------------------------------------------

void ts::ReactiveWebRequest::Guts::startReceive()
{
    // Use a new receive buffer. The shared pointer will be left to the receive handler class later.
    _received_data = std::make_shared<ByteBlock>(_buffer_size);

    // The received size will be asynchronously set by InternetReadFile().
    _received_size = 0;

    // Start the asynchronous data reception.
    calling(AsyncInternetReadFile);
    if (::InternetReadFile(_inet_request, _received_data->data(), ::DWORD(_received_data->size()), &_received_size)) {
        synchronousCompletion();
    }
    else {
        const ::DWORD err = ::GetLastError();
        if (err != ERROR_IO_PENDING) {
            // Actual error, not an asynchronous completion.
            _request.report().error(u"error receiving data from %s: %s", _host, WinErrorMessage(err));
            completeOperation(err);
        }
    }
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
        else if (status == INTERNET_STATUS_HANDLE_CREATED && info_size >= sizeof(::HINTERNET)) {
            // Intermediate phase where a handle is created.
            std::lock_guard<std::mutex> lock(guts->_mutex);
            guts->_last_handle = *static_cast<::HINTERNET*>(info);
            guts->_request.reactor().signalEvent(guts->_event);
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
    if (id != _event) {
        return;
    }

    // Local copies of a completion values, outside lock.
    std::optional<UString> url;
    std::optional<::HINTERNET> handle;
    std::optional<::INTERNET_ASYNC_RESULT> result;

    // Process data from the WinInet status callback under lock.
    {
        std::lock_guard<std::mutex> lock(_mutex);
        url.swap(_last_url);
        handle.swap(_last_handle);
        result.swap(_async_result);
    }

    // Abort the overall operation only if no asynchronous operation is in progress (or it just completed).
    if (_aborting && (_async_op == AsyncNone || _async_result.has_value())) {
        completeOperation(SYS_CANCELED);
        return;
    }

    // Process URL redirection.
    if (url.has_value() && !url->empty()) {
        _request.report().debug(u"redirected to %s", *url);
        _request._status.setFinalURL(*url);
    }

    // Process asynchronously received handles.
    if (handle.has_value()) {
        _request.reactor().trace(u"%s handle asynchronously received: @%X", AsyncName(_async_op), uintptr_t(*handle));
        if (_async_op == AsyncInternetOpenUrl || _async_op == AsyncHttpOpenRequest) {
            _inet_request = *handle;
        }
        else if (_async_op == AsyncInternetConnect) {
            _inet_connect = *handle;
        }
    }

    // Process asynchronous operation completion.
    if (result.has_value()) {
        _request.reactor().trace(u"processing %s completion", AsyncName(_async_op));

        // No more operation in progress.
        const AsyncOp op = _async_op;
        _async_op = AsyncNone;

        // If the asynchronous operation failed, terminate the request now.
        if (result->dwResult == 0 && !SysSuccess(result->dwError)) {
            _request.report().error(u"error downloading %s: %s", _request._status.finalURL(), WinErrorMessage(result->dwError));
            completeOperation(result->dwError);
            return;
        }

        // Process transfer progress.
        switch (op) {
            case AsyncInternetConnect: {
                // Build the request.
                static const wchar_t* accept_types[] = {L"*/*", nullptr};
                calling(AsyncHttpOpenRequest);
                _inet_request = ::HttpOpenRequestW(_inet_connect, _use_post ? L"POST" : L"GET", _path.wc_str(), nullptr, nullptr,
                                                   accept_types, _url_flags | INTERNET_FLAG_RELOAD, ::DWORD_PTR(this));
                ::DWORD err = ::GetLastError();
                if (_inet_request != nullptr) {
                    synchronousCompletion();
                }
                else if (err != ERROR_IO_PENDING) {
                    // Actual error, not an asynchronous completion.
                    _request.report().error(u"error opening request to %s: %s", _request._status.originalURL(), WinErrorMessage(err));
                    completeOperation(err);
                }
                break;
            }
            case AsyncHttpOpenRequest: {
                // Set additional insecure flags after HttpOpenRequest() and before HttpSendRequest().
                if (_use_https && _use_insecure) {
                    // Get current security flags.
                    ::DWORD cur_flags = 0;
                    ::DWORD ret_size = ::DWORD(sizeof(cur_flags));
                    if (!::InternetQueryOptionW(_inet_request, INTERNET_OPTION_SECURITY_FLAGS, &cur_flags, &ret_size)) {
                        ::DWORD err = ::GetLastError();
                        _request.report().error(u"error getting security flags on HTTP request: %s", WinErrorMessage(err));
                        completeOperation(err);
                        return;
                    }
                    // Now add other insecure flags.
                    cur_flags |= INTERNET_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID | SECURITY_FLAG_IGNORE_REVOCATION | SECURITY_FLAG_IGNORE_UNKNOWN_CA;
                    if (!::InternetSetOptionW(_inet_request, INTERNET_OPTION_SECURITY_FLAGS, &cur_flags, ::DWORD(sizeof(cur_flags)))) {
                        ::DWORD err = ::GetLastError();
                        _request.report().error(u"error setting insecure mode on HTTPS request: %s", WinErrorMessage(err));
                        completeOperation(err);
                        return;
                    }
                }

                // POST data to send. HttpSendRequestW needs a non-const pointer but the data are unmodified.
                void* post_data = nullptr;
                ::DWORD post_size = 0;
                if (_use_post) {
                    post_data = const_cast<uint8_t*>(_request.args().postData().data());
                    post_size = ::DWORD(_request.args().postData().size());
                }

                // Send the request.
                calling(AsyncHttpSendRequest);
                if (!::HttpSendRequestW(_inet_request, _headers_addr, _headers_len, post_data, post_size)) {
                    ::DWORD err = ::GetLastError();
                    _request.report().error(u"error sending HTTP request: %s", WinErrorMessage(err));
                    completeOperation(err);
                }
                break;
            }
            case AsyncHttpSendRequest:
            case AsyncInternetOpenUrl: {
                // Connection complete either after InternetOpenUrl or InternetConnect/HttpOpenRequest/HttpSendRequest.
                // Send the response headers to the WebRequest object.
                transmitResponseHeaders();
                _open_completed = true;
                if (_handler != nullptr) {
                    _handler->handleWebOpen(_request, SYS_SUCCESS, _handler_data);
                }

                // Start receiving the content.
                startReceive();
                break;
            }
            case AsyncInternetReadFile: {
                if (_received_size == 0) {
                    // Successful reception of zero bytes means end of file.
                    completeOperation(SYS_EOF);
                }
                else {
                    // Adjust received size.
                    _received_data->resize(size_t(_received_size));
                    _request._status.addContentSize(_received_data->size());
                    // Pass data to the application.
                    if (_handler != nullptr) {
                        _handler->handleWebReceive(_request, _received_data, SYS_SUCCESS, _handler_data);
                    }
                    // Let the ownership of the shared pointer to the application.
                    _received_data.reset();
                    // Start next receive operation.
                    startReceive();
                }
                break;
            }
            case AsyncNone:
            default: {
                _request.report().debug(u"completion of unexpected asynchronous operation: %d", op);
                break;
            }
        }
    }
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
// Abort the operation of receiving data from the web request.
//----------------------------------------------------------------------------

bool ts::ReactiveWebRequest::abort(bool silent)
{
    return _guts->abort(silent);
}

bool ts::ReactiveWebRequest::Guts::abort(bool silent)
{
    if (_inet == nullptr) {
        _request.report().log(SilentLevel(silent), u"no transfer in progress, cannot abort");
        return false;
    }
    else {
        _aborting = true;
        if (_async_op == AsyncNone) {
            // No asynchronous operation in progress, abort in user event reactor handler.
            _request.reactor().signalEvent(_event);
        }
        else {
            // Closing the main Internet handle should propagate the abort on current asynchronous operation.
            ::InternetCloseHandle(_inet);
            _inet = nullptr;
        }
        return true;
    }
}
