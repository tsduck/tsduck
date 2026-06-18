//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
// SSL/TLS connection - Windows specific parts with SChannel.
//
//----------------------------------------------------------------------------

#include "tsTLSConnection.h"
#include "tsSChannelContext.h"
#include "tsWinModuleInfo.h"
#include "tsNullReport.h"
#include "tsMemory.h"
#include "tsNames.h"


//----------------------------------------------------------------------------
// Library version.
//----------------------------------------------------------------------------

ts::UString ts::TLSConnection::GetLibraryVersion()
{
    return WinModuleInfo(u"schannel.dll").summary();
}


//----------------------------------------------------------------------------
// System-specific parts are stored in a private structure.
//----------------------------------------------------------------------------

class ts::TLSConnection::SystemGuts
{
    TS_NOBUILD_NOCOPY(SystemGuts);
public:
    TLSConnection&  conn;         // Parent object.
    SChannelContext sctx;         // TLS context.
    ByteBlock       incoming {};  // Incoming clear data.
    size_t          in_next = 0;  // Next index to return from incoming.

    // Constructor and destructor.
    SystemGuts(TLSConnection& c) : conn(c), sctx(&c, c) {}

    // Perform all required send and receive operation for TLS protocol.
    bool flushSession();
};


//----------------------------------------------------------------------------
// SystemGuts allocation.
//----------------------------------------------------------------------------

void ts::TLSConnection::allocateGuts()
{
    _guts = new SystemGuts(*this);
}

void ts::TLSConnection::deleteGuts()
{
    delete _guts;
}


//----------------------------------------------------------------------------
// Perform all required send and receive operation for TLS protocol.
//----------------------------------------------------------------------------

bool ts::TLSConnection::SystemGuts::flushSession()
{
    bool success = true;
    while (success && (sctx.needSend() || sctx.needReceive())) {

        // First, send all available outgoing data.
        while (success && sctx.needSend()) {
            success = conn.TCPConnection::send(sctx.sendAddress(), sctx.sendSize()) && sctx.sendCompleted();
        }

        // Then, receive more input.
        if (success && sctx.needReceive()) {
            size_t retsize = 0;
            success = conn.TCPConnection::receive(sctx.receiveAddress(), sctx.receiveSize(), retsize) && sctx.receiveCompleted(retsize, incoming);
        }
    }
    return success;
}


//----------------------------------------------------------------------------
// Pass information from server accepting new clients.
//----------------------------------------------------------------------------

bool ts::TLSConnection::setServerContext(const void* vcred)
{
    // Check that the application uses the right blocking mode.
    if (!checkBlocking()) {
        return false;
    }

    // Initialize input stream.
    _guts->incoming.clear();
    _guts->in_next = 0;

    // Perform server-side TLS handshake.
    const bool success = _guts->sctx.initServer(reinterpret_cast<::PCCERT_CONTEXT>(vcred)) && _guts->flushSession();
    if (!success) {
        _guts->sctx.reset();
        TCPConnection::disconnect(true);
    }
    return success;
}


//----------------------------------------------------------------------------
// Connect a client to a remote server address and port.
//----------------------------------------------------------------------------

bool ts::TLSConnection::connect(const IPSocketAddress& addr, IOSB* iosb)
{
    // Check that the application uses the right blocking mode.
    // Perform a TCP connection.
    if (!checkBlocking() || !TCPConnection::connect(addr, iosb)) {
        return false;
    }

    // Initialize input stream.
    _guts->incoming.clear();
    _guts->in_next = 0;

    // Perform client-side TLS handshake.
    const bool success = _guts->sctx.initClient() && _guts->flushSession();
    if (!success) {
        _guts->sctx.reset();
        TCPConnection::disconnect(true);
    }
    return success;
}


//----------------------------------------------------------------------------
// Close the write direction of the connection.
//----------------------------------------------------------------------------

bool ts::TLSConnection::closeWriter(bool silent)
{
    if (!isConnected()) {
        report().error(u"not connected");
        return false;
    }

    // Generate shutdown message and send outgoing data.
    return _guts->sctx.initShutdown() && _guts->flushSession();
}


//----------------------------------------------------------------------------
// Disconnect from remote partner.
//----------------------------------------------------------------------------

bool ts::TLSConnection::disconnect(bool silent)
{
    // Send the shutdown message (if not already done).
    bool success = closeWriter(silent);

    // Cleanup SChannel resources.
    _guts->sctx.reset();

    // Shutdown the socket, regardless of SChannel success.
    return TCPConnection::disconnect(silent) && success;
}


//----------------------------------------------------------------------------
// Send data.
//----------------------------------------------------------------------------

bool ts::TLSConnection::send(const void* data, size_t size, IOSB* iosb)
{
    // Check that the application uses the right blocking mode.
    if (!checkBlocking()) {
        return false;
    }
    if (!isConnected()) {
        report().error(u"not connected");
        return false;
    }

    // Generate and send chunks of output data.
    bool success = true;
    while (success && size > 0) {
        success = _guts->sctx.sendUserData(data, size) && _guts->flushSession();
    }
    return success;
}


//----------------------------------------------------------------------------
// Receive data.
//----------------------------------------------------------------------------

bool ts::TLSConnection::receive(void* buffer, size_t max_size, size_t& ret_size, const AbortInterface* abort, IOSB* iosb)
{
    ret_size = 0;

    // Check that the application uses the right blocking mode.
    if (!checkBlocking()) {
        return false;
    }
    if (!isConnected()) {
        report().error(u"not connected");
        return false;
    }

    // Loop until we have something to return.
    while (ret_size == 0) {
        if (_guts->in_next < _guts->incoming.size()) {
            // There are some data in the internal buffer. Read them.
            ret_size = std::min(max_size, _guts->incoming.size() - _guts->in_next);
            MemCopy(buffer, &_guts->incoming[_guts->in_next], ret_size);
            _guts->in_next += ret_size;
            if (_guts->in_next >= _guts->incoming.size()) {
                _guts->incoming.clear();
                _guts->in_next = 0;
            }
        }
        else {
            // Need to read from the network.
            if (_guts->sctx.receiveSize() == 0) {
                report().error(u"TLS error, input buffer full, no valid TLS packet found");
                return false;
            }

            // Read TLS protocol data from the peer.
            size_t received = 0;
            if (!TCPConnection::receive(_guts->sctx.receiveAddress(), _guts->sctx.receiveSize(), received, abort) ||
                !_guts->sctx.receiveCompleted(received, _guts->incoming) ||
                !_guts->flushSession())
            {
                return false;
            }
        }
    }
    return true;
}
