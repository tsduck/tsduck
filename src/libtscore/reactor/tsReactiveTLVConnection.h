//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  TLV-messages connection for use in a Reactor environment
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReactiveTLVConnectionHandlerInterface.h"
#include "tsReactiveTCPConnection.h"
#include "tstlvMessage.h"
#include "tstlvLogger.h"

namespace ts {
    //!
    //! TLV-messages connection for use in a Reactor environment
    //! @ingroup libtscore reactor
    //!
    //! The class ReactiveTLVConnection is a wrapper around ReactiveTCPConnection to handle reactive I/O.
    //!
    class TSCOREDLL ReactiveTLVConnection: private ReactiveTCPConnectionHandlerInterface
    {
        TS_NOBUILD_NOCOPY(ReactiveTLVConnection);
    public:
        //!
        //! Constructor.
        //! @param [in,out] logger Where to report errors and messages. An internal reference is kept.
        //! The @a logger object must remain valid as long as this object exists.
        //! @param [in] protocol The incoming messages are interpreted according to this protocol. The reference is kept in this object.
        //! @param [in,out] socket Associated reactive TCP socket. The @a socket object must remain valid as long as this object is valid.
        //! @param [in] auto_error_response When an invalid message is received, the corresponding error message is automatically
        //! sent back to the sender when @a auto_error_response is true.
        //! @param [in] max_invalid_msg When non-zero, the connection is automatically disconnected when the number of consecutive
        //! invalid messages has reached this value.
        //!
        ReactiveTLVConnection(tlv::Logger& logger, const tlv::Protocol& protocol, ReactiveTCPConnection& socket, bool auto_error_response = true, size_t max_invalid_msg = 0);

        //!
        //! Destructor.
        //!
        virtual ~ReactiveTLVConnection() override;

        //!
        //! Get a reference to the associated socket.
        //! @return A reference to the associated socket.
        //!
        ReactiveTCPConnection& socket() { return _socket; }

        //!
        //! Start the operation of sending a message over the TCP connection.
        //! There is no completion handler because the serialized data to send is kept in a dedicated buffer.
        //! @param [in] msg The message to send.
        //! @return True on success, false on error. Success means that the I/O was successfully started.
        //!
        bool startSendMessage(const tlv::Message& msg);

        //!
        //! Start the operation of receiving messages from the socket.
        //! @param [in] handler Handler class to call each time a message is received. Cannot be null.
        //! If a previous receive handler was registered, it is replaced.
        //! @param [in] buffer_size Size of input buffers to receive data. This is a tuning parameter only.
        //! It is automatically increased when necessary.
        //! @return True on success, false on error. Success means that the I/O was successfully started.
        //! The final status of the I/O will be transmitted in the @a handler.
        //!
        bool startReceive(ReactiveTLVConnectionHandlerInterface* handler, size_t buffer_size = ReactiveTCPConnection::DEFAULT_RECEIVE_BUFFER_SIZE);

        //!
        //! Get invalid incoming messages processing.
        //! @return True if, when an invalid message is received, the corresponding
        //! error message is automatically sent back to the sender.
        //!
        bool getAutoErrorResponse() const { return _auto_error_response; }

        //!
        //! Set invalid incoming messages processing.
        //! @param [in] on When an invalid message is received, the corresponding
        //! error message is automatically sent back to the sender when @a on is true.
        //!
        void setAutoErrorResponse(bool on) { _auto_error_response = on; }

        //!
        //! Get invalid message threshold.
        //! @return When non-zero, the connection is automatically disconnected
        //! when the number of consecutive invalid messages has reached this value.
        //!
        size_t getMaxInvalidMessages() const { return _max_invalid_msg; }

        //!
        //! Set invalid message threshold.
        //! @param [in] n When non-zero, the connection is automatically disconnected
        //! when the number of consecutive invalid messages has reached this value.
        //!
        void setMaxInvalidMessages(size_t n) { _max_invalid_msg = n; }

    private:
        // The send user-data is a buffer containing the formatted binary data to send.
        class TSCOREDLL SendUserData: public Object
        {
        public:
            ByteBlockPtr buffer{};
            virtual ~SendUserData() override;
        };
        using SendUserDataPtr = std::shared_ptr<SendUserData>;

        // ReactiveTextConnection private fields.
        tlv::Logger&           _logger;
        const tlv::Protocol&   _protocol;
        ReactiveTCPConnection& _socket;
        bool                   _auto_error_response = false;
        size_t                 _max_invalid_msg = 0;
        size_t                 _invalid_msg_count = 0;
        ReactiveTLVConnectionHandlerInterface* _receive_handler = nullptr;

        // Inherited methods.
        virtual void handleReadStream(ReactiveStream& stream, const ByteBlock& data, ReactiveInputControl& control, int error_code, const ObjectPtr& user_data) override;
    };
}
