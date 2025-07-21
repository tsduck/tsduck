//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
// SSL/TLS connection - UNIX specific parts with OpenSSL.
//
//----------------------------------------------------------------------------

#include "tsTLSConnection.h"
#include "tsOpenSSL.h"
#include "tsNullReport.h"


//----------------------------------------------------------------------------
// Stubs when OpenSSL is not available.
//----------------------------------------------------------------------------

#if defined(TS_NO_OPENSSL)

#define TS_NOT_IMPL { report.error(TS_NO_OPENSSL_MESSAGE); return false; }

class ts::TLSConnection::SystemGuts {};
void ts::TLSConnection::allocateGuts() { _guts = new SystemGuts; }
void ts::TLSConnection::deleteGuts() { delete _guts; }
bool ts::TLSConnection::connect(const IPSocketAddress&, Report& report) TS_NOT_IMPL
bool ts::TLSConnection::closeWriter(Report& report) TS_NOT_IMPL
bool ts::TLSConnection::disconnect(Report& report) TS_NOT_IMPL
bool ts::TLSConnection::send(const void*, size_t, Report& report) TS_NOT_IMPL
bool ts::TLSConnection::receive(void*, size_t, size_t&, const AbortInterface*, Report& report) TS_NOT_IMPL
ts::UString ts::TLSConnection::GetLibraryVersion() { return UString(); }

#else

//----------------------------------------------------------------------------
// Normal OpenSSL support
//----------------------------------------------------------------------------

ts::UString ts::TLSConnection::GetLibraryVersion()
{
    return OpenSSL::Version();
}


//----------------------------------------------------------------------------
// System-specific parts are stored in a private structure.
//----------------------------------------------------------------------------

class ts::TLSConnection::SystemGuts: public OpenSSL::Controlled
{
    TS_NOCOPY(SystemGuts);
public:
    // Constructor and destructor.
    SystemGuts() = default;
    virtual ~SystemGuts() override;

    // Implementation of OpenSSL::Controlled.
    virtual void terminate() override;

    // OpenSSL parameters. SSL_shutdown() shall be called up to tso times,
    // until the two-way shutdown is complete.
    SSL_CTX* ssl_ctx = nullptr;
    SSL*     ssl = nullptr;
    size_t   shutdown_count = 2;

    // Process a SSL returned status. Return the SSL_get_error() code.
    int processStatus(Report& report, const UChar* func, int status);
};


//----------------------------------------------------------------------------
// System-specific constructor and destructor.
//----------------------------------------------------------------------------

void ts::TLSConnection::allocateGuts()
{
    _guts = new SystemGuts;
}

void ts::TLSConnection::deleteGuts()
{
    delete _guts;
}

ts::TLSConnection::SystemGuts::~SystemGuts()
{
    SystemGuts::terminate();
}

void ts::TLSConnection::SystemGuts::terminate()
{
    if (ssl != nullptr) {
        SSL_free(ssl);
        ssl = nullptr;
    }
    if (ssl_ctx != nullptr) {
        SSL_CTX_free(ssl_ctx);
        ssl_ctx = nullptr;
    }
    shutdown_count = 2;
}


//----------------------------------------------------------------------------
// Process a SSL returned status.
//----------------------------------------------------------------------------

int ts::TLSConnection::SystemGuts::processStatus(Report& report, const UChar* func, int status)
{
    const int err = SSL_get_error(ssl, status);
    report.debug(u"OpenSSL: %s returned %d, error: %d", func, status, err);
    return err;
}


//----------------------------------------------------------------------------
// Connect to a remote address and port.
//----------------------------------------------------------------------------

bool ts::TLSConnection::connect(const IPSocketAddress& addr, Report& report)
{
    _guts->terminate();

    // Perform a TCP connection.
    if (!SuperClass::connect(addr, report)) {
        return false;
    }

    // Create SSL client context.
    if ((_guts->ssl_ctx = OpenSSL::CreateContext(false, _verify_server, report)) == nullptr) {
        SuperClass::disconnect(NULLREP);
        return false;
    }

    // Create an SSL session for that connection.
    const UChar* error = nullptr;
    _guts->ssl = SSL_new(_guts->ssl_ctx);
    if (_guts->ssl == nullptr) {
        error = u"error creating TLS client connection context";
    }
    // Associate the TCP socket file descriptor with that SSL session.
    else if (SSL_set_fd(_guts->ssl, getSocket()) <= 0) {
        error = u"error setting file descriptor in TLS client context";
    }
    // Perform TLS handshake with the server.
    else if (SSL_connect(_guts->ssl) <= 0) {
        error = u"error in TLS handshake with server";
    }

    // Error processing.
    if (error != nullptr) {
        report.error(error);
        OpenSSL::ReportErrors(report);
        SuperClass::disconnect(NULLREP);
        _guts->terminate();
        return false;
    }
    else {
        report.debug(u"TLS connection established with %s, protocol: %s", addr, SSL_get_cipher_version(_guts->ssl));
        return true;
    }
}


//----------------------------------------------------------------------------
// Receive an SSL* context from a server, as a new client connection.
//----------------------------------------------------------------------------

void ts::TLSConnection::setServerContext(void* ssl)
{
    _guts->terminate();
    _guts->ssl = reinterpret_cast<SSL*>(ssl);
}


//----------------------------------------------------------------------------
// Close the write direction of the connection.
//----------------------------------------------------------------------------

bool ts::TLSConnection::closeWriter(Report& report)
{
    // Call SSL_shutdown() once, if the disconnection process is not started yet.
    bool success = true;
    if (_guts->shutdown_count >= 2) {
        const int status = SSL_shutdown(_guts->ssl);
        if (status > 0) {
            // Shutdown complete.
            _guts->shutdown_count = 0;
        }
        else if (status == 0) {
            // Shutdown ongoing, can call once more.
            _guts->shutdown_count = 1;
        }
        else {
            // Shutdown error.
            report.error(u"TLS shutdown error");
            OpenSSL::ReportErrors(report);
            success = false;
        }
    }

    // Call superclass in all cases.
    return SuperClass::closeWriter(report) && success;
}


//----------------------------------------------------------------------------
// Disconnect from remote partner.
//----------------------------------------------------------------------------

bool ts::TLSConnection::disconnect(Report& report)
{
    bool success = true;
    while (_guts->shutdown_count > 0) {
        const int ret = SSL_shutdown(_guts->ssl);
        report.debug(u"called SSL_shutdown, shutdown_count: %d, returned: %d", _guts->shutdown_count, ret);
        _guts->processStatus(report, u"SSL_shutdown", ret);
        if (ret > 0) {
            // Shutdown complete.
            _guts->shutdown_count = 0;
        }
        else if (ret == 0) {
            // Shutdown ongoing, can call once more.
            _guts->shutdown_count--;
        }
        else {
            // Shutdown error. Do not report errors, only debug.
            OpenSSL::ReportErrors(report, Severity::Debug);
            _guts->shutdown_count = 0;
            success = false;
        }
    }

    // Call superclass in all cases.
    return SuperClass::disconnect(report) && success;
}


//----------------------------------------------------------------------------
// Send data.
//----------------------------------------------------------------------------

bool ts::TLSConnection::send(const void* data, size_t size, Report& report)
{
    if (_guts->ssl == nullptr) {
        report.error(u"TLS connection not established");
        return false;
    }
    if (size == 0) {
        // Writing zero-length TLS data creates issue.
        return true;
    }
    const int ret = SSL_write(_guts->ssl, data, int(size));
    if (ret <= 0) {
        _guts->processStatus(report, u"SSL_write", ret);
        report.error(u"TLS send error");
        OpenSSL::ReportErrors(report);
        return false;
    }
    else {
        return true;
    }
}


//----------------------------------------------------------------------------
// Receive data.
//----------------------------------------------------------------------------

bool ts::TLSConnection::receive(void* buffer, size_t max_size, size_t& ret_size, const AbortInterface* abort, Report& report)
{
    if (_guts->ssl == nullptr) {
        report.error(u"TLS connection not established");
        return false;
    }

    const int ret = SSL_read_ex(_guts->ssl, buffer, max_size, &ret_size);
    if (ret > 0) {
        // Received something.
        return true;
    }
    else if (_guts->processStatus(report, u"SSL_read_ex", ret) == SSL_ERROR_ZERO_RETURN) {
        // End of connection, return false but no error.
        ERR_clear_error();
        ret_size = 0;
        return false;
    }
    else {
        report.error(u"TLS receive error");
        OpenSSL::ReportErrors(report);
        return false;
    }
}

#endif // TS_NO_OPENSSL
