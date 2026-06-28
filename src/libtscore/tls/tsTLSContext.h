//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------
//!
//!  @file
//!  Context of a TLS connection.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReporterBase.h"
#include "tsTLSConnectionBase.h"

namespace ts {
    //!
    //! Context of a TLS connection.
    //! This is state machine working on pure data, non-blocking, without network access.
    //! @ingroup libtscore windows
    //!
    class TSCOREDLL TLSContext: public ReporterBase
    {
        TS_NOBUILD_NOCOPY(TLSContext);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        explicit TLSContext(Report* report, Object* owner = nullptr);

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        explicit TLSContext(ReporterBase* delegate, Object* owner = nullptr);

        //!
        //! Destructor.
        //!
        virtual ~TLSContext() override;

        //!
        //! Clear the context, free all resources.
        //!
        void reset();

        //!
        //! Initialize the client side of a connection.
        //! @param [in] params TLS connection parameters.
        //! @return True on success, false on error.
        //!
        bool initClient(const TLSConnectionBase& params);

        //!
        //! Initialize the server side of a connection.
        //! @param [in] cert Pointer to server certificate.
        //! - On UNIX systems with OpenSSL, a pointer to SSL_CTX.
        //! - On Windows systems whith SChannel, a pointer to CERT_CONTEXT.
        //! @return True on success, false on error.
        //!
        bool initServer(void* cert);

        //!
        //! Check if this is the server-side of the TLS connection.
        //! @return True if this is the server-side of the TLS connection.
        //!
        bool serverSide() const { return _server_side; }

        //!
        //! Send clear user data over the TLS connection.
        //! Most of the time, this results in a TLS protocol packet to be sent.
        //! If the user data are too long, only some of them can be sent in the next TLS protocol packet.
        //! @param [in] data Address of the clear user message to sent.
        //! @param [in] size Size in bytes of the clear user message to sent.
        //! @param [out] ret_size Size of data which where consumed. If @a ret_size is less than @a size, the rest shall be submitted later.
        //! @return True on success, false on error.
        //!
        bool provideClearData(const void* data, size_t size, size_t& ret_size);

        //!
        //! Check if some TLS protocol data must be sent.
        //! The application must send the corresponding data and then call sendCompleted().
        //! @return True if there are some TLS protocol data to send.
        //!
        bool needSend() const { return getDataSizeToSend() > 0; }

        //!
        //! Get the size in bytes of TLS protocol data which are ready to send.
        //! @return Size in bytes of TLS protocol data which are ready to send.
        //!
        size_t getDataSizeToSend() const;

        //!
        //! Get TLS protocol data to send.
        //! @param [in,out] tls_data Binary data to send over the wire are appended into @a tls_data.
        //! @return True on success, false on error.
        //!
        bool getDataToSend(ByteBlock& tls_data);

        //!
        //! Check if more TLS protocol data must be received in order to continue.
        //! @return True if more TLS protocol data must be received in order to continue.
        //!
        bool needReceive() const { return _need_receive; }

        //!
        //! Provide received TLS protocol data, collect clear data.
        //! If the received data are too long, only some of them can be processed.
        //! In that case, it is possible that some data must be sent before accepting the rest of the received data.
        //! @param [in] data Address of the received data.
        //! @param [in] size Size in bytes of the received data.
        //! @param [out] ret_size Size of data which where consumed. If @a ret_size is less than @a size, the rest shall be submitted later.
        //! @param [in,out] clear_data If any clear user data were extracted from the TLS protocol data, they are appended into @a clear_data.
        //! @return True on success, false on error.
        //!
        bool provideReceivedData(const void* data, size_t size, size_t& ret_size, ByteBlock& clear_data);

        //!
        //! Check if the input TLS stream is terminated (peer shutdown).
        //! @return True if the input TLS stream is terminated.
        //!
        bool eof() const { return _end_session; }

        //!
        //! Generate a shutdown message to send to the peer.
        //! @param [in] silent If true, do not report errors through the logger. This is typically useful when the socket
        //! is in some error condition and closing it is necessary although it may generate additional meaningless errors.
        //! @return True on success, false on error.
        //!
        bool initShutdown(bool silent);

        //!
        //! Check if a TLS shutdown was generated.
        //! @return True if a TLS shutdown was generated.
        //!
        bool shutdowning() const { return _shutdowning; }

        //!
        //! Get the version of the underlying SSL/TLS library.
        //! @return The library version.
        //!
        static UString GetLibraryVersion();

    private:
        bool _server_side = false;    // We are the server-side endpoint of the connection.
        bool _need_receive = false;   // The SSL context needs input data.
        bool _shutdowning = false;    // A shutdown message has been generated.
        bool _end_session = false;    // End of input data stream (peer shutdown).
        bool _renegotiating = false;  // TLS renegotiation in progress.

        // System-specific parts are stored in a private structure.
        // This is done to avoid inclusion of specialized headers in this public file.
        class SystemGuts;
        SystemGuts* _guts = nullptr;

        // Continue renegotiation if necessary.
        bool continueRenegotiation();

        // Issue a debug message with various context info.
        template <class... Args>
        void debug(const UChar* fmt, Args&&... args)
        {
            debug(Severity::Debug, fmt, std::forward<ArgMixIn>(args)...);
        }

        // Issue a debug message with various context info and explicit severity.
        template <class... Args>
        void debug(int severity, const UChar* fmt, Args&&... args)
        {
            if (report().maxSeverity() >= severity) {
                report().log(severity, u"TLS context (%s): %s, %d bytes to send, need receive: %s",
                             _server_side ? u"server" : u"client",
                             UString::Format(fmt, std::forward<ArgMixIn>(args)...),
                             getDataSizeToSend(), needReceive());
            }
        }

        // Allocate and deallocate guts (depend on implementations).
        void allocateGuts();
        void deleteGuts();
    };
}
