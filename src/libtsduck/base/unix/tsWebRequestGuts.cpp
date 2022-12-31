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
//  Perform a simple Web request - UNIX specific parts with libcurl.
//
//  IMPLEMENTATION NOTE:
//  There are two ways to use libcurl: "curl_easy" and "curl_multi".
//  The former is easier to use but it works in "push mode" only.
//
//  Initially, TSDuck used "curl_easy" with the consequence that all
//  HTTP-based plugins should work in push mode with an intermediate
//  packet queue (see class PushInputPlugin). Later, the implementation
//  was completely changed to use "curl_multi" and remove the intermediate
//  packet queue in HTTP-based plugins.
//
//  Also note that using curl_multi before version 7.66 is not very
//  efficient since there is some sort of sleep/wait cycles.
//
//----------------------------------------------------------------------------

#include "tsWebRequest.h"
#include "tsSingletonManager.h"
#include "tsMutex.h"
#include "tsGuardMutex.h"
#include "tsFileUtils.h"


//----------------------------------------------------------------------------
// Stubs when libcurl is not available.
//----------------------------------------------------------------------------

#if defined(TS_NO_CURL)

#define TS_NO_CURL_MESSAGE u"This version of TSDuck was compiled without Web support"

class ts::WebRequest::SystemGuts {};
void ts::WebRequest::allocateGuts() { _guts = new SystemGuts; }
void ts::WebRequest::deleteGuts() { delete _guts; }
bool ts::WebRequest::startTransfer() { _report.error(TS_NO_CURL_MESSAGE); return false; }
bool ts::WebRequest::receive(void*, size_t, size_t&) { _report.error(TS_NO_CURL_MESSAGE); return false; }
bool ts::WebRequest::close() { return true; }
void ts::WebRequest::abort() {}
ts::UString ts::WebRequest::GetLibraryVersion() { return UString(); }

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

// Check if curl_multi_wakeup() is present.
#if CURL_AT_LEAST_VERSION(7,68,0)
#define TS_CURL_WAKEUP 1
#endif

// Check if curl_multi_poll() is present.
#if CURL_AT_LEAST_VERSION(7,66,0)
#define TS_CURL_POLL 1
#endif

// Check if curl_multi_perform() can return CURLM_CALL_MULTI_PERFORM.
#if ! CURL_AT_LEAST_VERSION(7,20,0)
#define TS_CURL_CALLAGAIN 1
#endif

// URL of the latest official set of CA certificates from CURL.
#define FRESH_CACERT_URL u"https://curl.se/ca/cacert.pem"

// Define the various states of CA certificate processing.
// They are sorted in sequential order of operations.
namespace {
    enum CertState {
        CERT_INITIAL,   // Try without cacert file first, then use cacert file from CURL.
        CERT_EXISTING,  // Use existing cacert file from CURL.
        CERT_DOWNLOAD,  // Download cacert file from CURL.
        CERT_NONE,      // Do not use cacert file from CURL.
    };
}


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

    // Start the transfer using WebRequest parameters.
    bool startTransfer(CertState certState);

    // Close and cleanup everything.
    void clear();

    // Wait for data to be present in the reception buffer.
    // If maxSize is zero, wait until something is present in data buffer
    // without returning anything. If certError is not null and the error
    // is "about SSL/TLS certificates" (hard to specify), set this bool to true.
    bool receive(void* buffer, size_t maxSize, size_t* retSize, bool* certError);

    // Can be called from another thread to safely interrupt the current transfer.
    void abort();

    // Build error messages from curl_multi and curl_easy.
    template<typename ENUM> UString message(const UString& title, ENUM code, const char* (*strerror)(ENUM));
    UString easyMessage(const UString& title, ::CURLcode code) { return message(title, code, ::curl_easy_strerror); }
    UString multiMessage(const UString& title, ::CURLMcode code) { return message(title, code, ::curl_multi_strerror); }

private:
    WebRequest&   _request;                 // Reference to parent WebRequest.
#if defined(TS_CURL_WAKEUP)
    Mutex         _mutex;                   // Exclusive access to _curlm/_curl init/clear sequences.
#endif
    ::CURLM*      _curlm;                   // "curl_multi" handler.
    ::CURL*       _curl;                    // "curl_easy" handler.
    ::curl_slist* _headers;                 // Request headers.
    UString       _certFile;                // Latest CA certificates file.
    ByteBlock     _data;                    // Received data, filled by writeCallback(), emptied by receive().
    char          _error[CURL_ERROR_SIZE];  // Error message buffer for libcurl.

    // Handle an error while receiving data. Always return false.
    bool downloadError(const UString& message, bool* certError);

    // Libcurl callbacks for response headers and response data.
    // The userdata points to this SystemGuts object.
    static size_t headerCallback(char *ptr, size_t size, size_t nmemb, void *userdata);
    static size_t writeCallback(char *ptr, size_t size, size_t nmemb, void *userdata);
};


//----------------------------------------------------------------------------
// System-specific constructor and destructor.
//----------------------------------------------------------------------------

ts::WebRequest::SystemGuts::SystemGuts(WebRequest& request) :
    _request(request),
#if defined(TS_CURL_WAKEUP)
    _mutex(),
#endif
    _curlm(nullptr),
    _curl(nullptr),
    _headers(nullptr),
    _certFile(UserHomeDirectory() + u"/.tscacert.pem"),
    _data(),
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
// Build an error message from libcurl.
//----------------------------------------------------------------------------

template<typename ENUM>
ts::UString ts::WebRequest::SystemGuts::message(const UString& title, ENUM code, const char* (*strerror)(ENUM))
{
    UString msg(title);
    msg.append(u", ");
    const char* err = strerror(code);
    if (err != nullptr && err[0] != 0) {
        msg.append(UString::FromUTF8(err));
    }
    else {
        msg.format(u"error code %d", {int(code)});
    }
    if (_error[0] != 0) {
        msg.append(u", ");
        msg.append(UString::FromUTF8(_error));
    }
    return msg;
}


//----------------------------------------------------------------------------
// Download operations from the WebRequest class.
//----------------------------------------------------------------------------

bool ts::WebRequest::startTransfer()
{
    return _guts->startTransfer(CERT_INITIAL);
}

bool ts::WebRequest::receive(void* buffer, size_t maxSize, size_t& retSize)
{
    if (_isOpen) {
        return _guts->receive(buffer, maxSize, &retSize, nullptr);
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
    _interrupted = true;
    _guts->abort();
}


//----------------------------------------------------------------------------
// Initialize transfer.
//----------------------------------------------------------------------------

bool ts::WebRequest::SystemGuts::startTransfer(CertState certState)
{
    // Check that libcurl was correctly initialized.
    if (LibCurlInit::Instance()->initStatus != ::CURLE_OK) {
        _request._report.error(easyMessage(u"libcurl initialization error", LibCurlInit::Instance()->initStatus));
        return false;
    }

    // Make sure we start from a clean state.
    clear();

    // If no CA certificate file is specified, bypass certificate processing.
    if (_certFile.empty()) {
        certState = CERT_NONE;
    }

    // Download the CA certificate file from CURL if requested.
    const bool certFileExists = certState != CERT_NONE && FileExists(_certFile);
    if (certState == CERT_EXISTING && certFileExists && (Time::CurrentUTC() - GetFileModificationTimeUTC(_certFile)) < MilliSecPerDay) {
        // The cert file is "fresh" (updated less than one day aga), no need to retry to load it, let's pretend we just downloaded it.
        certState = CERT_DOWNLOAD;
        _request._report.debug(u"reusing recent CA cert file %s", {_certFile});
    }
    else if ((certState == CERT_EXISTING && !certFileExists) || certState == CERT_DOWNLOAD) {
        // We need to download it. Jump to CERT_DOWNLOAD if there was no file.
        certState = CERT_DOWNLOAD;
        _request._report.verbose(u"encountered certificate issue, downloading a fresh CA list from %s", {FRESH_CACERT_URL});

        WebRequest certRequest(_request._report);
        certRequest.setAutoRedirect(true);
        certRequest.setProxyHost(_request._proxyHost, _request._proxyPort);
        certRequest.setProxyUser(_request._proxyUser, _request._proxyPassword);
        certRequest.setReceiveTimeout(_request._receiveTimeout);
        certRequest.setConnectionTimeout(_request._connectionTimeout);
        certRequest._guts->_certFile.clear(); // don't recurse in case of cert issue!

        if (!certRequest.downloadFile(FRESH_CACERT_URL, _certFile) || !FileExists(_certFile)) {
            _request._report.verbose(u"failed to get a fresh CA list, use default list");
            certState = CERT_NONE;
        }
    }

    // The initialization and cleanup sequences of _curlm and _curl must be protected
    // when we have the ability to wakeup curl_multi from another thread.
    {
#if defined(TS_CURL_WAKEUP)
        GuardMutex lock(_mutex);
#endif
        // Initialize curl_multi and curl_easy
        if ((_curlm = ::curl_multi_init()) == nullptr) {
            _request._report.error(u"libcurl 'curl_multi' initialization error");
            return false;
        }
        if ((_curl = ::curl_easy_init()) == nullptr) {
            _request._report.error(u"libcurl 'curl_easy' initialization error");
            clear();
            return false;
        }

        // Register the curl_easy handle inside the curl_multi handle.
        ::CURLMcode mstatus = ::curl_multi_add_handle(_curlm, _curl);
        if (mstatus != ::CURLM_OK) {
            _request._report.error(multiMessage(u"curl_multi_add_handle error", mstatus));
            clear();
            return false;
        }
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

    // Set the CA certificate file.
    if (status == ::CURLE_OK && (certState == CERT_EXISTING || certState == CERT_DOWNLOAD)) {
        status = ::curl_easy_setopt(_curl, CURLOPT_CAINFO, _certFile.toUTF8().c_str());
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
        for (const auto& it : _request._requestHeaders) {
            const UString header(it.first + u": " + it.second);
            _headers = ::curl_slist_append(_headers, header.toUTF8().c_str());
        }
        status = ::curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, _headers);
    }

    // End of curl_easy_setopt() sequence.
    TS_POP_WARNING()

    // Now process setopt error.
    if (status != ::CURLE_OK) {
        _request._report.error(easyMessage(u"libcurl setopt error", status));
        clear();
        return false;
    }

    // There is no specific way to wait for connection and end of response header reception.
    // So, wait until at least one data byte of response body is received.
    // Make certificate error silent in phases CERT_INITIAL and CERT_EXISTING because we have other options later.
    bool certError = false;
    if (receive(nullptr, 0, nullptr, certState < CERT_DOWNLOAD ? &certError : nullptr)) {
        return true;
    }

    // Error, reset state.
    clear();

    // In case of certificate error, try with an updated list of CA certificates.
    if (certError) {
        return startTransfer(CertState(certState + 1));
    }
    else {
        return false;
    }
}


//----------------------------------------------------------------------------
// Handle an error while receiving data. Always return false.
//----------------------------------------------------------------------------

bool ts::WebRequest::SystemGuts::downloadError(const UString& msg, bool* certError)
{
    // There is no real deterministic way of diagnosing certificate error.
    // In practice, we get messages like this one:
    // "SSL peer certificate or SSH remote key was not OK, SSL certificate problem: unable to get local issuer certificate"
    if (certError != nullptr) {
        *certError = msg.contain(u"certificate", CASE_INSENSITIVE);
        if (*certError) {
            // In case of certificate error, fail silently.
            return false;
        }
    }

    // Normal error processing.
    _request._report.error(msg);
    return false;
}


//----------------------------------------------------------------------------
// Wait for data to be present in the reception buffer.
//----------------------------------------------------------------------------

bool ts::WebRequest::SystemGuts::receive(void* buffer, size_t maxSize, size_t* retSize, bool* certError)
{
    // Preset returned size as zero.
    if (retSize != nullptr) {
        *retSize = 0;
    }

    ::CURLMcode mstatus = ::CURLM_OK;
    int runningHandles = 0;

    // If the response buffer is empty, wait for data.
    while (_data.empty() && !_request._interrupted) {

        // Perform all immediate operations. Non-blocking call.
#if defined(TS_CURL_CALLAGAIN)
        // Older versions of curl may need to be called again immediately.
        do {
            mstatus = ::curl_multi_perform(_curlm, &runningHandles);
        } while (mstatus == CURLM_CALL_MULTI_PERFORM);
#else
        mstatus = ::curl_multi_perform(_curlm, &runningHandles);
#endif
        if (mstatus != ::CURLM_OK) {
            _request._report.error(multiMessage(u"download error", mstatus));
            return false;
        }

        // If there is no more running handle, no need to wait for more.
        if (runningHandles == 0 || _request._interrupted) {
            break;
        }

        // If there is still nothing in the response buffer, wait for something to be ready.
        if (_data.empty()) {

            // Wait for something to happen on the sockets or some t
            int numfds = 0;
#if defined(TS_CURL_POLL)
            // Recent versions of curl have an explicit poll. Wait no more than one second.
            mstatus = ::curl_multi_poll(_curlm, nullptr, 0, 1000, &numfds);
#else
            // Recent versions of curl have an explicit poll. Wait no more than one second.
            mstatus = ::curl_multi_wait(_curlm, nullptr, 0, 1000, &numfds);
#endif
            if (mstatus != ::CURLM_OK) {
                return downloadError(multiMessage(u"download error", mstatus), certError);
            }
        }
    }

    // Immediate error on interrupt.
    if (_request._interrupted) {
        _request._report.debug(u"curl: request was interrupted");
        return false;
    }

    // If the data buffer is empty and there is no more running transfer, check status.
    if (_data.empty() && runningHandles == 0) {
        ::CURLMsg* msg = nullptr;
        int remainingMsg = 0;
        while ((msg = ::curl_multi_info_read(_curlm, &remainingMsg)) != nullptr) {
            if (msg->msg == CURLMSG_DONE && msg->easy_handle == _curl) {
                // End of transfer.
                if (msg->data.result == ::CURLE_OK) {
                    // Successful end of transfer, return true and let retSize be zero.
                    _request._report.debug(u"curl: end of transfer");
                    return true;
                }
                else {
                    // Transfer error.
                    return downloadError(easyMessage(u"download error", msg->data.result), certError);
                }
            }
        }
        // At this point, there is no data, no completion, no running handler, we are lost...
        // It has been observed that this happens when there is no reponse data (only headers).
        // So, let's assume that the transfer was successful.
        _request._report.debug(u"curl: no data, no more running handle");
        return true;
    }

    // Now transfer data to the user.
    const size_t size = buffer == nullptr ? 0 : std::min(_data.size(), maxSize);
    if (size > 0) {
        ::memcpy(buffer, _data.data(), size);
        if (size >= _data.size()) {
            _data.clear();
        }
        else {
            _data.erase(0, size);
        }
    }
    if (retSize != nullptr) {
        *retSize = size;
    }
    return true;
}


//----------------------------------------------------------------------------
// Can be called from another thread to safely interrupt the current transfer.
//----------------------------------------------------------------------------

void ts::WebRequest::SystemGuts::abort()
{
    // On older versions of curl, without curl_multi_wakeup, there is no safe way to wake it up from another thread.
#if defined(TS_CURL_WAKEUP)
    GuardMutex lock(_mutex);
    if (_curlm != nullptr) {
        ::curl_multi_wakeup(_curlm);
    }
#endif
}


//----------------------------------------------------------------------------
// Close and cleanup everything.
//----------------------------------------------------------------------------

void ts::WebRequest::SystemGuts::clear()
{
#if defined(TS_CURL_WAKEUP)
    // Make sure we don't call curl_multi_wakeup() while deallocating.
    GuardMutex lock(_mutex);
#endif

    // Deallocate list of headers.
    if (_headers != nullptr) {
        ::curl_slist_free_all(_headers);
        _headers = nullptr;
    }

    // Remove curl_easy handler.
    if (_curl != nullptr && _curlm != nullptr) {
        ::curl_multi_remove_handle(_curlm, _curl);
    }

    // Make sure the curl_easy is clean.
    if (_curl != nullptr) {
        ::curl_easy_cleanup(_curl);
        _curl = nullptr;
    }

    // Make sure the curl_multi is clean.
    if (_curlm != nullptr) {
        ::curl_multi_cleanup(_curlm);
        _curlm = nullptr;
    }

    // Erase nul-terminated error message.
    _error[0] = 0;

    // Cleanup response data buffer.
    _data.clear();
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
        // Store headers in the WebRequest.
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
    // The userdata points to the guts object.
    SystemGuts* guts = reinterpret_cast<SystemGuts*>(userdata);
    if (guts == nullptr) {
        return 0; // error
    }
    else {
        // Store response data in the SystemGuts.
        const size_t dataSize = size * nmemb;
        guts->_data.append(ptr, dataSize);
        return dataSize;
    }
}


//----------------------------------------------------------------------------
// Get the version of the underlying HTTP library.
//----------------------------------------------------------------------------

ts::UString ts::WebRequest::GetLibraryVersion()
{
    UString result(u"libcurl");

    // Check if runtime version is same as compiled one.
    bool same = false;

    // Get version from libcurl.
    const ::curl_version_info_data* info = ::curl_version_info(CURLVERSION_NOW);
    if (info != nullptr) {
        same = info->version_num == LIBCURL_VERSION_NUM;
        if (info->version != nullptr) {
            result.format(u": %s", {info->version});
        }
        if (info->ssl_version != nullptr) {
            result.format(u", ssl: %s", {info->ssl_version});
        }
        if (info->libz_version != nullptr) {
            result.format(u", libz: %s", {info->libz_version});
        }
    }

    // Add compilation version if different.
    if (!same) {
        result.format(u", compiled with %s", {LIBCURL_VERSION});
    }
    return result;
}

#endif // TS_NO_CURL
