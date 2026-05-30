//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------
//!
//!  @file
//!  Encapsulate the SChannel context of TLS connection (Windows-specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReporterBase.h"
#include "tsTLSConnectionBase.h"
#include "tsSChannelBuffer.h"
#include "tsIPProtocols.h"

namespace ts {
    //!
    //! Encapsulate the SChannel context of TLS connection (Windows-specific).
    //! @ingroup libtscore windows
    //!
    //! No I/O is done in this class. Thus, it can be used in blocking and asynchronous
    //! implementation of TLS on Windows.
    //!
    class TSCOREDLL SChannelContext: public ReporterBase
    {
        TS_NOBUILD_NOCOPY(SChannelContext);
    public:
        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //! @param [in] params TLS connection parameters.
        //!
        SChannelContext(ReporterBase* delegate, const TLSConnectionBase& params);

        //!
        //! Destructor.
        //!
        virtual ~SChannelContext();

        //!
        //! Clear the context, free all resources.
        //!
        void reset();

        //!
        //! Initialize the client side of a connection.
        //! @return True on success, false on error.
        //!
        bool initClient();

        //!
        //! Initialize the server side of a connection.
        //! @param [in] cert Pointer to server certificate.
        //! @return True on success, false on error.
        //!
        bool initServer(::PCCERT_CONTEXT cert);

        //!
        //! Check if this is the server-side of the TLS connection.
        //! @return True if this is the server-side of the TLS connection.
        //!
        bool serverSide() const { return _server_side; }

        //!
        //! Send clear user data over the TLS connection.
        //! Most of the time, this results in a TLS protocol packet to be sent.
        //! If the user data are too long, only some of them can be sent in the next TLS protocol packet.
        //! @param [in,out] data Address of the clear user message to sent.
        //! On output, it is updated with the length of the actual part of the message which is ready to send.
        //! @param [in,out] size Siz in bytes of the clear user message to sent.
        //! On output, it is updated with the length of the actual part of the message which is ready to send.
        //! @return True on success, false on error.
        //! @see needSend()
        //!
        bool sendUserData(const void*& data, size_t& size);

        //!
        //! Check if some TLS protocol data must be sent.
        //! The application must send the corresponding data and then call sendCompleted().
        //! @return True if there are some TLS protocol data to send.
        //! @see sendAddress()
        //! @see sendSize()
        //! @see sendCompleted()
        //! 
        bool needSend() const { return _outgoing_size > 0; }

        //!
        //! Get the address of the TLS protocol data to send.
        //! @return The address of the TLS protocol data to send or a null pointer if there is none.
        //! @see needSend()
        //! @see sendSize()
        //! @see sendCompleted()
        //!
        const void* sendAddress() const { return _outgoing_addr; }

        //!
        //! Get the size in bytes of the TLS protocol data to send.
        //! @return The size in bytes of the TLS protocol data to send.
        //! @see needSend()
        //! @see sendAddress()
        //! @see sendCompleted()
        //!
        size_t sendSize() const { return _outgoing_size; }

        //!
        //! Acknowledge that the data to send are fully sent.
        //! @return True on success, false on error.
        //!
        bool sendCompleted();

        //!
        //! Check if more TLS protocol data must be received in order to continue.
        //! @return True if more TLS protocol data must be received in order to continue.
        //!
        bool needReceive() const { return _need_incoming; }

        //!
        //! Get the address of the input buffer for the TLS protocol data to receive.
        //! @return The address of the input buffer.
        //! @see needReceive()
        //! @see receiveSize()
        //! @see receiveCompleted()
        //!
        void* receiveAddress() { return _incoming_buffer + _incoming_size; }

        //!
        //! Get the size in bytes of the input buffer for the TLS protocol data to receive.
        //! @return The size in bytes of the input buffer.
        //! @see needReceive()
        //! @see receiveAddress()
        //! @see receiveCompleted()
        //!
        size_t receiveSize() const { return sizeof(_incoming_buffer) - _incoming_size; }

        //!
        //! Acknowledge the reception of data in the input buffer.
        //! @param [in] received_size Received size in bytes.
        //! @param [in,out] user_data If any clear user data were extracted from the TLS protocol data,
        //! they are appended into @a user_data.
        //! @return True on success, false on error.
        //!
        bool receiveCompleted(size_t received_size, ByteBlock& user_data);

        //!
        //! Check if the input TLS stream is terminated (peer shutdown).
        //! @return True if the input TLS stream is terminated.
        //!
        bool eof() const { return _end_session; }

        //!
        //! Generate a shutdown message to send to the peer.
        //! @return True on success, false on error.
        //! 
        bool initShutdown();

        //!
        //! Check if a TLS shutdown was generated.
        //! @return True if a TLS shutdown was generated.
        //!
        bool shutdowning() const { return _shutdowning; }

    private:
        const TLSConnectionBase& _params;

        // SChannel security stuff.
        ::SecPkgContext_StreamSizes _stream_sizes {};  // Standard sizes of header, trailer, etc.
        ::CredHandle   _credentials {0, 0};       // Handle to our credentials (our side of the connection).
        ::CtxtHandle   _security_context {0, 0};  // Handle to SChannel security context.
        ::ULONG        _security_attributes = 0;  // Context requirements/attributes (security flags).
        bool           _server_side = false;      // We are the server-side endpoint of the connection.
        bool           _renegotiating = false;    // TLS renegotiation in progress.
        bool           _shutdowning = false;      // A shutdown message has been generated.

        // Output management.
        SChannelBuffer _outbuffers {8};           // Output buffers, for protocol data to send to peer.
        const void*    _outgoing_addr = nullptr;  // Address of data to send. Can be in '_outgoing' or in allocated buffers.
        size_t         _outgoing_size = 0;        // Size of data to send.
        bool           _outgoing_free = false;    // The outgoing data are in outbuffers and must be freed using SChannelBuffer::freeContextBuffer().
        ByteBlock      _outgoing {};              // Outging buffer (data only, not negotiation protocol).

        // Input management.
        size_t         _incoming_size = 0;        // Data size in _incoming_buffer (TLS protocol and ciphertext).
        bool           _need_incoming = false;    // Need more incoming data.
        bool           _end_session = false;      // End of input data stream (peer shutdown).
        uint8_t        _incoming_buffer[TLS_MAX_PACKET_SIZE];  // Buffer of incoming data, at most one TLS packet.

        // Free output buffers if necessary.
        void freeOutputBuffer();

        // Continue renegotiation if necessary.
        bool continueRenegotiation();

        // Acquire TLS credentials.
        bool getCredentials(bool verify_peer, ::PCCERT_CONTEXT cert);

        // Format a description string for a SChannel protocol.
        static UString ProtocolToString(::DWORD protocol);

        // Log a level-2 debug message with trace information.
        void debug2(const UChar* title, const ::SecBufferDesc* bufs);

        // Stringify a pointer, with offset in incoming buffer when possible.
        UString debugName(const void* p);
    };
}
