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

// Some OpenSSL macros use C-style casts and we need to disable warnings.
TS_LLVM_NOWARNING(old-style-cast)
TS_GCC_NOWARNING(old-style-cast)


//----------------------------------------------------------------------------
// Stubs when OpenSSL is not available.
//----------------------------------------------------------------------------

#if defined(TS_NO_OPENSSL)

#define TS_NOT_IMPL { report.error(TS_NO_OPENSSL_MESSAGE); return false; }

class ts::TLSConnection::SystemGuts {};
void ts::TLSConnection::allocateGuts() { _guts = new SystemGuts; }
void ts::TLSConnection::deleteGuts() { delete _guts; }
bool ts::TLSConnection::connect(const IPSocketAddress&, Report& report) TS_NOT_IMPL
bool ts::TLSConnection::setServerContext(const void*, Report& report) TS_NOT_IMPL
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
    TS_NOBUILD_NOCOPY(SystemGuts);
public:
    // Constructor and destructor.
    SystemGuts(TLSConnection*);
    virtual ~SystemGuts() override;

    // Implementation of OpenSSL::Controlled.
    virtual void terminate() override;

    // OpenSSL parameters. SSL_shutdown() shall be called up to tso times,
    // until the two-way shutdown is complete.
    TLSConnection* conn;
    SSL_CTX*       ssl_ctx = nullptr;
    SSL*           ssl = nullptr;
    size_t         shutdown_count = 2;

    // Process a SSL returned status. Return the SSL_get_error() code.
    int processStatus(Report& report, const UChar* func, int status);

    // Abort a connection, closes everything, return false.
    bool abort(Report& report, const UString& error_message = UString());
};


//----------------------------------------------------------------------------
// System-specific constructor and destructor.
//----------------------------------------------------------------------------

void ts::TLSConnection::allocateGuts()
{
    _guts = new SystemGuts(this);
}

void ts::TLSConnection::deleteGuts()
{
    delete _guts;
}

ts::TLSConnection::SystemGuts::SystemGuts(TLSConnection* c) :
    conn(c)
{
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
// Abort a connection, closes everything, return false.
//----------------------------------------------------------------------------

bool ts::TLSConnection::SystemGuts::abort(Report& report, const UString& message)
{
    if (!message.empty()) {
        report.error(message);
    }
    OpenSSL::ReportErrors(report);
    terminate();
    conn->SuperClass::disconnect(NULLREP);
    return false;
}


//----------------------------------------------------------------------------
// Connect to a remote address and port.
//----------------------------------------------------------------------------

bool ts::TLSConnection::connect(const IPSocketAddress& addr, Report& report)
{
    _guts->terminate();

    // Create SSL client context.
    if ((_guts->ssl_ctx = OpenSSL::CreateContext(false, _verify_peer, report)) == nullptr) {
        return false;
    }

    // Create an SSL session for that connection.
    if ((_guts->ssl = ::SSL_new(_guts->ssl_ctx)) == nullptr) {
        return _guts->abort(report, u"error creating TLS client connection context");
    }

    // Set host name for SNI.
    if (!_server_name.empty() && !::SSL_set_tlsext_host_name(_guts->ssl, _server_name.toUTF8().c_str())) {
        return _guts->abort(report, u"error setting TLS SNI server name (SSL_set_tlsext_host_name)");
    }

    // Set DNS names for verification of the server's certificate.
    if (_verify_peer && !_server_name.empty()) {
        // Set main server name.
        if (!::SSL_set1_host(_guts->ssl, _server_name.toUTF8().c_str())) {
            return _guts->abort(report, u"error setting TLS server name (SSL_set1_host)");
        }
        // Set additional names.
        for (const auto& name : _additional_names) {
            if (!name.empty() && !::SSL_add1_host(_guts->ssl, name.toUTF8().c_str())) {
                return _guts->abort(report, u"error setting TLS additional server name (SSL_add1_host)");
            }
        }
    }

    // Perform a TCP connection.
    if (!SuperClass::connect(addr, report)) {
        return _guts->abort(report);
    }

    // Associate the TCP socket file descriptor with that SSL session.
    if (::SSL_set_fd(_guts->ssl, getSocket()) <= 0) {
        return _guts->abort(report, u"error setting file descriptor in TLS client context");
    }

    // Perform TLS handshake with the server.
    if (::SSL_connect(_guts->ssl) <= 0) {
        return _guts->abort(report, u"error in TLS handshake with server");
    }

    report.debug(u"TLS connection established with %s, protocol: %s", addr, ::SSL_get_cipher_version(_guts->ssl));
    return true;
}


//----------------------------------------------------------------------------
// Receive an SSL* context from a server, as a new client connection.
//----------------------------------------------------------------------------

bool ts::TLSConnection::setServerContext(const void* ssl, Report& report)
{
    _guts->terminate();
    _guts->ssl = const_cast<SSL*>(reinterpret_cast<const SSL*>(ssl));
    return true;
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
