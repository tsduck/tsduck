//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsWebRequestArgs.h"
#include "tsEnvironment.h"
#include "tsFileUtils.h"
#include "tsNullReport.h"
#include "tsArgs.h"


//----------------------------------------------------------------------------
// Default proxy.
//----------------------------------------------------------------------------

ts::UString ts::WebRequestArgs::_default_proxy_host(DefaultProxy::Instance().url.getHost());
uint16_t    ts::WebRequestArgs::_default_proxy_port = DefaultProxy::Instance().url.getPort();
ts::UString ts::WebRequestArgs::_default_proxy_user(DefaultProxy::Instance().url.getUserName());
ts::UString ts::WebRequestArgs::_default_proxy_assword(DefaultProxy::Instance().url.getPassword());

TS_DEFINE_SINGLETON(ts::WebRequestArgs::DefaultProxy);

ts::WebRequestArgs::DefaultProxy::DefaultProxy() :
    url(ts::GetEnvironment(u"https_proxy", ts::GetEnvironment(u"http_proxy")))
{
}

void ts::WebRequestArgs::SetDefaultProxyHost(const UString& host, uint16_t port)
{
    _default_proxy_host = host;
    _default_proxy_port = port;
}

void ts::WebRequestArgs::SetDefaultProxyUser(const UString& user, const UString& password)
{
    _default_proxy_user = user;
    _default_proxy_assword = password;
}


//----------------------------------------------------------------------------
// Set/get proxy options.
//----------------------------------------------------------------------------

void ts::WebRequestArgs::setProxyHost(const UString& host, uint16_t port)
{
    _proxy_host = host;
    _proxy_port = port;
}

void ts::WebRequestArgs::setProxyUser(const UString& user, const UString& password)
{
    _proxy_user = user;
    _proxy_password = password;
}

const ts::UString& ts::WebRequestArgs::proxyHost() const
{
    return _proxy_host.empty() ? _default_proxy_host : _proxy_host;
}

uint16_t ts::WebRequestArgs::proxyPort() const
{
    return _proxy_port == 0 ? _default_proxy_port : _proxy_port;
}

const ts::UString& ts::WebRequestArgs::proxyUser() const
{
    return _proxy_user.empty() ? _default_proxy_user : _proxy_user;
}

const ts::UString& ts::WebRequestArgs::proxyPassword() const
{
    return _proxy_password.empty() ? _default_proxy_assword : _proxy_password;
}


//----------------------------------------------------------------------------
// Set global cookie management.
//----------------------------------------------------------------------------

void ts::WebRequestArgs::enableCookies(const fs::path& file_name)
{
    // Delete previous cookies file.
    deleteTemporaryCookiesFile(NULLREP);

    _use_cookies = true;
    _temporary_cookies_file = file_name.empty();
    _cookies_file_name = _temporary_cookies_file ? TempFile(u".cookies") : file_name;
}

void ts::WebRequestArgs::disableCookies()
{
    deleteTemporaryCookiesFile(NULLREP);
    _use_cookies = false;
}

bool ts::WebRequestArgs::deleteTemporaryCookiesFile(Report& report) const
{
    if (_temporary_cookies_file && !_cookies_file_name.empty() && fs::exists(_cookies_file_name)) {
        report.debug(u"deleting cookies file %s", _cookies_file_name);
        return fs::remove(_cookies_file_name, &ErrCodeReport(report, u"error deleting", _cookies_file_name));
    }
    else {
        // No cookies file to delete.
        return true;
    }
}


//----------------------------------------------------------------------------
// Set request headers.
//----------------------------------------------------------------------------

void ts::WebRequestArgs::setRequestHeader(const UString& name, const UString& value)
{
    // Check for duplicates on key AND value (multiple headers with the same key are permitted)
    for (const auto& header : _request_headers) {
        if (header.first == name && header.second == value) {
            return;
        }
    }
    _request_headers.insert(std::make_pair(name, value));
}


//----------------------------------------------------------------------------
// Append all request headers to a text string.
//----------------------------------------------------------------------------

void ts::WebRequestArgs::appendRequestHeaders(UString& text) const
{
    for (const auto& it : _request_headers) {
        if (!text.empty() && text.back() != u'\n') {
            text.append(u"\r\n");
        }
        text.append(it.first);
        text.append(u": ");
        text.append(it.second);
    }
}


//----------------------------------------------------------------------------
// Set POST data.
//----------------------------------------------------------------------------

void ts::WebRequestArgs::setPostData(const UString& data, const UString content_type)
{
    data.toUTF8(_post_data);
    if (!content_type.empty() && !_post_data.empty()) {
        deleteRequestHeader(u"Content-Type");
        setRequestHeader(u"Content-Type", content_type);
    }
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::WebRequestArgs::defineArgs(Args& args)
{
    args.option(u"compressed", 0);
    args.help(u"compressed",
              u"Accept compressed HTTP responses. By default, compressed responses are "
              u"not accepted.");

    args.option<cn::milliseconds>(u"connection-timeout");
    args.help(u"connection-timeout",
              u"Specify the connection timeout. "
              u"By default, let the operating system decide.");

    args.option(u"proxy-host", 0, Args::STRING);
    args.help(u"proxy-host", u"name",
              u"Optional proxy host name for Internet access.");

    args.option(u"proxy-password", 0, Args::STRING);
    args.help(u"proxy-password", u"string",
              u"Optional proxy password for Internet access (for use with --proxy-user).");

    args.option(u"proxy-port", 0, Args::UINT16);
    args.help(u"proxy-port",
              u"Optional proxy port for Internet access (for use with --proxy-host).");

    args.option(u"proxy-user", 0, Args::STRING);
    args.help(u"proxy-user", u"name",
              u"Optional proxy user name for Internet access.");

    args.option<cn::milliseconds>(u"receive-timeout");
    args.help(u"receive-timeout",
              u"Specify the data reception timeout. "
              u"This timeout applies to each receive operation, individually. "
              u"By default, let the operating system decide.");

    args.option(u"user-agent", 0, Args::STRING);
    args.help(u"user-agent", u"'string'",
              u"Specify the user agent string to send in HTTP requests.");

    args.option(u"headers", 0, Args::STRING, 0, ts::Args::UNLIMITED_COUNT);
    args.help(u"headers", u"'string'",
              u"Custom header, e.g. 'x-header-name: value'. Can be set multiple times.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::WebRequestArgs::loadArgs(Args& args)
{
    // Preserve previous timeout values
    args.getChronoValue(_connection_timeout, u"connection-timeout", _connection_timeout);
    args.getChronoValue(_receive_timeout, u"receive-timeout", _receive_timeout);
    args.getIntValue(_proxy_port, u"proxy-port");
    args.getValue(_proxy_host, u"proxy-host");
    args.getValue(_proxy_user, u"proxy-user");
    args.getValue(_proxy_password, u"proxy-password");
    args.getValue(_user_agent, u"user-agent");
    _use_compression = args.present(u"compressed");

    UStringVector headerStrings;
    args.getValues(headerStrings, u"headers");
    for (const auto& headerString : headerStrings) {
        auto pos = headerString.find(':');
        if (pos == NPOS || pos == 0 || pos == headerString.size() - 1) {
            args.warning(u"Ignoring custom header '%s' - not of expected form 'x-header-name: value'", headerString);
        }
        else {
            UString headerKey = headerString.substr(0, pos);
            UString headerValue = headerString.substr(pos + 1);
            headerKey.trim();
            headerValue.trim();
            _request_headers.insert(std::make_pair(headerKey, headerValue));
        }
    }
    return true;
}
