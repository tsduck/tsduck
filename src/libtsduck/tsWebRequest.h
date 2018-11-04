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
//!
//!  @file
//!  Perform a simple Web request (HTTP, HTTPS, FTP).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsWebRequestHandlerInterface.h"
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
    //! The URL and optional proxy settings must be set before any download
    //! operation. By default, no proxy is used. On Windows, if no proxy is
    //! set, the default system proxy is used.
    //!
    //! The response headers are available after a successful download operation.
    //!
    class TSDUCKDLL WebRequest
    {
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
        //! Set the URL to get.
        //! @param [in] url The complete URL to fetch.
        //!
        void setURL(const UString& url);

        //!
        //! Get the original URL, as set by setURL().
        //! @return The original URL.
        //!
        UString originalURL() const
        {
            return _originalURL;
        }

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
        UString finalURL() const
        {
            return _finalURL;
        }

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
        //! Set the user agent name to use in HTTP headers.
        //! @param [in] name The user agent name.
        //!
        void setUserAgent(const UString& name)
        {
            _userAgent = name;
        }

        //!
        //! Enable or disable the automatic redirection of HTTP requests.
        //! This option is active by default.
        //! @param [in] on If true, allow automatic redirection of HTTP requests.
        //!
        void setAutoRedirect(bool on)
        {
            _autoRedirect = on;
        }

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
        //! Download the content of the URL as binary data.
        //! @param [out] data The content of the URL.
        //! @return True on success, false on error.
        //!
        bool downloadBinaryContent(ByteBlock& data);

        //!
        //! Download the content of the URL as text.
        //! The downloaded text is converted from UTF-8.
        //! End of lines are normalized as LF.
        //! @param [out] text The content of the URL.
        //! @return True on success, false on error.
        //!
        bool downloadTextContent(UString& text);

        //!
        //! Download the content of the URL in a file.
        //! No transformation is applied to the data.
        //! @param [in] fileName Name of the file to create.
        //! @return True on success, false on error.
        //!
        bool downloadFile(const UString& fileName);

        //!
        //! Download the content of the URL and pass data to the application.
        //! No transformation is applied to the data.
        //! @param [in] handler Application-defined handler to process received data.
        //! @return True on success, false on error.
        //!
        bool downloadToApplication(WebRequestHandlerInterface* handler);

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
        //! Get the size in bytes of the downloaded content.
        //! @return Size in bytes of the downloaded content.
        //!
        size_t contentSize() const
        {
            return _contentSize;
        }

        //!
        //! Get the HTTP status code (200, 404, etc).
        //! @return The HTTP status code.
        //!
        int httpStatus() const
        {
            return _httpStatus;
        }

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
        HeadersMap    _requestHeaders;           // all request headers (to send)
        HeadersMap    _responseHeaders;          // all response headers (received)
        int           _httpStatus;               // 200, 404, etc.
        size_t        _contentSize;              // actually downloaded size
        size_t        _headerContentSize;        // content size, as announced in response header
        ByteBlock*    _dlData;                   // download data buffer
        std::ofstream _dlFile;                   // download file
        WebRequestHandlerInterface* _dlHandler;  // application-defined handler
        volatile bool _interrupted;              // interrupted by application-defined handler
        SystemGuts*   _guts;                     // system-specific data

        static UString  _defaultProxyHost;
        static uint16_t _defaultProxyPort;
        static UString  _defaultProxyUser;
        static UString  _defaultProxyPassword;

        // Allocate and deallocate guts (depend on implementations).
        void allocateGuts();
        void deleteGuts();

        // Perform initialization before any download.
        bool downloadInitialize();

        // Close or abort initialized download.
        void downloadClose();

        // Perform actual download.
        bool download();

        // Process a list of response headers. Header lines are terminated by LF or CRLF.
        void processReponseHeaders(const UString& text);

        // Copy some downloaded data.
        bool copyData(const void* addr, size_t size);

        // Provide possible total download size.
        bool setPossibleContentSize(size_t totalSize);

        // Clear the transfer results, status, etc.
        bool clearTransferResults();

        // Get references to actual proxy characteristics.
        const UString&  proxyHost() const { return _proxyHost.empty() ? _defaultProxyHost : _proxyHost; }
        uint16_t        proxyPort() const { return _proxyPort == 0 ? _defaultProxyPort : _proxyPort; }
        const UString&  proxyUser() const { return _proxyUser.empty() ? _defaultProxyUser : _proxyUser; }
        const UString&  proxyPassword() const { return _proxyPassword.empty() ? _defaultProxyPassword : _proxyPassword; }

        // Inaccessible operations.
        WebRequest() = delete;
        WebRequest(const WebRequest&) = delete;
        WebRequest& operator=(const WebRequest&) = delete;
    };
}
