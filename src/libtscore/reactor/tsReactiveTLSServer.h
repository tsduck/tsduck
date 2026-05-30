//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  SSL/TLS server socket for use in a Reactor environment
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReactiveTCPServer.h"
#include "tsReactiveTLSConnection.h"
#include "tsTLSServerBase.h"

namespace ts {
    //!
    //! SSL/TLS server socket for use in a Reactor environment.
    //! @ingroup libtscore reactor
    //!
    //! The class ReactiveTLSServer is a wrapper around TCPServer to handle reactive I/O.
    //!
    //! The actual server socket is a separate object. It is initialized and configured by the application.
    //! The application shall not directly call accept(), or close() on this socket and delegate
    //! these operations to startAccept() and startClose() in class ReactiveTLSServer.
    //!
    class TSCOREDLL ReactiveTLSServer: public ReactiveTCPServer, public TLSServerBase, protected ReactiveTCPServerHandlerInterface
    {
        TS_NOBUILD_NOCOPY(ReactiveTLSServer);
    public:
        //!
        //! Constructor.
        //! @param [in,out] reactor Associated reactor. The reactor object must remain valid as long as this object is valid.
        //! @param [in,out] socket Associated server socket. The socket object must remain valid as long as this object is valid.
        //! The ReactiveTLSServer must be initialized before the @a socket is opened.
        //! Important: @a socket must be an instance of TCPServer, not an instance of TLSServer.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        ReactiveTLSServer(Reactor& reactor, TCPServer& socket, Object* owner = nullptr);

        //!
        //! Constructor with initial arguments.
        //! @param [in,out] reactor Associated reactor. The reactor object must remain valid as long as this object is valid.
        //! @param [in,out] socket Associated server socket. The socket object must remain valid as long as this object is valid.
        //! The ReactiveTLSServer must be initialized before the @a socket is opened.
        //! Important: @a socket must be an instance of TCPServer, not an instance of TLSServer.
        //! @param [in] args Initial TLS arguments.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        ReactiveTLSServer(Reactor& reactor, TCPServer& socket, const TLSArgs& args, Object* owner = nullptr);

        // Inherited methods.
        virtual ~ReactiveTLSServer() override;
        bool startAccept(ReactiveTCPServerHandlerInterface* handler, ReactiveTCPConnection& client, const ObjectPtr& user_data = ObjectPtr()) override;

    protected:
        // Inherited methods.
        virtual void handleTCPClientAccepted(ReactiveTCPServer& server, ReactiveTCPConnection& sock, const IPSocketAddress& addr, int error_code, const ObjectPtr& user_data) override;

    private:
        // System-specific parts are stored in a private structure.
        // This is done to avoid inclusion of specialized headers in this public file.
        class SystemGuts;
        SystemGuts* _guts = nullptr;

        // Allocate and deallocate guts (depend on implementations).
        void allocateGuts();
        void deleteGuts();

        // System-specific TLS initialization, called at the beginning of each startAccept().
        bool initTLS(ReactiveTLSConnection& client);

        // On server side, while performing the initial handshake, an accept request is carried in an object.
        class TSCOREDLL AcceptRequest: public Object
        {
        public:
            virtual ~AcceptRequest() override;
            HandlerType* handler = nullptr;
            ObjectPtr    user_data {};
        };
        using AcceptRequestPtr = std::shared_ptr<AcceptRequest>;
    };
}
