//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsWebRequestArgs.h"
#include "tsArgs.h"


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
    args.getChronoValue(connectionTimeout, u"connection-timeout", connectionTimeout);
    args.getChronoValue(receiveTimeout, u"receive-timeout", receiveTimeout);
    args.getIntValue(proxyPort, u"proxy-port");
    args.getValue(proxyHost, u"proxy-host");
    args.getValue(proxyUser, u"proxy-user");
    args.getValue(proxyPassword, u"proxy-password");
    args.getValue(userAgent, u"user-agent");
    useCompression = args.present(u"compressed");

    UStringVector headerStrings;
    args.getValues(headerStrings, u"headers");
    for (const auto& headerString : headerStrings) {
        auto pos = headerString.find(':');
        if (pos == NPOS || pos == 0 || pos == headerString.size() - 1) {
            args.warning(u"Ignoring custom header '%s' - not of expected form 'x-header-name: value'", headerString);
        }
        else {
            auto headerKey = headerString.substr(0, pos);
            auto headerValue = headerString.substr(pos + 1);
            headerKey.trim();
            headerValue.trim();
            headers.insert(std::make_pair(headerKey, headerValue));
        }
    }
    return true;
}
