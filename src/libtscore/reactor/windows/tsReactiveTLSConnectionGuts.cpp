//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveTLSConnection.h"
#include "tsReactiveTLSServer.h"
#include "tsReactiveTCPServerHandlerInterface.h"
#include "tsSChannelContext.h"
#include "tsMemory.h"


//----------------------------------------------------------------------------
// System-specific parts are stored in a private structure.
//----------------------------------------------------------------------------

class ts::ReactiveTLSConnection::SystemGuts: public ReactiveBase, public HandlerType
{
    TS_NOBUILD_NOCOPY(SystemGuts);
public:
    // Each of these requests may need several exchanges of TLS protocol. The request must be
    // kept as "current" as long as all exchanges are not completed. Other requests are queued.
    // The "current" request is the one at the front of the queue.
    class TSCOREDLL Request
    {
        TS_NOBUILD_NOCOPY(Request);
    public:
        SocketOp           type = SocketOp::NONE;
        HandlerType*       handler = nullptr;
        ObjectPtr          user_data {};
        const void*        send_data = nullptr;
        size_t             send_size = 0;
        ReactiveTLSServer* server = nullptr;  // ACCEPT only
        bool               silent = false;

        // Constructor and destructor.
        Request(SocketOp t, HandlerType* h, const ObjectPtr& u) : type(t), handler(h), user_data(u) {}
    };

    using RequestPtr = std::shared_ptr<Request>;
    using RequestQueue = std::list<RequestPtr>;

    // SystemGuts public fields.
    ReactiveTLSConnection&  conn;                     // Parent object.
    SChannelContext         sctx;                     // TLS context.
    RequestQueue            user_requests {};         // Queue of user request (connect, send, close).
    size_t                  send_position = 0;        // Clear data sent.
    bool                    send_in_progress = false; // Sending sctx data in progress, waiting for send completion.
    HandlerType*            recv_handler = nullptr;   // User handler for data reception.
    ObjectPtr               recv_user_data {};        // User data for data reception.
    ReactiveTCPInputControl recv_control {};          // User control of input data.
    ByteBlock               recv_tls_data {};         // Incoming TLS data which cannot be processed now.
    ByteBlock               recv_clear_data {};       // Incoming clear data.
    HandlerType*            accept_handler = nullptr; // User handler for accepted session, at TLS level.
    ObjectPtr               accept_user_data {};      // User data for accept_handler.

    // Constructor and destructor.
    SystemGuts(ReactiveTLSConnection& c);
    virtual ~SystemGuts() override;

    // Reset connection state.
    void reset();

    // Check if the current request is a connect/accept one.
    bool isConnecting();

    // Call the handler of the front/current request and remove it from the queue.
    void callFrontHandlerAndRemove(int error_code, SocketOp check_type = SocketOp::NONE);

    // Call receive handler with an error code.
    void callReceiveError(int error_code);

    // Handle received TLS data, pass to SChannel context, add decrypted data in recv_clear_data, call recv_handler.
    // Return the number of consumed bytes at beginning of tls_data.
    size_t handleReceivedData(const ByteBlock& tls_data, int error_code);

    // Implementation of reactive handlers.
    virtual void processQueuedOperations() override;
    virtual void handleTCPConnected(ReactiveTCPConnection& sock, int error_code, const ObjectPtr& user_data) override;
    virtual void handleTCPAccepted(ReactiveTCPServer& server, ReactiveTCPConnection& sock, int error_code, const ObjectPtr& user_data) override;
    virtual void handleTCPSend(ReactiveTCPConnection& sock, size_t position, int error_code, const ObjectPtr& user_data) override;
    virtual void handleTCPReceive(ReactiveTCPConnection& sock, const ByteBlock& data, ReactiveTCPInputControl& control, int error_code, const ObjectPtr& user_data) override;
    virtual void handleTCPClosed(ReactiveTCPConnection& sock, const ObjectPtr& user_data) override;
};


//----------------------------------------------------------------------------
// System guts constructor and destructor.
//----------------------------------------------------------------------------

void ts::ReactiveTLSConnection::allocateGuts()
{
    _guts = new SystemGuts(*this);
}

void ts::ReactiveTLSConnection::deleteGuts()
{
    delete _guts;
}

ts::ReactiveTLSConnection::SystemGuts::SystemGuts(ReactiveTLSConnection& c) :
    ReactiveBase(c.reactor()),
    conn(c),
    sctx(&c.reactor(), c)
{
}

ts::ReactiveTLSConnection::SystemGuts::~SystemGuts()
{
}

void ts::ReactiveTLSConnection::SystemGuts::reset()
{
    sctx.reset();
    user_requests.clear();
    send_position = 0;
    send_in_progress = false;
    recv_handler = nullptr;
    recv_user_data.reset();
    recv_control.reset();
    recv_tls_data.clear();
    recv_clear_data.clear();
    // Don't overwrite accept_handler / accept_user_data, it is set before connection.
}


//----------------------------------------------------------------------------
// Check if the current request is a connect/accept one.
//----------------------------------------------------------------------------

bool ts::ReactiveTLSConnection::SystemGuts::isConnecting()
{
    return !user_requests.empty() && (user_requests.front()->type == SocketOp::CONNECT || user_requests.front()->type == SocketOp::ACCEPT);
}


//----------------------------------------------------------------------------
// Call the handler of the front/current request and remove it from the queue.
//----------------------------------------------------------------------------

void ts::ReactiveTLSConnection::SystemGuts::callFrontHandlerAndRemove(int error_code, SocketOp check_type)
{
    if (!user_requests.empty() && (check_type == SocketOp::NONE || user_requests.front()->type == check_type)) {

        // Remove front request from the queue.
        const RequestPtr req = user_requests.front();
        user_requests.pop_front();

        // Call corresponding handler.
        if (req->handler != nullptr) {
            bool connection_completed = false;
            TS_PUSH_WARNING()
            TS_MSC_NOWARNING(4061)
            switch (req->type) {
                case SocketOp::CONNECT:
                    req->handler->handleTCPConnected(conn, error_code, req->user_data);
                    connection_completed = SysSuccess(error_code);
                    break;
                case SocketOp::ACCEPT:
                    assert(req->server != nullptr);
                    req->handler->handleTCPAccepted(*req->server, conn, error_code, req->user_data);
                    connection_completed = SysSuccess(error_code);
                    break;
                case SocketOp::SEND:
                    req->handler->handleTCPSend(conn, send_position, error_code, req->user_data);
                    break;
                case SocketOp::CLOSE_WRITER:
                    req->handler->handleTCPSend(conn, send_position, SYS_EOF, req->user_data);
                    break;
                case SocketOp::CLOSE:
                    req->handler->handleTCPClosed(conn, req->user_data);
                    break;
                default:
                    break;
            }
            TS_POP_WARNING()

            // When a connection completes, we need to process clear input data which was buffered.
            if (connection_completed) {
                conn.processReceiveBuffer(recv_clear_data, recv_control, recv_handler, error_code, recv_user_data);
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

void ts::ReactiveTLSConnection::SystemGuts::callReceiveError(int error_code)
{
    report().debug(u"receive error, error code: %d", error_code);
    if (recv_handler != nullptr) {
        static const ByteBlock empty_data;
        recv_control.reset();
        recv_handler->handleTCPReceive(conn, empty_data, recv_control, error_code, recv_user_data);
    }
}


//----------------------------------------------------------------------------
// Start the operation of connecting to a TLS server.
//----------------------------------------------------------------------------

bool ts::ReactiveTLSConnection::startConnect(HandlerType* handler, const IPSocketAddress& addr, const ObjectPtr& user_data)
{
    _guts->reset();

    // Build a connection request.
    SystemGuts::RequestPtr req = std::make_shared<SystemGuts::Request>(SocketOp::CONNECT, handler, user_data);
    _guts->user_requests.push_back(req);

    // Perform the TCP connection first. The TLS handshake will start in handleTCPConnected().
    // Call the superclass to perform the connection.
    return ReactiveTCPConnection::startConnect(_guts, addr);
}


//----------------------------------------------------------------------------
// Called when the TCP connection completes.
//----------------------------------------------------------------------------

void ts::ReactiveTLSConnection::SystemGuts::handleTCPConnected(ReactiveTCPConnection& sock, int error_code, const ObjectPtr& user_data)
{
    // The TCP connection completes and we need to perform the initial TLS handshake before notifying the application of the connection.
    // Because the TLS protocol needs to receive data out-of-band, we need to declare our own permanent internal receive handler.
    report().debug(u"reactive TLS: TCP connected");

    if (user_requests.empty() || user_requests.front()->type != SocketOp::CONNECT) {
        report().debug(u"spurious TCP connection completion, maybe canceled in the meantime");
    }
    else if (!SysSuccess(error_code)) {
        // Connection errors.
        callFrontHandlerAndRemove(error_code);
    }
    else if (!conn.ReactiveTCPConnection::startReceive(this, TLS_MAX_PACKET_SIZE) || !sctx.initClient()) {
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
    _guts->accept_handler = handler;
    _guts->accept_user_data = user_data;

    // Call superclass to set the handler to call when the session is accepted at TCP level.
    ReactiveTCPConnection::whenAccepted(_guts);
}


//----------------------------------------------------------------------------
// Pass information from server accepting new clients.
//----------------------------------------------------------------------------

bool ts::ReactiveTLSConnection::initServerContext(ReactiveTLSServer* server, const void* param)
{
    report().debug(u"reactive TLS: initialize TLS session (server side)");

    // Initialize the TLS session.
    _guts->reset();
    if (!_guts->sctx.initServer(reinterpret_cast<::PCCERT_CONTEXT>(param))) {
        return false;
    }

    // Build and enqueue an accept request.
    SystemGuts::RequestPtr req = std::make_shared<SystemGuts::Request>(SocketOp::ACCEPT, _guts->accept_handler, _guts->accept_user_data);
    req->server = server;
    _guts->user_requests.push_back(req);
    return true;
}


//----------------------------------------------------------------------------
// Called when a new session is accepted at TCP level on server side.
//----------------------------------------------------------------------------

void ts::ReactiveTLSConnection::SystemGuts::handleTCPAccepted(ReactiveTCPServer& server, ReactiveTCPConnection& sock, int error_code, const ObjectPtr& user_data)
{
    report().debug(u"reactive TLS: TCP client accepted, starting TLS handshake");

    if (user_requests.empty() || user_requests.front()->type != SocketOp::ACCEPT) {
        report().debug(u"spurious TCP accept completion, maybe canceled in the meantime");
    }
    else if (!SysSuccess(error_code)) {
        // Connection errors.
        callFrontHandlerAndRemove(error_code);
    }
    else if (!conn.ReactiveTCPConnection::startReceive(this, TLS_MAX_PACKET_SIZE)) {
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
    std::shared_ptr<SystemGuts::Request> req = std::make_shared<SystemGuts::Request>(SocketOp::SEND, handler, user_data);
    req->send_data = data;
    req->send_size = size;

    // Enqueue the request and signal to process it in reactor context.
    _guts->user_requests.push_back(req);
    return _guts->signalQueuedOperations();
}


//----------------------------------------------------------------------------
// Start the operation of receiving messages from the socket.
//----------------------------------------------------------------------------

bool ts::ReactiveTLSConnection::startReceive(HandlerType* handler, size_t buffer_size, const ObjectPtr& user_data)
{
    // Because TLS protocol requires to receive data, data reception is always on after connection.
    // Just set which handler to call upon extraction of clear data from the SChannel context.
    // Buffer size is ignored here.
    _guts->recv_handler = handler;
    _guts->recv_user_data = user_data;
    return true;
}


//----------------------------------------------------------------------------
// Cancel any pending send or receive operation on this socket.
//----------------------------------------------------------------------------

void ts::ReactiveTLSConnection::cancelSendReceive(bool silent)
{
    _guts->recv_handler = nullptr;
    _guts->recv_user_data.reset();

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
    SystemGuts::RequestPtr req = std::make_shared<SystemGuts::Request>(SocketOp::CLOSE_WRITER, handler, user_data);
    req->silent = silent;

    // Enqueue the request and signal to process it in reactor context.
    _guts->user_requests.push_back(req);
    return _guts->signalQueuedOperations();
}


//----------------------------------------------------------------------------
// Start closing the send direction of the socket.
//----------------------------------------------------------------------------

bool ts::ReactiveTLSConnection::startClose(HandlerType* handler, bool silent, const ObjectPtr& user_data)
{
    report().debug(u"reactive TLS: start close");

    // Build a close request.
    SystemGuts::RequestPtr req = std::make_shared<SystemGuts::Request>(SocketOp::CLOSE, handler, user_data);
    req->silent = silent;

    // Enqueue the request and signal to process it in reactor context.
    _guts->user_requests.push_back(req);
    return _guts->signalQueuedOperations();
}


//----------------------------------------------------------------------------
// Called when a close operation completes.
//----------------------------------------------------------------------------

void ts::ReactiveTLSConnection::SystemGuts::handleTCPClosed(ReactiveTCPConnection& sock, const ObjectPtr& user_data)
{
    report().debug(u"reactive TLS: TCP closed");
    sctx.reset();
    callFrontHandlerAndRemove(SYS_SUCCESS, SocketOp::CLOSE);
}


//----------------------------------------------------------------------------
// Called when a send operation completes.
//----------------------------------------------------------------------------

void ts::ReactiveTLSConnection::SystemGuts::handleTCPSend(ReactiveTCPConnection& sock, size_t position, int error_code, const ObjectPtr& user_data)
{
    // Acknowledge sent data in SChannel context.
    if (send_in_progress && SysSuccess(error_code)) {
        if (!sctx.sendCompleted()) {
            error_code = SYS_ERROR;
        }
        send_in_progress = false;
    }

    if (SysSuccess(error_code)) {
        // Continue processing TLS protocol.
        processQueuedOperations();
    }
    else {
        // A TCP send error is usually fatal for the connection.
        callFrontHandlerAndRemove(error_code);
        while (!user_requests.empty()) {
            callFrontHandlerAndRemove(SYS_CANCELED);
        }
    }
}


//----------------------------------------------------------------------------
// Called in reactor context after signalQueuedOperations().
//----------------------------------------------------------------------------

void ts::ReactiveTLSConnection::SystemGuts::processQueuedOperations()
{
    // Loop on operations on SChannel context. Don't try anything if we are closing the connection.
    while (conn.isOpen()) {

        // First priority: if the TLS context needs to send something, start the send operation and cannot do more.
        if (sctx.needSend()) {
            // Start to send only if not yet in progress.
            if (!send_in_progress) {
                send_in_progress = conn.ReactiveTCPConnection::startSend(this, sctx.sendAddress(), sctx.sendSize());
            }
            if (!send_in_progress) {
                callFrontHandlerAndRemove(SYS_ERROR);
            }
            break;
        }

        // Second priority: if the TLS context needs more input, cannot do anything until we get more data.
        // Last priority: process current user's request. If there is none, then nothing do do.
        if (sctx.needReceive() || user_requests.empty()) {
            break;
        }

        // Process current request. At this point, sctx is idle (no need to send or receive).
        const RequestPtr req = user_requests.front();
        if (req->type == SocketOp::CONNECT || req->type == SocketOp::ACCEPT) {
            // When the SChannel context becomes idle (no need to read or write), TLS connection is complete.
            report().debug(u"reactive TLS: %s completed", SocketOpNames().name(req->type));
            callFrontHandlerAndRemove(SYS_SUCCESS);
        }
        else if (req->type == SocketOp::SEND) {
            const size_t previous_size = req->send_size;
            if (req->send_data == nullptr || req->send_size == 0) {
                // Shortcut for empty requests.
                callFrontHandlerAndRemove(SYS_SUCCESS);
            }
            else if (!sctx.sendUserData(req->send_data, req->send_size)) {
                // Failed to encrypt some data in the SChannel context.
                callFrontHandlerAndRemove(SYS_ERROR);
            }
            else {
                // Send_data and send_size were updated by sctx.sendUserData.
                // Update actual number of sent bytes (sent in SChannel context).
                send_position += previous_size - req->send_size;
                // If all data are sent, complete the request.
                if (req->send_size == 0) {
                    callFrontHandlerAndRemove(SYS_SUCCESS);
                }
            }
        }
        else if (req->type == SocketOp::CLOSE_WRITER) {
            // Generate and send a shutdown message.
            report().debug(u"reactive TLS: processing close-writer request");
            callFrontHandlerAndRemove(sctx.initShutdown() ? SYS_SUCCESS : SYS_ERROR);
        }
        else if (req->type == SocketOp::CLOSE) {
            report().debug(u"reactive TLS: processing close request");
            if (!sctx.shutdowning()) {
                // Close-write not sent. Requeue a new close-writer request first.
                RequestPtr req2 = std::make_shared<Request>(SocketOp::CLOSE_WRITER, nullptr, nullptr);
                req2->silent = req->silent;
                user_requests.push_front(req2);
            }
            else if (conn.isOpen() && !conn.ReactiveTCPConnection::startClose(this, req->silent)) {
                callFrontHandlerAndRemove(SYS_ERROR);
            }
            else {
                // Now need to wait for end of close. Keep close request as current.
                break;
            }
        }
        else {
            // Other type of request, nothing more to do, just wait for external events.
            break;
        }
    }
}


//----------------------------------------------------------------------------
// Called when a receive operation completes.
//----------------------------------------------------------------------------

void ts::ReactiveTLSConnection::SystemGuts::handleTCPReceive(ReactiveTCPConnection& sock, const ByteBlock& data, ReactiveTCPInputControl& control, int error_code, const ObjectPtr& user_data)
{
    if (!SysSuccess(error_code)) {
        // Report receive errors to application.
        callReceiveError(error_code);
    }
    else if (sctx.needSend()) {
        // Cannot feed the SChannel context if it needs to send. Buffer TLS protocol data for later processing.
        recv_tls_data.append(data);
    }
    else {
        // Consume previously buffered TLS data first.
        size_t done = handleReceivedData(recv_tls_data, error_code);
        if (done >= recv_tls_data.size()) {
            // All previous TLS data processed, we can submit more from input, directly from reception buffer.
            done = handleReceivedData(data, error_code);
            if (done < data.size()) {
                // Not everything could be processed, buffer it.
                recv_tls_data.copy(data, done);
            }
            else {
                recv_tls_data.clear();
            }
        }
        else {
            // Not all previous data processed, can't submit more, store all incoming data in TLS data buffer.
            recv_tls_data.erase(0, done);
            recv_tls_data.append(data);
            // Retry with more data.
            done = handleReceivedData(recv_tls_data, error_code);
            if (done < data.size()) {
                // Not everything could be processed.
                recv_tls_data.erase(0, done);
            }
            else {
                recv_tls_data.clear();
            }
        }
        // Process additional send requests.
        processQueuedOperations();
    }
}


//----------------------------------------------------------------------------
// Handle received TLS data.
//----------------------------------------------------------------------------

size_t ts::ReactiveTLSConnection::SystemGuts::handleReceivedData(const ByteBlock& tls_data, int error_code)
{
    size_t start = 0;
    size_t chunk = 0;

    // We need to stop submitting data when
    // - the connection is closing (could be done in the application handler)
    // - the SChannel context can no longer process input data (receiveSize() == 0)
    // - the SChannel context needs to send, meaning that its output buffer is currently in use
    while (conn.isOpen() && (chunk = std::min(tls_data.size() - start, sctx.receiveSize())) > 0 && !sctx.needSend()) {
        // Submit one chunk of TLS protocol to SChannel context.
        MemCopy(sctx.receiveAddress(), tls_data.data() + start, chunk);
        if (!sctx.receiveCompleted(chunk, recv_clear_data)) {
            // TLS error while processing received data.
            if (!sctx.eof() && !user_requests.empty()) {
                // Not an end-of-file and there is a pending command, maybe a handshake error, abort that command.
                callFrontHandlerAndRemove(SYS_ERROR);
            }
            else if (recv_handler != nullptr) {
                // End of file or not pending command, report a receive error.
                static const ByteBlock empty;
                recv_control.reset();
                recv_handler->handleTCPReceive(conn, empty, recv_control, sctx.eof() ? SYS_EOF : SYS_ERROR, recv_user_data);
            }
            break;
        }
        // Then, process all clear data in the application. Do not start to process clear data while in the connection phase.
        // In the connection phase, the connected/accepted handler of the application has not been called yet. Therefore, the
        // application may not have set it receive handlers or initialized its connection context.
        if (!isConnecting()) {
            conn.processReceiveBuffer(recv_clear_data, recv_control, recv_handler, error_code, recv_user_data);
        }
        start += chunk;
    }

    // Return the number of consumed bytes at beginning of tls_data.
    return start;
}
