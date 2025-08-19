//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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
#include "tsTLSArgs.h"

namespace ts {
    //!
    //! Base class for a TLS session.
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
    class TSCOREDLL TLSConnection: public TCPConnection
    {
        TS_NOCOPY(TLSConnection);
    public:
        //!
        //! Reference to the superclass.
        //!
        using SuperClass = TCPConnection;

        //!
        //! Constructor.
        //!
        TLSConnection();

        //!
        //! Constructor with initial client arguments.
        //! @param [in] args Initial TLS client arguments.
        //!
        TLSConnection(const TLSArgs& args) : TLSConnection() { setArgs(args); }

        //!
        //! Set command line arguments for the client.
        //! @param [in] args TLS arguments.
        //!
        void setArgs(const TLSArgs& args);

        //!
        //! Check if the peer's certificate shall be verified.
        //! @param [in] on If true, the peer's certificate will be verified.
        //!
        void setVerifyPeer(bool on) { _verify_peer = on; }

        //!
        //! For a client connection, specify the server name to be used in SNI (Server Name Indication).
        //! @param [in] server_name Main server name, as specified in SNI (Server Name Indication).
        //! Also used to verify the server's certificate when setVerifyPeer() is true.
        //!
        void setServerName(const UString& server_name);

        //!
        //! For a client connection, add another accepted host name for the server's certificate verification during connect().
        //! The list is reset by setVerifyServer().
        //! @param [in] name Additional accepted host name used to verify the server's certificate.
        //! @see setVerifyServer()
        //!
        void addVerifyServer(const UString& name);

        // Inherited methods.
        virtual ~TLSConnection() override;
        virtual bool connect(const IPSocketAddress&, Report& = CERR) override;
        virtual bool closeWriter(Report& = CERR) override;
        virtual bool disconnect(Report& = CERR) override;
        virtual bool send(const void*, size_t, Report& = CERR) override;
        virtual bool receive(void*, size_t, size_t&, const AbortInterface* = nullptr, Report& = CERR) override;
        virtual bool receive(void*, size_t, const AbortInterface* = nullptr, Report& = CERR) override;

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

        // Common properties.
        bool        _verify_peer = false;
        UString     _server_name {};
        UStringList _additional_names {};

        // Allocate and deallocate guts (depend on implementations).
        void allocateGuts();
        void deleteGuts();

        // Pass information from server accepting new clients.
        // The parameter is:
        // - On UNIX systems with OpenSSL, a pointer to ::SSL.
        // - On Windows systems whith SChannel, a pointer to ::CERT_CONTEXT.
        friend class TLSServer;
        bool setServerContext(const void* param, Report& report);
    };
}
