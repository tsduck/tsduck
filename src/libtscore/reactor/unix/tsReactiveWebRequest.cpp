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
#include "tsFeatures.h"


//----------------------------------------------------------------------------
// Register for options --version and --support.
//----------------------------------------------------------------------------

#if defined(TS_NO_CURL)
#define SUPPORT ts::Features::UNSUPPORTED
#else
#define SUPPORT ts::Features::SUPPORTED
#endif

TS_REGISTER_FEATURE(u"http", u"Web library", SUPPORT, ts::ReactiveWebRequest::GetLibraryVersion);


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

class ts::ReactiveWebRequest::Guts: private ReactorHandlerInterface
{
    TS_NOBUILD_NOCOPY(Guts);
public:
    // Constructor with a reference to parent ReactiveWebRequest.
    Guts(ReactiveWebRequest& request);
    virtual ~Guts() override;

    // Check if transfer is open.
    bool isOpen() const { return _curlm != nullptr && !_aborted; }

    // Close and cleanup everything. If 'full' is true, also reset fields which are set for opening an URL.
    void reset(bool full);

    // Start the transfer operation.
    bool start(HandlerType* handler, size_t buffer_size, const ObjectPtr& user_data);

    // Abort the current transfer.
    bool abort(bool silent);

private:
    ReactiveWebRequest& _request;                       // Reference to parent instance.
    EventId             _event {};                      // Some event occurred, maybe call a handler.
    bool                _push_transfer = false;         // Continue transfer in a reactor callback.
    bool                _open_done = false;             // Open URL is complete.
    bool                _open_called = false;           // Open handler was called.
    bool                _aborted = false;               // The transfer is aborted by the user.
    bool                _completed = false;             // The transfer is complete.
    int                 _completion_code = SYS_SUCCESS; // Error code of open operation.
    HandlerType*        _handler = nullptr;             // Application-defined handler.
    ObjectPtr           _handler_data {};               // User-data for _handler.
    size_t              _buffer_size = 0;               // Default size for receive buffer.
    ::CURLM*            _curlm = nullptr;               // "curl multi" handler, for global curl access.
    ::CURL*             _curl = nullptr;                // "curl easy" handler, for one transfer.
    ::curl_slist*       _headers = nullptr;             // Request headers.
    bool                _can_retry = false;             // Can retry the connection later.
    size_t              _retries = 0;                   // Remaining retry count, if _can_retry.
    cn::milliseconds    _retry_interval {};             // Interval between two retries.
    EventId             _retry_timer {};                // Reactor timer for next retry.
    std::set<EventId>   _curl_timers {};                // Set of timers for curl.
    ByteBlockPtr        _received_data {};              // Received data, filled by CurlWriteCallback(), emptied by receive callback.
    int                 _running_handles = 0;           // Number of active handles inside curl. 0 means all completed.
    char                _error[CURL_ERROR_SIZE] {0};    // Error message buffer for libcurl (CURLOPT_ERRORBUFFER).

    // Initialize non-blocking state of one transfer attempt.
    bool startTransfer();

    // Continue processing the transfer based on events on a file descriptor (or CURL_SOCKET_TIMEOUT).
    bool continueTransfer(int fd, int event_mask);

    // Terminate the transfer and call the application handler.
    void terminateTransfer();

    // Build error messages from curl_multi and curl_easy.
    template<typename ENUM> UString message(const UString& title, ENUM code, const char* (*strerror)(ENUM));
    UString easyMessage(const UString& title, ::CURLcode code) { return message(title, code, ::curl_easy_strerror); }
    UString multiMessage(const UString& title, ::CURLMcode code) { return message(title, code, ::curl_multi_strerror); }

    // Get a curl information.
    template <typename Arg> bool curlGetInfo(int severity, const UChar* name, ::CURLINFO info, Arg arg);

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
    virtual void handleReadReady(Reactor& reactor, EventId id, int error_code) override;
    virtual void handleWriteReady(Reactor& reactor, EventId id, int error_code) override;
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
    reset(true);
}

void ts::ReactiveWebRequest::Guts::reset(bool full)
{
    _request.report().debug(u"closing reactive web request (full = %s)", full);

    for (const auto& id : _curl_timers) {
        _request.reactor().cancelTimer(id);
    }
    _curl_timers.clear();

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

    if (_event.isValid()) {
        _request.reactor().deleteEvent(_event, true);
        _event.invalidate();
    }

    if (_retry_timer.isValid()) {
        _request.reactor().cancelTimer(_retry_timer, true);
        _retry_timer.invalidate();
    }

    _buffer_size = 0;
    _push_transfer = _open_done = _open_called = _completed = false;
    _completion_code = SYS_SUCCESS;
    _received_data.reset();
    _running_handles = 0;
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
// Build an error message from libcurl.
//----------------------------------------------------------------------------

template<typename ENUM>
ts::UString ts::ReactiveWebRequest::Guts::message(const UString& title, ENUM code, const char* (*strerror)(ENUM))
{
    UString msg(title);
    if (!msg.empty()) {
        msg.append(u", ");
    }
    const char* err = strerror(code);
    if (err != nullptr && err[0] != 0) {
        msg.format(u"%s", err);
    }
    else {
        msg.format(u"error code %d", int(code));
    }
    if (_error[0] != 0) {
        msg.format(u", %s", _error);
    }
    return msg;
}


//----------------------------------------------------------------------------
// Start the operation of opening an URL.
//----------------------------------------------------------------------------

bool ts::ReactiveWebRequest::start(ReactiveWebHandlerInterface* handler, const UString& url, size_t buffer_size, const ObjectPtr& user_data)
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
        return _guts->start(handler, buffer_size, user_data);
    }
}


//----------------------------------------------------------------------------
// Start the open operation in Guts.
//----------------------------------------------------------------------------

bool ts::ReactiveWebRequest::Guts::start(HandlerType* handler, size_t buffer_size, const ObjectPtr& user_data)
{
    _handler = handler;
    _handler_data = user_data;

    // Enforce a buffer size within curl's limits.
    _buffer_size = std::min<size_t>(std::max<size_t>(buffer_size, 1024), CURL_MAX_READ_SIZE);

    // Get retry count and interval for the URL's host.
    _retries = 0;
    LibCurlInit::Instance().getRetry(_request._status.originalURL(), _retries, _retry_interval);
    _request.reactor().trace(u"downloading %s, curl retries: %d, interval: %!s", _request._status.originalURL(), _retries, _retry_interval);
    _can_retry = _retries > 0;

    // Start the first try.
    return startTransfer();
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

bool ts::ReactiveWebRequest::Guts::startTransfer()
{
    // Make sure we start from a clean state. Preserve opening data.
    reset(false);
    _received_data = std::make_shared<ByteBlock>();

    // Success will be set to true at the end, if no failure occurs.
    bool success = false;
    ::CURLcode status = ::CURLE_OK;
    ::CURLMcode mstatus = ::CURLM_OK;

    // Use a "do {} while (false)" pattern to allow early "break" and jump to cleanup in case of error.
    do {
        // Prepare a user event for the open handler.
        if (_handler != nullptr && !(_event = _request.reactor().newEvent(this)).isValid()) {
            break;
        }

        // Initialize curl_multi and curl_easy
        if ((_curlm = ::curl_multi_init()) == nullptr) {
            _request.report().error(u"libcurl 'curl_multi' initialization error");
            break;
        }

        // Set the callbacks that libcurl will call on socket actions and timers.
        TS_MULTI_OPT(CURLMOPT_SOCKETFUNCTION, &Guts::CurlSocketCallback);
        TS_MULTI_OPT(CURLMOPT_SOCKETDATA, this);
        TS_MULTI_OPT(CURLMOPT_TIMERFUNCTION, &Guts::CurlTimerCallback);
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

        // Read buffer size.
        TS_EASY_OPT(CURLOPT_BUFFERSIZE, long(_buffer_size));

        // Set the response callbacks.
        TS_EASY_OPT(CURLOPT_WRITEFUNCTION, &Guts::CurlWriteCallback);
        TS_EASY_OPT(CURLOPT_WRITEDATA, this);
        TS_EASY_OPT(CURLOPT_HEADERFUNCTION, &Guts::CurlHeaderCallback);
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

        // End of initialization sequence. Start the actual transfer.
        _request._status.setContentSize(0);
        success = continueTransfer(CURL_SOCKET_TIMEOUT, 0);

    } while (false);

    // Now process setopt error.
    if (!success) {
        reset(false);
    }
    return success;
}


//----------------------------------------------------------------------------
// Get a curl information.
//----------------------------------------------------------------------------

template <typename Arg>
bool ts::ReactiveWebRequest::Guts::curlGetInfo(int severity, const UChar* name, ::CURLINFO info, Arg arg)
{
    TS_PUSH_WARNING()
    TS_LLVM_NOWARNING(disabled-macro-expansion)
    const ::CURLcode status = ::curl_easy_getinfo(_curl, info, arg);
    TS_POP_WARNING()
    if (status == ::CURLE_OK) {
        return true;
    }
    else {
        _request.report().log(severity, u"error getting download info for %s: %s", _request.status().originalURL(), easyMessage(name, status));
        return false;
    }
}


//----------------------------------------------------------------------------
// Continue processing the transfer based on events on a file descriptor.
//----------------------------------------------------------------------------

bool ts::ReactiveWebRequest::Guts::continueTransfer(int fd, int event_mask)
{
    _request.reactor().trace(u"continue curl transfer, fd = %d, event_mask = 0x%02X", fd, event_mask);

    // Severity of error is just debug in case of possible retry.
    const int severity = _can_retry ? Severity::Debug : Severity::Error;

    // Execute whatever curl can do without blocking.
    ::CURLMcode mstatus = ::curl_multi_socket_action(_curlm, fd, event_mask, &_running_handles);
    if (mstatus != ::CURLM_OK) {
        _completed = true;
        _completion_code = SYS_ERROR;
        _request.report().error(multiMessage(u"curl processing error", mstatus));
        return false;
    }

    // Drain the message queue from libcurl.
    ::CURLMsg* msg = nullptr;
    int msg_count = 0;
    while ((msg = ::curl_multi_info_read(_curlm, &msg_count)) != nullptr) {
        // Transfer completion is the only defined message type.
        if (msg->msg == ::CURLMSG_DONE) {
            _completed = true;

            // We use only one curl_easy, it must be this one.
            assert(msg->easy_handle == _curl);

            // Can we retry in case of failure?
            if (_can_retry) {
                _can_retry = --_retries > 0;
            }

            // Transfer status.
            if (msg->data.result != ::CURLE_OK) {
                _completion_code = SYS_ERROR;
                _request.report().log(severity, u"error downloading %s", _request.status().originalURL());
                _request.report().log(severity, easyMessage(nullptr, msg->data.result));
                return false;
            }
            else {
                // Get HTTP status code and final URL (in case of redirections).
                long http_status = 0;
                if (curlGetInfo(severity, u" CURLINFO_RESPONSE_CODE", CURLINFO_RESPONSE_CODE, &http_status)) {
                    _request._status.setHttpStatus(int(http_status));
                }
                char* final_url = nullptr;
                if (curlGetInfo(severity, u"CURLINFO_EFFECTIVE_URL", CURLINFO_EFFECTIVE_URL, &final_url) && final_url != nullptr) {
                    // Do not update the final URL if similar to the original one. The reason is that, when auto-redirection is
                    // disabled, the "effective URL" is the original URL (not redirected) after the final URL has been set from
                    // a "Location" response header.
                    _request._status.setFinalURL(UString::FromUTF8(final_url), !_request._args.autoRedirect());
                }
            }

            // Process the transfer completion in some later reactor handler.
            _request.reactor().signalEvent(_event);
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Libcurl callback informed about what to wait for.
//----------------------------------------------------------------------------

int ts::ReactiveWebRequest::Guts::CurlSocketCallback(::CURL* easy, ::curl_socket_t fd, int what, void* clientp, void* socketp)
{
    const auto guts = static_cast<Guts*>(clientp);
    if (guts == nullptr) {
        return -1; // error
    }
    else {
        // Only INOUT enables read & write notification. If only IN or OUT is specified, the other one should be disabled.
        // Because we may disable a notification which was not set, we ignore errors on disabled.
        guts->_request.reactor().trace(u"curl socket callback, fd = %d, what = %d", fd, what);
        bool success = true;
        Reactor& reactor(guts->_request.reactor());
        switch (what) {
            case CURL_POLL_IN:
                success = reactor.newReadNotify(guts, fd).isValid();
                reactor.deleteWriteNotify(fd, true);
                break;
            case CURL_POLL_OUT:
                success = reactor.newWriteNotify(guts, fd).isValid();
                reactor.deleteReadNotify(fd, true);
                break;
            case CURL_POLL_INOUT:
                success = reactor.newReadNotify(guts, fd).isValid() && reactor.newWriteNotify(guts, fd).isValid();
                break;
            case CURL_POLL_REMOVE:
                reactor.deleteWriteNotify(fd, true);
                reactor.deleteReadNotify(fd, true);
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

int ts::ReactiveWebRequest::Guts::CurlTimerCallback(::CURLM* multi, long timeout_ms, void* clientp)
{
    const auto guts = static_cast<Guts*>(clientp);
    if (guts == nullptr) {
        return -1; // error
    }
    else if (timeout_ms < 0) {
        // Disarm curl's timer. Usually, curl uses only one at a time, but we support more. So, disarm all.
        guts->_request.reactor().trace(u"curl disarms timer");
        for (const auto& id : guts->_curl_timers) {
            guts->_request.reactor().cancelTimer(id);
        }
        guts->_curl_timers.clear();
        return 0; // success
    }
    else if (timeout_ms == 0) {
        // Just call curl again later.
        guts->_push_transfer = true;
        return 0; // success
    }
    else {
        // Set a timer, usually only one at a time.
        guts->_request.reactor().trace(u"curl arms %d ms timer", timeout_ms);
        EventId id = guts->_request.reactor().newTimer(guts, cn::milliseconds(timeout_ms), false);
        if (id.isValid()) {
            guts->_curl_timers.insert(id);
            return 0; // success
        }
        else {
            return -1; // error
        }
    }
}


//----------------------------------------------------------------------------
// Libcurl callback for writing received data.
//----------------------------------------------------------------------------

size_t ts::ReactiveWebRequest::Guts::CurlWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    const auto guts = static_cast<Guts*>(userdata);
    if (guts == nullptr) {
        return CURL_WRITEFUNC_ERROR;
    }
    else {
        guts->_request.reactor().trace(u"curl write callback, size: %d, nmemb: %d", size, nmemb);

        // After receiving some data, it is no longer possible to retry the connection.
        guts->_can_retry = false;

        // With libcurl, there is no way to be notified of "end of connection", after response headers.
        // If this is the first response data chunk, then this is the "end of connection".
        // Need to notify the application of end of startOpen().
        if (guts->_request._status.contentSize() == 0) {
            guts->_open_done = true;
            guts->_completion_code = SYS_SUCCESS;
            guts->_request.reactor().signalEvent(guts->_event);
        }

        // Store response data in the SystemGuts buffer only if a callback was specified.
        assert(guts->_received_data != nullptr);
        const size_t chunk_size = size * nmemb;
        if (guts->_handler != nullptr) {
            guts->_received_data->append(ptr, chunk_size);
            guts->_request.reactor().signalEvent(guts->_event);
        }
        guts->_request._status.addContentSize(chunk_size);
        return chunk_size;
    }
}


//----------------------------------------------------------------------------
// Libcurl callback callback that receives header data.
//----------------------------------------------------------------------------

size_t ts::ReactiveWebRequest::Guts::CurlHeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata)
{
    const auto guts = static_cast<Guts*>(userdata);
    if (guts == nullptr) {
        return 0; // error
    }
    else {
        guts->_request.reactor().trace(u"curl header callback, size: %d, nitems: %d", size, nitems);
        // Store the headers in the request status.
        const size_t total_size = size * nitems;
        guts->_request._status.processReponseHeaders(UString::FromUTF8(buffer, total_size), guts->_request.report());
        return total_size;
    }
}


//----------------------------------------------------------------------------
// Handle a timer from the Reactor.
//----------------------------------------------------------------------------

void ts::ReactiveWebRequest::Guts::handleTimer(Reactor& reactor, EventId id)
{
    if (id == _retry_timer) {
        // This a retry after a connection failure.
        _retry_timer.invalidate();

        // In case of restart failure, give up. Errors were already reported. We are in a reactor callback, we can directly call
        // the user handler. Moreover, the _event id has been reset by the failure and we cannot signal the event.
        if (!startTransfer() && _handler != nullptr) {
            _handler->handleWebOpen(_request, _aborted ? SYS_CANCELED : SYS_ERROR, _handler_data);
            reset(true);
        }
    }
    else if (_curl_timers.contains(id)) {
        // This is a time we set for curl (in CurlTimerCallback).
        _curl_timers.erase(id);

        // Inform curl that a timeout may have elapsed and continue processing.
        if (!continueTransfer(CURL_SOCKET_TIMEOUT, 0)) {
            terminateTransfer();
        }
    }
}


//----------------------------------------------------------------------------
// Handle a read-ready event in a Reactor.
//----------------------------------------------------------------------------

void ts::ReactiveWebRequest::Guts::handleReadReady(Reactor& reactor, EventId id, int error_code)
{
    // Inform curl that something has happened on the socket. We do not specify an event mask (set to 0).
    // We let curl decide what to do on the socket (in, out, err). We could specify CURL_CSELECT_IN (and
    // possibly CURL_CSELECT_ERR). However, we need to process all events on the socket at the same time,
    // in and out, while the Reactor separately notifies read and write.
    SysSocketType sock = reactor.getSocket(id);
    if (sock >= 0 && !continueTransfer(sock, 0)) {
        terminateTransfer();
    }
}


//----------------------------------------------------------------------------
// Handle a write-ready event in a Reactor.
//----------------------------------------------------------------------------

void ts::ReactiveWebRequest::Guts::handleWriteReady(Reactor& reactor, EventId id, int error_code)
{
    // See comment in handleReadReady().
    SysSocketType sock = reactor.getSocket(id);
    if (sock >= 0 && !continueTransfer(sock, 0)) {
        terminateTransfer();
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

    // Continue curl's transfer. This was requested in a curl's callback but couldn't be done there.
    if (_push_transfer) {
        _push_transfer = false;
        if (!_aborted && !_completed && !continueTransfer(CURL_SOCKET_TIMEOUT, 0)) {
            terminateTransfer();
            return;
        }
    }

    // Process aborted request (and return).
    if (_aborted) {
        terminateTransfer();
        return;
    }

    // End of open operation (and return on error).
    if (_open_done && !_open_called) {
        _open_called = true;
        if (_handler != nullptr) {
            _handler->handleWebOpen(_request, _completion_code, _handler_data);
        }
        if (!SysSuccess(_completion_code)) {
            reset(true);
            return;
        }
    }

    // Report received data.
    if (_received_data != nullptr && !_received_data->empty()) {
        // In case of null handler, we never save the received data.
        assert(_handler != nullptr);
        _handler->handleWebReceive(_request, _received_data, SYS_SUCCESS, _handler_data);
        // Let the ownership of the data pointer to the handler object if necessary.
        _received_data = std::make_shared<ByteBlock>();
    }

    // Report transfer termination.
    if (_completed) {
        terminateTransfer();
    }
}


//----------------------------------------------------------------------------
// Terminate the transfer and call the application handler.
//----------------------------------------------------------------------------

void ts::ReactiveWebRequest::Guts::terminateTransfer()
{
    if (_aborted) {
        _completion_code = SYS_CANCELED;
    }
    if (_handler != nullptr) {
        // Terminating the transfer during open means error or successful empty response. We must call the open handler.
        if (!_open_called) {
            _handler->handleWebOpen(_request, _completion_code, _handler_data);
        }
        // A successful completion means "end of file" and it must be reported as such.
        // In case of error after open, report that error code.
        if (_open_called || SysSuccess(_completion_code)) {
            _handler->handleWebReceive(_request, nullptr, SysSuccess(_completion_code) ? SYS_EOF : _completion_code, _handler_data);
        }
    }
    reset(true);
}


//----------------------------------------------------------------------------
// Check if the transfer is in progress.
//----------------------------------------------------------------------------

bool ts::ReactiveWebRequest::isOpen() const
{
    return _guts->isOpen();
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
    if (_curlm == nullptr || _curl == nullptr) {
        _request.report().log(SilentLevel(silent), u"web request not in progress, cannot abort");
        return false;
    }
    else {
        _aborted = true;
        _request.reactor().signalEvent(_event);
        return true;
    }
}


//----------------------------------------------------------------------------
// Get the version of the underlying HTTP library.
//----------------------------------------------------------------------------

ts::UString ts::ReactiveWebRequest::GetLibraryVersion()
{
    UString result(u"libcurl");

    // Enforce libcurl initialization.
    LibCurlInit::Instance();

    // Check if runtime version is same as compiled one.
    bool same = false;

    // Get version from libcurl.
    const ::curl_version_info_data* info = ::curl_version_info(CURLVERSION_NOW);
    if (info != nullptr) {
        same = info->version_num == LIBCURL_VERSION_NUM;
        if (info->version != nullptr) {
            result.format(u": %s", info->version);
        }
        if (info->ssl_version != nullptr) {
            result.format(u", ssl: %s", info->ssl_version);
        }
        if (info->libz_version != nullptr) {
            result.format(u", libz: %s", info->libz_version);
        }
    }

    // Add compilation version if different.
    if (!same) {
        result.format(u", compiled with %s", LIBCURL_VERSION);
    }
    return result;
}

#endif // TS_NO_CURL
