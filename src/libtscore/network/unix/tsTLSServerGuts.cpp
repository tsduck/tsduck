//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
// SSL/TLS server - UNIX specific parts with OpenSSL.
//
//----------------------------------------------------------------------------

#include "tsTLSServer.h"
#include "tsOpenSSLCertificate.h"


//----------------------------------------------------------------------------
// Stubs when OpenSSL is not available.
//----------------------------------------------------------------------------

#if defined(TS_NO_OPENSSL)

#define TS_NOT_IMPL { report().error(TS_NO_OPENSSL_MESSAGE); return false; }

class ts::TLSServer::SystemGuts {};
void ts::TLSServer::allocateGuts() { _guts = new SystemGuts; }
void ts::TLSServer::deleteGuts() { delete _guts; }
bool ts::TLSServer::listen(int) TS_NOT_IMPL
bool ts::TLSServer::acceptTLS(TLSConnection&, IPSocketAddress&, IOSB*) TS_NOT_IMPL
bool ts::TLSServer::closeImplementation(bool silent) TS_NOT_IMPL

#else

//----------------------------------------------------------------------------
// Normal OpenSSL support
// System-specific parts are stored in a private structure.
//----------------------------------------------------------------------------

class ts::TLSServer::SystemGuts
{
    TS_NOBUILD_NOCOPY(SystemGuts);
public:
    SystemGuts(TLSServer* s) : serv(s) {}

    TLSServer* serv;
    OpenSSLCertificate cert {serv};
};


//----------------------------------------------------------------------------
// System-specific constructor and destructor.
//----------------------------------------------------------------------------

void ts::TLSServer::allocateGuts()
{
    _guts = new SystemGuts(this);
}

void ts::TLSServer::deleteGuts()
{
    delete _guts;
}


//----------------------------------------------------------------------------
// Start the server
//----------------------------------------------------------------------------

bool ts::TLSServer::listen(int backlog)
{
    return _guts->cert.initServerCertificate(*this) && TCPServer::listen(backlog);
}


//----------------------------------------------------------------------------
// Wait for a TLS client.
//----------------------------------------------------------------------------

bool ts::TLSServer::acceptTLS(TLSConnection& client, IPSocketAddress& addr, IOSB* iosb)
{
    if (!_guts->cert.isValid()) {
        report().error(u"TLS server is not listening");
        return false;
    }

    // Accept one TCP client.
    if (!TCPServer::accept(client, addr, iosb)) {
        return false;
    }

    // Create an SSL context for that connection.
    SSL* ssl = SSL_new(_guts->cert.getCertificate());
    const UChar* error = nullptr;
    if (ssl == nullptr) {
        error = u"error creating TLS client context";
    }
    // Associate the TCP socket file descriptor with that SSL session.
    else if (SSL_set_fd(ssl, client.getSocket()) <= 0) {
        error = u"error setting file descriptor in TLS client context";
    }
    // Perform TLS handshake with the client.
    else if (SSL_accept(ssl) <= 0) {
        error = u"error in TLS handshake with new client";
    }

    // Error processing.
    if (error != nullptr) {
        report().error(error);
        OpenSSL::ReportErrors(report());
        client.close();
        if (ssl != nullptr) {
            SSL_free(ssl);
        }
        return false;
    }
    else {
        // The SSL context is passed to the TLSConnection object.
        report().debug(u"TLS connection established with %s, protocol: %s", addr, SSL_get_cipher_version(ssl));
        return client.setServerContext(ssl);
    }
}


//----------------------------------------------------------------------------
// Close the server resources.
//----------------------------------------------------------------------------

bool ts::TLSServer::closeImplementation(bool silent)
{
    _guts->cert.terminate();
    return TCPServer::closeImplementation(silent);
}

#endif // TS_NO_OPENSSL
