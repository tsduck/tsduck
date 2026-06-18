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
        TS_NOCOPY(TLSConnection);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        explicit TLSConnection(Report* report = nullptr, Object* owner = nullptr);

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

        // Inherited methods.
        virtual ~TLSConnection() override;
        virtual bool connect(const IPSocketAddress&, IOSB* = nullptr) override;
        virtual bool closeWriter(bool silent = false) override;
        virtual bool disconnect(bool silent = false) override;
        virtual bool send(const void*, size_t, IOSB* = nullptr) override;
        virtual bool receive(void*, size_t, size_t&, const AbortInterface* = nullptr, IOSB* = nullptr) override;
        virtual bool receive(void*, size_t, const AbortInterface* = nullptr) override;

        //!
        //! Get the version of the underlying SSL/TLS library.
        //! @return The library version.
        //!
        static UString GetLibraryVersion();

        // A symbol to reference to force the TLS feature in static link.
        //! @cond nodoxygen
        static const int FEATURE;
        //! @endcond

    private:
        // System-specific parts are stored in a private structure.
        // This is done to avoid inclusion of specialized headers in this public file.
        class SystemGuts;
        SystemGuts* _guts = nullptr;

        // Allocate and deallocate guts (depend on implementations).
        void allocateGuts();
        void deleteGuts();

        // TLSConnection must use blocking I/O. Use ReactiveTLSConnection in reactor environment.
        bool checkBlocking();

        // Pass information from server accepting new clients.
        // The parameter is:
        // - On UNIX systems with OpenSSL, a pointer to ::SSL.
        // - On Windows systems whith SChannel, a pointer to ::CERT_CONTEXT.
        friend class TLSServer;
        bool setServerContext(const void* param);
    };
}
