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

#include "tsWebRequestArgs.h"
#include "tsArgs.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::WebRequestArgs::WebRequestArgs() :
    connectionTimeout(0),
    receiveTimeout(0),
    proxyPort(0),
    proxyHost(),
    proxyUser(),
    proxyPassword(),
    userAgent(),
    useCookies(true),
    cookiesFile()
{
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::WebRequestArgs::defineArgs(Args& args)
{
    args.option(u"connection-timeout", 0, Args::POSITIVE);
    args.help(u"connection-timeout",
              u"Specify the connection timeout in milliseconds. By default, let the "
              u"operating system decide.");

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

    args.option(u"receive-timeout", 0, Args::POSITIVE);
    args.help(u"receive-timeout",
              u"Specify the data reception timeout in milliseconds. This timeout applies "
              u"to each receive operation, individually. By default, let the operating "
              u"system decide.");

    args.option(u"user-agent", 0, Args::STRING);
    args.help(u"user-agent", u"'string'",
              u"Specify the user agent string to send in HTTP requests.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
// Args error indicator is set in case of incorrect arguments
//----------------------------------------------------------------------------

bool ts::WebRequestArgs::loadArgs(DuckContext& duck, Args& args)
{
    // Preserve previous timeout values
    args.getIntValue(connectionTimeout, u"connection-timeout", connectionTimeout);
    args.getIntValue(receiveTimeout, u"receive-timeout", receiveTimeout);
    args.getIntValue(proxyPort, u"proxy-port");
    args.getValue(proxyHost, u"proxy-host");
    args.getValue(proxyUser, u"proxy-user");
    args.getValue(proxyPassword, u"proxy-password");
    args.getValue(userAgent, u"user-agent");
    return true;
}
