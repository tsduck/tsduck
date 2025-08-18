//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Perform a simple Web request (HTTP, HTTPS, FTP).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsWebRequestArgs.h"
#include "tsReport.h"
#include "tsByteBlock.h"
#include "tsUString.h"

namespace ts {
    //!
    //! Perform a simple Web request (HTTP, HTTPS, FTP).
    //! @ingroup libtscore net
    //!
    //! On UNIX systems, the implementation uses libcurl.
    //! On Windows systems, the implementation uses Microsoft Wininet.
    //! We could have used libcurl on Windows but building it was a pain...
    //!
    //! The proxy and transfer settings must be set before starting any
    //! download operation. The HTTP status and the response headers are
    //! available after a successful download start.
    //!
    //! By default, no proxy is used. If no proxy is set, the default proxy
    //! is used (system configuration on Windows, http_proxy environment on
    //! Unix systems).
    //!
    class TSCOREDLL WebRequest
    {
        TS_NOBUILD_NOCOPY(WebRequest);
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to report errors.
        //!
        WebRequest(Report& report);

        //!
        //! Destructor.
        //!
        virtual ~WebRequest();

        //!
        //! Use the Report object of this instance.
        //! @return A reference to the Report object of this instance.
        //!
        Report& report() { return _report; }

        //!
        //! Default TCP port for HTTP.
        //!
        static constexpr uint16_t DEFAULT_HTTP_PORT = 80;

        //!
        //! Default TCP port for HTTPS.
        //!
        static constexpr uint16_t DEFAULT_HTTPS_PORT = 443;

        //!
        //! Set the connection timeout for this request.
        //! @param [in] timeout Connection timeout in milliseconds.
        //!
        void setConnectionTimeout(cn::milliseconds timeout) { _connection_timeout = timeout; }

        //!
        //! Set the timeout for each receive operation.
        //! @param [in] timeout Reception timeout in milliseconds.
        //!
        void setReceiveTimeout(cn::milliseconds timeout) { _receive_timeout = timeout; }

        //!
        //! Set the optional proxy host and port for this request.
        //! @param [in] host Proxy host name or address.
        //! @param [in] port Proxy port number.
        //!
        void setProxyHost(const UString& host, uint16_t port);

        //!
        //! Set the optional proxy authentication for this request.
        //! @param [in] user Proxy user name.
        //! @param [in] password Proxy user's password.
        //!
        void setProxyUser(const UString& user, const UString& password);

        //!
        //! Set the default proxy host and port for all subsequent requests.
        //! @param [in] host Proxy host name or address.
        //! @param [in] port Proxy port number.
        //!
        static void SetDefaultProxyHost(const UString& host, uint16_t port);

        //!
        //! Set the default proxy authentication for all subsequent requests.
        //! @param [in] user Proxy user name.
        //! @param [in] password Proxy user's password.
        //!
        static void SetDefaultProxyUser(const UString& user, const UString& password);

        //!
        //! Get the current actual proxy host.
        //! @return A constant reference to the proxy host name.
        //!
        const UString& proxyHost() const;

        //!
        //! Get the current actual proxy port number.
        //! @return The proxy port number.
        //!
        uint16_t proxyPort() const;

        //!
        //! Get the current actual proxy user name.
        //! @return A constant reference to the proxy user name.
        //!
        const UString& proxyUser() const;

        //!
        //! Get the current actual proxy user password.
        //! @return A constant reference to the proxy user password.
        //!
        const UString& proxyPassword() const;

        //!
        //! Enable the use of cookies for all requests using this instance.
        //! @param [in] fileName The name of the file to use to load and store cookies.
        //! On Windows, there is an implicit per-user cookie repository and @a fileName
        //! is ignored. On Unix systems, this file is used to store and retrieve cookies
        //! in the libcurl format. When @a fileName is empty, use a temporary file name.
        //!
        void enableCookies(const fs::path& fileName = fs::path());

        //!
        //! Disable the use of cookies for all requests.
        //! Cookies are initially disabled by default.
        //!
        void disableCookies();

        //!
        //! Get the file name to use for cookies for all requests using this instance.
        //! - On Linux, return the current cookie file name, possibly the name of a
        //!   temporary file if EnableCookies() was called with an empty string.
        //! - On Windows, the cookie repository is defined per user. There is no specific
        //!   per-application file and this method always report an empty string.
        //! @return The cookie file name.
        //!
        fs::path getCookiesFileName() const;

        //!
        //! Delete the cookies file, if one was defined.
        //! @return True on success, false on error.
        //!
        bool deleteCookiesFile() const;

        //!
        //! Default user agent string ("tsduck").
        //!
        static constexpr const UChar* DEFAULT_USER_AGENT = u"tsduck";

        //!
        //! Set the user agent name to use in HTTP headers.
        //! @param [in] name The user agent name. If empty, DEFAULT_USER_AGENT is used.
        //!
        void setUserAgent(const UString& name = UString()) { _user_agent = name.empty() ? DEFAULT_USER_AGENT : name; }

        //!
        //! Get the current user agent name to use in HTTP headers.
        //! @return A constant reference to the user agent name to use in HTTP headers.
        //!
        const UString& userAgent() const { return _user_agent; }

        //!
        //! Enable compression.
        //! Compression is disabled by default.
        //! @param [in] on Boolean setting compression on or off.
        //!
        void enableCompression(bool on = true) { _use_compression = on; }

        //!
        //! Enable or disable HTTPS security (certificate validation).
        //! Certificate validation is enabled by default.
        //! @param [in] on If true, disable certificate validation.
        //!
        void setInsecure(bool on = true) { _insecure = on; }

        //!
        //! Enable or disable the automatic redirection of HTTP requests.
        //! This option is active by default.
        //! @param [in] on If true, allow automatic redirection of HTTP requests.
        //!
        void setAutoRedirect(bool on) { _auto_redirect = on; }

        //!
        //! Set various arguments from command line.
        //! @param [in] args Command line arguments.
        //!
        void setArgs(const WebRequestArgs& args);

        //!
        //! Set a header which will be sent with the request.
        //! If the same header already exists with another value, a new header is added.
        //! @param [in] name The header name.
        //! @param [in] value The header value.
        //!
        void setRequestHeader(const UString& name, const UString& value);

        //!
        //! Delete all headers with a given name.
        //! @param [in] name The header name.
        //!
        void deleteRequestHeader(const UString& name);

        //!
        //! Clear all headers which will be sent with the request.
        //!
        void clearRequestHeaders();

        //!
        //! Set data to POST.
        //! The request will be a POST one.
        //! @param [in] data Text POST data. The text will be sent in UTF-8 format.
        //! @param [in] content_type The content type to set in the request headers.
        //! The default "Content-Type" header is "text/plain; charset=utf-8", which is usually appropriate.
        //! When set to the empty string, no header is set.
        //!
        void setPostData(const UString& data, const UString content_type = u"text/plain; charset=utf-8");

        //!
        //! Set data to POST.
        //! The request will be a POST one.
        //! @param [in] data Binary POST data.
        //!
        void setPostData(const ByteBlock& data);

        //!
        //! Clear previous POST data.
        //! The request will be a GET one.
        //!
        void clearPostData();

        //!
        //! Open an URL and start the transfer.
        //! For HTTP request, perform all redirections and get response headers.
        //! @param [in] url The complete URL to fetch.
        //! @return True on success, false on error.
        //!
        bool open(const UString& url);

        //!
        //! Check if a transfer is open.
        //! @return True if a transfer is open, false otherwise.
        //!
        bool isOpen() const { return _is_open; }

        //!
        //! Get the HTTP status code (200, 404, etc).
        //! @return The HTTP status code.
        //!
        int httpStatus() const { return _http_status; }

        //!
        //! Check if the HTTP status code indicates success.
        //! The HTTP status codes are classified as follow (Wikipedia):
        //! - 1xx informational response – the request was received, continuing process
        //! - 2xx successful – the request was successfully received, understood, and accepted
        //! - 3xx redirection – further action needs to be taken in order to complete the request
        //! - 4xx client error – the request contains bad syntax or cannot be fulfilled
        //! - 5xx server error – the server failed to fulfil an apparently valid request
        //! @return True if the HTTP status code indicates success.
        //!
        int httpSuccess() const { return _http_status < 400; }

        //!
        //! Get the announced content size in bytes.
        //! This is the value which was sent in the content headers.
        //! This mat  be zero, this may not be the actual size of the content to download.
        //! @return Announced content size in bytes.
        //!
        size_t announdedContentSize() const { return _header_content_size; }

        //!
        //! Get all response headers.
        //! @param [out] headers A multimap of all response headers.
        //!
        void getResponseHeaders(UStringToUStringMultiMap& headers) const { headers = _response_headers; }

        //!
        //! Get all response headers.
        //! @return A constant reference to a map of response headers.
        //!
        const UStringToUStringMultiMap& responseHeaders() const { return _response_headers; }

        //!
        //! Get the value of one header.
        //! @param [in] name Header name, case sensitive.
        //! @return Header value or an empty string when the header is not found.
        //! If the header is present more than once, the first value is returned.
        //!
        UString reponseHeader(const UString& name) const;

        //!
        //! Get the MIME type in the response headers.
        //! @param [in] simple If true, simple type name. If false, return the full specification with options.
        //! @param [in] lowercase Force lowercase in the result.
        //! @return The MIME type.
        //!
        UString mimeType(bool simple = true, bool lowercase = true) const;

        //!
        //! Get the original URL, as set by setURL().
        //! @return The original URL.
        //!
        UString originalURL() const { return _original_url; }

        //!
        //! Get the final URL of the actual download operation.
        //!
        //! It can be different from originalURL() if some HTTP redirections were performed.
        //! When called before a download operation, return originalURL().
        //!
        //! If redirections are disabled using setAutoRedirect() and the site
        //! returned a redirection, finalURL() returns the redirected URL.
        //!
        //! @return The final / redirected URL.
        //!
        UString finalURL() const { return _final_url; }

        //!
        //! Receive data.
        //!
        //! @param [out] buffer Address of the buffer for the received data.
        //! @param [in] maxSize Size in bytes of the reception buffer.
        //! @param [out] retSize Size in bytes of the received data. Will never be larger than @a max_size.
        //! When @a ret_size is zero, this is the end of the transfer.
        //! @return True on success, false on error. A successful end of transfer is reported when
        //! @a ret_size is zero and the returned value is true.
        //!
        bool receive(void* buffer, size_t maxSize, size_t& retSize);

        //!
        //! Close the transfer.
        //! @return True on success, false on error.
        //!
        bool close();

        //!
        //! Abort a transfer in progress.
        //! Can be called from another thread.
        //!
        void abort();

        //!
        //! Get the size in bytes of the downloaded content.
        //! @return Size in bytes of the downloaded content.
        //!
        size_t contentSize() const { return _content_size; }

        //!
        //! Default download chunk size for bulk transfers.
        //!
        static constexpr size_t DEFAULT_CHUNK_SIZE = 64 * 1024;

        //!
        //! Download the content of the URL as binary data in one operation.
        //! The open/read/close session is embedded in this method.
        //! @param [in] url The complete URL to fetch.
        //! @param [out] data The content of the URL.
        //! @param [in] chunkSize Individual download chunk size.
        //! @return True on success, false on error.
        //!
        bool downloadBinaryContent(const UString& url, ByteBlock& data, size_t chunkSize = DEFAULT_CHUNK_SIZE);

        //!
        //! Download the content of the URL as text in one operation.
        //! The open/read/close session is embedded in this method..
        //! The downloaded text is converted from UTF-8.
        //! End of lines are normalized as LF.
        //! @param [in] url The complete URL to fetch.
        //! @param [out] text The content of the URL.
        //! @param [in] chunkSize Individual download chunk size.
        //! @return True on success, false on error.
        //!
        bool downloadTextContent(const UString& url, UString& text, size_t chunkSize = DEFAULT_CHUNK_SIZE);

        //!
        //! Download the content of the URL in a file in one operation.
        //! The open/read/close session is embedded in this method..
        //! No transformation is applied to the data.
        //! @param [in] url The complete URL to fetch.
        //! @param [in] fileName Name of the file to create.
        //! @param [in] chunkSize Individual download chunk size.
        //! @return True on success, false on error.
        //!
        bool downloadFile(const UString& url, const fs::path& fileName, size_t chunkSize = DEFAULT_CHUNK_SIZE);

        //!
        //! Get the version of the underlying HTTP library.
        //! @return The library version.
        //!
        static UString GetLibraryVersion();

    private:
        // System-specific parts are stored in a private structure.
        // This is done to avoid inclusion of specialized headers in this public file.
        class SystemGuts;

        Report&          _report;
        UString          _user_agent {DEFAULT_USER_AGENT};
        UString          _original_url {};
        UString          _final_url {};
        cn::milliseconds _connection_timeout {};
        cn::milliseconds _receive_timeout {};
        UString          _proxy_host {};
        uint16_t         _proxy_port = 0;
        UString          _proxy_user {};
        UString          _proxy_password {};
        bool             _use_cookies = false;
        bool             _auto_redirect = true;
        bool             _use_compression = false;
        bool             _insecure = false;
        bool             _delete_cookies_file = false; // delete the cookies file on close
        fs::path         _cookies_file_name {};
        UStringToUStringMultiMap _request_headers {};  // all request headers (to send)
        UStringToUStringMultiMap _response_headers {}; // all response headers (received)
        ByteBlock        _post_data {};                // if non empty, use a POST request
        int              _http_status = 0;             // 200, 404, etc.
        size_t           _content_size = 0;            // actually downloaded size
        size_t           _header_content_size = 0;     // content size, as announced in response header
        volatile bool    _is_open = false;             // the transfer is open/started.
        volatile bool    _interrupted = false;         // interrupted by application-defined handler
        SystemGuts*      _guts = nullptr;              // system-specific data

        static UString   _default_proxy_host;
        static uint16_t  _default_proxy_port;
        static UString   _default_proxy_user;
        static UString   _default_proxy_assword;

        // Allocate and deallocate guts (depend on implementations).
        void allocateGuts();
        void deleteGuts();

        // Process a list of response headers. Header lines are terminated by LF or CRLF.
        void processReponseHeaders(const UString& text);

        // System-specific transfer initialization.
        bool startTransfer();
    };
}
