//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Command line arguments for the class WebRequest.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsByteBlock.h"
#include "tsURL.h"

namespace ts {

    class Report;
    class Args;

    //!
    //! Command line arguments for the class WebRequest.
    //! @ingroup libtscore net
    //!
    class TSCOREDLL WebRequestArgs
    {
    public:
        //!
        //! Constructor.
        //!
        WebRequestArgs() = default;

        //!
        //! Default TCP port for HTTP.
        //!
        static constexpr uint16_t DEFAULT_HTTP_PORT = 80;

        //!
        //! Default TCP port for HTTPS.
        //!
        static constexpr uint16_t DEFAULT_HTTPS_PORT = 443;

        //!
        //! Default user agent string ("tsduck").
        //!
        static constexpr const UChar* DEFAULT_USER_AGENT = u"tsduck";

        //!
        //! Add command line option definitions in an Args.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineArgs(Args& args);

        //!
        //! Load arguments from command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] args Command line arguments.
        //! @return True on success, false on error in argument line.
        //!
        bool loadArgs(Args& args);

        //!
        //! Set the connection timeout for the request (option -\-connection-timeout).
        //! @param [in] timeout Connection timeout in milliseconds.
        //!
        void setConnectionTimeout(cn::milliseconds timeout) { _connection_timeout = timeout; }

        //!
        //! Get the connection timeout for the request (option -\-connection-timeout).
        //! @return Connection timeout in milliseconds.
        //!
        cn::milliseconds connectionTimeout() const { return _connection_timeout; }

        //!
        //! Set the timeout for each receive operation (option -\-receive-timeout).
        //! @param [in] timeout Reception timeout in milliseconds.
        //!
        void setReceiveTimeout(cn::milliseconds timeout) { _receive_timeout = timeout; }

        //!
        //! Get the timeout for each receive operation (option -\-receive-timeout).
        //! @return Reception timeout in milliseconds.
        //!
        cn::milliseconds receiveTimeout() const { return _receive_timeout; }

        //!
        //! Set the optional proxy host and port for this request (options -\-proxy-host and -\-proxy-port).
        //! @param [in] host Proxy host name or address.
        //! @param [in] port Proxy port number.
        //!
        void setProxyHost(const UString& host, uint16_t port);

        //!
        //! Set the default proxy host and port for all subsequent Web requests in the application.
        //! Initially loaded from environment variables https_proxy or http_proxy.
        //! @param [in] host Proxy host name or address.
        //! @param [in] port Proxy port number.
        //!
        static void SetDefaultProxyHost(const UString& host, uint16_t port);

        //!
        //! Set the optional proxy authentication for this request (options -\-proxy-user and -\-proxy-password).
        //! @param [in] user Proxy user name.
        //! @param [in] password Proxy user's password.
        //!
        void setProxyUser(const UString& user, const UString& password);

        //!
        //! Set the default proxy authentication for all subsequent requests.
        //! Initially loaded from environment variables https_proxy or http_proxy.
        //! @param [in] user Proxy user name.
        //! @param [in] password Proxy user's password.
        //!
        static void SetDefaultProxyUser(const UString& user, const UString& password);

        //!
        //! Get the current actual proxy host (option -\-proxy-host or default).
        //! @return A constant reference to the proxy host name.
        //!
        const UString& proxyHost() const;

        //!
        //! Get the current actual proxy port number (option -\-proxy-port or default).
        //! @return The proxy port number.
        //!
        uint16_t proxyPort() const;

        //!
        //! Get the current actual proxy user name (option -\-proxy-user or default).
        //! @return A constant reference to the proxy user name.
        //!
        const UString& proxyUser() const;

        //!
        //! Get the current actual proxy user password (option -\-proxy-password or default).
        //! @return A constant reference to the proxy user password.
        //!
        const UString& proxyPassword() const;

        //!
        //! Enable the propagation of cookies for all requests using the same instance of a web request.
        //! Cookies are initially enabled by default.
        //! @param [in] file_name The name of the file to use to load and store cookies.
        //! On Windows, there is an implicit per-user cookie repository and @a fileName
        //! is ignored. On Unix systems, this file is used to store and retrieve cookies
        //! in the libcurl format. When @a fileName is empty, use a temporary file name.
        //!
        void enableCookies(const fs::path& file_name = fs::path());

        //!
        //! Check if cookies are enabled.
        //! @return True if cookies are enabled, false otherwise.
        //!
        bool cookiesEnabled() const { return _use_cookies; }

        //!
        //! Disable the use of cookies for all requests.
        //!
        void disableCookies();

        //!
        //! Get the file name to use for cookies for all requests using this instance.
        //! - On Linux, return the current cookie file name, possibly the name of a
        //!   temporary file if EnableCookies() was called with an empty string.
        //! - On Windows, the cookie repository is defined per user. There is no specific
        //!   per-application file and this method always report an empty string.
        //! @return A constant reference to the cookie file name.
        //!
        const fs::path& cookiesFileName() const { return _cookies_file_name; }

        //!
        //! Delete the cookies file, if a temporary one was defined.
        //! @param [in,out] report Where to display error messages.
        //! @return True on success, false on error.
        //!
        bool deleteTemporaryCookiesFile(Report& report) const;

        //!
        //! Set the user agent name to use in HTTP headers (option -\-user-agent).
        //! @param [in] name The user agent name. If empty, DEFAULT_USER_AGENT is used.
        //!
        void setUserAgent(const UString& name = UString()) { _user_agent = name.empty() ? DEFAULT_USER_AGENT : name; }

        //!
        //! Get the current user agent name to use in HTTP headers (option -\-user-agent).
        //! @return A constant reference to the user agent name to use in HTTP headers.
        //!
        const UString& userAgent() const { return _user_agent; }

        //!
        //! Enable compression (optoin -\-compressed).
        //! Compression is disabled by default.
        //! @param [in] on Boolean setting compression on or off.
        //!
        void enableCompression(bool on) { _use_compression = on; }

        //!
        //! Check if compression is enabled.
        //! @return True if compression is enabled.
        //!
        bool compressionEnabled() const { return _use_compression; }

        //!
        //! Enable or disable HTTPS security (certificate validation).
        //! Certificate validation is enabled by default.
        //! @param [in] on If true, disable certificate validation.
        //!
        void setInsecure(bool on) { _https_insecure = on; }

        //!
        //! Check if HTTPS security is disabled.
        //! @return True if HTTPS security is disabled.
        //!
        bool isInsecure() const { return _https_insecure; }

        //!
        //! Enable or disable the automatic redirection of HTTP requests.
        //! Automatic redirection is enabled by default.
        //! @param [in] on If true, allow automatic redirection of HTTP requests.
        //!
        void setAutoRedirect(bool on) { _auto_redirect = on; }

        //!
        //! Check if automatic redirection of HTTP requests is enabled.
        //! @return True if automatic redirection of HTTP requests is enabled.
        //!
        bool autoRedirect() const { return _auto_redirect; }

        //!
        //! Set a header which will be sent with the request (option -\-headers).
        //! If the same header already exists with another value, a new header is added.
        //! @param [in] name The header name.
        //! @param [in] value The header value.
        //!
        void setRequestHeader(const UString& name, const UString& value);

        //!
        //! Delete all request headers with a given name.
        //! @param [in] name The header name.
        //!
        void deleteRequestHeader(const UString& name) { _request_headers.erase(name); }

        //!
        //! Clear all headers which will be sent with the request.
        //!
        void clearRequestHeaders() { _request_headers.clear(); }

        //!
        //! Get all request headers.
        //! @return A constant reference to a map of request headers.
        //!
        const UStringToUStringMultiMap& requestHeaders() const { return _request_headers; }

        //!
        //! Append all request headers to a text string.
        //! Header lines are separated by CR-LF.
        //! @param [in,out] text Where to append the text lines.
        //!
        void appendRequestHeaders(UString& text) const;

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
        void setPostData(const ByteBlock& data) { _post_data = data; }

        //!
        //! Clear previous POST data.
        //! The request will be a GET one.
        //!
        void clearPostData() { _post_data.clear(); }

        //!
        //! Check if the request is a POST one.
        //! @return True if the request is a POST one.
        //!
        bool isPost() const { return !_post_data.empty(); }

        //!
        //! Get the POST data.
        //! @return A constant reference to the binary POST data.
        //!
        const ByteBlock& postData() const { return _post_data; }

        //!
        //! Get the HTTP request name.
        //! @return "GET" or "POST".
        //!
        UString requestName() const { return _post_data.empty() ? u"GET" : u"POST"; }

    private:
        cn::milliseconds _connection_timeout {};
        cn::milliseconds _receive_timeout {};
        uint16_t         _proxy_port = 0;
        UString          _proxy_host {};
        UString          _proxy_user {};
        UString          _proxy_password {};
        UString          _user_agent {DEFAULT_USER_AGENT};
        bool             _auto_redirect = true;
        bool             _use_compression = false;
        bool             _https_insecure = false;
        bool             _use_cookies = true;
        bool             _temporary_cookies_file = false;
        fs::path         _cookies_file_name {};
        ByteBlock        _post_data {};
        UStringToUStringMultiMap _request_headers {};

        // Static variables containing the default proxy.
        static UString  _default_proxy_host;
        static uint16_t _default_proxy_port;
        static UString  _default_proxy_user;
        static UString  _default_proxy_assword;

        // A private singleton to initialize the default proxy from environment variables https_proxy and http_proxy.
        class DefaultProxy
        {
            TS_SINGLETON(DefaultProxy);
        public:
            const ts::URL url;
        };
    };
}
