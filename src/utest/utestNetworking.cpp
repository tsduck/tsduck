//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for networking classes.
//
//----------------------------------------------------------------------------

#include "tsIPAddress.h"
#include "tsIPAddressMask.h"
#include "tsIPSocketAddress.h"
#include "tsTCPConnection.h"
#include "tsTCPServer.h"
#include "tsUDPSocket.h"
#include "tsMACAddress.h"
#include "tsNetworkInterface.h"
#include "tsIPPacket.h"
#include "tsNullReport.h"
#include "tsIPUtils.h"
#include "tsCerrReport.h"
#include "utestTSUnitThread.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class NetworkingTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(SystemStructures);
    TSUNIT_DECLARE_TEST(IPv4AddressConstructors);
    TSUNIT_DECLARE_TEST(IPv4Address);
    TSUNIT_DECLARE_TEST(IPv6Address);
    TSUNIT_DECLARE_TEST(Conversion);
    TSUNIT_DECLARE_TEST(IPAddressMask);
    TSUNIT_DECLARE_TEST(MACAddress);
    TSUNIT_DECLARE_TEST(LocalHost);
    TSUNIT_DECLARE_TEST(GetLocalIPAddresses);
    TSUNIT_DECLARE_TEST(IPv4SocketAddressConstructors);
    TSUNIT_DECLARE_TEST(IPv4SocketAddress);
    TSUNIT_DECLARE_TEST(IPv6SocketAddress);
    TSUNIT_DECLARE_TEST(TCPSocket);
    TSUNIT_DECLARE_TEST(UDPSocket);
    TSUNIT_DECLARE_TEST(IPHeader);
    TSUNIT_DECLARE_TEST(IPProtocol);
    TSUNIT_DECLARE_TEST(TCPPacket);
    TSUNIT_DECLARE_TEST(UDPPacket);

public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

private:
    int _previousSeverity = 0;
};

TSUNIT_REGISTER(NetworkingTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void NetworkingTest::beforeTest()
{
    _previousSeverity = CERR.maxSeverity();
    if (tsunit::Test::debugMode()) {
        CERR.setMaxSeverity(ts::Severity::Debug);
    }
}

// Test suite cleanup method.
void NetworkingTest::afterTest()
{
    CERR.setMaxSeverity(_previousSeverity);
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(SystemStructures)
{
    debug() << "NetworkingTest::SystemStructures: sizeof(::in_addr) = " << sizeof(::in_addr) << std::endl
            << "NetworkingTest::SystemStructures: sizeof(::in6_addr) = " << sizeof(::in6_addr) << std::endl
            << "NetworkingTest::SystemStructures: sizeof(::sockaddr) = " << sizeof(::sockaddr) << std::endl
            << "NetworkingTest::SystemStructures: sizeof(::sockaddr_in) = " << sizeof(::sockaddr_in) << std::endl
            << "NetworkingTest::SystemStructures: sizeof(::sockaddr_in6) = " << sizeof(::sockaddr_in6) << std::endl
            << "NetworkingTest::SystemStructures: sizeof(::sockaddr_storage) = " << sizeof(::sockaddr_storage) << std::endl;
}

TSUNIT_DEFINE_TEST(IPv4AddressConstructors)
{
    TSUNIT_ASSERT(ts::IPInitialize());

    TSUNIT_EQUAL(0, ts::IPAddress::AnyAddress4.address4());
    TSUNIT_EQUAL(0x7F000001, ts::IPAddress::LocalHost4.address4()); // 127.0.0.1

    ts::IPAddress a1;
    TSUNIT_EQUAL(0, a1.address4());

    ts::IPAddress a2(0x01020304);
    TSUNIT_EQUAL(0x01020304, a2.address4());

    ts::IPAddress a3(1, 2, 3, 4);
    TSUNIT_EQUAL(0x01020304, a3.address4());

    ::in_addr ia4;
    ia4.s_addr = htonl(0x01020304);
    ts::IPAddress a4(ia4);
    TSUNIT_EQUAL(0x01020304, a4.address4());

    ::sockaddr sa5;
    TSUNIT_ASSERT(sizeof(::sockaddr) >= sizeof(::sockaddr_in));
    ::sockaddr_in* sai5 = reinterpret_cast<::sockaddr_in*>(&sa5);
    sai5->sin_family = AF_INET;
    sai5->sin_addr.s_addr = htonl(0x01020304);
    sai5->sin_port = 0;
    ts::IPAddress a5 (sa5);
    TSUNIT_EQUAL(0x01020304, a5.address4());

    ::sockaddr_in sa6;
    sa6.sin_family = AF_INET;
    sa6.sin_addr.s_addr = htonl(0x01020304);
    sa6.sin_port = 0;
    ts::IPAddress a6 (sa6);
    TSUNIT_EQUAL(0x01020304, a6.address4());

    ts::IPAddress a7(u"2.3.4.5", CERR);
    TSUNIT_EQUAL(0x02030405, a7.address4());
}

TSUNIT_DEFINE_TEST(IPv4Address)
{
    TSUNIT_ASSERT(ts::IPInitialize());

    ts::IPAddress a1 (1, 2, 3, 4);
    ts::IPAddress a2 (1, 2, 3, 4);
    ts::IPAddress a3 (2, 3, 4, 5);

    TSUNIT_EQUAL(u"IPv4", a1.familyName());
    TSUNIT_ASSERT(a1 == a2);
    TSUNIT_ASSERT(a1 != a3);

    a1.setAddress4(0x02030405);
    TSUNIT_ASSERT(a1 == a3);

    a1.setAddress4(1, 2, 3, 4);
    TSUNIT_ASSERT(a1 == a2);

    a2.setAddress4(224, 1, 2, 3);
    TSUNIT_ASSERT(!a1.isMulticast());
    TSUNIT_ASSERT(a2.isMulticast());

    TSUNIT_ASSERT(a1.hasAddress());
    a1.clear();
    TSUNIT_ASSERT(!a1.hasAddress());
    TSUNIT_EQUAL(0, a1.address4());

    a1.setAddress4(1, 2, 3, 4);
    ::in_addr ia;
    a1.getAddress4(ia);
    TSUNIT_EQUAL(htonl(0x01020304), ia.s_addr);

    ::sockaddr_storage sa;
    TSUNIT_EQUAL(sizeof(::sockaddr_in), a1.getAddress(sa, 80));
    const ::sockaddr_in* saip = reinterpret_cast<const ::sockaddr_in*>(&sa);
    TSUNIT_EQUAL(AF_INET, saip->sin_family);
    TSUNIT_EQUAL(htonl(0x01020304), saip->sin_addr.s_addr);
    TSUNIT_EQUAL(htons(80), saip->sin_port);

    ::sockaddr_in sai;
    a1.getAddress4(sai, 80);
    TSUNIT_EQUAL(AF_INET, sai.sin_family);
    TSUNIT_EQUAL(htonl(0x01020304), sai.sin_addr.s_addr);
    TSUNIT_EQUAL(htons(80), sai.sin_port);

    TSUNIT_ASSERT(a1.resolve(u"2.3.4.5", CERR));
    TSUNIT_EQUAL(0x02030405, a1.address4());

    a1.setAddress4(2, 3, 4, 5);
    const ts::UString s1(a1.toString());
    TSUNIT_EQUAL(u"2.3.4.5", s1);

    // Note: fail if not connected to a network.
    debug() << "NetworkingTest: www.google.com = " << ts::IPAddress(u"www.google.com", CERR) << std::endl;
}

TSUNIT_DEFINE_TEST(IPv6Address)
{
    ts::IPAddress a1(ts::IPAddress::AnyAddress6);
    TSUNIT_EQUAL(u"IPv6", a1.familyName());
    TSUNIT_EQUAL(ts::IP::v6, a1.generation());
    TSUNIT_ASSERT(!a1.hasAddress());
    TSUNIT_ASSERT(!a1.isMulticast());

    TSUNIT_ASSERT(!ts::IPAddress::AnyAddress6.hasAddress());
    TSUNIT_ASSERT(ts::IPAddress::LocalHost6.hasAddress());
    TSUNIT_EQUAL(0, ts::IPAddress::LocalHost6.networkPrefix6());
    TSUNIT_EQUAL(1, ts::IPAddress::LocalHost6.interfaceIdentifier6());

    TSUNIT_ASSERT(!a1.resolve(u":", NULLREP));
    TSUNIT_ASSERT(!a1.hasAddress());

    TSUNIT_ASSERT(a1.resolve(u"::", CERR));
    TSUNIT_ASSERT(!a1.hasAddress());
    TSUNIT_ASSERT(a1 == ts::IPAddress::AnyAddress6);

    TSUNIT_ASSERT(a1.resolve(u"::1", CERR));
    TSUNIT_ASSERT(a1.hasAddress());
    TSUNIT_ASSERT(a1 == ts::IPAddress::LocalHost6);

    TSUNIT_ASSERT(!a1.resolve(u"", NULLREP));
    TSUNIT_ASSERT(!a1.hasAddress());

    a1.setAddress6(0, 1, 2, 3, 4, 5, 6, 7);
    TSUNIT_ASSERT(a1.hasAddress());
    TSUNIT_ASSERT(!a1.isIPv4Mapped());
    TSUNIT_EQUAL(0x0000000100020003, a1.networkPrefix6());
    TSUNIT_EQUAL(0x0004000500060007, a1.interfaceIdentifier6());
    TSUNIT_EQUAL(u"0:1:2:3:4:5:6:7", a1.toString());
    TSUNIT_EQUAL(u"0000:0001:0002:0003:0004:0005:0006:0007", a1.toFullString());

    a1.setAddress6(0x12, 0x345, 0x6789, 0xFFFF, 0, 0, 0, 0xBEEF);
    TSUNIT_ASSERT(a1.hasAddress());
    TSUNIT_ASSERT(!a1.isIPv4Mapped());
    TSUNIT_EQUAL(0x001203456789FFFF, a1.networkPrefix6());
    TSUNIT_EQUAL(0x000000000000BEEF, a1.interfaceIdentifier6());
    TSUNIT_EQUAL(u"12:345:6789:ffff::beef", a1.toString());
    TSUNIT_EQUAL(u"0012:0345:6789:ffff:0000:0000:0000:beef", a1.toFullString());

    TSUNIT_ASSERT(a1.resolve(u"fe80::93a3:dea0:2108:b81e", CERR));
    TSUNIT_ASSERT(a1.hasAddress());
    TSUNIT_ASSERT(!a1.isIPv4Mapped());
    TSUNIT_EQUAL(0xFE80000000000000, a1.networkPrefix6());
    TSUNIT_EQUAL(0x93A3DEA02108B81E, a1.interfaceIdentifier6());
    TSUNIT_EQUAL(u"fe80::93a3:dea0:2108:b81e", a1.toString());
    TSUNIT_EQUAL(u"fe80:0000:0000:0000:93a3:dea0:2108:b81e", a1.toFullString());

    TSUNIT_ASSERT(a1.convert(ts::IP::Any));
    TSUNIT_ASSERT(a1.convert(ts::IP::v6));
    TSUNIT_ASSERT(!a1.convert(ts::IP::v4));
    TSUNIT_EQUAL(u"IPv6", a1.familyName());
    TSUNIT_EQUAL(ts::IP::v6, a1.generation());

    ts::IPAddress a2;
    TSUNIT_ASSERT(a2.resolve(u"0:0::ffff:12.13.14.15", CERR));
    TSUNIT_ASSERT(a2.hasAddress());
    TSUNIT_EQUAL(u"IPv6", a2.familyName());
    TSUNIT_EQUAL(ts::IP::v6, a2.generation());
    TSUNIT_ASSERT(a2.isIPv4Mapped());
    TSUNIT_EQUAL(0x0000000000000000, a2.networkPrefix6());
    TSUNIT_EQUAL(0x0000FFFF0C0D0E0F, a2.interfaceIdentifier6());
    TSUNIT_EQUAL(u"::ffff:12.13.14.15", a2.toString());
    TSUNIT_EQUAL(u"0000:0000:0000:0000:0000:ffff:0c0d:0e0f", a2.toFullString());

    TSUNIT_ASSERT(a2.convert(ts::IP::Any));
    TSUNIT_ASSERT(a2.convert(ts::IP::v6));
    TSUNIT_ASSERT(a2.convert(ts::IP::v4));

    TSUNIT_ASSERT(a2.hasAddress());
    TSUNIT_EQUAL(u"IPv4", a2.familyName());
    TSUNIT_EQUAL(ts::IP::v4, a2.generation());
    TSUNIT_EQUAL(u"12.13.14.15", a2.toString());
    TSUNIT_EQUAL(u"12.13.14.15", a2.toFullString());
    TSUNIT_EQUAL(0x0C0D0E0F, a2.address4());
}

TSUNIT_DEFINE_TEST(Conversion)
{
    ts::IPAddress a1(0x12345678);
    TSUNIT_EQUAL(u"IPv4", a1.familyName());
    TSUNIT_EQUAL(ts::IP::v4, a1.generation());
    TSUNIT_EQUAL(u"18.52.86.120", a1.toString());
    TSUNIT_EQUAL(u"18.52.86.120", a1.toFullString());
    TSUNIT_EQUAL(0x12345678, a1.address4());

    TSUNIT_ASSERT(a1.convert(ts::IP::Any));
    TSUNIT_ASSERT(a1.convert(ts::IP::v4));
    TSUNIT_ASSERT(a1.convert(ts::IP::v6));

    TSUNIT_EQUAL(u"IPv6", a1.familyName());
    TSUNIT_EQUAL(ts::IP::v6, a1.generation());
    TSUNIT_ASSERT(a1.isIPv4Mapped());
    TSUNIT_EQUAL(0x0000000000000000, a1.networkPrefix6());
    TSUNIT_EQUAL(0x0000FFFF12345678, a1.interfaceIdentifier6());
    TSUNIT_EQUAL(u"::ffff:18.52.86.120", a1.toString());
    TSUNIT_EQUAL(u"0000:0000:0000:0000:0000:ffff:1234:5678", a1.toFullString());

    ts::IPAddress a2(ts::IPAddress::AnyAddress4);
    TSUNIT_ASSERT(a2.convert(ts::IP::Any));
    TSUNIT_ASSERT(a2 == ts::IPAddress::AnyAddress4);
    TSUNIT_ASSERT(a2.convert(ts::IP::v4));
    TSUNIT_ASSERT(a2 == ts::IPAddress::AnyAddress4);
    TSUNIT_ASSERT(a2.convert(ts::IP::v6));
    TSUNIT_ASSERT(a2 == ts::IPAddress::AnyAddress6);

    TSUNIT_ASSERT(a2.convert(ts::IP::Any));
    TSUNIT_ASSERT(a2 == ts::IPAddress::AnyAddress6);
    TSUNIT_ASSERT(a2.convert(ts::IP::v6));
    TSUNIT_ASSERT(a2 == ts::IPAddress::AnyAddress6);
    TSUNIT_ASSERT(a2.convert(ts::IP::v4));
    TSUNIT_ASSERT(a2 == ts::IPAddress::AnyAddress4);

    a2 = ts::IPAddress::LocalHost4;
    TSUNIT_ASSERT(a2.convert(ts::IP::Any));
    TSUNIT_ASSERT(a2 == ts::IPAddress::LocalHost4);
    TSUNIT_ASSERT(a2.convert(ts::IP::v4));
    TSUNIT_ASSERT(a2 == ts::IPAddress::LocalHost4);
    TSUNIT_ASSERT(a2.convert(ts::IP::v6));
    TSUNIT_ASSERT(a2 == ts::IPAddress::LocalHost6);

    TSUNIT_ASSERT(a2.convert(ts::IP::Any));
    TSUNIT_ASSERT(a2 == ts::IPAddress::LocalHost6);
    TSUNIT_ASSERT(a2.convert(ts::IP::v6));
    TSUNIT_ASSERT(a2 == ts::IPAddress::LocalHost6);
    TSUNIT_ASSERT(a2.convert(ts::IP::v4));
    TSUNIT_ASSERT(a2 == ts::IPAddress::LocalHost4);
}

TSUNIT_DEFINE_TEST(IPAddressMask)
{
    ts::IPAddressMask a1(ts::IPAddress(u"1.2.4.5", CERR), 23);
    TSUNIT_ASSERT(a1.hasAddress());
    TSUNIT_EQUAL(u"IPv4", a1.familyName());
    TSUNIT_EQUAL(ts::IP::v4, a1.generation());
    TSUNIT_EQUAL(0x01020405, a1.address4());
    TSUNIT_EQUAL(23, a1.prefixSize());
    TSUNIT_EQUAL(u"255.255.254.0", a1.mask().toString());
    TSUNIT_EQUAL(u"1.2.5.255", a1.broadcastAddress().toString());
    TSUNIT_EQUAL(u"1.2.4.5/23", a1.toString());
    TSUNIT_EQUAL(u"1.2.4.5/23", a1.toFullString());

    a1.setMask(ts::IPAddress(255, 128, 0, 0));
    TSUNIT_EQUAL(9, a1.prefixSize());
    TSUNIT_EQUAL(u"255.128.0.0", a1.mask().toString());
    TSUNIT_EQUAL(u"1.127.255.255", a1.broadcastAddress().toString());
    TSUNIT_EQUAL(u"1.2.4.5/9", a1.toString());
    TSUNIT_EQUAL(u"1.2.4.5/9", a1.toFullString());
}

TSUNIT_DEFINE_TEST(MACAddress)
{
    ts::MACAddress a1;
    TSUNIT_EQUAL(u"MAC", a1.familyName());
    TSUNIT_ASSERT(!a1.hasAddress());
    TSUNIT_ASSERT(!a1.isMulticast());

    TSUNIT_ASSERT(a1.resolve(u"52:54:00:26:92:b4", CERR));
    TSUNIT_ASSERT(a1.hasAddress());
    TSUNIT_ASSERT(!a1.isMulticast());
    TSUNIT_EQUAL(0x5254002692B4, a1.address());
    TSUNIT_EQUAL(u"52:54:00:26:92:B4", a1.toString());

    TSUNIT_ASSERT(a1.resolve(u" 23:b3-A6 . bE : 56-4D", CERR));
    TSUNIT_ASSERT(a1.hasAddress());
    TSUNIT_ASSERT(!a1.isMulticast());
    TSUNIT_EQUAL(0x23B3A6BE564D, a1.address());
    TSUNIT_EQUAL(u"23:B3:A6:BE:56:4D", a1.toString());

    TSUNIT_ASSERT(a1.toMulticast(ts::IPAddress(225, 1, 2, 3)));
    TSUNIT_ASSERT(a1.hasAddress());
    TSUNIT_ASSERT(a1.isMulticast());
    TSUNIT_EQUAL(0x01005E010203, a1.address());
    TSUNIT_EQUAL(u"01:00:5E:01:02:03", a1.toString());

    TSUNIT_ASSERT(!a1.toMulticast(ts::IPAddress(192, 168, 2, 3)));
    TSUNIT_ASSERT(!a1.hasAddress());
    TSUNIT_ASSERT(!a1.isMulticast());
}

TSUNIT_DEFINE_TEST(LocalHost)
{
    // Force resolution in IPv4.
    ts::IPAddress a1;
    TSUNIT_ASSERT(a1.resolve(u"localhost", CERR, ts::IP::v4));
    TSUNIT_EQUAL(0x7F000001, a1.address4()); // 127.0.0.1
    TSUNIT_ASSERT(a1 == ts::IPAddress::LocalHost4);

    // Some hosts can return localhost in IPv4 or IPv6.
    debug() << "NetworkingTest: localhost = " << ts::IPAddress(u"localhost", CERR) << std::endl;

    ts::IPAddress a2(u"localhost", CERR);
    if (a2.generation() == ts::IP::v6) {
        TSUNIT_EQUAL(0, a2.hexlet6(0));
        TSUNIT_EQUAL(0, a2.hexlet6(1));
        TSUNIT_EQUAL(0, a2.hexlet6(2));
        TSUNIT_EQUAL(0, a2.hexlet6(3));
        TSUNIT_EQUAL(0, a2.hexlet6(4));
        TSUNIT_EQUAL(0, a2.hexlet6(5));
        TSUNIT_EQUAL(0, a2.hexlet6(8));
        TSUNIT_EQUAL(1, a2.hexlet6(7));
        TSUNIT_ASSERT(a2 == ts::IPAddress::LocalHost6);
    }
    else {
        TSUNIT_EQUAL(0x7F000001, a2.address4()); // 127.0.0.1
        TSUNIT_ASSERT(a2 == ts::IPAddress::LocalHost4);
    }
}

TSUNIT_DEFINE_TEST(GetLocalIPAddresses)
{
    TSUNIT_ASSERT(ts::IPInitialize());

    // We cannot assume that the local system has any local address.
    // We only requires that the call does not fail.
    ts::IPAddressVector addr;
    TSUNIT_ASSERT(ts::NetworkInterface::GetAll(addr));

    ts::NetworkInterfaceVector netif;
    TSUNIT_ASSERT(ts::NetworkInterface::GetAll(netif));

    // The two calls must return the same number of addresses.
    TSUNIT_ASSERT(addr.size() == netif.size());

    debug() << "NetworkingTest: GetLocalIPAddresses: " << netif.size() << " local addresses" << std::endl;
    for (size_t i = 0; i < netif.size(); ++i) {
        debug() << "NetworkingTest: local address " << i
                << ": " << netif[i]
                << ", mask: " << netif[i].address.mask()
                << ", broadcast: " << netif[i].address.broadcastAddress() << std::endl;
    }

    for (size_t i = 0; i < addr.size(); ++i) {
        TSUNIT_ASSERT(ts::NetworkInterface::IsLocal(addr[i]));
    }

    for (size_t i = 0; i < netif.size(); ++i) {
        TSUNIT_ASSERT(ts::NetworkInterface::IsLocal(netif[i].address));
    }
}

TSUNIT_DEFINE_TEST(IPv4SocketAddressConstructors)
{
    TSUNIT_ASSERT(ts::IPInitialize());

    TSUNIT_EQUAL(0, ts::IPSocketAddress::AnyAddress4.address4());
    TSUNIT_EQUAL(0x7F000001, ts::IPSocketAddress::LocalHost4.address4()); // 127.0.0.1

    ts::IPSocketAddress a1;
    TSUNIT_EQUAL(0, a1.address4());
    TSUNIT_EQUAL(0, a1.port());

    ts::IPSocketAddress a2a (ts::IPAddress(0x01020304), 80);
    TSUNIT_EQUAL(0x01020304, a2a.address4());
    TSUNIT_EQUAL(80, a2a.port());

    ts::IPSocketAddress a2b(0x01020304, 80);
    TSUNIT_EQUAL(0x01020304, a2b.address4());
    TSUNIT_EQUAL(80, a2b.port());

    ts::IPSocketAddress a3(1, 2, 3, 4, 80);
    TSUNIT_EQUAL(0x01020304, a3.address4());
    TSUNIT_EQUAL(80, a3.port());

    ::in_addr ia4;
    ia4.s_addr = htonl(0x01020304);
    ts::IPSocketAddress a4(ia4, 80);
    TSUNIT_EQUAL(0x01020304, a4.address4());
    TSUNIT_EQUAL(80, a4.port());

    ::sockaddr sa5;
    TSUNIT_ASSERT(sizeof(::sockaddr) >= sizeof(::sockaddr_in));
    ::sockaddr_in* sai5 = reinterpret_cast<::sockaddr_in*>(&sa5);
    sai5->sin_family = AF_INET;
    sai5->sin_addr.s_addr = htonl(0x01020304);
    sai5->sin_port = htons (80);
    ts::IPSocketAddress a5 (sa5);
    TSUNIT_EQUAL(0x01020304, a5.address4());
    TSUNIT_EQUAL(80, a5.port());

    ::sockaddr_in sa6;
    sa6.sin_family = AF_INET;
    sa6.sin_addr.s_addr = htonl(0x01020304);
    sa6.sin_port = htons(80);
    ts::IPSocketAddress a6(sa6);
    TSUNIT_EQUAL(0x01020304, a6.address4());
    TSUNIT_EQUAL(80, a6.port());

    ts::IPSocketAddress a7(u"2.3.4.5", CERR);
    TSUNIT_EQUAL(0x02030405, a7.address4());
    TSUNIT_EQUAL(ts::IPSocketAddress::AnyPort, a7.port());

    // Some hosts can return localhost in IPv4 or IPv6.
    ts::IPSocketAddress a8(u"localhost", CERR);
    if (a8.generation() == ts::IP::v6) {
        TSUNIT_EQUAL(0, a8.hexlet6(0));
        TSUNIT_EQUAL(0, a8.hexlet6(1));
        TSUNIT_EQUAL(0, a8.hexlet6(2));
        TSUNIT_EQUAL(0, a8.hexlet6(3));
        TSUNIT_EQUAL(0, a8.hexlet6(4));
        TSUNIT_EQUAL(0, a8.hexlet6(5));
        TSUNIT_EQUAL(0, a8.hexlet6(8));
        TSUNIT_EQUAL(1, a8.hexlet6(7));
        TSUNIT_ASSERT(ts::IPAddress(a8) == ts::IPAddress::LocalHost6);
    }
    else {
        TSUNIT_EQUAL(0x7F000001, a8.address4()); // 127.0.0.1
        TSUNIT_ASSERT(ts::IPAddress(a8) == ts::IPAddress::LocalHost4);
    }
    TSUNIT_EQUAL(ts::IPSocketAddress::AnyPort, a8.port());

    ts::IPSocketAddress a9(u"2.3.4.5:80", CERR);
    TSUNIT_EQUAL(0x02030405, a9.address4());
    TSUNIT_EQUAL(80, a9.port());

    ts::IPSocketAddress a10(u":80", CERR);
    TSUNIT_EQUAL(0, a10.address4());
    TSUNIT_EQUAL(80, a10.port());

    ts::IPSocketAddress a11(u"83", CERR);
    TSUNIT_EQUAL(0, a11.address4());
    TSUNIT_EQUAL(83, a11.port());

    ts::IPSocketAddress a12(u"2.3.4.5:", CERR);
    TSUNIT_EQUAL(0x02030405, a12.address4());
    TSUNIT_EQUAL(0, a12.port());

    ts::IPSocketAddress a13(u":", CERR);
    TSUNIT_EQUAL(0, a13.address4());
    TSUNIT_EQUAL(0, a13.port());

    ts::IPSocketAddress a14(u"", CERR);
    TSUNIT_EQUAL(0, a14.address4());
    TSUNIT_EQUAL(0, a14.port());
}

TSUNIT_DEFINE_TEST(IPv4SocketAddress)
{
    TSUNIT_ASSERT(ts::IPInitialize());

    ts::IPSocketAddress a1(1, 2, 3, 4, 80);
    ts::IPSocketAddress a2(1, 2, 3, 4, 80);
    ts::IPSocketAddress a3(1, 3, 4, 5, 81);

    TSUNIT_ASSERT(a1 == a2);
    TSUNIT_ASSERT(a1 != a3);

    a1.setAddress4(1, 3, 4, 5);
    a1.setPort(81);
    TSUNIT_ASSERT(a1 == a3);

    a1.setPort(80);
    a1.setAddress4(1, 2, 3, 4);
    TSUNIT_ASSERT(a1 == a2);

    a2.setAddress4(5, 1, 2, 3);
    a2.setPort(8080);
    TSUNIT_EQUAL(0x05010203, a2.address4());
    TSUNIT_EQUAL(8080, a2.port());

    TSUNIT_ASSERT(a2.hasAddress());
    TSUNIT_ASSERT(a2.hasPort());
    a2.clear();
    TSUNIT_ASSERT(!a2.hasAddress());
    TSUNIT_ASSERT(!a2.hasPort());
    TSUNIT_EQUAL(0, a2.address4());
    TSUNIT_EQUAL(0, a2.port());

    a1.setAddress4(1, 2, 3, 4);
    a1.setPort(80);
    ::in_addr ia;
    a1.getAddress4(ia);
    TSUNIT_EQUAL(htonl(0x01020304), ia.s_addr);

    ::sockaddr_storage sa;
    TSUNIT_EQUAL(sizeof(::sockaddr_in), a1.get(sa));
    const ::sockaddr_in* saip = reinterpret_cast<const ::sockaddr_in*>(&sa);
    TSUNIT_EQUAL(AF_INET, saip->sin_family);
    TSUNIT_EQUAL(htonl(0x01020304), saip->sin_addr.s_addr);
    TSUNIT_EQUAL(htons(80), saip->sin_port);

    ::sockaddr_in sai;
    a1.get4(sai);
    TSUNIT_EQUAL(AF_INET, sai.sin_family);
    TSUNIT_EQUAL(htonl(0x01020304), sai.sin_addr.s_addr);
    TSUNIT_EQUAL(htons(80), sai.sin_port);

    a1.setAddress4(2, 3, 4, 5);
    a1.setPort(80);
    const ts::UString s1(a1.toString());
    TSUNIT_EQUAL(u"2.3.4.5:80", s1);

    a1.clearPort();
    const ts::UString s2(a1.toString());
    TSUNIT_EQUAL(u"2.3.4.5", s2);

    TSUNIT_ASSERT(a1.resolve(u"192.168.233.2:51823", CERR));
    TSUNIT_ASSERT(a1.hasAddress());
    TSUNIT_ASSERT(a1.hasPort());
    TSUNIT_EQUAL((uint32_t(192) << 24) | (168 << 16) | (233 << 8) | 2, a1.address4());
    TSUNIT_EQUAL(51823, a1.port());

    TSUNIT_ASSERT(a2.resolve(u"192.168.233.2:51824", CERR));
    TSUNIT_ASSERT(a2.hasAddress());
    TSUNIT_ASSERT(a2.hasPort());
    TSUNIT_EQUAL((uint32_t(192) << 24) | (168 << 16) | (233 << 8) | 2, a2.address4());
    TSUNIT_EQUAL(51824, a2.port());

    TSUNIT_ASSERT(a1 != a2);
    TSUNIT_ASSERT(!(a1 == a2));
    TSUNIT_ASSERT(a1 < a2);
}

TSUNIT_DEFINE_TEST(IPv6SocketAddress)
{
    TSUNIT_ASSERT(ts::IPInitialize());

    ts::IPSocketAddress sa1(ts::IPSocketAddress::AnySocketAddress6);
    TSUNIT_ASSERT(!sa1.hasAddress());
    TSUNIT_ASSERT(!sa1.hasPort());
    TSUNIT_EQUAL(ts::IP::v6, sa1.generation());

    sa1.setAddress6(0, 1, 2, 3, 4, 5, 6, 7);
    sa1.setPort(1234);
    TSUNIT_ASSERT(sa1.hasAddress());
    TSUNIT_ASSERT(sa1.hasPort());
    TSUNIT_EQUAL(0x0000000100020003, sa1.networkPrefix6());
    TSUNIT_EQUAL(0x0004000500060007, sa1.interfaceIdentifier6());
    TSUNIT_EQUAL(u"[0:1:2:3:4:5:6:7]:1234", sa1.toString());
    TSUNIT_EQUAL(u"[0000:0001:0002:0003:0004:0005:0006:0007]:1234", sa1.toFullString());
    TSUNIT_EQUAL(1234, sa1.port());

    ts::IPSocketAddress sa2(0, 1, 2, 3, 4, 5, 6, 7, 1235);
    TSUNIT_ASSERT(sa2.hasAddress());
    TSUNIT_ASSERT(sa2.hasPort());
    TSUNIT_EQUAL(ts::IP::v6, sa2.generation());
    TSUNIT_EQUAL(1235, sa2.port());
    TSUNIT_ASSERT(sa1 != sa2);
    TSUNIT_ASSERT(!(sa1 == sa2));
    TSUNIT_ASSERT(sa1 < sa2);

    TSUNIT_ASSERT(sa1.resolve(u"[fe80::93a3:dea0:2108:b81e]", CERR));
    TSUNIT_ASSERT(sa1.hasAddress());
    TSUNIT_ASSERT(!sa1.hasPort());
    TSUNIT_EQUAL(0xFE80000000000000, sa1.networkPrefix6());
    TSUNIT_EQUAL(0x93A3DEA02108B81E, sa1.interfaceIdentifier6());
    TSUNIT_EQUAL(u"fe80::93a3:dea0:2108:b81e", sa1.toString());
    TSUNIT_EQUAL(u"fe80:0000:0000:0000:93a3:dea0:2108:b81e", sa1.toFullString());

    TSUNIT_ASSERT(sa1.resolve(u"[fe80::93a3:dea0:2108:b82f]", CERR));
    TSUNIT_ASSERT(sa1.hasAddress());
    TSUNIT_ASSERT(!sa1.hasPort());
    TSUNIT_EQUAL(0xFE80000000000000, sa1.networkPrefix6());
    TSUNIT_EQUAL(0x93A3DEA02108B82F, sa1.interfaceIdentifier6());
    TSUNIT_EQUAL(u"fe80::93a3:dea0:2108:b82f", sa1.toString());
    TSUNIT_EQUAL(u"fe80:0000:0000:0000:93a3:dea0:2108:b82f", sa1.toFullString());

    TSUNIT_ASSERT(sa2.resolve(u"[FE80::93A3:DEA0:2108:B81E]:1234", CERR));
    TSUNIT_ASSERT(sa2.hasAddress());
    TSUNIT_ASSERT(sa2.hasPort());
    TSUNIT_EQUAL(0xFE80000000000000, sa2.networkPrefix6());
    TSUNIT_EQUAL(0x93A3DEA02108B81E, sa2.interfaceIdentifier6());
    TSUNIT_EQUAL(u"[fe80::93a3:dea0:2108:b81e]:1234", sa2.toString());
    TSUNIT_EQUAL(u"[fe80:0000:0000:0000:93a3:dea0:2108:b81e]:1234", sa2.toFullString());
}

// A thread class which implements a TCP/IP client.
// It sends one message and wait from the same message to be replied.
namespace {
    class TCPClient: public utest::TSUnitThread
    {
        TS_NOBUILD_NOCOPY(TCPClient);
    private:
        uint16_t _portNumber;
    public:
        // Constructor
        explicit TCPClient(uint16_t portNumber) :
            utest::TSUnitThread(),
            _portNumber(portNumber)
        {
        }

        // Destructor
        virtual ~TCPClient() override
        {
            waitForTermination();
            CERR.debug(u"TCPSocketTest: client thread: destroyed");
        }

        // Thread execution
        virtual void test() override
        {
            CERR.debug(u"TCPSocketTest: client thread: started");

            // Connect to the server.
            const ts::IPSocketAddress serverAddress(ts::IPAddress::LocalHost4, _portNumber);
            const ts::IPSocketAddress clientAddress(ts::IPAddress::LocalHost4, ts::IPSocketAddress::AnyPort);
            ts::TCPConnection session;
            TSUNIT_ASSERT(!session.isOpen());
            TSUNIT_ASSERT(!session.isConnected());
            TSUNIT_ASSERT(session.open(ts::IP::v4, CERR));
            TSUNIT_ASSERT(session.setSendBufferSize(1024, CERR));
            TSUNIT_ASSERT(session.setReceiveBufferSize(1024, CERR));
            TSUNIT_ASSERT(session.bind(clientAddress, CERR));
            TSUNIT_ASSERT(session.connect(serverAddress, CERR));
            TSUNIT_ASSERT(session.isOpen());
            TSUNIT_ASSERT(session.isConnected());

            ts::IPSocketAddress peer;
            TSUNIT_ASSERT(session.getPeer(peer, CERR));
            TSUNIT_ASSERT(peer == serverAddress);
            TSUNIT_ASSERT(ts::IPAddress(peer) == ts::IPAddress::LocalHost4);
            TSUNIT_ASSERT(peer.port() == _portNumber);

            // Send a message
            const char message[] = "Hello";
            CERR.debug(u"TCPSocketTest: client thread: sending \"%s\", %d bytes", message, sizeof(message));
            TSUNIT_ASSERT(session.send(message, sizeof(message), CERR));
            CERR.debug(u"TCPSocketTest: client thread: data sent");

            // Say we won't send no more
            TSUNIT_ASSERT(session.closeWriter(CERR));

            // Loop until on server response.
            size_t totalSize = 0;
            char buffer [1024];
            size_t size = 0;
            while (totalSize < sizeof(buffer) && session.receive(buffer + totalSize, sizeof(buffer) - totalSize, size, nullptr, CERR)) {
                CERR.debug(u"TCPSocketTest: client thread: data received, %d bytes", size);
                totalSize += size;
            }
            CERR.debug(u"TCPSocketTest: client thread: end of data stream");
            TSUNIT_EQUAL(sizeof(message), totalSize);
            TSUNIT_EQUAL(0, ts::MemCompare(message, buffer, totalSize));

            // Fully disconnect the session
            session.disconnect(CERR);
            session.close(CERR);
            CERR.debug(u"TCPSocketTest: client thread: terminated");
        }
    };
}

// Test cases
TSUNIT_DEFINE_TEST(TCPSocket)
{
    TSUNIT_ASSERT(ts::IPInitialize());

    const uint16_t portNumber = 12345;

    // Create server socket
    CERR.debug(u"TCPSocketTest: main thread: create server");
    const ts::IPSocketAddress serverAddress(ts::IPAddress::LocalHost4, portNumber);
    ts::TCPServer server;
    TSUNIT_ASSERT(!server.isOpen());
    TSUNIT_ASSERT(server.open(ts::IP::v4, CERR));
    TSUNIT_ASSERT(server.isOpen());
    TSUNIT_ASSERT(server.reusePort(true, CERR));
    TSUNIT_ASSERT(server.setSendBufferSize(1024, CERR));
    TSUNIT_ASSERT(server.setReceiveBufferSize(1024, CERR));
    TSUNIT_ASSERT(server.setTTL(1, CERR));
    TSUNIT_ASSERT(server.bind(serverAddress, CERR));
    TSUNIT_ASSERT(server.listen(5, CERR));

    CERR.debug(u"TCPSocketTest: main thread: starting client thread");
    TCPClient client(portNumber);
    client.start();

    CERR.debug(u"TCPSocketTest: main thread: waiting for a client");
    ts::TCPConnection session;
    ts::IPSocketAddress clientAddress;
    TSUNIT_ASSERT(server.accept(session, clientAddress, CERR));
    CERR.debug(u"TCPSocketTest: main thread: got a client");
    TSUNIT_ASSERT(ts::IPAddress(clientAddress) == ts::IPAddress::LocalHost4);

    CERR.debug(u"TCPSocketTest: main thread: waiting for data");
    ts::IPSocketAddress sender;
    char buffer [1024];
    size_t size = 0;
    while (session.receive(buffer, sizeof(buffer), size, nullptr, CERR)) {
        CERR.debug(u"TCPSocketTest: main thread: data received, %d bytes", size);
        TSUNIT_ASSERT(session.send(buffer, size, CERR));
        CERR.debug(u"TCPSocketTest: main thread: data sent back");
    }

    CERR.debug(u"TCPSocketTest: main thread: end of client session");
    session.disconnect(CERR);
    session.close(CERR);
    TSUNIT_ASSERT(server.close(CERR));

    CERR.debug(u"TCPSocketTest: main thread: terminated");
}

// A thread class which sends one UDP message and wait from the same message to be replied.
namespace {
    class UDPClient: public utest::TSUnitThread
    {
        TS_NOBUILD_NOCOPY(UDPClient);
    private:
        uint16_t _portNumber;
    public:
        // Constructor
        explicit UDPClient(uint16_t portNumber) :
            utest::TSUnitThread(),
            _portNumber(portNumber)
        {
        }

        // Destructor
        virtual ~UDPClient() override
        {
            waitForTermination();
            CERR.debug(u"UDPSocketTest: client thread destroyed");
        }

        // Thread execution
        virtual void test() override
        {
            CERR.debug(u"UDPSocketTest: client thread started");
            // Create the client socket
            ts::UDPSocket sock(true, ts::IP::v4);
            TSUNIT_ASSERT(sock.isOpen());
            TSUNIT_ASSERT(sock.setSendBufferSize(1024, CERR));
            TSUNIT_ASSERT(sock.setReceiveBufferSize(1024, CERR));
            TSUNIT_ASSERT(sock.bind(ts::IPSocketAddress(ts::IPAddress::LocalHost4, ts::IPSocketAddress::AnyPort), CERR));
            TSUNIT_ASSERT(sock.setDefaultDestination(ts::IPSocketAddress(ts::IPAddress::LocalHost4, _portNumber), CERR));
            TSUNIT_ASSERT(ts::IPAddress(sock.getDefaultDestination()) == ts::IPAddress::LocalHost4);
            TSUNIT_ASSERT(sock.getDefaultDestination().port() == _portNumber);

            // Send a message
            const char message[] = "Hello";
            CERR.debug(u"UDPSocketTest: client thread: sending \"%s\", %d bytes", message, sizeof(message));
            TSUNIT_ASSERT(sock.send(message, sizeof(message), CERR));
            CERR.debug(u"UDPSocketTest: client thread: request sent");

            // Wait for a reply
            ts::IPSocketAddress sender;
            ts::IPSocketAddress destination;
            char buffer [1024];
            size_t size;
            TSUNIT_ASSERT(sock.receive(buffer, sizeof(buffer), size, sender, destination, nullptr, CERR));
            CERR.debug(u"UDPSocketTest: client thread: reply received, %d bytes, sender: %s, destination: %s", size, sender, destination);
            TSUNIT_EQUAL(sizeof(message), size);
            TSUNIT_EQUAL(0, ts::MemCompare(message, buffer, size));
            TSUNIT_ASSERT(ts::IPAddress(sender) == ts::IPAddress::LocalHost4);
            TSUNIT_ASSERT(sender.port() == _portNumber);

            CERR.debug(u"UDPSocketTest: client thread terminated");
        }
    };
}

// Test cases
TSUNIT_DEFINE_TEST(UDPSocket)
{
    TSUNIT_ASSERT(ts::IPInitialize());

    const uint16_t portNumber = 12345;

    // Create server socket
    ts::UDPSocket sock;
    TSUNIT_ASSERT(!sock.isOpen());
    TSUNIT_ASSERT(sock.open(ts::IP::v4, CERR));
    TSUNIT_ASSERT(sock.isOpen());
    TSUNIT_ASSERT(sock.setSendBufferSize(1024, CERR));
    TSUNIT_ASSERT(sock.setReceiveBufferSize(1024, CERR));
    TSUNIT_ASSERT(sock.reusePort(true, CERR));
    TSUNIT_ASSERT(sock.setTTL(1, false, CERR));
    TSUNIT_ASSERT(sock.bind(ts::IPSocketAddress(ts::IPAddress::LocalHost4, portNumber), CERR));

    CERR.debug(u"UDPSocketTest: main thread: starting client thread");
    UDPClient client(portNumber);
    client.start();

    CERR.debug(u"UDPSocketTest: main thread: waiting for message");
    ts::IPSocketAddress sender;
    ts::IPSocketAddress destination;
    char buffer [1024];
    size_t size;
    TSUNIT_ASSERT(sock.receive(buffer, sizeof(buffer), size, sender, destination, nullptr, CERR));
    CERR.debug(u"UDPSocketTest: main thread: request received, %d bytes, sender: %s, destination: %s", size, sender, destination);
    TSUNIT_ASSERT(ts::IPAddress(sender) == ts::IPAddress::LocalHost4);

    TSUNIT_ASSERT(sock.send(buffer, size, sender, CERR));
    CERR.debug(u"UDPSocketTest: main thread: reply sent");
}

TSUNIT_DEFINE_TEST(IPHeader)
{
    static const uint8_t reference_header[] = {
        0x45, 0x00, 0x05, 0xBE, 0xFB, 0x6E, 0x00, 0x00, 0x32, 0x06,
        0x32, 0x8B, 0xD8, 0x3A, 0xCC, 0x8E, 0xAC, 0x14, 0x04, 0x63,
    };

    TSUNIT_EQUAL(sizeof(reference_header), ts::IPPacket::IPHeaderSize(reference_header, sizeof(reference_header)));
    TSUNIT_EQUAL(0x328B, ts::IPPacket::IPHeaderChecksum(reference_header, sizeof(reference_header)));
    TSUNIT_ASSERT(ts::IPPacket::VerifyIPHeaderChecksum(reference_header, sizeof(reference_header)));

    uint8_t header[sizeof(reference_header)];
    ts::MemCopy(header, reference_header, sizeof(header));

    TSUNIT_ASSERT(ts::IPPacket::VerifyIPHeaderChecksum(header, sizeof(header)));
    header[ts::IPv4_CHECKSUM_OFFSET] = 0xFF;
    header[ts::IPv4_CHECKSUM_OFFSET + 1] = 0xFF;
    TSUNIT_ASSERT(!ts::IPPacket::VerifyIPHeaderChecksum(header, sizeof(header)));
    TSUNIT_EQUAL(0x328B, ts::IPPacket::IPHeaderChecksum(header, sizeof(header)));

    TSUNIT_ASSERT(ts::IPPacket::UpdateIPHeaderChecksum(header, sizeof(header)));
    TSUNIT_ASSERT(ts::IPPacket::VerifyIPHeaderChecksum(header, sizeof(header)));
    TSUNIT_EQUAL(0x328B, ts::IPPacket::IPHeaderChecksum(header, sizeof(header)));
}

TSUNIT_DEFINE_TEST(IPProtocol)
{
    TSUNIT_EQUAL(u"TCP", ts::IPProtocolName(ts::IP_SUBPROTO_TCP));
    TSUNIT_EQUAL(u"UDP", ts::IPProtocolName(ts::IP_SUBPROTO_UDP));
}

TSUNIT_DEFINE_TEST(TCPPacket)
{
    static const uint8_t data[] = {
        0x45, 0x00, 0x04, 0x76, 0x26, 0xC7, 0x40, 0x00, 0x40, 0x06, 0x1E, 0x5F, 0xC0, 0xA8, 0x38, 0x0A,
        0xC0, 0xA8, 0x38, 0x01, 0xCF, 0xEA, 0x13, 0x88, 0x2B, 0x68, 0x2B, 0x8D, 0x42, 0x84, 0xE7, 0xB9,
        0x50, 0x18, 0x01, 0xF6, 0x03, 0x28, 0x00, 0x00, 0x02, 0x02, 0x11, 0x02, 0x22, 0x00, 0x03, 0x00,
        0x02, 0x00, 0x01, 0x00, 0x04, 0x00, 0x02, 0x00, 0x01, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x08, 0x00, 0x02, 0x00, 0x00, 0x00, 0x05, 0x00, 0x64, 0x8F, 0x70, 0x61, 0x4B, 0x4B,
        0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B,
        0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B,
        0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B,
        0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B,
        0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B,
        0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x00,
        0x05, 0x00, 0x64, 0x82, 0x70, 0x61, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C,
        0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C,
        0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C,
        0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C,
        0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C,
        0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C,
        0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x00, 0x05, 0x00, 0x64, 0x83, 0x70, 0x61, 0x4D, 0x4D,
        0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D,
        0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D,
        0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D,
        0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D,
        0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D,
        0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x00,
        0x05, 0x00, 0x64, 0x84, 0x70, 0x61, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E,
        0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E,
        0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E,
        0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E,
        0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E,
        0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E,
        0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x00, 0x05, 0x00, 0x64, 0x85, 0x70, 0x61, 0x4F, 0x4F,
        0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F,
        0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F,
        0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F,
        0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F,
        0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F,
        0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x02,
        0x02, 0x11, 0x02, 0x22, 0x00, 0x03, 0x00, 0x02, 0x00, 0x01, 0x00, 0x04, 0x00, 0x02, 0x00, 0x01,
        0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x02, 0x00, 0x00, 0x00, 0x05,
        0x00, 0x64, 0x86, 0x70, 0x61, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50,
        0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50,
        0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50,
        0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50,
        0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50,
        0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50,
        0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x00, 0x05, 0x00, 0x64, 0x87, 0x70, 0x61, 0x51, 0x51, 0x51,
        0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51,
        0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51,
        0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51,
        0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51,
        0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51,
        0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x00, 0x05,
        0x00, 0x64, 0x88, 0x70, 0x61, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52,
        0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52,
        0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52,
        0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52,
        0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52,
        0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52,
        0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x00, 0x05, 0x00, 0x64, 0x89, 0x70, 0x61, 0x53, 0x53, 0x53,
        0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53,
        0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53,
        0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53,
        0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53,
        0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53,
        0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x00, 0x05,
        0x00, 0x64, 0x8A, 0x70, 0x61, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54,
        0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54,
        0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54,
        0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54,
        0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54,
        0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54,
        0x54, 0x54, 0x54, 0x54, 0x54, 0x54,
    };

    ts::IPPacket ip(data, sizeof(data));

    TSUNIT_ASSERT(ip.isValid());
    TSUNIT_EQUAL(ts::IP_SUBPROTO_TCP, ip.protocol());
    TSUNIT_ASSERT(ip.isTCP());
    TSUNIT_ASSERT(!ip.isUDP());
    TSUNIT_ASSERT(ip.data() != nullptr);
    TSUNIT_EQUAL(0x45, *ip.data());
    TSUNIT_EQUAL(1142, ip.size());
    TSUNIT_EQUAL(ip.data(), ip.ipHeader());
    TSUNIT_EQUAL(20, ip.ipHeaderSize());
    TSUNIT_EQUAL(ip.data() + 20, ip.protocolHeader());
    TSUNIT_EQUAL(20, ip.protocolHeaderSize());
    TSUNIT_EQUAL(ip.data() + 40, ip.protocolData());
    TSUNIT_EQUAL(1102, ip.protocolDataSize());
    TSUNIT_EQUAL(u"192.168.56.10:53226", ip.source().toString());
    TSUNIT_EQUAL(u"192.168.56.1:5000", ip.destination().toString());
}

TSUNIT_DEFINE_TEST(UDPPacket)
{
    static const uint8_t data[] = {
        0x45, 0x00, 0x01, 0x94, 0xB4, 0xF8, 0x40, 0x00, 0x40, 0x11, 0x93, 0x04, 0xC0, 0xA8, 0x38, 0x0A,
        0xC0, 0xA8, 0x38, 0x01, 0xA3, 0x94, 0x13, 0x88, 0x01, 0x80, 0xC8, 0x14, 0x47, 0x1F, 0xFF, 0x10,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x47, 0x1F, 0xFF, 0x10, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
    };

    ts::IPPacket ip(data, sizeof(data));

    TSUNIT_ASSERT(ip.isValid());
    TSUNIT_EQUAL(ts::IP_SUBPROTO_UDP, ip.protocol());
    TSUNIT_ASSERT(!ip.isTCP());
    TSUNIT_ASSERT(ip.isUDP());
    TSUNIT_ASSERT(ip.data() != nullptr);
    TSUNIT_EQUAL(0x45, *ip.data());
    TSUNIT_EQUAL(404, ip.size());
    TSUNIT_EQUAL(ip.data(), ip.ipHeader());
    TSUNIT_EQUAL(20, ip.ipHeaderSize());
    TSUNIT_EQUAL(ip.data() + 20, ip.protocolHeader());
    TSUNIT_EQUAL(8, ip.protocolHeaderSize());
    TSUNIT_EQUAL(ip.data() + 28, ip.protocolData());
    TSUNIT_EQUAL(376, ip.protocolDataSize());
    TSUNIT_EQUAL(u"192.168.56.10:41876", ip.source().toString());
    TSUNIT_EQUAL(u"192.168.56.1:5000", ip.destination().toString());

    ip.reset(data, sizeof(data) - 1);
    TSUNIT_ASSERT(!ip.isValid());
}
