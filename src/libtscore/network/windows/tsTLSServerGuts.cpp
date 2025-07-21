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
    // Constructor and destructor.
    SystemGuts() = default;
    ~SystemGuts();
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

ts::TLSServer::SystemGuts::~SystemGuts()
{
}


//----------------------------------------------------------------------------
// Start the server
//----------------------------------------------------------------------------

bool ts::TLSServer::listen(int backlog, Report& report)
{
    // We need a TLS server certificate.
    ::PCCERT_CONTEXT cert = GetCertificate(getCertificateStore(), getCertificatePath(), report);
    if (cert == nullptr) {
        return false;
    }

    // Acquire credentials.
    ::CredHandle cred;
    if (!GetCredentials(cred, true, false, cert, report)) {
        ::CertFreeCertificateContext(cert);
        return false;
    }

    //@@@@ TO BE CONTINUED
    ::FreeCredentialsHandle(&cred);
    ::CertFreeCertificateContext(cert);
    report.error(u"not yet implemented");
    return false;

    // Create the TCP server.
    //@@@@ return SuperClass::listen(backlog, report);
}


//----------------------------------------------------------------------------
// Wait for a TLS client.
//----------------------------------------------------------------------------

bool ts::TLSServer::acceptTLS(TLSConnection& client, IPSocketAddress& addr, Report& report)
{
    // Accept one TCP client.
    //@@@@ if (!SuperClass::accept(client, addr, report)) {
    //@@@@     return false;
    //@@@@ }

    //@@@@ TO BE CONTINUED
    report.error(u"not yet implemented");
    return false;
}


//----------------------------------------------------------------------------
// Close the server resources.
//----------------------------------------------------------------------------

bool ts::TLSServer::close(Report& report)
{
    //@@@@ TO BE CONTINUED
    report.error(u"not yet implemented");

    return SuperClass::close(report);
}
