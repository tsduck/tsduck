//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Line oriented Telnet connection for use in a Reactor environment
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReactiveSocketBase.h"
#include "tsReactiveTelnetConnectionHandlerInterface.h"
#include "tsReactiveTCPConnection.h"

namespace ts {
    //!
    //! Line oriented Telnet connection for use in a Reactor environment
    //! @ingroup libtscore reactor
    //!
    //! The class ReactiveTelnetConnection is a wrapper around ReactiveTCPConnection to handle reactive I/O.
    //!
    class TSCOREDLL ReactiveTelnetConnection:
        public ReactiveSocketBase,
        private ReactiveTCPConnectionHandlerInterface,
        private SocketHandlerInterface
    {
        TS_NOBUILD_NOCOPY(ReactiveTelnetConnection);
    public:
        //!
        //! Constructor.
        //! @param [in,out] socket Associated reactive TCP socket. The socket object must remain valid as long as this object is valid.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        explicit ReactiveTelnetConnection(ReactiveTCPConnection& socket, Object* owner = nullptr);

        //!
        //! Destructor.
        //!
        virtual ~ReactiveTelnetConnection() override;

        //!
        //! Get a reference to the associated socket.
        //! @return A reference to the associated socket.
        //!
        ReactiveTCPConnection& socket() { return _socket; }

        //!
        //! Start the operation of sending a line of text over the TCP connection.
        //! There is no completion handler because the sent data is kept in a dedicated buffer.
        //! @param [in] line Text line to send. The corresponding memory area is no longer used upon return.
        //! An end-of-line marker is automatically added.
        //! @param [in] flush If false, data are buffered into the ReactiveTelnetConnection and no completion
        //! handler will be called. When true, all previously buffered data are sent with data from this call.
        //! @return True on success, false on error. Success means that the I/O was successfully started.
        //!
        bool startSendLine(const std::string& line, bool flush = true);

        //!
        //! Start the operation of sending a line of text over the TCP connection.
        //! There is no completion handler because the sent data is kept in a dedicated buffer.
        //! @param [in] line Text line to send. The corresponding memory area is no longer used upon return.
        //! An end-of-line marker is automatically added.
        //! @param [in] flush If false, data are buffered into the ReactiveTelnetConnection and no completion
        //! handler will be called. When true, all previously buffered data are sent with data from this call.
        //! @return True on success, false on error. Success means that the I/O was successfully started.
        //!
        bool startSendLine(const UString& line, bool flush = true);

        //!
        //! Start the operation of sending text over the TCP connection.
        //! There is no completion handler because the sent data is kept in a dedicated buffer.
        //! @param [in] text Text to send. The corresponding memory area is no longer used upon return. No end-of-line marker is added.
        //! @param [in] flush If false, data are buffered into the ReactiveTelnetConnection and no completion
        //! handler will be called. When true, all previously buffered data are sent with data from this call.
        //! @return True on success, false on error. Success means that the I/O was successfully started.
        //!
        bool startSendText(const std::string& text, bool flush = true);

        //!
        //! Start the operation of sending text over the TCP connection.
        //! There is no completion handler because the sent data is kept in a dedicated buffer.
        //! @param [in] text Text to send. The corresponding memory area is no longer used upon return. No end-of-line marker is added.
        //! @param [in] flush If false, data are buffered into the ReactiveTelnetConnection and no completion
        //! handler will be called. When true, all previously buffered data are sent with data from this call.
        //! @return True on success, false on error. Success means that the I/O was successfully started.
        //!
        bool startSendText(const UString& text, bool flush = true);

        //!
        //! Start the operation of receiving messages from the socket.
        //! @param [in] handler Handler class to call each time a text line is received. The method handleTelnetLine() will
        //! be called on each received line. Cannot be null. If a previous receive handler was registered, it is replaced.
        //! @param [in] buffer_size Size of input buffers to receive data.
        //! @return True on success, false on error. Success means that the I/O was successfully started.
        //! The final status of the I/O will be transmitted in the @a handler.
        //!
        bool startReceive(ReactiveTelnetConnectionHandlerInterface* handler, size_t buffer_size = ReactiveTCPConnection::DEFAULT_RECEIVE_BUFFER_SIZE);

    private:
        // The send user-data is a buffer containing the formatted binary data to send.
        // As long as we don't flush, we append in the _unflushed_data buffer.
        // When we flush, the buffer must not change until the I/O is in progress.
        class TSCOREDLL SendUserData: public Object
        {
        public:
            std::string buffer{};
            virtual ~SendUserData() override;
        };
        using SendUserDataPtr = std::shared_ptr<SendUserData>;

        // ReactiveTelnetConnection private fields.
        ReactiveTCPConnection&                    _socket;
        ReactiveTelnetConnectionHandlerInterface* _receive_handler = nullptr;
        SendUserDataPtr                           _unflushed_data {};

        // Get and send a formatted buffer.
        SendUserDataPtr getBuffer(bool flush);
        bool startSendData(SendUserDataPtr& buf, bool eol, bool flush);

        // Inherited methods.
        virtual void handleTCPReceive(ReactiveTCPConnection& sock, const ByteBlock& data, ReactiveTCPInputControl& control, int error_code, const ObjectPtr& user_data) override;
        virtual void handleSocketConnected(TCPConnection& sock) override;
        virtual void handleSocketDisconnected(TCPConnection& sock, bool silent) override;
    };
}
