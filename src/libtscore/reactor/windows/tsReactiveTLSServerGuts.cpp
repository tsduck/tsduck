//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveTLSServer.h"
#include "tsSChannelCertificate.h"


//----------------------------------------------------------------------------
// System-specific parts are stored in a private structure.
//----------------------------------------------------------------------------

class ts::ReactiveTLSServer::SystemGuts
{
    TS_NOBUILD_NOCOPY(SystemGuts);
public:
    SChannelCertificate cert;
    SystemGuts(ReactiveTLSServer* server) : cert(&server->reactor()) {}
};


//----------------------------------------------------------------------------
// System guts constructor and destructor.
//----------------------------------------------------------------------------

void ts::ReactiveTLSServer::allocateGuts()
{
    _guts = new SystemGuts(this);
}

void ts::ReactiveTLSServer::deleteGuts()
{
    delete _guts;
}


//----------------------------------------------------------------------------
// TLS initialization, called at the beginning of each startAccept().
//----------------------------------------------------------------------------

bool ts::ReactiveTLSServer::initTLS(ReactiveTLSConnection& client)
{
    // Get or create the TLS server certificate the first time listen is called.
    return _guts->cert.initServerCertificate(*this) && client.initServerContext(this, _guts->cert.getCertificate());
}
