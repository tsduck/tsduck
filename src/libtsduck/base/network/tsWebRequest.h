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
    //! @ingroup net
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
    class TSDUCKDLL WebRequest
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
        //! Set the connection timeout for this request.
        //! @param [in] timeout Connection timeout in milliseconds.
        //!
        void setConnectionTimeout(MilliSecond timeout);

        //!
        //! Set the timeout for each receive operation.
        //! @param [in] timeout Reception timeout in milliseconds.
        //!
        void setReceiveTimeout(MilliSecond timeout);

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
        void enableCookies(const UString& fileName = UString());

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
        UString getCookiesFileName() const;

        //!
        //! Delete the cookies file, if one was defined.
        //! @return True on success, false on error.
        //!
        bool deleteCookiesFile() const;

        //!
        //! Default user agent string ("tsduck").
        //!
        static const UString DEFAULT_USER_AGENT;

        //!
        //! Set the user agent name to use in HTTP headers.
        //! @param [in] name The user agent name. If empty, DEFAULT_USER_AGENT is used.
        //!
        void setUserAgent(const UString& name = UString());

        //!
        //! Get the current user agent name to use in HTTP headers.
        //! @return A constant reference to the user agent name to use in HTTP headers.
        //!
        const UString& userAgent() const { return _userAgent; }

        //!
        //! Enable or disable the automatic redirection of HTTP requests.
        //! This option is active by default.
        //! @param [in] on If true, allow automatic redirection of HTTP requests.
        //!
        void setAutoRedirect(bool on) { _autoRedirect = on; }

        //!
        //! Set various arguments from command line.
        //! @param [in] args Command line arguments.
        //!
        void setArgs(const WebRequestArgs& args);

        //!
        //! Set a header which will be sent with the request.
        //! @param [in] name The header name.
        //! @param [in] value The header value.
        //!
        void setRequestHeader(const UString& name, const UString& value);

        //!
        //! Clear all headers which will be sent with the request.
        //!
        void clearRequestHeaders();

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
        bool isOpen() const { return _isOpen; }

        //!
        //! Get the HTTP status code (200, 404, etc).
        //! @return The HTTP status code.
        //!
        int httpStatus() const { return _httpStatus; }

        //!
        //! Get the announced content size in bytes.
        //! This is the value which was sent in the content headers.
        //! This mat  be zero, this may not be the actual size of the content to download.
        //! @return Announced content size in bytes.
        //!
        size_t announdedContentSize() const { return _headerContentSize; }

        //!
        //! Representation of request or reponse headers.
        //! The keys of the map are the header names.
        //!
        typedef std::multimap<UString,UString> HeadersMap;

        //!
        //! Get all response headers.
        //! @param [out] headers A multimap of all response headers.
        //!
        void getResponseHeaders(HeadersMap& headers) const;

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
        UString originalURL() const { return _originalURL; }

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
        UString finalURL() const { return _finalURL; }

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
        size_t contentSize() const { return _contentSize; }

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
        bool downloadFile(const UString& url, const UString& fileName, size_t chunkSize = DEFAULT_CHUNK_SIZE);

        //!
        //! Get the version of the underlying HTTP library.
        //! @return The library version.
        //!
        static UString GetLibraryVersion();

    private:
        // System-specific parts are stored in a private structure.
        // This is done to avoid inclusion of specialized headers in this public file.
        class SystemGuts;

        Report&       _report;
        UString       _userAgent;
        bool          _autoRedirect;
        UString       _originalURL;
        UString       _finalURL;
        MilliSecond   _connectionTimeout;
        MilliSecond   _receiveTimeout;
        UString       _proxyHost;
        uint16_t      _proxyPort;
        UString       _proxyUser;
        UString       _proxyPassword;
        UString       _cookiesFileName;
        bool          _useCookies;
        bool          _deleteCookiesFile;  // delete the cookies file on close
        HeadersMap    _requestHeaders;     // all request headers (to send)
        HeadersMap    _responseHeaders;    // all response headers (received)
        int           _httpStatus;         // 200, 404, etc.
        size_t        _contentSize;        // actually downloaded size
        size_t        _headerContentSize;  // content size, as announced in response header
        volatile bool _isOpen;             // The transfer is open/started.
        volatile bool _interrupted;        // interrupted by application-defined handler
        SystemGuts*   _guts;               // system-specific data

        static UString  _defaultProxyHost;
        static uint16_t _defaultProxyPort;
        static UString  _defaultProxyUser;
        static UString  _defaultProxyPassword;

        // Allocate and deallocate guts (depend on implementations).
        void allocateGuts();
        void deleteGuts();

        // Process a list of response headers. Header lines are terminated by LF or CRLF.
        void processReponseHeaders(const UString& text);

        // System-specific transfer initialization.
        bool startTransfer();
    };
}
