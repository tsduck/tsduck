//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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
#include "tsIPSocketAddress.h"
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
    //! @ingroup libtscore net
    //!
    class TSCOREDLL UDPSocket: public Socket
    {
        TS_NOCOPY(UDPSocket);
    public:
        //!
        //! Constructor.
        //! @param [in] auto_open If true, call open() immediately.
        //! @param [in] gen IP generation, IPv4 or IPv6.
        //! If set to IP::Any, this socket can receive IPv4 and IPv6 datagrams.
        //! If @a gen is IP::v6, the socket is created with option IPV6_V6ONLY set.
        //! @param [in,out] report Where to report error.
        //!
        UDPSocket(bool auto_open = false, IP gen = IP::Any, Report& report = CERR);

        //!
        //! Destructor.
        //!
        virtual ~UDPSocket() override;

        //!
        //! Bind to a local address and port.
        //!
        //! The IP address part of the socket address must one of:
        //! - IPAddress::AnyAddress4. Any local interface may be used
        //!   to send or receive UDP datagrams. For each outgoing packet, the actual
        //!   interface is selected by the kernel based on the routing rules. Incoming
        //!   UDP packets for the selected port will be accepted from any local interface.
        //! - The IP address of an interface of the local system. Outgoing packets will be
        //!   unconditionally sent through this interface. Incoming UDP packets for the
        //!   selected port will be accepted only when they arrive through the selected
        //!   interface.
        //!
        //! Special note for receiving multicast on most Unix systems (at least Linux
        //! and macOS): The IP address shall be either AnyAddress4 or the <b>multicast
        //! group address</b>. Do not specify a local address to receive multicast on Unix.
        //!
        //! The port number part of the socket address must be one of:
        //! - IPSocketAddress::AnyPort. The socket is bound to an
        //!   arbitrary unused local UDP port.
        //! - A specific port number. If this UDP port is already bound by another
        //!   local UDP socket, the bind operation fails, unless the "reuse port"
        //!   option has already been set.
        //!
        //! @param [in] addr Local socket address to bind to.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool bind(const IPSocketAddress& addr, Report& report = CERR);

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
        //! be set to IPAddress::AnyAddress4 or IPSocketAddress::AnyPort.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool setDefaultDestination(const IPSocketAddress& addr, Report& report = CERR);

        //!
        //! Set a default destination address and port for outgoing messages.
        //!
        //! There are two versions of the send() method. One of them explicitly
        //! specifies the destination of the packet to send. The second version
        //! does not specify a destination; the packet is sent to the <i>default
        //! destination</i>.
        //!
        //! @param [in] name A string describing the socket address of the destination.
        //! See IPSocketAddress::resolve() for a description of the expected string format.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool setDefaultDestination(const UString& name, Report& report = CERR);

        //!
        //! Get the default destination address and port for outgoing messages.
        //! @return The default destination address and port for outgoing messages.
        //!
        IPSocketAddress getDefaultDestination() const {return _default_destination;}

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
        //! See IPAddress::resolve() for a description of the expected string format.
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
        //! Set the Type Of Service (TOS) or Traffic Class (IPv6) option.
        //!
        //! The interpretation of the @a tos parameter depends in the IP generation.
        //! With IPv4, this is a "type of service" value.
        //! With IPv6, this is a "traffic class" value.
        //!
        //! Note that correct support for this option depends on the operating
        //! system. Typically, it never worked correctly on Windows.
        //!
        //! @param [in] tos The type of service (IPv4) or traffic class (IPv6) value.
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
        //! This option is supported on Linux and macOS only. It is ignored on other systems.
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
        bool setBroadcastIfRequired(const IPAddress destination, Report& report = CERR);

        //!
        //! Join a multicast group.
        //!
        //! This method indicates that the application wishes to receive multicast
        //! packets which are sent to a specific multicast address. Specifying a
        //! non-default @a source address, source-specific multicast (SSM) is used.
        //! Note that source-specific multicast exists on IPv4 only.
        //!
        //! @param [in] multicast Multicast IP address to listen to.
        //! @param [in] local IP address of a local interface on which to listen.
        //! If set to IPAddress::AnyAddress4, the application lets the system selects
        //! the appropriate local interface.
        //! @param [in] source Source address for SSM. Ignored on IPv6 socket.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool addMembership(const IPAddress& multicast, const IPAddress& local, const IPAddress& source = IPAddress(), Report& report = CERR)
        {
            return addMembershipImpl(multicast, local, -1, source, report);
        }

        //!
        //! Join a multicast group.
        //!
        //! This method indicates that the application wishes to receive multicast
        //! packets which are sent to a specific multicast address. Specifying a
        //! non-default @a source address, source-specific multicast (SSM) is used.
        //! Note that source-specific multicast exists on IPv4 only.
        //!
        //! @param [in] multicast Multicast IP address to listen to.
        //! @param [in] interface_index Index of a local interface on which to listen.
        //! If set to zero, the application lets the system selects the appropriate local interface.
        //! @param [in] source Source address for SSM. Ignored on IPv6 socket.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool addMembership(const IPAddress& multicast, int interface_index, const IPAddress& source = IPAddress(), Report& report = CERR)
        {
            return addMembershipImpl(multicast, IPAddress(), interface_index, source, report);
        }

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
        //! @param [in] link_local If true, also add membership on link-local addresses, otherwise ignore them.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool addMembershipAll(const IPAddress& multicast, const IPAddress& source = IPAddress(), bool link_local = true, Report& report = CERR);

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
        bool addMembershipDefault(const IPAddress& multicast, const IPAddress& source = IPAddress(), Report& report = CERR)
        {
            return addMembershipImpl(multicast, IPAddress(), -1, source, report);
        }

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
        //! be set to IPAddress::AnyAddress4 or IPSocketAddress::AnyPort.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        virtual bool send(const void* data, size_t size, const IPSocketAddress& destination, Report& report = CERR);

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
        //! Type of timestamp which is returned by receive().
        //!
        enum class TimeStampType {
            NONE,      //!< No timestamp.
            SOFTWARE,  //!< Software time stamp, generated by the kernel.
            HARDWARE,  //!< Hardware time stamp, generated by the NIC, when supported.
        };

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
        //! @param [out] timestamp_type When not null, return the type of receive timestamp.
        //! @return True on success, false on error.
        //!
        virtual bool receive(void* data,
                             size_t max_size,
                             size_t& ret_size,
                             IPSocketAddress& sender,
                             IPSocketAddress& destination,
                             const AbortInterface* abort = nullptr,
                             Report& report = CERR,
                             cn::microseconds* timestamp = nullptr,
                             TimeStampType* timestamp_type = nullptr);

        // Implementation of Socket interface.
        virtual bool open(IP gen, Report& report = CERR) override;
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
                return MemCompare(&data, &other.data, sizeof(data)) < 0;
            }
        };

        // Encapsulate an ip_mreq
        struct MReq : public POCS<::ip_mreq>
        {
            using SuperClass = POCS<::ip_mreq>;
            MReq() = default;
            MReq(const IPAddress& multicast, const IPAddress& interface) : SuperClass()
            {
                multicast.getAddress4(data.imr_multiaddr);
                interface.getAddress4(data.imr_interface);
            }
        };
        using MReqSet = std::set<MReq>;

        // Encapsulate an ipv6_mreq
        struct MReq6 : public POCS<::ipv6_mreq>
        {
            using SuperClass = POCS<::ipv6_mreq>;
            MReq6() = default;
            MReq6(const IPAddress& multicast, int interface_index) : SuperClass()
            {
                multicast.getAddress6(data.ipv6mr_multiaddr);
                data.ipv6mr_interface = static_cast<unsigned int>(interface_index);
            }
        };
        using MReq6Set = std::set<MReq6>;

        // Encapsulate an ip_mreq_source
#if !defined(TS_NO_SSM)
        struct SSMReq : public POCS<::ip_mreq_source>
        {
            using SuperClass = POCS<::ip_mreq_source>;
            SSMReq() = default;
            SSMReq(const IPAddress& multicast_, const IPAddress& interface_, const IPAddress& source_) : SuperClass()
            {
                multicast_.getAddress4(data.imr_multiaddr);
                interface_.getAddress4(data.imr_interface);
                source_.getAddress4(data.imr_sourceaddr);
            }
        };
        using SSMReqSet = std::set<SSMReq>;
#endif

        // Private members
        IPSocketAddress _local_address {};
        IPSocketAddress _default_destination {};
        MReqSet         _mcast {};    // Current set of IPv4 multicast memberships
        MReq6Set        _mcast6 {};   // Current set of IPv6 multicast memberships
#if !defined(TS_NO_SSM)
        SSMReqSet       _ssmcast {};  // Current set of source-specific multicast memberships
#endif

        // Perform one receive operation. Hide the system mud. Return a system socket error code.
        int receiveOne(void* data, size_t max_size, size_t& ret_size, IPSocketAddress& sender, IPSocketAddress& destination,
                       Report& report, cn::microseconds* timestamp, TimeStampType* timestamp_type);

        // Add multicast membership common code, local interface by index or by address.
        bool addMembershipImpl(const IPAddress& multicast, const IPAddress& local, int interface_index, const IPAddress& source, Report& report);
    };
}
