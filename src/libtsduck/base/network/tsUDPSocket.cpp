//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsUDPSocket.h"
#include "tsNullReport.h"
#include "tsSysUtils.h"

// Network timestampting feature in Linux.
#if defined(TS_LINUX)
    #include <linux/net_tstamp.h>
#endif

// Furiously idiotic Windows feature, see comment in receiveOne()
#if defined(TS_WINDOWS)
    volatile ::LPFN_WSARECVMSG ts::UDPSocket::_wsaRevcMsg = 0;
#endif


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::UDPSocket::UDPSocket(bool auto_open, Report& report)
{
    if (auto_open) {
        UDPSocket::open(report);
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

bool ts::UDPSocket::open(Report& report)
{
    // Create a datagram socket.
    if (!createSocket(PF_INET, SOCK_DGRAM, IPPROTO_UDP, report)) {
        return false;
    }

    // Set the IP_PKTINFO option. This option is used to get the destination address of all
    // UDP packets arriving on this socket. Actual socket option is an int.
    // On FreeBSD, this option is replaced by IP_RECVDSTADDR.
#if defined(IP_PKTINFO)
    int opt = 1;
    if (::setsockopt(getSocket(), IPPROTO_IP, IP_PKTINFO, SysSockOptPointer(&opt), sizeof(opt)) != 0) {
        report.error(u"error setting socket IP_PKTINFO option: %s", {SysErrorCodeMessage()});
        return false;
    }
#elif defined(IP_RECVDSTADDR)
    int opt = 1;
    if (::setsockopt(getSocket(), IPPROTO_IP, IP_RECVDSTADDR, SysSockOptPointer(&opt), sizeof(opt)) != 0) {
        report.error(u"error setting socket IP_RECVDSTADDR option: %s", {SysErrorCodeMessage()});
        return false;
    }
#endif

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
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::UDPSocket::bind(const IPv4SocketAddress& addr, Report& report)
{
    ::sockaddr sock_addr;
    addr.copy(sock_addr);

    report.debug(u"binding socket to %s", {addr});
    if (::bind(getSocket(), &sock_addr, sizeof(sock_addr)) != 0) {
        report.error(u"error binding socket to local address: %s", {SysErrorCodeMessage()});
        return false;
    }

    // Keep a cached value of the bound local address.
    return getLocalAddress(_local_address, report);
}


//----------------------------------------------------------------------------
// Set outgoing local address for multicast messages.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::UDPSocket::setOutgoingMulticast(const UString& name, Report& report)
{
    IPv4Address addr;
    return addr.resolve(name, report) && setOutgoingMulticast(addr, report);
}

bool ts::UDPSocket::setOutgoingMulticast(const IPv4Address& addr, Report& report)
{
    ::in_addr iaddr;
    addr.copy(iaddr);

    if (::setsockopt(getSocket(), IPPROTO_IP, IP_MULTICAST_IF, SysSockOptPointer(&iaddr), sizeof(iaddr)) != 0) {
        report.error(u"error setting outgoing local address: %s", {SysErrorCodeMessage()});
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Set a default destination address and port for outgoing messages.
// Both address and port are mandatory in socket address.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::UDPSocket::setDefaultDestination(const UString& name, Report& report)
{
    IPv4SocketAddress addr;
    return addr.resolve(name, report) && setDefaultDestination(addr, report);
}

bool ts::UDPSocket::setDefaultDestination(const IPv4SocketAddress& addr, Report& report)
{
    if (!addr.hasAddress()) {
        report.error(u"missing IP address in UDP destination");
        return false;
    }
    else if (!addr.hasPort()) {
        report.error(u"missing port number in UDP destination");
        return false;
    }
    else {
        _default_destination = addr;
        return true;
    }
}


//----------------------------------------------------------------------------
// Set the Time To Live (TTL) option.
//----------------------------------------------------------------------------

bool ts::UDPSocket::setTTL(int ttl, bool multicast, Report& report)
{
    if (multicast) {
        SysSocketMulticastTTLType mttl = SysSocketMulticastTTLType(ttl);
        if (::setsockopt(getSocket(), IPPROTO_IP, IP_MULTICAST_TTL, SysSockOptPointer(&mttl), sizeof(mttl)) != 0) {
            report.error(u"socket option multicast TTL: %s", {SysErrorCodeMessage()});
            return false;
        }
    }
    else {
        SysSocketTTLType uttl = SysSocketTTLType(ttl);
        if (::setsockopt(getSocket(), IPPROTO_IP, IP_TTL, SysSockOptPointer(&uttl), sizeof(uttl)) != 0) {
            report.error(u"socket option unicast TTL: %s", {SysErrorCodeMessage()});
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Set the Type Of Service (TOS) option.
//----------------------------------------------------------------------------

bool ts::UDPSocket::setTOS(int tos, Report& report)
{
    SysSocketTOSType utos = SysSocketTOSType(tos);
    if (::setsockopt(getSocket(), IPPROTO_IP, IP_TOS, SysSockOptPointer(&utos), sizeof(utos)) != 0) {
        report.error(u"socket option TOS: %s", {SysErrorCodeMessage()});
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Set the multicast loop option.
//----------------------------------------------------------------------------

bool ts::UDPSocket::setMulticastLoop(bool on, Report& report)
{
    SysSocketMulticastLoopType mloop = SysSocketMulticastLoopType(on);
    report.debug(u"setting socket IP_MULTICAST_LOOP to %d", {mloop});
    if (::setsockopt(getSocket(), IPPROTO_IP, IP_MULTICAST_LOOP, SysSockOptPointer(&mloop), sizeof(mloop)) != 0) {
        report.error(u"socket option multicast loop: %s", {SysErrorCodeMessage()});
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Enable or disable the generation of receive timestamps.
//----------------------------------------------------------------------------

bool ts::UDPSocket::setReceiveTimestamps(bool on, Report& report)
{
    // The option exists only on Linux and is silently ignored on other systems.
#if defined(TS_LINUX)
    // Set SO_TIMESTAMPNS option which reports timestamps in nanoseconds (struct timespec).
    int enable = int(on);
    if (::setsockopt(getSocket(), SOL_SOCKET, SO_TIMESTAMPNS, &enable, sizeof(enable)) != 0) {
        report.error(u"socket option SO_TIMESTAMPNS: %s", {SysErrorCodeMessage()});
        return false;
    }
#endif

    return true;
}


//----------------------------------------------------------------------------
// Enable or disable the broadcast option.
//----------------------------------------------------------------------------

bool ts::UDPSocket::setBroadcast(bool on, Report& report)
{
    int enable = int(on);
    if (::setsockopt(getSocket(), SOL_SOCKET, SO_BROADCAST, SysSockOptPointer(&enable), sizeof(enable)) != 0) {
        report.error(u"socket option broadcast: %s", {SysErrorCodeMessage()});
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Enable or disable the broadcast option, based on an IP address.
//----------------------------------------------------------------------------

bool ts::UDPSocket::setBroadcastIfRequired(const IPv4Address destination, Report& report)
{
    // Get all local interfaces.
    IPv4AddressMaskVector locals;
    if (!GetLocalIPAddresses(locals, report)) {
        return false;
    }

    // Loop on all local addresses and set broadcast when we match a local broadcast address.
    for (const auto& it : locals) {
        if (destination == it.broadcastAddress()) {
            return setBroadcast(true, report);
        }
    }

    // Not a broadcast address, nothing was done.
    return true;
}


//----------------------------------------------------------------------------
// Join one multicast group on one local interface.
//----------------------------------------------------------------------------

bool ts::UDPSocket::addMembership(const IPv4Address& multicast, const IPv4Address& local, const IPv4Address& source, Report& report)
{
    // Verbose message about joining the group.
    UString groupString;
    if (source.hasAddress()) {
        groupString = source.toString() + u"@";
    }
    groupString += multicast.toString();
    if (local.hasAddress()) {
        report.verbose(u"joining multicast group %s from local address %s", {groupString, local});
    }
    else {
        report.verbose(u"joining multicast group %s from default interface", {groupString});
    }

    // Now join the group.
    if (source.hasAddress()) {
        // Source-specific multicast (SSM).
#if defined(TS_NO_SSM)
        report.error(u"source-specific multicast (SSM) is not supported on this operating system");
        return false;
#else
        SSMReq req(multicast, local, source);
        if (::setsockopt(getSocket(), IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP, SysSockOptPointer(&req.data), sizeof(req.data)) != 0) {
            report.error(u"error adding SSM membership to %s from local address %s: %s", {groupString, local, SysErrorCodeMessage()});
            return false;
        }
        else {
            _ssmcast.insert(req);
            return true;
        }
#endif
    }
    else {
        // Standard multicast.
        MReq req(multicast, local);
        if (::setsockopt(getSocket(), IPPROTO_IP, IP_ADD_MEMBERSHIP, SysSockOptPointer(&req.data), sizeof(req.data)) != 0) {
            report.error(u"error adding multicast membership to %s from local address %s: %s", {groupString, local, SysErrorCodeMessage()});
            return false;
        }
        else {
            _mcast.insert(req);
            return true;
        }
    }
}


//----------------------------------------------------------------------------
// Join one multicast group, let the system select the local interface.
//----------------------------------------------------------------------------

bool ts::UDPSocket::addMembershipDefault(const IPv4Address& multicast, const IPv4Address& source, Report& report)
{
    return addMembership(multicast, IPv4Address(), source, report);
}


//----------------------------------------------------------------------------
// Join one multicast group on all local interfaces.
//----------------------------------------------------------------------------

bool ts::UDPSocket::addMembershipAll(const IPv4Address& multicast, const IPv4Address& source, Report& report)
{
    // There is no implicit way to listen on all interfaces.
    // If no local address is specified, we must get the list
    // of all local interfaces and send a multicast membership
    // request on each of them.

    // Get all local interfaces.
    IPv4AddressVector loc_if;
    if (!GetLocalIPAddresses(loc_if, report)) {
        return false;
    }

    // Add all memberships
    bool ok = true;
    for (size_t i = 0; i < loc_if.size(); ++i) {
        if (loc_if[i].hasAddress()) {
            ok = addMembership(multicast, loc_if[i], source, report) && ok;
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

    // Drop all standard multicast groups.
    for (const auto& it : _mcast) {
        report.verbose(u"leaving multicast group %s from local address %s", {IPv4Address(it.data.imr_multiaddr), IPv4Address(it.data.imr_interface)});
        if (::setsockopt(getSocket(), IPPROTO_IP, IP_DROP_MEMBERSHIP, SysSockOptPointer(&it.data), sizeof(it.data)) != 0) {
            report.error(u"error dropping multicast membership: %s", {SysErrorCodeMessage()});
            ok = false;
        }
    }
    _mcast.clear();

    // Drop all source-specific multicast groups.
#if !defined(TS_NO_SSM)
    for (const auto& it : _ssmcast) {
        report.verbose(u"leaving multicast group %s@%s from local address %s",
                       {IPv4Address(it.data.imr_sourceaddr), IPv4Address(it.data.imr_multiaddr), IPv4Address(it.data.imr_interface)});
        if (::setsockopt(getSocket(), IPPROTO_IP, IP_DROP_SOURCE_MEMBERSHIP, SysSockOptPointer(&it.data), sizeof(it.data)) != 0) {
            report.error(u"error dropping multicast membership: %s", {SysErrorCodeMessage()});
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

bool ts::UDPSocket::send(const void* data, size_t size, const IPv4SocketAddress& dest, Report& report)
{
    ::sockaddr addr;
    dest.copy(addr);

    if (::sendto(getSocket(), SysSendBufferPointer(data), SysSendSizeType(size), 0, &addr, sizeof(addr)) < 0) {
        report.error(u"error sending UDP message: %s", {SysErrorCodeMessage()});
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Receive a message.
// If abort interface is non-zero, invoke it when I/O is interrupted
// (in case of user-interrupt, return, otherwise retry).
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::UDPSocket::receive(void* data,
                            size_t max_size,
                            size_t& ret_size,
                            IPv4SocketAddress& sender,
                            IPv4SocketAddress& destination,
                            const AbortInterface* abort,
                            Report& report,
                            MicroSecond* timestamp)
{
    // Clear timestamp if specified.
    if (timestamp != nullptr) {
        *timestamp = -1;
    }

    // Loop on unsollicited interrupts
    for (;;) {

        // Wait for a message.
        const int err = receiveOne(data, max_size, ret_size, sender, destination, report, timestamp);

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
                report.error(u"error receiving from UDP socket: %s", {SysErrorCodeMessage(err)});
            }
            return false;
        }
    }
}


//----------------------------------------------------------------------------
// Perform one receive operation. Hide the system mud.
//----------------------------------------------------------------------------

int ts::UDPSocket::receiveOne(void* data,
                              size_t max_size,
                              size_t& ret_size,
                              IPv4SocketAddress& sender,
                              IPv4SocketAddress& destination,
                              Report& report,
                              MicroSecond* timestamp)
{
    // Clear returned values
    ret_size = 0;
    sender.clear();
    destination.clear();

    // Reserve a socket address to receive the sender address.
    ::sockaddr sender_sock;
    TS_ZERO(sender_sock);

    // Normally, this operation should be done quite easily using recvmsg.
    // On Windows, all socket operations are smoothly emulated, including
    // recvfrom, allowing a reasonable portability. However, in the specific
    // case of recvmsg, there is no equivalent but a similar - and carefully
    // incompatible - function named WSARecvMsg. Not only this function is
    // different from recvmsg, but it is also not exported from any DLL.
    // Its address must be queried dynamically. The stupid idiot who had
    // this pervert idea at Microsoft deserves to burn in hell (twice) !!

#if defined(TS_WINDOWS)

    // First, get the address of WSARecvMsg the first time we use it.
    if (_wsaRevcMsg == 0) {
        ::LPFN_WSARECVMSG funcAddress = 0;
        ::GUID guid = WSAID_WSARECVMSG;
        ::DWORD dwBytes = 0;
        const ::SOCKET sock = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (sock == INVALID_SOCKET) {
            return LastSysErrorCode();
        }
        if (::WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &funcAddress, sizeof(funcAddress), &dwBytes, 0, 0) != 0) {
            const int err = LastSysErrorCode();
            ::closesocket(sock);
            return err;
        }
        ::closesocket(sock);
        // Now update the volatile value.
        _wsaRevcMsg = funcAddress;
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
    msg.name = &sender_sock;
    msg.namelen = sizeof(sender_sock);
    msg.lpBuffers = &vec;
    msg.dwBufferCount = 1; // number of WSAMSG
    msg.Control.buf = ancil_data;
    msg.Control.len = ::ULONG(sizeof(ancil_data));

    // Wait for a message.
    ::DWORD insize = 0;
    if (_wsaRevcMsg(getSocket(), &msg, &insize, 0, 0)  != 0) {
        return LastSysErrorCode();
    }

    // Browse returned ancillary data.
    for (::WSACMSGHDR* cmsg = WSA_CMSG_FIRSTHDR(&msg); cmsg != 0; cmsg = WSA_CMSG_NXTHDR(&msg, cmsg)) {
        if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_PKTINFO) {
            const ::IN_PKTINFO* info = reinterpret_cast<const ::IN_PKTINFO*>(WSA_CMSG_DATA(cmsg));
            destination = IPv4SocketAddress(info->ipi_addr, _local_address.port());
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
            destination = IPv4SocketAddress(info->ipi_addr, _local_address.port());
        }
#elif defined(IP_RECVDSTADDR)
        if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_RECVDSTADDR && cmsg->cmsg_len >= sizeof(::in_addr)) {
            const ::in_addr* info = reinterpret_cast<const ::in_addr*>(CMSG_DATA(cmsg));
            destination = IPv4SocketAddress(*info, _local_address.port());
        }
#endif

        // On Linux, look for receive timestamp.
#if defined(TS_LINUX)
        else if (timestamp != nullptr && cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SO_TIMESTAMPNS && cmsg->cmsg_len >= sizeof(::timespec)) {
            // System time stamp in nanosecond.
            const ::timespec* ts = reinterpret_cast<const ::timespec*>(CMSG_DATA(cmsg));
            const NanoSecond nano = NanoSecond(ts->tv_sec) * NanoSecPerSec + NanoSecond(ts->tv_nsec);
            // System time stamp is valid when not zero, convert it to micro-seconds.
            if (nano != 0) {
                *timestamp = nano / NanoSecPerMicroSec;
            }
        }
#endif
    }

    TS_POP_WARNING()

#endif // Windows vs. UNIX

    // Successfully received a message
    ret_size = size_t(insize);
    sender = IPv4SocketAddress(sender_sock);

    return 0; // success
}
