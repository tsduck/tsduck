//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  SSL/TLS connected socket for use in a Reactor environment
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReactiveTCPConnection.h"
#include "tsTLSConnectionBase.h"
#include "tsTLSArgs.h"

namespace ts {

    class ReactiveTLSServer;
    class ReactiveTCPServerHandlerInterface;

    //!
    //! SSL/TLS connected socket for use in a Reactor environment.
    //! @ingroup libtscore reactor
    //!
    //! The class ReactiveTLSConnection is a wrapper around TCPConnection to handle reactive I/O.
    //!
    //! The actual socket is a separate object. It is initialized and configured by the application.
    //! The application shall not directly call connect(), send(), receive(), closeWriter(), or close() on this socket and
    //! delegate these operations to startConnect(), startSend(), startReceive(), startCloseWriter() and startClose() in
    //! class ReactiveTLSConnection.
    //!
    class TSCOREDLL ReactiveTLSConnection: public ReactiveTCPConnection, public TLSConnectionBase
    {
        TS_NOBUILD_NOCOPY(ReactiveTLSConnection);
    public:
        //!
        //! Constructor.
        //! @param [in,out] reactor Associated reactor. The reactor object must remain valid as long as this object is valid.
        //! @param [in,out] socket Associated socket. The socket object must remain valid as long as this object is valid.
        //! The ReactiveTLSConnection must be initialized before the @a socket is opened.
        //! Important: @a socket must be an instance of TCPConnection, not an instance of TLSConnection.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        ReactiveTLSConnection(Reactor& reactor, TCPConnection& socket, Object* owner = nullptr);

        //!
        //! Constructor with initial client arguments.
        //! @param [in,out] reactor Associated reactor. The reactor object must remain valid as long as this object is valid.
        //! @param [in,out] socket Associated socket. The socket object must remain valid as long as this object is valid.
        //! The ReactiveTLSConnection must be initialized before the @a socket is opened.
        //! Important: @a socket must be an instance of TCPConnection, not an instance of TLSConnection.
        //! @param [in] args Initial TLS client arguments.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        ReactiveTLSConnection(Reactor& reactor, TCPConnection& socket, const TLSArgs& args, Object* owner = nullptr);

        // Inherited methods.
        virtual ~ReactiveTLSConnection() override;
        virtual bool startConnect(ReactiveTCPConnectionHandlerInterface* handler, const IPSocketAddress& addr, const ObjectPtr& user_data = ObjectPtr()) override;
        virtual void whenAccepted(ReactiveTCPConnectionHandlerInterface* handler, const ObjectPtr& user_data = ObjectPtr()) override;
        virtual bool startSend(ReactiveTCPConnectionHandlerInterface* handler, const void* data, size_t size, const ObjectPtr& user_data = ObjectPtr()) override;
        virtual bool startCloseWriter(ReactiveTCPConnectionHandlerInterface* handler, bool silent = false, const ObjectPtr& user_data = ObjectPtr()) override;
        virtual bool startReceive(ReactiveTCPConnectionHandlerInterface* handler, size_t buffer_size = DEFAULT_RECEIVE_BUFFER_SIZE, const ObjectPtr& user_data = ObjectPtr()) override;
        virtual void cancelSendReceive(bool silent = false) override;
        virtual bool startClose(ReactiveTCPConnectionHandlerInterface* handler, bool silent = false, const ObjectPtr& user_data = ObjectPtr()) override;

    private:
        // System-specific parts are stored in a private structure.
        // This is done to avoid inclusion of specialized headers in this public file.
        class SystemGuts;
        SystemGuts* _guts = nullptr;

        // Allocate and deallocate guts (depend on implementations).
        void allocateGuts();
        void deleteGuts();

        // Pass information from server accepting new clients.
        // This information will be used when a connection is accepted, to start TLS handshake.
        // The parameter is:
        // - On UNIX systems with OpenSSL, a pointer to ::SSL.
        // - On Windows systems whith SChannel, a pointer to ::CERT_CONTEXT.
        friend class ReactiveTLSServer;
        bool initServerContext(ReactiveTLSServer* server, const void* param);
    };
}
