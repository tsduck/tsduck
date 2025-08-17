//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
// SSL/TLS server - Windows specific parts with SChannel.
//
//----------------------------------------------------------------------------

#include "tsTLSServer.h"
#include "tsWinTLS.h"


//----------------------------------------------------------------------------
// System-specific parts are stored in a private structure.
//----------------------------------------------------------------------------

class ts::TLSServer::SystemGuts
{
    TS_NOCOPY(SystemGuts);
public:
    ::PCCERT_CONTEXT cert = nullptr;

    // Constructor and destructor.
    SystemGuts() = default;
    ~SystemGuts();
    void clear();
};


//----------------------------------------------------------------------------
// System-specific constructor and destructor.
//----------------------------------------------------------------------------

void ts::TLSServer::allocateGuts()
{
    _guts = new SystemGuts;
}

void ts::TLSServer::deleteGuts()
{
    delete _guts;
}

void ts::TLSServer::SystemGuts::clear()
{
    if (cert != nullptr) {
        ::CertFreeCertificateContext(cert);
        cert = nullptr;
    }
}

ts::TLSServer::SystemGuts::~SystemGuts()
{
    clear();
}


//----------------------------------------------------------------------------
// Start the server
//----------------------------------------------------------------------------

bool ts::TLSServer::listen(int backlog, Report& report)
{
    // Get TLS server certificate the first time listen is called.
    if (_guts->cert == nullptr && (_guts->cert = GetCertificate(getCertificateStore(), getCertificatePath(), report)) == nullptr) {
        return false;
    }

    // Create the TCP server.
    return SuperClass::listen(backlog, report);
}


//----------------------------------------------------------------------------
// Wait for a TLS client.
//----------------------------------------------------------------------------

bool ts::TLSServer::acceptTLS(TLSConnection& client, IPSocketAddress& addr, Report& report)
{
    // Accept one TCP client.
    if (!SuperClass::accept(client, addr, report)) {
        return false;
    }

    // Perform handshake with the client.
    if (!client.setServerContext(_guts->cert, report)) {
        // Close the underlying TCP socket.
        client.SuperClass::close(report);
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Close the server resources.
//----------------------------------------------------------------------------

bool ts::TLSServer::close(Report& report)
{
    _guts->clear();
    return SuperClass::close(report);
}
