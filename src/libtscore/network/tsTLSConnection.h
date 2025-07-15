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
    class TSCOREDLL TLSConnection: public TCPConnection
    {
        TS_NOCOPY(TLSConnection);
    public:
        //!
        //! Reference to the superclass.
        //!
        using SuperClass = TCPConnection;

        //!
        //! Constructor
        //!
        TLSConnection();

        //!
        //! For a client connection, specify if the server's certificate is verified during connect().
        //! @param [in] verify If true (the default), the server's certificate is verified during
        //! connact() and the connection fails if the certificate is invalid (e.g. auto-signed or
        //! not issued by a trusted CA).
        //!
        //! LIMITATION: Currently, only the validity of the server certificate is verified.
        //! The server name is not verified yet. To be implemented later. Currently, TLS is
        //! only a way to encrypt TCP sessions in TSDuck. It does not prevent MitM attacks.
        //!
        void setVerifyServer(bool verify) { _verify_server = verify; }

        //!
        //! For a client connection, check if the server's certificate is verified during connect().
        //! @return True if the server's certificate is verified during connact().
        //! @see setVerifyServer()
        //!
        bool getVerifyServer() const { return _verify_server; }

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
        bool _verify_server = true;

        // Allocate and deallocate guts (depend on implementations).
        void allocateGuts();
        void deleteGuts();

        // Pass information from server accepting new clients.
        friend class TLSServer;
    #if defined(TS_WINDOWS)
        //@@@
    #else
        // The parameter is an SSL*.
        void setServerContext(void* ssl);
    #endif
    };
}
