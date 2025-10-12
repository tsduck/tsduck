//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsUDPSocket.h"
#include "tsNetworkInterface.h"
#include "tsNullReport.h"
#include "tsSysUtils.h"

#if defined(TS_WINDOWS)
    #include "tsBeforeStandardHeaders.h"
    #include <mstcpip.h>
    #include "tsAfterStandardHeaders.h"
#elif defined(TS_LINUX)
    #include "tsBeforeStandardHeaders.h"
    #include <linux/errqueue.h>
    #include <linux/net_tstamp.h>
    #include <linux/sockios.h>
    #include "tsAfterStandardHeaders.h"
    #if defined(SO_TIMESTAMPING_NEW)
        #define TS_SO_TIMESTAMPING SO_TIMESTAMPING_NEW
        #define TS_SCM_TIMESTAMPING SO_TIMESTAMPING_NEW  // SCM_TIMESTAMPING_NEW not defined
        #define TS_STRUCT_SCM_TIMESTAMPING ::scm_timestamping64
    #elif defined(SO_TIMESTAMPING)
        #define TS_SO_TIMESTAMPING SO_TIMESTAMPING
        #define TS_SCM_TIMESTAMPING SCM_TIMESTAMPING
        #define TS_STRUCT_SCM_TIMESTAMPING ::scm_timestamping
    #endif
#endif


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::UDPSocket::UDPSocket(bool auto_open, IP gen, Report& report)
{
    if (auto_open) {
        UDPSocket::open(gen, report);
    }
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::UDPSocket::~UDPSocket()
{
    UDPSocket::close(NULLREP);
}


//----------------------------------------------------------------------------
// Open the socket
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::UDPSocket::open(IP gen, Report& report)
{
    // Create a datagram socket.
    if (!createSocket(gen, SOCK_DGRAM, IPPROTO_UDP, report)) {
        return false;
    }

    // Set option to get the destination address of all UDP packets arriving on this socket.
    if (generation() == IP::v4) {
        // On IPv4 socket, use IP_PKTINFO (IP_RECVDSTADDR on FreeBSD).
#if defined(IP_PKTINFO)
        int opt_pktinfo = 1;
        if (::setsockopt(getSocket(), IPPROTO_IP, IP_PKTINFO, SysSockOptPointer(&opt_pktinfo), sizeof(opt_pktinfo)) != 0) {
            report.error(u"error setting socket IP_PKTINFO option: %s", SysErrorCodeMessage());
            return false;
        }
#elif defined(IP_RECVDSTADDR)
        int opt_recvdstaddr = 1;
        if (::setsockopt(getSocket(), IPPROTO_IP, IP_RECVDSTADDR, SysSockOptPointer(&opt_recvdstaddr), sizeof(opt_recvdstaddr)) != 0) {
            report.error(u"error setting socket IP_RECVDSTADDR option: %s", SysErrorCodeMessage());
            return false;
        }
#endif
    }
    else {
        // On IPv6 socket, use IPV6_RECVPKTINFO on Unix and IPV6_PKTINFO on Windows.
#if defined(IPV6_RECVPKTINFO)
        int opt = 1;
        if (::setsockopt(getSocket(), IPPROTO_IPV6, IPV6_RECVPKTINFO, SysSockOptPointer(&opt), sizeof(opt)) != 0) {
            report.error(u"error setting socket IPV6_RECVPKTINFO option: %s", SysErrorCodeMessage());
            return false;
        }
#elif defined(TS_WINDOWS)
        int opt = 1;
        if (::setsockopt(getSocket(), IPPROTO_IPV6, IPV6_PKTINFO, SysSockOptPointer(&opt), sizeof(opt)) != 0) {
            report.error(u"error setting socket IPV6_PKTINFO option: %s", SysErrorCodeMessage());
            return false;
        }
#endif
    }

    return true;
}


//----------------------------------------------------------------------------
// Close the socket
//----------------------------------------------------------------------------

bool ts::UDPSocket::close(Report& report)
{
    // Leave all multicast groups.
    if (isOpen()) {
        dropMembership(report);
    }

    // Close socket
    return Socket::close(report);
}


//----------------------------------------------------------------------------
// Bind to a local address and port.
//----------------------------------------------------------------------------

bool ts::UDPSocket::bind(const IPSocketAddress& addr, Report& report)
{
    IPSocketAddress addr2(addr);
    if (!convert(addr2, report)) {
        return false;
    }

    ::sockaddr_storage sock_addr;
    const size_t sock_size = addr2.get(sock_addr);

    report.debug(u"binding socket to %s", addr2);
    if (::bind(getSocket(), reinterpret_cast<::sockaddr*>(&sock_addr), socklen_t(sock_size)) != 0) {
        report.error(u"error binding socket to local address %s: %s", addr2, SysErrorCodeMessage());
        return false;
    }

    // Keep a cached value of the bound local address.
    return getLocalAddress(_local_address, report);
}


//----------------------------------------------------------------------------
// Set outgoing local address for multicast messages.
//----------------------------------------------------------------------------

bool ts::UDPSocket::setOutgoingMulticast(const UString& name, Report& report)
{
    IPAddress addr;
    return addr.resolve(name, report, generation()) && setOutgoingMulticast(addr, report);
}

bool ts::UDPSocket::setOutgoingMulticast(const IPAddress& addr, Report& report)
{
    IPAddress local(addr);
    if (!local.convert(generation())) {
        report.error(u"cannot use IPv%d address %s in IPv%d socket", int(addr.generation()), addr, int(generation()));
        return false;
    }

    bool ok = true;
    if (local.generation() == IP::v4) {
        // With IPv4, the local interface is identified by its IPv4 address.
        ::in_addr iaddr;
        local.getAddress4(iaddr);
        report.debug(u"setting socket IP_MULTICAST_IF to %s", local);
        ok = ::setsockopt(getSocket(), IPPROTO_IP, IP_MULTICAST_IF, SysSockOptPointer(&iaddr), sizeof(iaddr)) == 0;
    }
    else {
        // With IPv6, the local interface is identified by its system-defined interface index.
        int index = NetworkInterface::ToIndex(local, false, report);
        report.debug(u"setting socket IPV6_MULTICAST_IF to %d", index);
        ok = index >= 0 && ::setsockopt(getSocket(), IPPROTO_IPV6, IPV6_MULTICAST_IF, SysSockOptPointer(&index), sizeof(index)) == 0;
    }
    if (!ok) {
        report.error(u"error setting outgoing local address %s: %s", local, SysErrorCodeMessage());
    }
    return ok;
}


//----------------------------------------------------------------------------
// Set a default destination address and port for outgoing messages.
//----------------------------------------------------------------------------

bool ts::UDPSocket::setDefaultDestination(const UString& name, Report& report)
{
    IPSocketAddress addr;
    return addr.resolve(name, report, generation()) && setDefaultDestination(addr, report);
}

bool ts::UDPSocket::setDefaultDestination(const IPSocketAddress& addr, Report& report)
{
    if (!addr.hasAddress()) {
        report.error(u"missing IP address in UDP destination %s", addr);
        return false;
    }
    else if (!addr.hasPort()) {
        report.error(u"missing port number in UDP destination %s", addr);
        return false;
    }
    else {
        report.debug(u"setting UDP socket default destination to %s", addr);
        _default_destination = addr;
        return true;
    }
}


//----------------------------------------------------------------------------
// Set the Time To Live (TTL) option.
//----------------------------------------------------------------------------

bool ts::UDPSocket::setTTL(int ttl, bool multicast, Report& report)
{
    bool ok = true;
    if (generation() == IP::v4) {
        if (multicast) {
            SysSocketMulticastTTLType mttl = SysSocketMulticastTTLType(ttl);
            report.debug(u"setting socket IP_MULTICAST_TTL to %d", int(mttl));
            ok = ::setsockopt(getSocket(), IPPROTO_IP, IP_MULTICAST_TTL, SysSockOptPointer(&mttl), sizeof(mttl)) == 0;
        }
        else {
            SysSocketTTLType uttl = SysSocketTTLType(ttl);
            report.debug(u"setting socket IP_TTL to %d", int(uttl));
            ok = ::setsockopt(getSocket(), IPPROTO_IP, IP_TTL, SysSockOptPointer(&uttl), sizeof(uttl)) == 0;
        }
    }
    else {
        if (multicast) {
            SysSocketMulticastTTLType mttl = SysSocketMulticastTTLType(ttl);
            report.debug(u"setting socket IPV6_MULTICAST_HOPS to %d", int(mttl));
            ok = ::setsockopt(getSocket(), IPPROTO_IPV6, IPV6_MULTICAST_HOPS, SysSockOptPointer(&mttl), sizeof(mttl)) == 0;
        }
        else {
            SysSocketTTLType uttl = SysSocketTTLType(ttl);
            report.debug(u"setting socket IPV6_UNICAST_HOPS to %d", int(uttl));
            ok = ::setsockopt(getSocket(), IPPROTO_IPV6, IPV6_UNICAST_HOPS, SysSockOptPointer(&uttl), sizeof(uttl)) == 0;
        }
    }
    if (!ok) {
        report.error(u"socket option %s TTL: %s", multicast ? u"multicast" : u"unicast", SysErrorCodeMessage());
    }
    return ok;
}


//----------------------------------------------------------------------------
// Set the Type Of Service (TOS) option.
//----------------------------------------------------------------------------

bool ts::UDPSocket::setTOS(int tos, Report& report)
{
    if (generation() == IP::v4) {
        // IPv4: this is a "type of service" value.
        SysSocketTOSType utos = SysSocketTOSType(tos);
        report.debug(u"setting socket IP_TOS to %d", int(utos));
        if (::setsockopt(getSocket(), IPPROTO_IP, IP_TOS, SysSockOptPointer(&utos), sizeof(utos)) != 0) {
            report.error(u"socket option TOS: %s", SysErrorCodeMessage());
            return false;
        }
    }
    else {
        // IPv6: this is a "traffic class" value.
        // This is not supported on all systems.
#if defined(IPV6_TCLASS)
        SysSocketTClassType tclass = SysSocketTClassType(tos);
        report.debug(u"setting socket IPV6_TCLASS to %d", int(tclass));
        if (::setsockopt(getSocket(), IPPROTO_IPV6, IPV6_TCLASS, SysSockOptPointer(&tclass), sizeof(tclass)) != 0) {
            report.error(u"socket option IPV6_TCLASS: %s", SysErrorCodeMessage());
            return false;
        }
#endif
    }
    return true;
}


//----------------------------------------------------------------------------
// Set the multicast loop option.
//----------------------------------------------------------------------------

bool ts::UDPSocket::setMulticastLoop(bool on, Report& report)
{
    bool ok = true;
    if (generation() == IP::v4) {
        SysSocketMulticastLoopType mloop = SysSocketMulticastLoopType(on);
        report.debug(u"setting socket IP_MULTICAST_LOOP to %d", int(mloop));
        ok = ::setsockopt(getSocket(), IPPROTO_IP, IP_MULTICAST_LOOP, SysSockOptPointer(&mloop), sizeof(mloop)) == 0;
    }
    else {
        // Warning: on Unix systems, the option type is not the same as IPv4.
        SysSocketMulticastLoopType6 mloop = SysSocketMulticastLoopType6(on);
        report.debug(u"setting socket IPV6_MULTICAST_LOOP to %d", int(mloop));
        ok = ::setsockopt(getSocket(), IPPROTO_IPV6, IPV6_MULTICAST_LOOP, SysSockOptPointer(&mloop), sizeof(mloop)) == 0;
    }
    if (!ok) {
        report.error(u"socket option multicast loop: %s", SysErrorCodeMessage());
    }
    return ok;
}


//----------------------------------------------------------------------------
// Enable or disable the generation of receive timestamps.
//----------------------------------------------------------------------------

bool ts::UDPSocket::setReceiveTimestamps(bool on, Report& report)
{
#if defined(TS_WINDOWS)

    ::TIMESTAMPING_CONFIG config;
    TS_ZERO(config);
    config.Flags = TIMESTAMPING_FLAG_RX;
    ::DWORD bytes = 0;
    if (::WSAIoctl(getSocket(), SIO_TIMESTAMPING, &config, sizeof(config), nullptr, 0, &bytes, nullptr, nullptr) != 0) {
        report.error(u"socket option SIO_TIMESTAMPING: %s", SysErrorCodeMessage(::WSAGetLastError()));
        return false;
    }

#else

#if defined(SO_TIMESTAMPNS)
    // Set SO_TIMESTAMPNS option which reports timestamps in nanoseconds (struct timespec).
    int enable = int(on);
    report.debug(u"setting socket SO_TIMESTAMPNS to %d", enable);
    if (::setsockopt(getSocket(), SOL_SOCKET, SO_TIMESTAMPNS, &enable, sizeof(enable)) != 0) {
        report.error(u"socket option SO_TIMESTAMPNS: %s", SysErrorCodeMessage());
        return false;
    }
#elif defined(SO_TIMESTAMP)
    // Set SO_TIMESTAMP option which reports timestamps in microseconds (struct timeval).
    int enable = int(on);
    report.debug(u"setting socket SO_TIMESTAMP to %d", enable);
    if (::setsockopt(getSocket(), SOL_SOCKET, SO_TIMESTAMP, &enable, sizeof(enable)) != 0) {
        report.error(u"socket option SO_TIMESTAMP: %s", SysErrorCodeMessage());
        return false;
    }
#endif

#if defined(TS_SO_TIMESTAMPING)
    // Set SO_TIMESTAMPING to request hardware timestamps, when available.
    int val = SOF_TIMESTAMPING_RX_HARDWARE | SOF_TIMESTAMPING_RX_SOFTWARE |
              SOF_TIMESTAMPING_SOFTWARE | SOF_TIMESTAMPING_RAW_HARDWARE;
    report.debug(u"setting socket SO_TIMESTAMPING to %d", val);
    if (::setsockopt(getSocket(), SOL_SOCKET, TS_SO_TIMESTAMPING, &val, sizeof(val)) != 0) {
        report.error(u"socket option SO_TIMESTAMPING: %s", SysErrorCodeMessage());
        return false;
    }
#endif

#endif // Windows vs. UNIX

    return true;
}


//----------------------------------------------------------------------------
// Enable or disable the broadcast option.
//----------------------------------------------------------------------------

bool ts::UDPSocket::setBroadcast(bool on, Report& report)
{
    int enable = int(on);
    report.debug(u"setting socket SO_BROADCAST to %d", enable);
    if (::setsockopt(getSocket(), SOL_SOCKET, SO_BROADCAST, SysSockOptPointer(&enable), sizeof(enable)) != 0) {
        report.error(u"socket option broadcast: %s", SysErrorCodeMessage());
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Enable or disable the broadcast option, based on an IP address.
//----------------------------------------------------------------------------

bool ts::UDPSocket::setBroadcastIfRequired(const IPAddress destination, Report& report)
{
    // Get all local interfaces.
    NetworkInterfaceVector locals;
    if (!NetworkInterface::GetAll(locals, false, destination.generation(), false, report)) {
        return false;
    }

    // Loop on all local addresses and set broadcast when we match a local broadcast address.
    for (const auto& it : locals) {
        if (destination == it.address.broadcastAddress()) {
            return setBroadcast(true, report);
        }
    }

    // Not a broadcast address, nothing was done.
    return true;
}


//----------------------------------------------------------------------------
// Join one multicast group on one local interface.
//----------------------------------------------------------------------------

bool ts::UDPSocket::addMembershipImpl(const IPAddress& multicast_in, const IPAddress& local_in, int interface_index, const IPAddress& source_in, Report& report)
{
    // Make sure the addresses have the same generation as the socket.
    // The multicast address cannot be converted and conversion will fail if not at the right generation.
    IPAddress multicast(multicast_in);
    IPAddress local(local_in);
    IPAddress source(source_in);
    if (!convert(multicast, report) || !convert(local, report) || !convert(source, report)) {
        return false;
    }

    // Verbose message about joining the group.
    UString group_string;
    if (source.hasAddress()) {
        group_string = source.toString() + u"@";
    }
    group_string += multicast.toString();
    if (local.hasAddress()) {
        report.verbose(u"joining multicast group %s from local address %s", group_string, local);
    }
    else if (interface_index >= 0) {
        report.verbose(u"joining multicast group %s from local interface %d", group_string, interface_index);
    }
    else {
        report.verbose(u"joining multicast group %s from default interface", group_string);
    }

    // Now join the group.
    if (generation() == IP::v4) {
        // With IPv4, the local interface must be identified by IP address.
        // Find IP address of local interface if identified by index.
        if (!local.hasAddress() && interface_index > 0 && !NetworkInterface::ToAddress(local, interface_index,IP::v4, false, report)) {
            return false;
        }
        // SSM vs. standard multicast.
        if (source.hasAddress()) {
            // Source-specific multicast (SSM).
#if defined(TS_NO_SSM)
            report.error(u"source-specific multicast (SSM) is not supported on this operating system");
            return false;
#else
            SSMReq req(multicast, local, source);
            if (::setsockopt(getSocket(), IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP, SysSockOptPointer(&req.data), sizeof(req.data)) != 0) {
                report.error(u"error adding SSM membership to %s from local address %s: %s", group_string, local, SysErrorCodeMessage());
                return false;
            }
            else {
                _ssmcast.insert(req);
                return true;
            }
#endif
        }
        else {
            // Standard IPv4 multicast.
            MReq req(multicast, local);
            if (::setsockopt(getSocket(), IPPROTO_IP, IP_ADD_MEMBERSHIP, SysSockOptPointer(&req.data), sizeof(req.data)) != 0) {
                report.error(u"error adding multicast membership to %s from local address %s: %s", group_string, local, SysErrorCodeMessage());
                return false;
            }
            else {
                _mcast.insert(req);
                return true;
            }
        }
    }
    else {
        // With IPv6, the local interface must be identified by index.
        // Find index of local interface if identified by IP address.
        if (interface_index < 0) {
            if (!local.hasAddress()) {
                interface_index = 0; // any interface
            }
            else if ((interface_index = NetworkInterface::ToIndex(local, false, report)) < 0) {
                return false;
            }
        }
        // SSM vs. standard multicast.
        if (source.hasAddress()) {
            // IPv6: SSM does not exist.
            report.error(u"SSM is not available on IPv6 socket");
            return false;
        }
        else {
            // Standard IPv6 multicast.
            MReq6 req(multicast, interface_index);
            if (::setsockopt(getSocket(), IPPROTO_IPV6, IPV6_JOIN_GROUP, SysSockOptPointer(&req.data), sizeof(req.data)) != 0) {
                report.error(u"error adding multicast membership to %s from local address %s: %s", group_string, local, SysErrorCodeMessage());
                return false;
            }
            else {
                _mcast6.insert(req);
                return true;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Join one multicast group on all local interfaces.
//----------------------------------------------------------------------------

bool ts::UDPSocket::addMembershipAll(const IPAddress& multicast, const IPAddress& source, bool link_local, Report& report)
{
    // There is no implicit way to listen on all interfaces. If no local address is specified,
    // we must get the list of all local interfaces and send a multicast membership request on each of them.

    // Get all local interfaces.
    const IP gen = multicast.generation();
    NetworkInterfaceVector locals;
    if (!NetworkInterface::GetAll(locals, false, gen, false, report)) {
        return false;
    }

    // When an interface has several IP addresses, we shall not send the request multiple times on the same
    // interface when used by index. On macOS, at least, it generates an error "Address already in use".
    std::set<int> indexes;

    // Add all memberships
    bool ok = true;
    for (const auto& loc : locals) {
        if (link_local || !loc.address.isLinkLocal()) {
            if (gen == IP::v4 || loc.index < 0) {
                // On IPv4, use local IP address. Also on IPv6 if interface index is unknown.
                ok = addMembershipImpl(multicast, loc.address, -1, source, report) && ok;
            }
            else if (!indexes.contains(loc.index)) {
                // On IPv6, use interface index. Keep track of indexes to send only one request per interface.
                indexes.insert(loc.index);
                ok = addMembershipImpl(multicast, IPAddress(), loc.index, source, report) && ok;
            }
        }
    }
    return ok;
}


//----------------------------------------------------------------------------
// Leave all multicast groups.
//----------------------------------------------------------------------------

bool ts::UDPSocket::dropMembership(Report& report)
{
    bool ok = true;

    // Drop all standard IPv4 multicast groups (none on IPv6 sockets).
    for (const auto& it : _mcast) {
        report.verbose(u"leaving multicast group %s from local address %s", IPAddress(it.data.imr_multiaddr), IPAddress(it.data.imr_interface));
        if (::setsockopt(getSocket(), IPPROTO_IP, IP_DROP_MEMBERSHIP, SysSockOptPointer(&it.data), sizeof(it.data)) != 0) {
            report.error(u"error dropping multicast membership: %s", SysErrorCodeMessage());
            ok = false;
        }
    }
    _mcast.clear();

    // Drop all standard IPv6 multicast groups (none on IPv4 sockets).
    for (const auto& it : _mcast6) {
        report.verbose(u"leaving multicast group %s from local interface %d", IPAddress(it.data.ipv6mr_multiaddr), it.data.ipv6mr_interface);
        if (::setsockopt(getSocket(), IPPROTO_IPV6, IPV6_LEAVE_GROUP, SysSockOptPointer(&it.data), sizeof(it.data)) != 0) {
            report.error(u"error dropping multicast membership: %s", SysErrorCodeMessage());
            ok = false;
        }
    }
    _mcast6.clear();

    // Drop all source-specific multicast groups.
#if !defined(TS_NO_SSM)
    for (const auto& it : _ssmcast) {
        report.verbose(u"leaving multicast group %s@%s from local address %s",
                       IPAddress(it.data.imr_sourceaddr), IPAddress(it.data.imr_multiaddr), IPAddress(it.data.imr_interface));
        if (::setsockopt(getSocket(), IPPROTO_IP, IP_DROP_SOURCE_MEMBERSHIP, SysSockOptPointer(&it.data), sizeof(it.data)) != 0) {
            report.error(u"error dropping multicast membership: %s", SysErrorCodeMessage());
            ok = false;
        }
    }
    _ssmcast.clear();
#endif

    return ok;
}


//----------------------------------------------------------------------------
// Send a message to a destination address and port.
//----------------------------------------------------------------------------

bool ts::UDPSocket::send(const void* data, size_t size, Report& report)
{
    return send(data, size, _default_destination, report);
}

bool ts::UDPSocket::send(const void* data, size_t size, const IPSocketAddress& dest_in, Report& report)
{
    IPSocketAddress dest(dest_in);
    if (!convert(dest, report)) {
        return false;
    }

    ::sockaddr_storage addr;
    const size_t addr_size = dest.get(addr);

    if (::sendto(getSocket(), SysSendBufferPointer(data), SysSendSizeType(size), 0, reinterpret_cast<::sockaddr*>(&addr), socklen_t(addr_size)) < 0) {
        report.error(u"error sending UDP message: %s", SysErrorCodeMessage());
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Receive a message.
//----------------------------------------------------------------------------

bool ts::UDPSocket::receive(void* data,
                            size_t max_size,
                            size_t& ret_size,
                            IPSocketAddress& sender,
                            IPSocketAddress& destination,
                            const AbortInterface* abort,
                            Report& report,
                            cn::microseconds* timestamp,
                            TimeStampType* timestamp_type)
{
    // Clear timestamp if specified.
    if (timestamp != nullptr) {
        *timestamp = cn::microseconds(-1);
    }

    // Loop on unsollicited interrupts
    for (;;) {

        // Wait for a message.
        const int err = receiveOne(data, max_size, ret_size, sender, destination, report, timestamp, timestamp_type);

        if (abort != nullptr && abort->aborting()) {
            // Aborting, no error message.
            return false;
        }
        else if (err == 0) {
            // Sometimes, we get "successful" empty message coming from nowhere. Ignore them.
            if (ret_size > 0 || sender.hasAddress()) {
                return true;
            }
        }
        else if (abort != nullptr && abort->aborting()) {
            // User-interrupt, end of processing but no error message
            return false;
        }
#if defined(TS_UNIX)
        else if (err == EINTR) {
            // Got a signal, not a user interrupt, will ignore it
            report.debug(u"signal, not user interrupt");
        }
#endif
        else {
            // Abort on non-interrupt errors.
            if (isOpen()) {
                // Report the error only if the error does not result from a close in another thread.
                report.error(u"error receiving from UDP socket: %s", SysErrorCodeMessage(err));
            }
            return false;
        }
    }
}


//----------------------------------------------------------------------------
// Dynamically resolve WSARecvMsg() on Windows.
// Implemented as a static function to allow "initialize once" later.
//
// On all operating systems, recvmsg() is used to receive a UDP message with
// additional information such as sender address, timestamps and other info.
// On Windows, all socket operations are smoothly emulated, including recvfrom,
// allowing a reasonable portability. However, in the specific case of recvmsg,
// there is no equivalent but a similar - and carefully incompatible - function
// named WSARecvMsg. Not only this function is different from recvmsg, but it
// is also not exported from any DLL. Its address must be queried dynamically
// using WSAIoctl(). The stupid idiot who had this pervert idea at Microsoft
// deserves to burn in hell (twice) !!
//----------------------------------------------------------------------------

#if defined(TS_WINDOWS)
namespace {
    // Get the address of a WSA extension function.
    ::LPFN_WSARECVMSG GetWSAFunction(::GUID& guid, int& error)
    {
        ::LPFN_WSARECVMSG func_address = nullptr;
        ::DWORD bytes = 0;
        const ::SOCKET sock = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (sock == INVALID_SOCKET) {
            error = ::WSAGetLastError();
            return nullptr;
        }
        if (::WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &func_address, sizeof(func_address), &bytes, nullptr, nullptr) != 0) {
            error = ::WSAGetLastError();
        }
        ::closesocket(sock);
        return func_address;
    }
}
#endif


//----------------------------------------------------------------------------
// Perform one receive operation. Hide the system mud.
//----------------------------------------------------------------------------

int ts::UDPSocket::receiveOne(void* data,
                              size_t max_size,
                              size_t& ret_size,
                              IPSocketAddress& sender,
                              IPSocketAddress& destination,
                              Report& report,
                              cn::microseconds* timestamp,
                              TimeStampType* timestamp_type)
{
    // Clear returned values
    ret_size = 0;
    sender.clear();
    destination.clear();
    if (timestamp != nullptr) {
        *timestamp = cn::microseconds(-1);
    }
    if (timestamp_type != nullptr) {
        *timestamp_type = TimeStampType::NONE;
    }

    // Reserve a socket address to receive the sender address.
    ::sockaddr_storage sender_sock;
    TS_ZERO(sender_sock);

#if defined(TS_WINDOWS)

    // Get the address of WSARecvMsg the first time we use it.
    // Thread-safe init-safe static data pattern:
    static ::GUID wsa_recvmsg_guid = WSAID_WSARECVMSG;
    static int wsa_recvmsg_error = 0;
    static const ::LPFN_WSARECVMSG wsa_recvmsg = GetWSAFunction(wsa_recvmsg_guid, wsa_recvmsg_error);
    if (wsa_recvmsg == nullptr) {
        return wsa_recvmsg_error;
    }

    // Build an WSABUF pointing to the message.
    ::WSABUF vec;
    TS_ZERO(vec);
    vec.buf = reinterpret_cast<CHAR*>(data);
    vec.len = ::ULONG(max_size);

    // Reserve a buffer to receive packet ancillary data.
    ::CHAR ancil_data[1024];
    TS_ZERO(ancil_data);

    // Build a WSAMSG for WSARecvMsg.
    ::WSAMSG msg;
    TS_ZERO(msg);
    msg.name = reinterpret_cast<::sockaddr*>(&sender_sock);
    msg.namelen = sizeof(sender_sock);
    msg.lpBuffers = &vec;
    msg.dwBufferCount = 1; // number of WSAMSG
    msg.Control.buf = ancil_data;
    msg.Control.len = ::ULONG(sizeof(ancil_data));

    // Wait for a message.
    ::DWORD insize = 0;
    if (wsa_recvmsg(getSocket(), &msg, &insize, nullptr, nullptr) != 0) {
        return LastSysErrorCode();
    }

    // Browse returned ancillary data.
    for (::WSACMSGHDR* cmsg = WSA_CMSG_FIRSTHDR(&msg); cmsg != nullptr; cmsg = WSA_CMSG_NXTHDR(&msg, cmsg)) {
        if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_PKTINFO && cmsg->cmsg_len >= sizeof(::IN_PKTINFO)) {
            const ::IN_PKTINFO* info = reinterpret_cast<const ::IN_PKTINFO*>(WSA_CMSG_DATA(cmsg));
            destination = IPSocketAddress(info->ipi_addr, _local_address.port());
        }
        else if (cmsg->cmsg_level == IPPROTO_IPV6 && cmsg->cmsg_type == IPV6_PKTINFO && cmsg->cmsg_len >= sizeof(::IN6_PKTINFO)) {
            const ::IN6_PKTINFO* info = reinterpret_cast<const ::IN6_PKTINFO*>(WSA_CMSG_DATA(cmsg));
            destination = IPSocketAddress(info->ipi6_addr, _local_address.port());
        }
        else if (timestamp != nullptr && cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SO_TIMESTAMP && cmsg->cmsg_len >= sizeof(uint64_t)) {
            const uint64_t* ts = reinterpret_cast<const uint64_t*>(WSA_CMSG_DATA(cmsg));
            if (ts != nullptr && *ts != 0) {
                // Got a timestamp. Its frequency is returned by QueryPerformanceFrequency().
                ::LARGE_INTEGER freq;
                TS_ZERO(freq);
                if (QueryPerformanceFrequency(&freq) && freq.QuadPart != 0) {
                    *timestamp = cn::microseconds((*ts * 1'000'000) / freq.QuadPart);
                    if (timestamp_type != nullptr) {
                        *timestamp_type = TimeStampType::SOFTWARE;
                    }
                }
            }
        }
    }

#else
    // UNIX implementation, use a standard recvmsg sequence.

    // Build an iovec pointing to the message.
    ::iovec vec;
    TS_ZERO(vec);
    vec.iov_base = data;
    vec.iov_len = max_size;

    // Reserve a buffer to receive packet ancillary data.
    uint8_t ancil_data[1024];
    TS_ZERO(ancil_data);

    // Build a msghdr structure for recvmsg().
    ::msghdr hdr;
    TS_ZERO(hdr);
    hdr.msg_name = &sender_sock;
    hdr.msg_namelen = sizeof(sender_sock);
    hdr.msg_iov = &vec;
    hdr.msg_iovlen = 1; // number of iovec structures
    hdr.msg_control = ancil_data;
    hdr.msg_controllen = sizeof(ancil_data);

    // Wait for a message.
    SysSocketSignedSizeType insize = ::recvmsg(getSocket(), &hdr, 0);

    if (insize < 0) {
        return LastSysErrorCode();
    }

    // On Linux, keep timestamp from SO_TIMESTAMPING over SO_TIMESTAMPNS when both are available.
    [[maybe_unused]] bool got_timestamp = false;

    TS_PUSH_WARNING()
    TS_GCC_NOWARNING(zero-as-null-pointer-constant) // invalid definition of CMSG_NXTHDR in musl libc (Alpine Linux)
#if defined(TS_OPENBSD)
    TS_LLVM_NOWARNING(cast-align) // invalid definition of CMSG_NXTHDR on OpenBSD
#endif

    // Browse returned ancillary data.
    for (::cmsghdr* cmsg = CMSG_FIRSTHDR(&hdr); cmsg != nullptr; cmsg = CMSG_NXTHDR(&hdr, cmsg)) {

        // Look for destination IP address.
        // IP_PKTINFO is used on all Unix, except FreeBSD.
#if defined(IP_PKTINFO)
        if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_PKTINFO && cmsg->cmsg_len >= sizeof(::in_pktinfo)) {
            const ::in_pktinfo* info = reinterpret_cast<const ::in_pktinfo*>(CMSG_DATA(cmsg));
            destination = IPSocketAddress(info->ipi_addr, _local_address.port());
        }
#endif
#if defined(IPV6_PKTINFO)
        if (cmsg->cmsg_level == IPPROTO_IPV6 && cmsg->cmsg_type == IPV6_PKTINFO && cmsg->cmsg_len >= sizeof(::in6_pktinfo)) {
            const ::in6_pktinfo* info = reinterpret_cast<const ::in6_pktinfo*>(CMSG_DATA(cmsg));
            destination = IPSocketAddress(info->ipi6_addr, _local_address.port());
        }
#endif
#if defined(IP_RECVDSTADDR)
        if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_RECVDSTADDR && cmsg->cmsg_len >= sizeof(::in_addr)) {
            const ::in_addr* info = reinterpret_cast<const ::in_addr*>(CMSG_DATA(cmsg));
            destination = IPSocketAddress(*info, _local_address.port());
        }
#endif

        // Look for receive timestamp.
        if (timestamp != nullptr && !got_timestamp && cmsg->cmsg_level == SOL_SOCKET) {

#if defined(SO_TIMESTAMP)
            if (cmsg->cmsg_type == SCM_TIMESTAMP && cmsg->cmsg_len >= sizeof(::timeval)) {
                // System timestamp in microseconds.
                const ::timeval* ts = reinterpret_cast<const ::timeval*>(CMSG_DATA(cmsg));
                const cn::microseconds::rep micro = cn::microseconds::rep(ts->tv_sec) * 1'000'000 + cn::microseconds::rep(ts->tv_usec);
                // System time stamp is valid when not zero.
                if (micro != 0) {
                    *timestamp = cn::microseconds(micro);
                    if (timestamp_type != nullptr) {
                        *timestamp_type = TimeStampType::SOFTWARE;
                    }
                }
            }
#endif

#if defined(SO_TIMESTAMPNS)
            if (cmsg->cmsg_type == SCM_TIMESTAMPNS && cmsg->cmsg_len >= sizeof(::timespec)) {
                // System timestamp in nanoseconds.
                const ::timespec* ts = reinterpret_cast<const ::timespec*>(CMSG_DATA(cmsg));
                const cn::nanoseconds::rep nano = cn::nanoseconds::rep(ts->tv_sec) * 1'000'000'000 + cn::nanoseconds::rep(ts->tv_nsec);
                // System time stamp is valid when not zero, convert it to micro-seconds.
                if (nano != 0) {
                    *timestamp = cn::duration_cast<cn::microseconds>(cn::nanoseconds(nano));
                    if (timestamp_type != nullptr) {
                        *timestamp_type = TimeStampType::SOFTWARE;
                    }
                }
            }
#endif

#if defined(TS_SO_TIMESTAMPING)
            if (cmsg->cmsg_type == TS_SCM_TIMESTAMPING && cmsg->cmsg_len >= sizeof(TS_STRUCT_SCM_TIMESTAMPING)) {
                const TS_STRUCT_SCM_TIMESTAMPING* ts = reinterpret_cast<const TS_STRUCT_SCM_TIMESTAMPING*>(CMSG_DATA(cmsg));
                // Try hardware timestamp at index 2.
                cn::nanoseconds::rep nano = cn::nanoseconds::rep(ts->ts[2].tv_sec) * 1'000'000'000 + cn::nanoseconds::rep(ts->ts[2].tv_nsec);
                if (nano != 0) {
                    // Got a hardware timestamp.
                    got_timestamp = true;
                    *timestamp = cn::duration_cast<cn::microseconds>(cn::nanoseconds(nano));
                    if (timestamp_type != nullptr) {
                        *timestamp_type = TimeStampType::HARDWARE;
                    }
                }
                else {
                    // Try software timestamp at index 0.
                    nano = cn::nanoseconds::rep(ts->ts[0].tv_sec) * 1'000'000'000 + cn::nanoseconds::rep(ts->ts[0].tv_nsec);
                    if (nano != 0) {
                        // Got a software timestamp.
                        got_timestamp = true;
                        *timestamp = cn::duration_cast<cn::microseconds>(cn::nanoseconds(nano));
                        if (timestamp_type != nullptr) {
                            *timestamp_type = TimeStampType::SOFTWARE;
                        }
                    }
                }
            }
#endif
        }
    }

    TS_POP_WARNING()

#endif // Windows vs. UNIX

    // Successfully received a message
    ret_size = size_t(insize);
    sender = IPSocketAddress(sender_sock);

    return 0; // success
}
