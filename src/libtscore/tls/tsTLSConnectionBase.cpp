//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTLSConnectionBase.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::TLSConnectionBase::~TLSConnectionBase()
{
}


//----------------------------------------------------------------------------
// Set command line arguments for the client.
//----------------------------------------------------------------------------

void ts::TLSConnectionBase::setArgs(const TLSArgs& args)
{
    setServerName(args.server_name);
    _tls_verify_peer = !args.insecure;
}


//----------------------------------------------------------------------------
// For a client connection, specify the server names.
//----------------------------------------------------------------------------

void ts::TLSConnectionBase::setServerName(const UString& server_name)
{
    _tls_server_name = server_name;
    _tls_additional_names.clear();
    IPSocketAddress::RemovePort(_tls_server_name);
}

void ts::TLSConnectionBase::addVerifyServer(const UString& name)
{
    _tls_additional_names.push_back(name);
}
