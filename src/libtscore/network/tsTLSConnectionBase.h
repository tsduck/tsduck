//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Common properties for all forms of SSL/TLS connected sockets.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTLSArgs.h"

namespace ts {
    //!
    //! Common properties for all forms of SSL/TLS connected sockets.
    //! @ingroup libtscore net
    //!
    class TSCOREDLL TLSConnectionBase
    {
        TS_DEFAULT_COPY_MOVE(TLSConnectionBase);
    public:
        //!
        //! Set command line arguments for the client.
        //! @param [in] args TLS arguments.
        //!
        void setArgs(const TLSArgs& args);

        //!
        //! Define if the peer's certificate shall be verified.
        //! @param [in] on If true, the peer's certificate will be verified.
        //!
        void setVerifyPeer(bool on) { _tls_verify_peer = on; }

        //!
        //! Check if the peer's certificate shall be verified.
        //! @return True if the peer's certificate will be verified.
        //!
        bool verifyPeer() const { return _tls_verify_peer; }

        //!
        //! For a client connection, specify the server name to be used in SNI (Server Name Indication).
        //! @param [in] server_name Main server name, as specified in SNI (Server Name Indication).
        //! Also used to verify the server's certificate when setVerifyPeer() is true.
        //!
        void setServerName(const UString& server_name);

        //!
        //! For a client connection, get the server name to be used in SNI (Server Name Indication).
        //! @return A constant reference to the main server name.
        //!
        const UString& serverName() const { return _tls_server_name;  }

        //!
        //! For a client connection, add another accepted host name for the server's certificate verification during connect().
        //! The list is reset by setServerName().
        //! @param [in] name Additional accepted host name used to verify the server's certificate.
        //! @see setServerName()
        //!
        void addVerifyServer(const UString& name);

        //!
        //! For a client connection, get all addition host names for the server's certificate verification during connect().
        //! @return A constant reference to all addition host names.
        //!
        const UStringList& additionalServerNames() const { return _tls_additional_names; }

        //!
        //! Virtual destructor.
        //!
        virtual ~TLSConnectionBase();

    protected:
        //!
        //! Constructor is accessible to subclasses only.
        //! 
        TLSConnectionBase() = default;

    private:
        bool        _tls_verify_peer = true;   //!< Check if the peer's certificate shall be verified.
        UString     _tls_server_name {};       //!< Server name to use in SNI (Server Name Indication).
        UStringList _tls_additional_names {};  //!< Other accepted host names for the server's certificate verification.
    };
}
