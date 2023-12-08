//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  UDP Socket
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSocket.h"
#include "tsIPv4SocketAddress.h"
#include "tsIPUtils.h"
#include "tsAbortInterface.h"
#include "tsReport.h"
#include "tsMemory.h"

#if defined(DOXYGEN) || defined(TS_OPENBSD) || defined(TS_NETBSD) || defined(TS_DRAGONFLYBSD)
    //!
    //! Defined when the operating system does not support UDP/IP source-specific multicast (SSM).
    //!
    #define TS_NO_SSM 1
#endif

namespace ts {
    //!
    //! UDP Socket.
    //! @ingroup net
    //!
    class TSDUCKDLL UDPSocket: public Socket
    {
        TS_NOCOPY(UDPSocket);
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
        virtual ~UDPSocket() override;

        //!
        //! Bind to a local address and port.
        //!
        //! The IP address part of the socket address must one of:
        //! - IPv4Address::AnyAddress. Any local interface may be used
        //!   to send or receive UDP datagrams. For each outgoing packet, the actual
        //!   interface is selected by the kernel based on the routing rules. Incoming
        //!   UDP packets for the selected port will be accepted from any local interface.
        //! - The IP address of an interface of the local system. Outgoing packets will be
        //!   unconditionally sent through this interface. Incoming UDP packets for the
        //!   selected port will be accepted only when they arrive through the selected
        //!   interface.
        //!
        //! Special note for receiving multicast on most Unix systems (at least Linux
        //! and macOS): The IP address shall be either AnyAddress or the <b>multicast
        //! group address</b>. Do not specify a local address to receive multicast on Unix.
        //!
        //! The port number part of the socket address must be one of:
        //! - IPv4SocketAddress::AnyPort. The socket is bound to an
        //!   arbitrary unused local UDP port.
        //! - A specific port number. If this UDP port is already bound by another
        //!   local UDP socket, the bind operation fails, unless the "reuse port"
        //!   option has already been set.
        //!
        //! @param [in] addr Local socket address to bind to.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool bind(const IPv4SocketAddress& addr, Report& report = CERR);

        //!
        //! Set a default destination address and port for outgoing messages.
        //!
        //! There are two versions of the send() method. One of them explicitly
        //! specifies the destination of the packet to send. The second version
        //! does not specify a destination; the packet is sent to the <i>default
        //! destination</i>.
        //!
        //! @param [in] addr Socket address of the destination.
        //! Both address and port are mandatory in the socket address, they cannot
        //! be set to IPv4Address::AnyAddress or IPv4SocketAddress::AnyPort.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool setDefaultDestination(const IPv4SocketAddress& addr, Report& report = CERR);

        //!
        //! Set a default destination address and port for outgoing messages.
        //!
        //! There are two versions of the send() method. One of them explicitly
        //! specifies the destination of the packet to send. The second version
        //! does not specify a destination; the packet is sent to the <i>default
        //! destination</i>.
        //!
        //! @param [in] name A string describing the socket address of the destination.
        //! See IPv4SocketAddress::resolve() for a description of the expected string format.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool setDefaultDestination(const UString& name, Report& report = CERR);

        //!
        //! Get the default destination address and port for outgoing messages.
        //! @return The default destination address and port for outgoing messages.
        //!
        IPv4SocketAddress getDefaultDestination() const {return _default_destination;}

        //!
        //! Set the outgoing local interface for multicast messages.
        //!
        //! @param [in] addr IP address of a local interface.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool setOutgoingMulticast(const IPv4Address& addr, Report& report = CERR);

        //!
        //! Set the outgoing local interface for multicast messages.
        //!
        //! @param [in] name A string describing the IP address of a local interface.
        //! See IPv4Address::resolve() for a description of the expected string format.
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
        //! Set the Type Of Service (TOS) option.
        //!
        //! Note that correct support for this option depends on the operating
        //! system. Typically, it never worked correctly on Windows.
        //!
        //! @param [in] tos The TOS value.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool setTOS(int tos, Report& report = CERR);

        //!
        //! Set the multicast loop option.
        //!
        //! By default, the multicast packets are looped back on local interfaces.
        //! Use this to disable multicast loopback.
        //!
        //! @param [in] on It true, multicast loopback is on. When false, it is off.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool setMulticastLoop(bool on, Report& report = CERR);

        //!
        //! Enable or disable the generation of receive timestamps.
        //!
        //! When enabled, each received UDP packets comes with a time stamp (see receive()).
        //! When possible, a hardware timestamp from the NIC is received. Otherwise, a software
        //! timestamp is generated by the kernel.
        //!
        //! When enabled, this option is a @æ request, not a requirement.
        //! Currently, this option is supported on Linux only. It is ignored on other systems.
        //!
        //! @param [in] on If true, receive timestamps are activated on the socket. Otherwise, they are disabled.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool setReceiveTimestamps(bool on, Report& report = CERR);

        //!
        //! Enable or disable the broadcast option.
        //!
        //! @param [in] on If true, broadcast is activated on the socket. Otherwise, it is disabled.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool setBroadcast(bool on, Report& report = CERR);

        //!
        //! Enable or disable the broadcast option, based on an IP address.
        //!
        //! @param [in] destination An hypothetical destination address. If this address
        //! is the broadcast address of a local interface, the broadcast option is set.
        //! Otherwise, the broadcast option is unchanged.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool setBroadcastIfRequired(const IPv4Address destination, Report& report = CERR);

        //!
        //! Join a multicast group.
        //!
        //! This method indicates that the application wishes to receive multicast
        //! packets which are sent to a specific multicast address. Specifying a
        //! non-default @a source address, source-specific multicast (SSM) is used.
        //!
        //! @param [in] multicast Multicast IP address to listen to.
        //! @param [in] local IP address of a local interface on which to listen.
        //! If set to IPv4Address::AnyAddress, the application lets the system selects
        //! the appropriate local interface.
        //! @param [in] source Source address for SSM.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool addMembership(const IPv4Address& multicast, const IPv4Address& local, const IPv4Address& source = IPv4Address(), Report& report = CERR);

        //!
        //! Join a multicast group.
        //!
        //! This method indicates that the application wishes to receive multicast
        //! packets which are sent to a specific multicast address. Specifying a
        //! non-default @a source address, source-specific multicast (SSM) is used.
        //!
        //! Using this method, the application listens on all local interfaces.
        //!
        //! @param [in] multicast Multicast IP address to listen to.
        //! @param [in] source Source address for SSM.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool addMembershipAll(const IPv4Address& multicast, const IPv4Address& source = IPv4Address(), Report& report = CERR);

        //!
        //! Join a multicast group.
        //!
        //! This method indicates that the application wishes to receive multicast
        //! packets which are sent to a specific multicast address. Specifying a
        //! non-default @a source address, source-specific multicast (SSM) is used.
        //!
        //! Using this method, the application lets the system selects the appropriate
        //! local interface.
        //!
        //! @param [in] multicast Multicast IP address to listen to.
        //! @param [in] source Source address for SSM.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool addMembershipDefault(const IPv4Address& multicast, const IPv4Address& source = IPv4Address(), Report& report = CERR);

        //!
        //! Drop all multicast membership requests, including source-specific multicast.
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
        //! be set to IPv4Address::AnyAddress or IPv4SocketAddress::AnyPort.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        virtual bool send(const void* data, size_t size, const IPv4SocketAddress& destination, Report& report = CERR);

        //!
        //! Send a message to the default destination address and port.
        //!
        //! @param [in] data Address of the message to send.
        //! @param [in] size Size in bytes of the message to send.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        virtual bool send(const void* data, size_t size, Report& report = CERR);

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
        //! @param [out] timestamp When not null, return the receive timestamp in micro-seconds.
        //! Use setReceiveTimestamps() to enable the generation of receive timestamps.
        //! If the returned value is negative, no timestamp is available.
        //! @return True on success, false on error.
        //!
        virtual bool receive(void* data,
                             size_t max_size,
                             size_t& ret_size,
                             IPv4SocketAddress& sender,
                             IPv4SocketAddress& destination,
                             const AbortInterface* abort = nullptr,
                             Report& report = CERR,
                             MicroSecond* timestamp = nullptr);

        // Implementation of Socket interface.
        virtual bool open(Report& report = CERR) override;
        virtual bool close(Report& report = CERR) override;

    private:
        // Encapsulate a Plain Old C Structure.
        template <typename STRUCT>
        struct POCS
        {
            // Encapsulated structure
            STRUCT data;

            // Default constructor, zeroe the C structure.
            POCS() : data()
            {
                TS_ZERO(data);
            }

            // Comparator for containers, no real semantic
            bool operator<(const POCS<STRUCT>& other) const
            {
                return std::memcmp(&data, &other.data, sizeof(data)) < 0;
            }
        };

        // Encapsulate an ip_mreq
        struct MReq : public POCS<::ip_mreq>
        {
            typedef POCS<::ip_mreq> SuperClass;
            MReq() = default;
            MReq(const IPv4Address& multicast_, const IPv4Address& interface_) : SuperClass()
            {
                multicast_.copy(data.imr_multiaddr);
                interface_.copy(data.imr_interface);
            }
        };
        typedef std::set<MReq> MReqSet;

        // Encapsulate an ip_mreq_source
#if !defined(TS_NO_SSM)
        struct SSMReq : public POCS<::ip_mreq_source>
        {
            typedef POCS<::ip_mreq_source> SuperClass;
            SSMReq() = default;
            SSMReq(const IPv4Address& multicast_, const IPv4Address& interface_, const IPv4Address& source_) : SuperClass()
            {
                multicast_.copy(data.imr_multiaddr);
                interface_.copy(data.imr_interface);
                source_.copy(data.imr_sourceaddr);
            }
        };
        typedef std::set<SSMReq> SSMReqSet;
#endif

        // Private members
        IPv4SocketAddress _local_address {};
        IPv4SocketAddress _default_destination {};
#if !defined(TS_NO_SSM)
        SSMReqSet         _ssmcast {};  // Current set of source-specific multicast memberships
#endif
        MReqSet           _mcast {};    // Current set of multicast memberships

        // Perform one receive operation. Hide the system mud. Return a system socket error code.
        int receiveOne(void* data, size_t max_size, size_t& ret_size, IPv4SocketAddress& sender, IPv4SocketAddress& destination, Report& report, MicroSecond* timestamp);

        // Furiously idiotic Windows feature, see comment in receiveOne()
#if defined(TS_WINDOWS)
        static volatile ::LPFN_WSARECVMSG _wsaRevcMsg;
#endif
    };
}
