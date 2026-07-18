//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  SSL/TLS connected socket, for data communication.
//!  Can be used as TLS client (using connect() method).
//!  Can be used by TLS server to receive a client connection.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTCPConnection.h"
#include "tsTLSConnectionBase.h"
#include "tsTLSContext.h"

namespace ts {
    //!
    //! SSL/TLS connected socket, for data communication.
    //! @ingroup libtscore net
    //!
    //! This class is used in two contexts:
    //! - A TLS client creates a TLSConnection instance and @e connects to a server.
    //! - A TLS server creates a TLSServer instance and @e waits for clients. For each
    //!   client session, a TLSConnection instance is created.
    //!
    //! Possible public servers to test various invalid certificates:
    //! - https://expired.badssl.com/
    //! - https://wrong.host.badssl.com/
    //! - https://self-signed.badssl.com/
    //! - https://untrusted-root.badssl.com/
    //! - https://revoked.badssl.com/
    //! - https://pinning-test.badssl.com/
    //! - see more details at https://badssl.com/
    //!
    class TSCOREDLL TLSConnection: public TCPConnection, public TLSConnectionBase
    {
        TS_NOBUILD_NOCOPY(TLSConnection);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        explicit TLSConnection(Report* report, Object* owner = nullptr);

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        explicit TLSConnection(ReporterBase* delegate, Object* owner = nullptr);

        //!
        //! Constructor with initial client arguments.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //! @param [in] args Initial TLS client arguments.
        //!
        TLSConnection(Report* report, const TLSArgs& args) : TLSConnection(report) { setArgs(args); }

        //!
        //! Constructor with initial client arguments.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //! @param [in] args Initial TLS client arguments.
        //!
        TLSConnection(ReporterBase* delegate, const TLSArgs& args) : TLSConnection(delegate) { setArgs(args); }

        // Inherited methods from TCPConnection.
        virtual bool connect(const IPSocketAddress&, IOSB* = nullptr) override;
        virtual bool closeWriter(bool silent = false) override;
        virtual bool disconnect(bool silent = false) override;

        // Implementation of StreamInterface.
        virtual bool readStream(void* addr, size_t size, const AbortInterface* abort = nullptr) override;
        virtual bool readStream(void* addr, size_t max_size, size_t& ret_size, const AbortInterface* abort = nullptr, IOSB* iosb = nullptr) override;
        virtual bool writeStream(const void* addr, size_t size, IOSB* iosb = nullptr) override;
        virtual bool writeStream(const void* addr, size_t size, size_t& written_size, IOSB* iosb = nullptr) override;

    private:
        TLSContext _sctx {this, this};
        ByteBlock  _tls_data {};          // Incoming TLS data which cannot be processed now.
        ByteBlock  _clear_data {};        // Incoming clear data.
        size_t     _tls_data_next = 0;    // Next index to consume from _recv_tls_data.
        size_t     _clear_data_next = 0;  // Next index to consume from _recv_clear_data.

        // TLSConnection must use blocking I/O. Use ReactiveTLSConnection in reactor environment.
        bool checkBlocking();

        // Process some incoming TLS data, wait for network data if necessary.
        bool flushInput();

        // Perform all required send and receive operations for TLS protocol.
        bool flushSession();

        // Pass information from server accepting new clients.
        // The parameter is:
        // - On UNIX systems with OpenSSL, a pointer to ::SSL_CTX.
        // - On Windows systems whith SChannel, a pointer to ::CERT_CONTEXT.
        friend class TLSServer;
        bool setServerContext(void* param);
    };
}
