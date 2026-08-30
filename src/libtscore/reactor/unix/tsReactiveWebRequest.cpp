//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Perform a simple Web request (HTTP, HTTPS, FTP) in a Reactor environment.
//  UNIX specific parts with libcurl.
//
//  RETRY POLICY:
//  In rare cases, it has been noted that curl fails with "connection reset
//  by peer" right after sending SSL client hello. Retrying may either
//  succeed or fail. This is typically seen on some specific servers.
//  All other clients, including all browsers and Windows WinInet library,
//  work on the same host. Only curl command and libcurl fail. As a dirty
//  workaround, the environment variable TS_CURL_RETRY can be set to specify
//  a per-site retry policy. The value must be a comma-separated list of
//  directives:
//    RETRY=value : number of retries for following hosts.
//    INTERVAL=value : milliseconds between retries for following hosts.
//    HOST=name : host FQDN
//
//----------------------------------------------------------------------------

#include "tsReactiveWebRequest.h"
#include "tsEnvironment.h"


//----------------------------------------------------------------------------
// Stubs when libcurl is not available.
//----------------------------------------------------------------------------

#if defined(TS_NO_CURL)

#define TS_NO_CURL_MESSAGE u"This version of TSDuck was compiled without Web support"
#define TS_NOT_IMPLEMENTED { report().error(TS_NO_CURL_MESSAGE); return false; }

ts::ReactiveWebRequest::ReactiveWebRequest(Reactor& reactor) : ReactiveBase(reactor) {}
ts::ReactiveWebRequest::~ReactiveWebRequest() {}
bool ts::ReactiveWebRequest::startOpen(ReactiveWebHandlerInterface*, const UString&, const ObjectPtr&) TS_NOT_IMPLEMENTED
bool ts::ReactiveWebRequest::startReceive(ReactiveWebHandlerInterface*, void*, size_t, const ObjectPtr&) TS_NOT_IMPLEMENTED
bool ts::ReactiveWebRequest::startClose(ReactiveWebHandlerInterface*, bool, const ObjectPtr&) TS_NOT_IMPLEMENTED
ts::UString ts::ReactiveWebRequest::GetLibraryVersion() { return TS_NO_CURL_MESSAGE; }

#else


//----------------------------------------------------------------------------
// Normal libcurl support
//----------------------------------------------------------------------------

// Some curl macros contains "old style" casts.
TS_LLVM_NOWARNING(old-style-cast)

#include <curl/curl.h>

// CURL_AT_LEAST_VERSION is defined in libcurl 7.44 and later only
#if !defined(CURL_AT_LEAST_VERSION)
#define CURL_AT_LEAST_VERSION(x,y,z) (LIBCURL_VERSION_NUM >= ((x)<<16|(y)<<8|(z)))
#endif

#if !defined(CURL_WRITEFUNC_ERROR)
#define CURL_WRITEFUNC_ERROR 0xFFFFFFFF
#endif


//----------------------------------------------------------------------------
// Global libcurl initialization using a singleton.
//----------------------------------------------------------------------------

namespace {

    // This singleton initialized libcurl in its constructor.
    class LibCurlInit
    {
        TS_SINGLETON(LibCurlInit);
    public:
        // Status code of libcurl initialization.
        const ::CURLcode init_status;

        // Get number of retries for an URL.
        void getRetry(const ts::UString& url, size_t& retries, cn::milliseconds& interval);

    private:
        // Per-host retry policy.
        struct Retry {
            size_t retries = 0;
            cn::milliseconds interval {};
        };
        std::map<ts::UString, Retry> _retries {};
    };

    TS_DEFINE_SINGLETON(LibCurlInit);

    // Constructor of the libcurl initialization.
    LibCurlInit::LibCurlInit() :
        init_status(::curl_global_init(CURL_GLOBAL_ALL))
    {
        // Load the retry policy from an environment variable (see comment in header of this file).
        ts::UStringList dirs;
        ts::GetEnvironment(u"TS_CURL_RETRY").split(dirs, ts::COMMA, true, true);
        Retry retry;
        for (const auto& dir : dirs) {
            const size_t eq = dir.find(u'=');
            if (eq != ts::NPOS) {
                if (dir.starts_with(u"RETRY=", ts::CASE_INSENSITIVE)) {
                    dir.substr(eq + 1).toInteger(retry.retries);
                }
                else if (dir.starts_with(u"INTERVAL=", ts::CASE_INSENSITIVE)) {
                    dir.substr(eq + 1).toChrono(retry.interval);
                }
                else if (dir.starts_with(u"HOST=", ts::CASE_INSENSITIVE)) {
                    _retries.insert(std::make_pair(dir.substr(eq + 1).toLower(), retry));
                }
            }
        }
    }

    // Get number of retries for an URL.
    void LibCurlInit::getRetry(const ts::UString& url, size_t& retries, cn::milliseconds& interval)
    {
        const ts::URL u(url);
        const auto it = _retries.find(u.getHost().toLower());
        if (it != _retries.end()) {
            retries = it->second.retries;
            interval = it->second.interval;
        }
        else {
            retries = 0;
            interval = cn::milliseconds::zero();
        }
    }
}


//----------------------------------------------------------------------------
// System-specific parts are stored in a private structure.
//----------------------------------------------------------------------------

class ts::ReactiveWebRequest::SystemGuts: private ReactorHandlerInterface
{
    TS_NOBUILD_NOCOPY(SystemGuts);
public:
    // Constructor with a reference to parent WebRequest.
    SystemGuts(ReactiveWebRequest& request);
    virtual ~SystemGuts() override;

    // Check if transfer is open.
    bool isOpen() const { return _curlm != nullptr; }

    // Close and cleanup everything. If 'full' is true, also reset fields which are set for opening an URL.
    void reset(bool full);

    // Start the trandfer operation.
    bool start(HandlerType* handler, const ObjectPtr& user_data);

    // Initialize non-blocking state of one transfer attempt.
    bool initTransferState();

    // Build error messages from curl_multi and curl_easy.
    template<typename ENUM> UString message(const UString& title, ENUM code, const char* (*strerror)(ENUM));
    UString easyMessage(const UString& title, ::CURLcode code) { return message(title, code, ::curl_easy_strerror); }
    UString multiMessage(const UString& title, ::CURLMcode code) { return message(title, code, ::curl_multi_strerror); }

private:
    ReactiveWebRequest& _request;                     // Reference to parent instance.
    EventId             _open_event {};               // Call open_handler.
    bool                _open_called = false;         // Open_handler was called.
    int                 _open_error = SYS_SUCCESS;    // Error code of open operation.
    HandlerType*        _handler = nullptr;           // Application-defined handler.
    ObjectPtr           _handler_data {};             // User-data for _handler.
    ::CURLM*            _curlm = nullptr;             // "curl multi" handler, for global curl access.
    ::CURL*             _curl = nullptr;              // "curl easy" handler, for one transfer.
    ::curl_slist*       _headers = nullptr;           // Request headers.
    bool                _aborted = false;             // The transfer is aborted by the user.
    bool                _can_retry = false;           // Can retry the connection later.
    size_t              _retries = 0;                 // Remaining retry count, if _can_retry.
    cn::milliseconds    _retry_interval {};           // Interval between two retries.
    ByteBlock           _received_data {};            // Received data, filled by CurlWriteCallback(), emptied by receive().
    size_t              _total_received_size = 0;     // All received data.
    char                _error[CURL_ERROR_SIZE] {0};  // Error message buffer for libcurl (CURLOPT_ERRORBUFFER).

    // Libcurl callback informed about what to wait for.
    static int CurlSocketCallback(::CURL* easy, ::curl_socket_t socket, int what, void* clientp, void* socketp);

    // Libcurl callback to receive timeout values.
    static int CurlTimerCallback(::CURLM* multi, long timeout_ms, void* clientp);

    // Libcurl callback for writing received data.
    static size_t CurlWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata);

    // Libcurl callback callback that receives header data.
    static size_t CurlHeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata);

    // Implementation of ReactorHandlerInterface.
    virtual void handleTimer(Reactor& reactor, EventId id) override;
    virtual void handleUserEvent(Reactor& reactor, EventId id) override;
};


//----------------------------------------------------------------------------
// System-specific constructors and destructor.
//----------------------------------------------------------------------------

ts::ReactiveWebRequest::SystemGuts::SystemGuts(ReactiveWebRequest& request) :
    _request(request)
{
}

ts::ReactiveWebRequest::SystemGuts::~SystemGuts()
{
    reset(true);
}

void ts::ReactiveWebRequest::SystemGuts::reset(bool full)
{
    if (_headers != nullptr) {
        ::curl_slist_free_all(_headers);
        _headers = nullptr;
    }

    if (_curl != nullptr && _curlm != nullptr) {
        ::curl_multi_remove_handle(_curlm, _curl);
    }

    if (_curl != nullptr) {
        ::curl_easy_cleanup(_curl);
        _curl = nullptr;
    }

    if (_curlm != nullptr) {
        ::curl_multi_cleanup(_curlm);
        _curlm = nullptr;
    }

    if (_open_event.isValid()) {
        _request.reactor().deleteEvent(_open_event, true);
        _open_event.invalidate();
    }

    _open_called = false;
    _open_error = SYS_SUCCESS;
    _received_data.clear();
    _total_received_size = 0;
    _error[0] = 0;

    // These fields must be preserved when opening a URL in various retries.
    if (full) {
        _handler = nullptr;
        _handler_data.reset();
        _aborted = false;
        _can_retry = false;
        _retries = 0;
        _retry_interval = cn::milliseconds::zero();
    }
}


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::ReactiveWebRequest::ReactiveWebRequest(Reactor& reactor) :
    ReactiveBase(reactor),
    _guts(new SystemGuts(*this))
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
// Build an error message from libcurl.
//----------------------------------------------------------------------------

template<typename ENUM>
ts::UString ts::ReactiveWebRequest::SystemGuts::message(const UString& title, ENUM code, const char* (*strerror)(ENUM))
{
    UString msg(title);
    msg.append(u", ");
    const char* err = strerror(code);
    if (err != nullptr && err[0] != 0) {
        msg.append(UString::FromUTF8(err));
    }
    else {
        msg.format(u"error code %d", int(code));
    }
    if (_error[0] != 0) {
        msg.append(u", ");
        msg.append(UString::FromUTF8(_error));
    }
    return msg;
}


//----------------------------------------------------------------------------
// Start the open operation in SystemGuts.
//----------------------------------------------------------------------------

bool ts::ReactiveWebRequest::SystemGuts::start(HandlerType* handler, const ObjectPtr& user_data)
{
    _handler = handler;
    _handler_data = user_data;

    // Get retry count and interval for the URL's host.
    _retries = 0;
    LibCurlInit::Instance().getRetry(_request._status.originalURL(), _retries, _retry_interval);
    _request.report().debug(u"curl retries: %d, interval: %!s", _retries, _retry_interval);
    _can_retry = _retries > 0;

    // Start the first try.
    return initTransferState();
}


//----------------------------------------------------------------------------
// Convenience macros to set libcurl options in initTransferState().
// The curl_multi_setopt() and curl_easy_setopt() functions are strange
// macros which trigger warnings.
//----------------------------------------------------------------------------

#define TS_MULTI_OPT(name, value)                                        \
    TS_PUSH_WARNING()                                                    \
    TS_LLVM_NOWARNING(disabled-macro-expansion)                          \
    mstatus = ::curl_multi_setopt(_curlm, name, value);                  \
    TS_POP_WARNING()                                                     \
    if (mstatus != ::CURLM_OK) {                                         \
        _request.report().error(multiMessage(#name u" error", mstatus)); \
        break;                                                           \
    }                                                                    \
    using TS_UNIQUE_NAME(for_trailing_semicolon) [[maybe_unused]] = int

#define TS_EASY_OPT(name, value)                                         \
    TS_PUSH_WARNING()                                                    \
    TS_LLVM_NOWARNING(disabled-macro-expansion)                          \
    status = ::curl_easy_setopt(_curl, name, value);                     \
    TS_POP_WARNING()                                                     \
    if (status != ::CURLE_OK) {                                          \
        _request.report().error(easyMessage(#name u" error", status));   \
        break;                                                           \
    }                                                                    \
    using TS_UNIQUE_NAME(for_trailing_semicolon) [[maybe_unused]] = int


//----------------------------------------------------------------------------
// Initialize non-blocking state of one transfer attempt.
//----------------------------------------------------------------------------

bool ts::ReactiveWebRequest::SystemGuts::initTransferState()
{
    // Make sure we start from a clean state. Preserve opening data.
    reset(false);

    // Success will be set to true at the end, if no failure occurs.
    bool success = false;
    ::CURLcode status = ::CURLE_OK;
    ::CURLMcode mstatus = ::CURLM_OK;

    // Use a "do {} while (false)" pattern to allow early "break" and jump to cleanup in case of error.
    do {
        // Prepare a user event for the open handler.
        if (_handler != nullptr && !(_open_event = _request.reactor().newEvent(this)).isValid()) {
            break;
        }

        // Initialize curl_multi and curl_easy
        if ((_curlm = ::curl_multi_init()) == nullptr) {
            _request.report().error(u"libcurl 'curl_multi' initialization error");
            break;
        }

        // Set the callbacks that libcurl will call on socket actions and timers.
        TS_MULTI_OPT(CURLMOPT_SOCKETFUNCTION, &SystemGuts::CurlSocketCallback);
        TS_MULTI_OPT(CURLMOPT_SOCKETDATA, this);
        TS_MULTI_OPT(CURLMOPT_TIMERFUNCTION, &SystemGuts::CurlTimerCallback);
        TS_MULTI_OPT(CURLMOPT_TIMERDATA, this);

        // Initialize curl_easy.
        if ((_curl = ::curl_easy_init()) == nullptr) {
            _request.report().error(u"libcurl 'curl_easy' initialization error");
            break;
        }

        // Setup the curl_easy error message buffer.
        TS_EASY_OPT(CURLOPT_ERRORBUFFER, _error);

        // Set the user agent.
        if (!_request.args().userAgent().empty()) {
            TS_EASY_OPT(CURLOPT_USERAGENT, _request.args().userAgent().toUTF8().c_str());
        }

        // Set compression.
        if (_request.args().compressionEnabled()) {
            // From https://curl.se/libcurl/c/CURLOPT_ACCEPT_ENCODING.html :
            // "To aid applications not having to bother about what specific algorithms this particular libcurl build
            // supports, libcurl allows a zero-length string to be set ("") to ask for an Accept-Encoding: header to
            // be used that contains all built-in supported encodings."
            TS_EASY_OPT(CURLOPT_ACCEPT_ENCODING, "");
        }

        // Set the starting URL.
        TS_EASY_OPT(CURLOPT_URL, _request._status.originalURL().toUTF8().c_str());

        // Set HTTPS insecure mode.
        if (_request.args().isInsecure()) {
            TS_EASY_OPT(CURLOPT_SSL_VERIFYPEER, 0L);
            TS_EASY_OPT(CURLOPT_SSL_VERIFYHOST, 0L);
            TS_EASY_OPT(CURLOPT_SSL_VERIFYSTATUS, 0L);
        }

        // Set the connection timeout.
        if (_request.args().connectionTimeout() > cn::milliseconds::zero()) {
            TS_EASY_OPT(CURLOPT_CONNECTTIMEOUT_MS, long(_request.args().connectionTimeout().count()));
        }

        // Set the receive timeout. There is no such parameter in libcurl.
        // Note: the option CURLOPT_TIMEOUT specifies the duration of the complete transfer,
        // which can be infinite when receiving a live stream.
        if (_request.args().receiveTimeout() > cn::milliseconds::zero()) {
            // The LOW_SPEED_TIME option is in seconds. Round to higher.
            TS_EASY_OPT(CURLOPT_LOW_SPEED_TIME, long((_request.args().receiveTimeout().count() + 999) / 1000));
            // The CURLOPT_LOW_SPEED_LIMIT is in bytes/second, this is the minimum transfer rate during CURLOPT_LOW_SPEED_TIME.
            // Saying that we need at least 1 B/s during LOW_SPEED_TIME is roughly euivalent to a receive timeout.
            TS_EASY_OPT(CURLOPT_LOW_SPEED_LIMIT, 1L);
        }

        // Disable signals to avoid interferences between SIGALRM and timeouts.
        TS_EASY_OPT(CURLOPT_NOSIGNAL, 1L);

        // Set the response callbacks.
        TS_EASY_OPT(CURLOPT_WRITEFUNCTION, &SystemGuts::CurlWriteCallback);
        TS_EASY_OPT(CURLOPT_WRITEDATA, this);
        TS_EASY_OPT(CURLOPT_HEADERFUNCTION, &SystemGuts::CurlHeaderCallback);
        TS_EASY_OPT(CURLOPT_HEADERDATA, this);

        // Follow redirections. Hard-coded limit of 32 redirections max.
        TS_EASY_OPT(CURLOPT_FOLLOWLOCATION, _request.args().autoRedirect() ? 1L : 0L);
        TS_EASY_OPT(CURLOPT_MAXREDIRS, _request.args().autoRedirect() ? 32L : 0L);

        // Set the proxy settings.
        if (!_request.args().proxyHost().empty()) {
            TS_EASY_OPT(CURLOPT_PROXY, _request.args().proxyHost().toUTF8().c_str());
            if (_request.args().proxyPort() != 0) {
                TS_EASY_OPT(CURLOPT_PROXYPORT, long(_request.args().proxyPort()));
            }
            if (!_request.args().proxyUser().empty()) {
                TS_EASY_OPT(CURLOPT_PROXYAUTH, CURLAUTH_ANY);
                TS_EASY_OPT(CURLOPT_PROXYUSERNAME, _request.args().proxyUser().toUTF8().c_str());
                if (!_request.args().proxyPassword().empty()) {
                    TS_EASY_OPT(CURLOPT_PROXYPASSWORD, _request.args().proxyPassword().toUTF8().c_str());
                }
            }
        }

        // Set the cookie file.
        if (_request.args().cookiesEnabled()) {
            // COOKIEFILE can be empty.
            TS_EASY_OPT(CURLOPT_COOKIEFILE, _request.args().cookiesFileName().c_str());
        }
        if (_request.args().cookiesEnabled() && !_request.args().cookiesFileName().empty()) {
            // COOKIEJAR cannot be empty.
            TS_EASY_OPT(CURLOPT_COOKIEJAR, _request.args().cookiesFileName().c_str());
        }

        // Set the request headers.
        if (!_request.args().requestHeaders().empty()) {
            for (const auto& it : _request.args().requestHeaders()) {
                const UString header(it.first + u": " + it.second);
                _headers = ::curl_slist_append(_headers, header.toUTF8().c_str());
            }
            TS_EASY_OPT(CURLOPT_HTTPHEADER, _headers);
        }

        // Set the POST data.
        if (_request.args().isPost()) {
            TS_EASY_OPT(CURLOPT_POSTFIELDS, _request.args().postData().data());
            TS_EASY_OPT(CURLOPT_POSTFIELDSIZE, _request.args().postData().size());
        }

        // Register the curl_easy handle inside the curl_multi handle.
        if ((mstatus = ::curl_multi_add_handle(_curlm, _curl)) != ::CURLM_OK) {
            _request.report().error(multiMessage(u"curl_multi_add_handle error", mstatus));
            break;
        }

        // End of initialization sequence. Inform the cleanup phase that we succeeded.
        success = true;

    } while (false);

    // Now process setopt error.
    if (!success) {
        reset(false);
    }
    return success;
}


//----------------------------------------------------------------------------
// Libcurl callback informed about what to wait for.
//----------------------------------------------------------------------------

int ts::ReactiveWebRequest::SystemGuts::CurlSocketCallback(::CURL* easy, ::curl_socket_t socket, int what, void* clientp, void* socketp)
{
    const auto guts = static_cast<SystemGuts*>(clientp);
    if (guts == nullptr) {
        return -1; // error
    }
    else {
        // Only INOUT enables read & write notification. If only IN or OUT is specified, the other one should be disabled.
        // Because we may disable a notification which was not set, we ignore errors on disabled.
        bool success = true;
        Reactor& reactor(guts->_request.reactor());
        switch (what) {
            case CURL_POLL_IN:
                success = reactor.newReadNotify(guts, socket).isValid();
                reactor.deleteWriteNotify(socket, true);
                break;
            case CURL_POLL_OUT:
                success = reactor.newWriteNotify(guts, socket).isValid();
                reactor.deleteReadNotify(socket, true);
                break;
            case CURL_POLL_INOUT:
                success = reactor.newReadNotify(guts, socket).isValid() && reactor.newWriteNotify(guts, socket).isValid();
                break;
            case CURL_POLL_REMOVE:
                reactor.deleteWriteNotify(socket, true);
                reactor.deleteReadNotify(socket, true);
                break;
            case CURL_POLL_NONE:
            default:
                break;
        }
        return success ? 0 : -1;
    }
}


//----------------------------------------------------------------------------
// Libcurl callback to receive timeout values.
//----------------------------------------------------------------------------

int ts::ReactiveWebRequest::SystemGuts::CurlTimerCallback(::CURLM* multi, long timeout_ms, void* clientp)
{
    const auto guts = static_cast<SystemGuts*>(clientp);
    if (guts == nullptr) {
        return -1; // error
    }
    else {
        const EventId id = guts->_request.reactor().newTimer(guts, cn::milliseconds(timeout_ms), false);
        return id.isValid() ? 0 : -1;
    }
}


//----------------------------------------------------------------------------
// Libcurl callback for writing received data.
//----------------------------------------------------------------------------

size_t ts::ReactiveWebRequest::SystemGuts::CurlWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    const auto guts = static_cast<SystemGuts*>(userdata);
    if (guts == nullptr) {
        return CURL_WRITEFUNC_ERROR;
    }
    else {
        // After receiving some data, it is no longer possible to retry the connection.
        guts->_can_retry = false;

        // With libcurl, there is no way to be notified of "end of connection", after response headers.
        // If this is the first response data chunk, then this is the "end of connection".
        // Need to notify the application of end of startOpen().
        if (guts->_total_received_size == 0 && guts->_handler != nullptr && !guts->_open_called) {
            guts->_open_error = SYS_SUCCESS;
            guts->_request.reactor().signalEvent(guts->_open_event);
        }

        // Store response data in the SystemGuts buffer.
        const size_t chunk_size = size * nmemb;
        guts->_received_data.append(ptr, chunk_size);
        guts->_total_received_size += chunk_size;
        return chunk_size;
    }
}


//----------------------------------------------------------------------------
// Libcurl callback callback that receives header data.
//----------------------------------------------------------------------------

size_t ts::ReactiveWebRequest::SystemGuts::CurlHeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata)
{
    const auto guts = static_cast<SystemGuts*>(userdata);
    if (guts == nullptr) {
        return 0; // error
    }
    else {
        // Store the headers in the request status.
        const size_t total_size = size * nitems;
        guts->_request._status.processReponseHeaders(UString::FromUTF8(buffer, total_size), guts->_request.report());
        return total_size;
    }
}


//----------------------------------------------------------------------------
// Handle a timer from the Reactor.
//----------------------------------------------------------------------------

void ts::ReactiveWebRequest::SystemGuts::handleTimer(Reactor& reactor, EventId id)
{
    //@@@
}


//----------------------------------------------------------------------------
// Handle a user-defined event in a Reactor.
//----------------------------------------------------------------------------

void ts::ReactiveWebRequest::SystemGuts::handleUserEvent(Reactor& reactor, EventId id)
{
    // Call open handler if necessary.
    if (id == _open_event && _handler != nullptr && !_open_called) {
        _open_called = true;
        _handler->handleWebOpen(_request, _open_error, _handler_data);
    }
}


//----------------------------------------------------------------------------
// Start the operation of opening an URL.
//----------------------------------------------------------------------------

bool ts::ReactiveWebRequest::start(ReactiveWebHandlerInterface* handler, const UString& url, const ObjectPtr& user_data)
{
    if (LibCurlInit::Instance().init_status != 0) {
        report().error(u"libcurl initialization (curl_global_init) failed");
        return false;
    }
    else if (url.empty()) {
        report().error(u"no URL specified");
        return false;
    }
    else if (_guts->isOpen()) {
        report().error(u"internal error, transfer already started, cannot download %s", url);
        return false;
    }
    else {
        _status.reset(url);
        return _guts->start(handler, user_data);
    }
}


//----------------------------------------------------------------------------
// Abort the operation of receiving data from the web request.
//----------------------------------------------------------------------------

bool ts::ReactiveWebRequest::abort(bool silent)
{
    //@@@@
    report().error(u"ReactiveWebRequest is not yet implemented");
    return false;
}

#endif // TS_NO_CURL
