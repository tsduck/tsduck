//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  SSL/TLS connected socket for use in a Reactor environment
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReactiveTCPConnection.h"
#include "tsTLSConnectionBase.h"
#include "tsTLSContext.h"
#include "tsSocketOp.h"

namespace ts {

    class ReactiveTLSServer;
    class ReactiveTCPServerHandlerInterface;

    //!
    //! SSL/TLS connected socket for use in a Reactor environment.
    //! @ingroup libtscore reactor
    //!
    //! The class ReactiveTLSConnection is a wrapper around TCPConnection to handle reactive I/O.
    //!
    //! The actual socket is a separate object. It is initialized and configured by the application.
    //! The application shall not directly call connect(), send(), receive(), closeWriter(), or close() on this socket and
    //! delegate these operations to startConnect(), startSend(), startReceive(), startCloseWriter() and startClose() in
    //! class ReactiveTLSConnection.
    //!
    class TSCOREDLL ReactiveTLSConnection: public ReactiveTCPConnection, public TLSConnectionBase, protected ReactiveTCPConnectionHandlerInterface
    {
        TS_NOBUILD_NOCOPY(ReactiveTLSConnection);
    public:
        //!
        //! Constructor.
        //! @param [in,out] reactor Associated reactor. The reactor object must remain valid as long as this object is valid.
        //! @param [in,out] socket Associated socket. The socket object must remain valid as long as this object is valid.
        //! The ReactiveTLSConnection must be initialized before the @a socket is opened.
        //! Important: @a socket must be an instance of TCPConnection, not an instance of TLSConnection.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        ReactiveTLSConnection(Reactor& reactor, TCPConnection& socket, Object* owner = nullptr);

        //!
        //! Constructor with initial client arguments.
        //! @param [in,out] reactor Associated reactor. The reactor object must remain valid as long as this object is valid.
        //! @param [in,out] socket Associated socket. The socket object must remain valid as long as this object is valid.
        //! The ReactiveTLSConnection must be initialized before the @a socket is opened.
        //! Important: @a socket must be an instance of TCPConnection, not an instance of TLSConnection.
        //! @param [in] args Initial TLS client arguments.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        ReactiveTLSConnection(Reactor& reactor, TCPConnection& socket, const TLSArgs& args, Object* owner = nullptr);

        // Inherited methods.
        virtual ~ReactiveTLSConnection() override;
        virtual bool startConnect(ReactiveTCPConnectionHandlerInterface* handler, const IPSocketAddress& addr, const ObjectPtr& user_data = ObjectPtr()) override;
        virtual void whenAccepted(ReactiveTCPConnectionHandlerInterface* handler, const ObjectPtr& user_data = ObjectPtr()) override;
        virtual bool startSend(ReactiveTCPConnectionHandlerInterface* handler, const void* data, size_t size, const ObjectPtr& user_data = ObjectPtr()) override;
        virtual bool startCloseWriter(ReactiveTCPConnectionHandlerInterface* handler, bool silent = false, const ObjectPtr& user_data = ObjectPtr()) override;
        virtual bool startReceive(ReactiveTCPConnectionHandlerInterface* handler, size_t buffer_size = DEFAULT_RECEIVE_BUFFER_SIZE, const ObjectPtr& user_data = ObjectPtr()) override;
        virtual void cancelSendReceive(bool silent = false) override;
        virtual bool startClose(ReactiveTCPConnectionHandlerInterface* handler, bool silent = false, const ObjectPtr& user_data = ObjectPtr()) override;

    protected:
        // Implementation of ReactiveBase.
        virtual void processQueuedOperations() override;

        // Implementation of ReactiveTCPConnectionHandlerInterface.
        virtual void handleTCPConnected(ReactiveTCPConnection& sock, int error_code, const ObjectPtr& user_data) override;
        virtual void handleTCPAccepted(ReactiveTCPServer& server, ReactiveTCPConnection& sock, int error_code, const ObjectPtr& user_data) override;
        virtual void handleTCPSend(ReactiveTCPConnection& sock, size_t position, int error_code, const ObjectPtr& user_data) override;
        virtual void handleTCPReceive(ReactiveTCPConnection& sock, const ByteBlock& data, ReactiveInputControl& control, int error_code, const ObjectPtr& user_data) override;
        virtual void handleTCPClosed(ReactiveTCPConnection& sock, const ObjectPtr& user_data) override;

    private:
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

        // ReactiveTLSConnection private fields.
        TLSContext              _sctx {this, this};
        RequestQueue            _user_requests {};         // Queue of user request (connect, send, close).
        size_t                  _send_position = 0;        // Clear data sent.
        ByteBlock               _send_tls_data {};         // Outgoing TLS data, to send over the network.
        bool                    _send_in_progress = false; // Sending TLS data in progress, waiting for send completion.
        bool                    _eof_reported = false;     // End of input stream already reported to application.
        HandlerType*            _recv_handler = nullptr;   // User handler for data reception.
        ObjectPtr               _recv_user_data {};        // User data for data reception.
        ReactiveInputControl _recv_control {};          // User control of input data.
        ByteBlock               _recv_tls_data {};         // Incoming TLS data which cannot be processed now.
        ByteBlock               _recv_clear_data {};       // Incoming clear data.
        HandlerType*            _accept_handler = nullptr; // User handler for accepted session, at TLS level.
        ObjectPtr               _accept_user_data {};      // User data for _accept_handler.

        // Reset connection state.
        void reset();

        // Check if the current request is a connect/accept one.
        bool isConnecting();

        // Call the handler of the front/current request and remove it from the queue.
        void callFrontHandlerAndRemove(int error_code, SocketOp check_type = SocketOp::NONE);

        // Call receive handler with an error code.
        void callReceiveError(int error_code);

        // Handle received TLS data, pass to SChannel context, add decrypted data in _recv_clear_data, call _recv_handler.
        // Return the number of consumed bytes at beginning of tls_data.
        size_t handleReceivedData(const ByteBlock& tls_data, int error_code);

        // Pass information from server accepting new clients.
        // This information will be used when a connection is accepted, to start TLS handshake.
        // The parameter is:
        // - On UNIX systems with OpenSSL, a pointer to ::SSL_CTX.
        // - On Windows systems whith SChannel, a pointer to ::CERT_CONTEXT.
        friend class ReactiveTLSServer;
        bool initServerContext(ReactiveTLSServer* server, void* param);
    };
}
