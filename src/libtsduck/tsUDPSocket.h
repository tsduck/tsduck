//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  UDP Socket
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSocket.h"
#include "tsSocketAddress.h"
#include "tsAbortInterface.h"
#include "tsReport.h"
#include "tsMemoryUtils.h"

namespace ts {
    //!
    //! UDP Socket.
    //!
    class TSDUCKDLL UDPSocket: public Socket
    {
    public:
        //!
        //! Constructor.
        //! @param [in] auto_open If true, call open() immediately.
        //! @param [in,out] report Where to report error.
        //!
        UDPSocket(bool auto_open = false, Report& report = CERR);

        //!
        //! Destructor.
        //!
        virtual ~UDPSocket();

        //!
        //! Bind to a local address and port.
        //!
        //! The IP address part of the socket address must one of:
        //! - @link IPAddress::AnyAddress @endlink. Any local interface may be used
        //!   to send or receive UDP datagrams. For each outgoing packet, the actual
        //!   interface is selected by the kernel based on the routing rules. Incoming
        //!   UDP packets for the selected port will be accepted from any local interface.
        //! - The IP address of an interface of the local system. Outgoing packets will be
        //!   unconditionally sent through this interface. Incoming UDP packets for the
        //!   selected port will be accepted only when they arrive through the selected
        //!   interface.
        //!
        //! The port number part of the socket address must be one of:
        //! - @link SocketAddress::AnyPort @endlink. The socket is bound to an
        //!   arbitrary unused local UDP port.
        //! - A specific port number. If this UDP port is already bound by another
        //!   local UDP socket, the bind operation fails, unless the "reuse port"
        //!   option has already been set.
        //!
        //! @param [in] addr Local socket address to bind to.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool bind(const SocketAddress& addr, Report& report = CERR);

        //!
        //! Set a default destination address and port for outgoing messages.
        //!
        //! There are two versions of the send() method. One of them explicitely
        //! specifies the destination of the packet to send. The second version
        //! does not specify a destination; the packet is sent to the <i>default
        //! destination</i>.
        //!
        //! @param [in] addr Socket address of the destination.
        //! Both address and port are mandatory in the socket address, they cannot
        //! be set to @link IPAddress::AnyAddress @endlink or
        //! @link SocketAddress::AnyPort @endlink.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool setDefaultDestination(const SocketAddress& addr, Report& report = CERR);

        //!
        //! Set a default destination address and port for outgoing messages.
        //!
        //! There are two versions of the send() method. One of them explicitely
        //! specifies the destination of the packet to send. The second version
        //! does not specify a destination; the packet is sent to the <i>default
        //! destination</i>.
        //!
        //! @param [in] name A string describing the socket address of the destination.
        //! See @link SocketAddress::resolve() @endlink for a description of the expected string format.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool setDefaultDestination(const UString& name, Report& report = CERR);

        //!
        //! Get the default destination address and port for outgoing messages.
        //! @return The default destination address and port for outgoing messages.
        //!
        SocketAddress getDefaultDestination() const {return _default_destination;}

        //!
        //! Set the outgoing local interface for multicast messages.
        //!
        //! @param [in] addr IP address of a local interface.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool setOutgoingMulticast(const IPAddress& addr, Report& report = CERR);

        //!
        //! Set the outgoing local interface for multicast messages.
        //!
        //! @param [in] name A string describing the IP address of a local interface.
        //! See @link IPAddress::resolve() @endlink for a description of the expected string format.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool setOutgoingMulticast(const UString& name, Report& report = CERR);

        //!
        //! Set the Time To Live (TTL) option.
        //!
        //! @param [in] ttl The TTL value, ie. the maximum number of "hops" between
        //! routers before an IP packet is dropped.
        //! @param [in] multicast When true, set the <i>multicast TTL</i> option.
        //! When false, set the <i>unicast TTL</i> option.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool setTTL(int ttl, bool multicast, Report& report = CERR);

        //!
        //! Set the Time To Live (TTL) option.
        //!
        //! If the <i>default destination</i> is a multicast address, set the
        //! <i>multicast TTL</i> option. Otherwise, set the <i>unicast TTL</i> option.
        //!
        //! @param [in] ttl The TTL value, ie. the maximum number of "hops" between
        //! routers before an IP packet is dropped.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool setTTL(int ttl, Report& report = CERR)
        {
            return setTTL(ttl, _default_destination.isMulticast(), report);
        }

        //!
        //! Join a multicast group.
        //!
        //! This method indicates that the application wishes to receive multicast
        //! packets which are sent to a specific multicast address.
        //!
        //! @param [in] multicast Multicast IP address to listen to.
        //! @param [in] local IP address of a local interface on which to listen.
        //! If set to @link IPAddress::AnyAddress @endlink, the applications
        //! listens on all local interfaces.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool addMembership(const IPAddress& multicast, const IPAddress& local, Report& report = CERR);

        //!
        //! Join a multicast group.
        //!
        //! This method indicates that the application wishes to receive multicast
        //! packets which are sent to a specific multicast address.
        //!
        //! Using this version of addMembership(), the applications listens on all
        //! local interfaces.
        //!
        //! @param [in] multicast Multicast IP address to listen to.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool addMembership(const IPAddress& multicast, Report& report = CERR);

        //!
        //! Drop all multicast membership requests.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool dropMembership(Report& report = CERR);

        //!
        //! Send a message to a destination address and port.
        //!
        //! @param [in] data Address of the message to send.
        //! @param [in] size Size in bytes of the message to send.
        //! @param [in] destination Socket address of the destination.
        //! Both address and port are mandatory in the socket address, they cannot
        //! be set to @link IPAddress::AnyAddress @endlink or
        //! @link SocketAddress::AnyPort @endlink.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool send(const void* data, size_t size, const SocketAddress& destination, Report& report = CERR);

        //!
        //! Send a message to the default destination address and port.
        //!
        //! @param [in] data Address of the message to send.
        //! @param [in] size Size in bytes of the message to send.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool send(const void* data, size_t size, Report& report = CERR)
        {
            return send(data, size, _default_destination, report);
        }

        //!
        //! Receive a message.
        //!
        //! @param [out] data Address of the buffer for the received message.
        //! @param [in] max_size Size in bytes of the reception buffer.
        //! @param [out] ret_size Size in bytes of the received message.
        //! Will never be larger than @a max_size.
        //! @param [out] sender Socket address of the sender.
        //! @param [out] destination Socket address of the packet destination.
        //! Can be useful to check in multicast packets.
        //! @param [in] abort If non-zero, invoked when I/O is interrupted
        //! (in case of user-interrupt, return, otherwise retry).
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool receive(void* data,
                     size_t max_size,
                     size_t& ret_size,
                     SocketAddress& sender,
                     SocketAddress& destination,
                     const AbortInterface* abort = 0,
                     Report& report = CERR);

        // Implementation of Socket interface.
        virtual bool open(Report& report = CERR) override;
        virtual bool close(Report& report = CERR) override;

    private:
        // Encapsulate an ::ip_mreq
        struct MReq {
            // Encapsulated structure
            ::ip_mreq req;

            // Constructor
            MReq() : req()
            {
                TS_ZERO(req);
            }

            // Constructor
            MReq(const IPAddress& multicast_, const IPAddress& interface_) : req()
            {
                TS_ZERO(req);
                multicast_.copy(req.imr_multiaddr);
                interface_.copy(req.imr_interface);
            }

            // Comparator for containers, no real semantic
            bool operator<(const MReq& other) const
            {
                return ::memcmp(&req, &other.req, sizeof(req)) < 0;
            }
        };
        typedef std::set<MReq> MReqSet;

        // Private members
        SocketAddress _local_address;
        SocketAddress _default_destination;
        MReqSet       _mcast; // Current list of multicast memberships

        // Perform one receive operation. Hide the system mud.
        SocketErrorCode receiveOne(void* data, size_t max_size, size_t& ret_size, SocketAddress& sender, SocketAddress& destination, Report& report);

        // Furiously idiotic Windows feature, see comment in receiveOne()
#if defined(TS_WINDOWS)
        static volatile ::LPFN_WSARECVMSG _wsaRevcMsg;
#endif

        // Unreachable operations
        UDPSocket(const UDPSocket&) = delete;
        UDPSocket& operator=(const UDPSocket&) = delete;
    };
}
