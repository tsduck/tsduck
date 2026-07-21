//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  TCP connection using TLV messages.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTCPConnection.h"
#include "tstlvProtocol.h"
#include "tstlvMessage.h"
#include "tstlvLogger.h"

namespace ts {
    //!
    //! TCP connection using TLV messages.
    //! @ingroup libtscore net tlv
    //!
    class TSCOREDLL TLVConnection: protected SocketHandlerInterface
    {
        TS_NOBUILD_NOCOPY(TLVConnection);
    public:
        //!
        //! Constructor.
        //! @param [in,out] logger Where to report errors and messages. An internal reference is kept.
        //! The @a logger object must remain valid as long as this object exists.
        //! @param [in] protocol The incoming messages are interpreted according to this protocol. The reference is kept in this object.
        //! @param [in,out] socket Associated TCP socket. The @a socket object must remain valid as long as this object is valid.
        //! @param [in] auto_error_response When an invalid message is received, the corresponding error message is automatically
        //! sent back to the sender when @a auto_error_response is true.
        //! @param [in] max_invalid_msg When non-zero, the connection is automatically disconnected when the number of consecutive
        //! invalid messages has reached this value.
        //!
        TLVConnection(tlv::Logger& logger, const tlv::Protocol& protocol, TCPConnection& socket, bool auto_error_response = true, size_t max_invalid_msg = 0);

        //!
        //! Get a reference to the associated socket.
        //! @return A reference to the associated socket.
        //!
        TCPConnection& socket() { return _socket; }

        //!
        //! Serialize and sendMessage a TLV message.
        //! @param [in] msg The message to send.
        //! @return True on success, false on error.
        //!
        bool sendMessage(const tlv::Message& msg);

        //!
        //! Receive a TLV message.
        //! Wait for the message, deserialize it and validate it.
        //! Process invalid messages and loop until a valid message is received.
        //! @param [out] msg A safe pointer to the received message.
        //! @param [in] abort If non-zero, invoked when I/O is interrupted
        //! (in case of user-interrupt, return, otherwise retry).
        //! @return True on success, false on error.
        //!
        bool receiveMessage(tlv::MessagePtr& msg, const AbortInterface* abort = nullptr);

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

    protected:
        // Inherited methods.
        virtual void handleSocketConnected(TCPConnection& sock) override;

    private:
        tlv::Logger&         _logger;
        const tlv::Protocol& _protocol;
        TCPConnection&       _socket;
        bool                 _auto_error_response = false;
        size_t               _max_invalid_msg = 0;
        size_t               _invalid_msg_count = 0;
        std::mutex           _send_mutex {};     // mutex are used to enforce all bytes of the same message to be
        std::mutex           _receive_mutex {};  // contiguously read/written when several threads access the connection
    };
}
