//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveTLSServer.h"
#include "tsOpenSSLCertificate.h"


//----------------------------------------------------------------------------
// Stubs when OpenSSL is not available.
//----------------------------------------------------------------------------

#if defined(TS_NO_OPENSSL)

#define TS_NOT_IMPL { report().error(TS_NO_OPENSSL_MESSAGE); return false; }

class ts::ReactiveTLSServer::SystemGuts {};
void ts::ReactiveTLSServer::allocateGuts() { _guts = new SystemGuts; }
void ts::ReactiveTLSServer::deleteGuts() { delete _guts; }
bool ts::ReactiveTLSServer::initTLS(ReactiveTLSConnection&) TS_NOT_IMPL

#else


//----------------------------------------------------------------------------
// System-specific parts are stored in a private structure.
//----------------------------------------------------------------------------

class ts::ReactiveTLSServer::SystemGuts
{
    TS_NOBUILD_NOCOPY(SystemGuts);
public:
    // Constructor and destructor.
    SystemGuts(ReactiveTLSServer* s) : server(s) {}

    ReactiveTLSServer* server;
    OpenSSLCertificate cert {&server->reactor()};
};


//----------------------------------------------------------------------------
// System-specific constructor and destructor.
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
    return false; //@@@
}

#endif // TS_NO_OPENSSL
