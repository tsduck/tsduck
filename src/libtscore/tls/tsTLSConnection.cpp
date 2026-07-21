//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTLSConnection.h"
#include "tsIPProtocols.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::TLSConnection::TLSConnection(Report* report) :
    TCPConnection(report, false)
{
}

ts::TLSConnection::TLSConnection(ReporterBase* delegate) :
    TCPConnection(delegate, false)
{
}


//----------------------------------------------------------------------------
// TLSConnection must use blocking I/O.
//----------------------------------------------------------------------------

bool ts::TLSConnection::checkBlocking()
{
    if (isNonBlocking()) {
        report().error(u"internal error: TLSConnection called in non-blocking mode, use ReactiveTLSConnection");
        return false;
    }
    else {
        return true;
    }
}


//----------------------------------------------------------------------------
// Process some incoming TLS data, wait for network data if necessary.
//----------------------------------------------------------------------------

bool ts::TLSConnection::flushInput()
{
    size_t ret_size = 0;

    // Receive more data from the network if none is buffered.
    if (_tls_data_next >= _tls_data.size()) {
        if (_tls_data.size() < TLS_MAX_PACKET_SIZE) {
            _tls_data.resize(TLS_MAX_PACKET_SIZE);
        }
        _tls_data_next = 0;
        if (!TCPConnection::readStream(_tls_data.data(), _tls_data.size(), ret_size)) {
            return false;
        }
        _tls_data.resize(ret_size);
    }

    // Provide received TLS data to TLS context and collect incoming clear data.
    const bool success = _sctx.provideReceivedData(_tls_data.data() + _tls_data_next, _tls_data.size() - _tls_data_next, ret_size, _clear_data);
    _tls_data_next += ret_size;
    return success;
}


//----------------------------------------------------------------------------
// Perform all required send and receive operation for TLS protocol.
//----------------------------------------------------------------------------

bool ts::TLSConnection::flushSession()
{
    bool success = true;
    while (success && (_sctx.needSend() || _sctx.needReceive())) {

        // First, send all available outgoing data.
        while (success && _sctx.needSend()) {
            ByteBlock data;
            _sctx.getDataToSend(data);
            success = TCPConnection::writeStream(data.data(), data.size());
        }

        // Then, receive and process more input.
        if (success && _sctx.needReceive()) {
            success = flushInput();
        }
    }
    return success;
}


//----------------------------------------------------------------------------
// Pass information from server accepting new clients.
//----------------------------------------------------------------------------

bool ts::TLSConnection::setServerContext(void* param)
{
    // Check that the application uses the right blocking mode.
    if (!checkBlocking()) {
        return false;
    }

    // Initialize input stream.
    _tls_data.clear();
    _clear_data.clear();
    _tls_data_next = _clear_data_next = 0;

    // Perform server-side TLS handshake.
    const bool success = _sctx.initServer(param) && flushSession();
    if (!success) {
        _sctx.reset();
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
    _tls_data.clear();
    _clear_data.clear();
    _tls_data_next = _clear_data_next = 0;

    // Perform client-side TLS handshake.
    const bool success = _sctx.initClient(*this) && flushSession();
    if (!success) {
        _sctx.reset();
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
        report().log(SilentLevel(silent), u"not connected");
        return false;
    }

    // Generate shutdown message and send outgoing data.
    return _sctx.initShutdown(silent) && flushSession() && TCPConnection::closeWriter(silent);
}


//----------------------------------------------------------------------------
// Disconnect from remote partner.
//----------------------------------------------------------------------------

bool ts::TLSConnection::disconnect(bool silent)
{
    // Send the shutdown message (if not already done).
    bool success = !isConnected() || closeWriter(silent);

    // Cleanup TLS resources.
    _sctx.reset();

    // Shutdown the socket, regardless of TLS context success.
    return TCPConnection::disconnect(silent) && success;
}


//----------------------------------------------------------------------------
// Send data. Implementation of StreamInterface.
//----------------------------------------------------------------------------

bool ts::TLSConnection::writeStream(const void* buffer, size_t data_size, IOSB* iosb)
{
    return WriteStreamHelper<TLSConnection>(this, buffer, data_size, iosb);
}

bool ts::TLSConnection::writeStream(const void* data, size_t size, size_t& written_size, IOSB* iosb)
{
    written_size = 0;

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
        size_t ret_size = 0;
        success = _sctx.provideClearData(data, size, ret_size) && flushSession();
        assert(ret_size <= size);
        data = reinterpret_cast<const uint8_t*>(data) + ret_size;
        written_size += ret_size;
        size -= ret_size;
    }
    return success;
}


//----------------------------------------------------------------------------
// Receive data. Implementation of StreamInterface.
//----------------------------------------------------------------------------

bool ts::TLSConnection::readStream(void* buffer, size_t size, const AbortInterface* abort)
{
    return ReadStreamHelper<TLSConnection>(this, buffer, size, abort);
}

bool ts::TLSConnection::readStream(void* buffer, size_t max_size, size_t& ret_size, const AbortInterface* abort, IOSB* iosb)
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
        // If there are some clear data in the internal buffer, return them.
        if (_clear_data_next < _clear_data.size()) {
            ret_size = std::min(max_size, _clear_data.size() - _clear_data_next);
            MemCopy(buffer, &_clear_data[_clear_data_next], ret_size);
            _clear_data_next += ret_size;
            if (_clear_data_next >= _clear_data.size()) {
                // All buffered clear data are now returned.
                _clear_data.clear();
                _clear_data_next = 0;
            }
        }
        // Otherwise, read from the network.
        else if (!flushInput() || !flushSession()) {
            return false;
        }
    }
    return true;
}
