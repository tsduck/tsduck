//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsRestArgs.h"
#include "tsEnvironment.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::RestArgs::RestArgs(const UString& description, const UString& prefix) :
    SuperClass(description, prefix),
    _opt_token(_prefix + u"token")
{
}

ts::RestArgs::~RestArgs()
{
}


//----------------------------------------------------------------------------
// Command line options for a TLS server.
//----------------------------------------------------------------------------

void ts::RestArgs::defineServerArgs(Args& args)
{
    SuperClass::defineServerArgs(args);

    args.option(_opt_token.c_str(), 0, Args::STRING);
    args.help(_opt_token.c_str(), u"string",
              u"Optional authentication token that clients are required to provide to the " + _description + u". "
              u"The default value is the value of environment variable TSDUCK_TOKEN. "
              u"For security reasons, use only with --" + _opt_tls + u".");
}

bool ts::RestArgs::loadServerArgs(Args& args, const UChar* server_option)
{
    args.getValue(auth_token, _opt_token.c_str(), GetEnvironment(u"TSDUCK_TOKEN").c_str());
    return SuperClass::loadServerArgs(args, server_option);
}


//----------------------------------------------------------------------------
// Command line options for a TLS client.
//----------------------------------------------------------------------------

void ts::RestArgs::defineClientArgs(Args& args)
{
    SuperClass::defineClientArgs(args);

    args.option(_opt_token.c_str(), 0, Args::STRING);
    args.help(_opt_token.c_str(), u"string",
              u"Authentication token for the " + _description + u", if required. "
              u"The default value is the value of environment variable TSDUCK_TOKEN.");

}

bool ts::RestArgs::loadClientArgs(Args& args, const UChar* server_option)
{
    args.getValue(auth_token, _opt_token.c_str(), GetEnvironment(u"TSDUCK_TOKEN").c_str());
    return SuperClass::loadClientArgs(args, server_option);
}
