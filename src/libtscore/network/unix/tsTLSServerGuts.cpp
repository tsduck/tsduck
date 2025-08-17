//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
// SSL/TLS server - UNIX specific parts with OpenSSL.
//
//----------------------------------------------------------------------------

#include "tsTLSServer.h"
#include "tsOpenSSL.h"
#include "tsNullReport.h"


//----------------------------------------------------------------------------
// Stubs when OpenSSL is not available.
//----------------------------------------------------------------------------

#if defined(TS_NO_OPENSSL)

#define TS_NOT_IMPL { report.error(TS_NO_OPENSSL_MESSAGE); return false; }

class ts::TLSServer::SystemGuts {};
void ts::TLSServer::allocateGuts() { _guts = new SystemGuts; }
void ts::TLSServer::deleteGuts() { delete _guts; }
bool ts::TLSServer::listen(int, Report& report) TS_NOT_IMPL
bool ts::TLSServer::acceptTLS(TLSConnection&, IPSocketAddress&, Report& report) TS_NOT_IMPL
bool ts::TLSServer::close(Report& report) TS_NOT_IMPL

#else

//----------------------------------------------------------------------------
// Normal OpenSSL support
// System-specific parts are stored in a private structure.
//----------------------------------------------------------------------------

class ts::TLSServer::SystemGuts: public OpenSSL::Controlled
{
    TS_NOCOPY(SystemGuts);
public:
    // Constructor and destructor.
    SystemGuts() = default;
    virtual ~SystemGuts() override;

    // Implementation of OpenSSL::Controlled.
    virtual void terminate() override;

    // OpenSSL parameters.
    SSL_CTX* ssl_ctx = nullptr;
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
    SystemGuts::terminate();
}

void ts::TLSServer::SystemGuts::terminate()
{
    if (ssl_ctx != nullptr) {
        SSL_CTX_free(ssl_ctx);
        ssl_ctx = nullptr;
    }
}


//----------------------------------------------------------------------------
// Start the server
//----------------------------------------------------------------------------

bool ts::TLSServer::listen(int backlog, Report& report)
{
    // We need a certificate and a key.
    const UString certificate_path(getCertificatePath());
    const UString key_path(getKeyPath());
    if (certificate_path.empty()) {
        report.error(u"no certificate set in TLS server");
        return false;
    }
    if (key_path.empty()) {
        report.error(u"no private key set in TLS server");
        return false;
    }

    // Cleanup previous SSL context, if any.
    if (_guts->ssl_ctx != nullptr) {
        SSL_CTX_free(_guts->ssl_ctx);
    }

    // Create SSL server context.
    if ((_guts->ssl_ctx = OpenSSL::CreateContext(true, false, report)) == nullptr) {
        report.error(u"error creating TLS server context");
        OpenSSL::ReportErrors(report);
        return false;
    }

    // Load certificate file (public key).
    if (SSL_CTX_use_certificate_file(_guts->ssl_ctx, certificate_path.toUTF8().c_str(), SSL_FILETYPE_PEM) <= 0) {
        report.error(u"error loading TLS certificate file %s", certificate_path);
        OpenSSL::ReportErrors(report);
        return false;
    }

    // Load private key file.
    if (SSL_CTX_use_PrivateKey_file(_guts->ssl_ctx, key_path.toUTF8().c_str(), SSL_FILETYPE_PEM) <= 0) {
        report.error(u"error loading TLS private key file %s", key_path);
        OpenSSL::ReportErrors(report);
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
    const UChar* error = nullptr;

    if (_guts->ssl_ctx == nullptr) {
        report.error(u"TLS server is not listening");
        return false;
    }

    // Accept one TCP client.
    if (!SuperClass::accept(client, addr, report)) {
        return false;
    }

    // Create an SSL context for that connection.
    SSL* ssl = SSL_new(_guts->ssl_ctx);
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
        report.error(error);
        OpenSSL::ReportErrors(report);
        client.close(NULLREP);
        if (ssl != nullptr) {
            SSL_free(ssl);
        }
        return false;
    }
    else {
        // The SSL context is passed to the TLSConnection object.
        report.debug(u"TLS connection established with %s, protocol: %s", addr, SSL_get_cipher_version(ssl));
        return client.setServerContext(ssl, report);
    }
}


//----------------------------------------------------------------------------
// Close the server resources.
//----------------------------------------------------------------------------

bool ts::TLSServer::close(Report& report)
{
    _guts->terminate();
    return SuperClass::close(report);
}

#endif // TS_NO_OPENSSL
