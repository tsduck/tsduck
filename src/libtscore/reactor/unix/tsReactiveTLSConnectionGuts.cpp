//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveTLSConnection.h"
#include "tsOpenSSL.h"

// Some OpenSSL macros use C-style casts and we need to disable warnings.
TS_LLVM_NOWARNING(old-style-cast)
TS_GCC_NOWARNING(old-style-cast)


//----------------------------------------------------------------------------
// Stubs when OpenSSL is not available.
//----------------------------------------------------------------------------

#if defined(TS_NO_OPENSSL)

#define TS_NOT_IMPL { report().error(TS_NO_OPENSSL_MESSAGE); return false; }

class ts::ReactiveTLSConnection::SystemGuts {};
void ts::ReactiveTLSConnection::allocateGuts() { _guts = new SystemGuts; }
void ts::ReactiveTLSConnection::deleteGuts() { delete _guts; }
void ts::ReactiveTLSConnection::whenAccepted(ReactiveTCPConnectionHandlerInterface*, const ObjectPtr&) {}
bool ts::ReactiveTLSConnection::startConnect(ReactiveTCPConnectionHandlerInterface*, const IPSocketAddress&, const ObjectPtr&) TS_NOT_IMPL
bool ts::ReactiveTLSConnection::startSend(ReactiveTCPConnectionHandlerInterface*, const void*, size_t, const ObjectPtr&) TS_NOT_IMPL
bool ts::ReactiveTLSConnection::startCloseWriter(ReactiveTCPConnectionHandlerInterface*, bool, const ObjectPtr&) TS_NOT_IMPL
bool ts::ReactiveTLSConnection::startReceive(ReactiveTCPConnectionHandlerInterface*, size_t, const ObjectPtr&) TS_NOT_IMPL
void ts::ReactiveTLSConnection::cancelSendReceive(bool) {}
bool ts::ReactiveTLSConnection::startClose(ReactiveTCPConnectionHandlerInterface*, bool, const ObjectPtr&) TS_NOT_IMPL

#else


//----------------------------------------------------------------------------
// System-specific parts are stored in a private structure.
//----------------------------------------------------------------------------

class ts::ReactiveTLSConnection::SystemGuts: public OpenSSL::Controlled
{
    TS_NOBUILD_NOCOPY(SystemGuts);
public:
    // Constructor and destructor.
    SystemGuts(ReactiveTLSConnection* c) : conn(c) {}
    virtual ~SystemGuts() override;

    // Implementation of OpenSSL::Controlled.
    virtual void terminate() override;

    // OpenSSL parameters. SSL_shutdown() shall be called up to two times, until the two-way shutdown is complete.
    ReactiveTLSConnection* conn;
    SSL_CTX* ssl_ctx = nullptr;
    SSL*     ssl = nullptr;
    size_t   shutdown_count = 2;

    // Process a SSL returned status. Return the SSL_get_error() code.
    int processStatus(const UChar* func, int status);

    // Abort a connection, closes everything, return false.
    bool abort(const UString& error_message = UString());
};


//----------------------------------------------------------------------------
// System-specific constructor and destructor.
//----------------------------------------------------------------------------

void ts::ReactiveTLSConnection::allocateGuts()
{
    _guts = new SystemGuts(this);
}

void ts::ReactiveTLSConnection::deleteGuts()
{
    delete _guts;
}

ts::ReactiveTLSConnection::SystemGuts::~SystemGuts()
{
    SystemGuts::terminate();
}

void ts::ReactiveTLSConnection::SystemGuts::terminate()
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
// Handler to call when accepted as a client session by a server.
//----------------------------------------------------------------------------

void ts::ReactiveTLSConnection::whenAccepted(ReactiveTCPConnectionHandlerInterface* handler, const ObjectPtr& user_data)
{
    //@@@
}


//----------------------------------------------------------------------------
// Start the operation of connecting to a TLS server.
//----------------------------------------------------------------------------

bool ts::ReactiveTLSConnection::startConnect(ReactiveTCPConnectionHandlerInterface* handler, const IPSocketAddress& addr, const ObjectPtr& user_data)
{
    return false; //@@@
}


//----------------------------------------------------------------------------
// Start the operation of sending data over the TCP connection.
//----------------------------------------------------------------------------

bool ts::ReactiveTLSConnection::startSend(ReactiveTCPConnectionHandlerInterface* handler, const void* data, size_t size, const ObjectPtr& user_data)
{
    return false; //@@@
}


//----------------------------------------------------------------------------
// Start closing the send direction of the socket.
//----------------------------------------------------------------------------

bool ts::ReactiveTLSConnection::startCloseWriter(ReactiveTCPConnectionHandlerInterface* handler, bool silent, const ObjectPtr& user_data)
{
    return false; //@@@
}


//----------------------------------------------------------------------------
// Start the operation of receiving messages from the socket.
//----------------------------------------------------------------------------

bool ts::ReactiveTLSConnection::startReceive(ReactiveTCPConnectionHandlerInterface* handler, size_t buffer_size, const ObjectPtr& user_data)
{
    return false; //@@@
}


//----------------------------------------------------------------------------
// Cancel any pending send or receive operation on this socket.
//----------------------------------------------------------------------------

void ts::ReactiveTLSConnection::cancelSendReceive(bool silent)
{
    //@@@
}


//----------------------------------------------------------------------------
// Start closing the send direction of the socket.
//----------------------------------------------------------------------------

bool ts::ReactiveTLSConnection::startClose(ReactiveTCPConnectionHandlerInterface* handler, bool silent, const ObjectPtr& user_data)
{
    return false; //@@@
}

#endif // TS_NO_OPENSSL
