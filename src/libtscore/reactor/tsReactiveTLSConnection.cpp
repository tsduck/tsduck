//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveTLSConnection.h"
#include "tsReactiveTLSServer.h"
#include "tsTLSConnection.h"
#include "tsIPProtocols.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::ReactiveTLSConnection::ReactiveTLSConnection(Reactor& reactor, TCPConnection& socket, Object* owner) :
    ReactiveTCPConnection(reactor, socket, owner)
{
    // The socket must be an instance of TCPConnection, not an instance of TLSConnection.
    // Detect and report trivial misusages.
    if (dynamic_cast<TLSConnection*>(&socket) != nullptr) {
        report().fatal(u"internal error: ReactiveTLSConnection needs a TCPConnection, not a TLSConnection");
    }
}

ts::ReactiveTLSConnection::ReactiveTLSConnection(Reactor& reactor, TCPConnection& socket, const TLSArgs& args, Object* owner) :
    ReactiveTLSConnection(reactor, socket, owner)
{
    setArgs(args);
}

ts::ReactiveTLSConnection::~ReactiveTLSConnection()
{
}

void ts::ReactiveTLSConnection::reset()
{
    _sctx.reset();
    _user_requests.clear();
    _send_position = 0;
    _send_tls_data.clear();
    _send_in_progress = false;
    _eof_reported = false;
    _recv_handler = nullptr;
    _recv_user_data.reset();
    _recv_control.reset();
    _recv_tls_data.clear();
    _recv_clear_data.clear();
    // Don't overwrite _accept_handler / _accept_user_data, it is set before connection.
}


//----------------------------------------------------------------------------
// Check if the current request is a connect/accept one.
//----------------------------------------------------------------------------

bool ts::ReactiveTLSConnection::isConnecting()
{
    return !_user_requests.empty() && (_user_requests.front()->type == SocketOp::CONNECT || _user_requests.front()->type == SocketOp::ACCEPT);
}


//----------------------------------------------------------------------------
// Call the handler of the front/current request and remove it from the queue.
//----------------------------------------------------------------------------

void ts::ReactiveTLSConnection::callFrontHandlerAndRemove(int error_code, SocketOp check_type)
{
    if (!_user_requests.empty() && (check_type == SocketOp::NONE || _user_requests.front()->type == check_type)) {

        // Remove front request from the queue.
        const RequestPtr req = _user_requests.front();
        _user_requests.pop_front();

        // Call corresponding handler.
        if (req->handler != nullptr) {
            bool connection_completed = false;
            TS_PARTIAL_SWITCH_BEGIN()
            switch (req->type) {
                case SocketOp::CONNECT:
                    req->handler->handleTCPConnected(*this, error_code, req->user_data);
                    connection_completed = SysSuccess(error_code);
                    break;
                case SocketOp::ACCEPT:
                    assert(req->server != nullptr);
                    req->handler->handleTCPAccepted(*req->server, *this, error_code, req->user_data);
                    connection_completed = SysSuccess(error_code);
                    break;
                case SocketOp::SEND:
                    req->handler->handleTCPSend(*this, _send_position, error_code, req->user_data);
                    break;
                case SocketOp::CLOSE_WRITER:
                    req->handler->handleTCPSend(*this, _send_position, SYS_EOF, req->user_data);
                    break;
                case SocketOp::CLOSE:
                    req->handler->handleTCPClosed(*this, req->user_data);
                    break;
                default:
                    break;
            }
            TS_PARTIAL_SWITCH_END()

            // When a connection completes, we need to process clear input data which was buffered.
            if (connection_completed) {
                processReceiveBuffer(_recv_clear_data, _recv_control, _recv_handler, error_code, _recv_user_data);
            }
        }
    }
    else if (!SysSuccess(error_code)) {
        // No or unexpected request, consider it as a receive error.
        callReceiveError(error_code);
    }
}


//----------------------------------------------------------------------------
// Call receive handler with an error code.
//----------------------------------------------------------------------------

void ts::ReactiveTLSConnection::callReceiveError(int error_code)
{
    report().debug(u"TLS receive error, error code: %d", error_code);
    // If the connection is broken, interpret as an end of input.
    if (error_code != SYS_EOF && (!socket().isConnected() || _sctx.eof() || _sctx.shutdowning())) {
        report().debug(u"forcing end-of-file on TLS socket");
        error_code = SYS_EOF;
    }
    // Report error to application. Report EOF only once.
    if (_recv_handler != nullptr && (error_code != SYS_EOF || !_eof_reported)) {
        static const ByteBlock empty_data;
        _recv_control.reset();
        _recv_handler->handleTCPReceive(*this, empty_data, _recv_control, error_code, _recv_user_data);
        _eof_reported = _eof_reported || error_code == SYS_EOF;
    }
}


//----------------------------------------------------------------------------
// Start the operation of connecting to a TLS server.
//----------------------------------------------------------------------------

bool ts::ReactiveTLSConnection::startConnect(HandlerType* handler, const IPSocketAddress& addr, const ObjectPtr& user_data)
{
    reset();

    // Build a connection request.
    RequestPtr req = std::make_shared<Request>(SocketOp::CONNECT, handler, user_data);
    _user_requests.push_back(req);

    // Perform the TCP connection first. The TLS handshake will start in handleTCPConnected().
    // Call the superclass to perform the connection.
    return ReactiveTCPConnection::startConnect(this, addr);
}


//----------------------------------------------------------------------------
// Called when the TCP connection completes.
//----------------------------------------------------------------------------

void ts::ReactiveTLSConnection::handleTCPConnected(ReactiveTCPConnection& sock, int error_code, const ObjectPtr& user_data)
{
    // The TCP connection completes and we need to perform the initial TLS handshake before notifying the application of the connection.
    // Because the TLS protocol needs to receive data out-of-band, we need to declare our own permanent internal receive handler.
    report().debug(u"reactive TLS: TCP connected");

    if (_user_requests.empty() || _user_requests.front()->type != SocketOp::CONNECT) {
        report().debug(u"spurious TCP connection completion, maybe canceled in the meantime");
    }
    else if (!SysSuccess(error_code)) {
        // Connection errors.
        callFrontHandlerAndRemove(error_code);
    }
    else if (!ReactiveTCPConnection::startReceive(this, TLS_MAX_PACKET_SIZE) || !_sctx.initClient(*this)) {
        // Error starting TLS handshake.
        callFrontHandlerAndRemove(SYS_ERROR);
    }
    else {
        // Start sending initial handshake data (Client Hello).
        processQueuedOperations();
    }
}


//----------------------------------------------------------------------------
// Set the handler to call when a session is accepted on server side.
//----------------------------------------------------------------------------

void ts::ReactiveTLSConnection::whenAccepted(HandlerType* handler, const ObjectPtr& user_data)
{
    // Store application handler for accepted at TLS level. These values are *not* affected by reset().
    _accept_handler = handler;
    _accept_user_data = user_data;

    // Call superclass to set the handler to call when the session is accepted at TCP level.
    ReactiveTCPConnection::whenAccepted(this);
}


//----------------------------------------------------------------------------
// Pass information from server accepting new clients.
//----------------------------------------------------------------------------

bool ts::ReactiveTLSConnection::initServerContext(ReactiveTLSServer* server, void* param)
{
    report().debug(u"reactive TLS: initialize TLS session (server side)");

    // Initialize the TLS session.
    reset();
    if (!_sctx.initServer(param)) {
        return false;
    }

    // Build and enqueue an accept request.
    RequestPtr req = std::make_shared<Request>(SocketOp::ACCEPT, _accept_handler, _accept_user_data);
    req->server = server;
    _user_requests.push_back(req);
    return true;
}


//----------------------------------------------------------------------------
// Called when a new session is accepted at TCP level on server side.
//----------------------------------------------------------------------------

void ts::ReactiveTLSConnection::handleTCPAccepted(ReactiveTCPServer& server, ReactiveTCPConnection& sock, int error_code, const ObjectPtr& user_data)
{
    report().debug(u"reactive TLS: TCP client accepted, starting TLS handshake");

    if (_user_requests.empty() || _user_requests.front()->type != SocketOp::ACCEPT) {
        report().debug(u"spurious TCP accept completion, maybe canceled in the meantime");
    }
    else if (!SysSuccess(error_code)) {
        // Connection errors.
        callFrontHandlerAndRemove(error_code);
    }
    else if (!ReactiveTCPConnection::startReceive(this, TLS_MAX_PACKET_SIZE)) {
        // Cannot set the reception handler.
        callFrontHandlerAndRemove(SYS_ERROR);
    }

    // At this point, the server side of the session simply waits for receiving the Client Hello message.
}


//----------------------------------------------------------------------------
// Start the operation of sending data over the TCP connection.
//----------------------------------------------------------------------------

bool ts::ReactiveTLSConnection::startSend(HandlerType* handler, const void* data, size_t size, const ObjectPtr& user_data)
{
    // Build a send request.
    std::shared_ptr<Request> req = std::make_shared<Request>(SocketOp::SEND, handler, user_data);
    req->send_data = data;
    req->send_size = size;

    // Enqueue the request and signal to process it in reactor context.
    _user_requests.push_back(req);
    return signalQueuedOperations();
}


//----------------------------------------------------------------------------
// Start the operation of receiving messages from the socket.
//----------------------------------------------------------------------------

bool ts::ReactiveTLSConnection::startReceive(HandlerType* handler, size_t buffer_size, const ObjectPtr& user_data)
{
    // Because TLS protocol requires to receive data, data reception is always on after connection.
    // Just set which handler to call upon extraction of clear data from the TLS context.
    // Buffer size is ignored here.
    _recv_handler = handler;
    _recv_user_data = user_data;
    return true;
}


//----------------------------------------------------------------------------
// Cancel any pending send or receive operation on this socket.
//----------------------------------------------------------------------------

void ts::ReactiveTLSConnection::cancelSendReceive(bool silent)
{
    _recv_handler = nullptr;
    _recv_user_data.reset();

    // Break underlying TLS protocol stream.
    ReactiveTCPConnection::cancelSendReceive(silent);
}


//----------------------------------------------------------------------------
// Start closing the send direction of the socket.
//----------------------------------------------------------------------------

bool ts::ReactiveTLSConnection::startCloseWriter(HandlerType* handler, bool silent, const ObjectPtr& user_data)
{
    report().debug(u"reactive TLS: start close-writer");

    // Build a close request.
    RequestPtr req = std::make_shared<Request>(SocketOp::CLOSE_WRITER, handler, user_data);
    req->silent = silent;

    // Enqueue the request and signal to process it in reactor context.
    _user_requests.push_back(req);
    return signalQueuedOperations();
}


//----------------------------------------------------------------------------
// Start closing the send direction of the socket.
//----------------------------------------------------------------------------

bool ts::ReactiveTLSConnection::startClose(HandlerType* handler, bool silent, const ObjectPtr& user_data)
{
    report().debug(u"reactive TLS: start close");

    // Build a close request.
    RequestPtr req = std::make_shared<Request>(SocketOp::CLOSE, handler, user_data);
    req->silent = silent;

    // Enqueue the request and signal to process it in reactor context.
    _user_requests.push_back(req);
    return signalQueuedOperations();
}


//----------------------------------------------------------------------------
// Called when a close operation completes.
//----------------------------------------------------------------------------

void ts::ReactiveTLSConnection::handleTCPClosed(ReactiveTCPConnection& sock, const ObjectPtr& user_data)
{
    report().debug(u"reactive TLS: TCP closed");
    _sctx.reset();
    callFrontHandlerAndRemove(SYS_SUCCESS, SocketOp::CLOSE);
}


//----------------------------------------------------------------------------
// Called when a send operation completes.
//----------------------------------------------------------------------------

void ts::ReactiveTLSConnection::handleTCPSend(ReactiveTCPConnection& sock, size_t position, int error_code, const ObjectPtr& user_data)
{
    _send_in_progress = false;

    if (SysSuccess(error_code)) {
        // Continue processing TLS protocol.
        processQueuedOperations();
    }
    else {
        // A TCP send error is usually fatal for the connection.
        callFrontHandlerAndRemove(error_code);
        while (!_user_requests.empty()) {
            callFrontHandlerAndRemove(SYS_CANCELED);
        }
    }
}


//----------------------------------------------------------------------------
// Called in reactor context after signalQueuedOperations().
//----------------------------------------------------------------------------

void ts::ReactiveTLSConnection::processQueuedOperations()
{
    report().debug(u"@@@ processQueuedOperations: begin, %d requests, first: %s, need send: %s, need receive: %s", _user_requests.size(), _user_requests.empty() ? u"(none)" :SocketOpNames().name(_user_requests.front()->type), _sctx.needSend(), _sctx.needReceive());
    // Call superclass first.
    ReactiveTCPConnection::processQueuedOperations();

    // Loop on operations on TLS context. Don't try anything if we are closing the connection.
    for (bool terminate = false; !terminate && isOpen(); ) {

        // First priority: if the TLS context needs to send something, start the send operation and cannot do more.
        if (_sctx.needSend()) {
            // Start to send only if not yet in progress.
            if (!_send_in_progress) {
                _send_tls_data.clear();
                _sctx.getDataToSend(_send_tls_data);
                _send_in_progress = ReactiveTCPConnection::startSend(this, _send_tls_data.data(), _send_tls_data.size());
            }
            if (!_send_in_progress) {
                callFrontHandlerAndRemove(SYS_ERROR);
            }
            break;
        }

        // Second priority: if the TLS context needs more input, cannot do anything until we get more data.
        // Last priority: process current user's request. If there is none, then nothing do do.
        if (_sctx.needReceive() || _user_requests.empty()) {
            break;
        }

        // Process current request. At this point, sctx is idle (no need to send or receive).
        const RequestPtr req = _user_requests.front();
        TS_PARTIAL_SWITCH_BEGIN()
        switch (req->type) {
            case SocketOp::CONNECT:
            case SocketOp::ACCEPT: {
                // When the TLS context becomes idle (no need to read or write when we reach this point), TLS connection is complete.
                report().debug(u"reactive TLS: %s completed", SocketOpNames().name(req->type));
                callFrontHandlerAndRemove(SYS_SUCCESS);
                break;
            }
            case SocketOp::SEND: {
                size_t ret_size = 0;
                if (req->send_data == nullptr || req->send_size == 0) {
                    // Shortcut for empty requests.
                    callFrontHandlerAndRemove(SYS_SUCCESS);
                }
                else if (!_sctx.provideClearData(req->send_data, req->send_size, ret_size)) {
                    // Failed to encrypt some data in the TLS context.
                    callFrontHandlerAndRemove(SYS_ERROR);
                }
                else {
                    assert(ret_size <= req->send_size);
                    req->send_data = reinterpret_cast<const uint8_t*>(req->send_data) + ret_size;
                    req->send_size -= ret_size;
                    _send_position += ret_size;
                    // If all data are sent, complete the request.
                    if (req->send_size == 0) {
                        callFrontHandlerAndRemove(SYS_SUCCESS);
                    }
                }
                break;
            }
            case SocketOp::CLOSE_WRITER: {
                // Generate and send a shutdown message.
                report().debug(u"reactive TLS: processing close-writer request");
                callFrontHandlerAndRemove(_sctx.initShutdown(req->silent) ? SYS_SUCCESS : SYS_ERROR);
                break;
            }
            case SocketOp::CLOSE: {
                report().debug(u"reactive TLS: processing close request");
                if (!_sctx.shutdowning()) {
                    // Close-write not sent. Requeue a new close-writer request first.
                    RequestPtr req2 = std::make_shared<Request>(SocketOp::CLOSE_WRITER, nullptr, nullptr);
                    req2->silent = req->silent;
                    _user_requests.push_front(req2);
                }
                else if (isOpen() && !ReactiveTCPConnection::startClose(this, req->silent)) {
                    callFrontHandlerAndRemove(SYS_ERROR);
                }
                else {
                    // Now need to wait for end of close. Keep close request as current.
                    terminate = true;
                }
                break;
            }
            default: {
                // Other type of request, nothing more to do, just wait for external events.
                terminate = true;
                break;
            }
        }
        TS_PARTIAL_SWITCH_END()
    }
}


//----------------------------------------------------------------------------
// Called when a receive operation completes.
//----------------------------------------------------------------------------

void ts::ReactiveTLSConnection::handleTCPReceive(ReactiveTCPConnection& sock, const ByteBlock& data, ReactiveTCPInputControl& control, int error_code, const ObjectPtr& user_data)
{
    report().debug(u"@@@ handleTCPReceive: %d bytes", data.size());
    if (!SysSuccess(error_code)) {
        // Report receive errors to application.
        callReceiveError(error_code);
    }
    else if (_sctx.needSend()) {
        // Cannot feed the TLS context if it needs to send. Buffer TLS protocol data for later processing.
        _recv_tls_data.append(data);
    }
    else {
        // Consume previously buffered TLS data first.
        size_t done = handleReceivedData(_recv_tls_data, error_code);
        if (done >= _recv_tls_data.size()) {
            // All previous TLS data processed, we can submit more from input, directly from reception buffer.
            done = handleReceivedData(data, error_code);
            if (done < data.size()) {
                // Not everything could be processed, buffer it.
                _recv_tls_data.copy(data, done);
            }
            else {
                _recv_tls_data.clear();
            }
        }
        else {
            // Not all previous data processed, can't submit more, store all incoming data in TLS data buffer.
            _recv_tls_data.erase(0, done);
            _recv_tls_data.append(data);
            // Retry with more data.
            done = handleReceivedData(_recv_tls_data, error_code);
            if (done < data.size()) {
                // Not everything could be processed.
                _recv_tls_data.erase(0, done);
            }
            else {
                _recv_tls_data.clear();
            }
        }
        // Process additional send requests.
        processQueuedOperations();
    }
}


//----------------------------------------------------------------------------
// Handle received TLS data.
//----------------------------------------------------------------------------

size_t ts::ReactiveTLSConnection::handleReceivedData(const ByteBlock& tls_data, int error_code)
{
    size_t start = 0;

    // We need to stop submitting data when
    // - the connection is closing (could be done in the application handler)
    // - the TLS context needs to send, meaning that its output buffer is currently in use
    while (start < tls_data.size() && isOpen() && !_sctx.needSend()) {

        // Submit one chunk of TLS protocol to TLS context.
        size_t ret_size = 0;
        if (!_sctx.provideReceivedData(tls_data.data() + start, tls_data.size() - start, ret_size, _recv_clear_data)) {
            // TLS error while processing received data.
            if (!_sctx.eof() && !_user_requests.empty()) {
                // Not an end-of-file and there is a pending command, maybe a handshake error, abort that command.
                callFrontHandlerAndRemove(SYS_ERROR);
            }
            else {
                // End of file or no pending command, report a receive error.
                callReceiveError(_sctx.eof() ? SYS_EOF : SYS_ERROR);
            }
            break;
        }
        start += ret_size;

        // Then, process all clear data in the application. Do not start to process clear data while in the connection phase.
        // In the connection phase, the connected/accepted handler of the application has not been called yet. Therefore, the
        // application may not have set it receive handlers or initialized its connection context.
        if (!isConnecting()) {
            processReceiveBuffer(_recv_clear_data, _recv_control, _recv_handler, error_code, _recv_user_data);
        }
    }

    // Return the number of consumed bytes at beginning of tls_data.
    return start;
}
