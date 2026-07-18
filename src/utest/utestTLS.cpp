//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for TLS classes.
//
//----------------------------------------------------------------------------

#include "tsIPAddress.h"
#include "tsIPSocketAddress.h"
#include "tsTelnetConnection.h"
#include "tsTLSConnection.h"
#include "tsTLSServer.h"
#include "tsEnvironment.h"
#include "tsIPUtils.h"
#include "tsCerrReport.h"
#include "utestTSUnitThread.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class TLSTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(Client);
    TSUNIT_DECLARE_TEST(Server);

public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

private:
    int _previousSeverity = 0;
};

TSUNIT_REGISTER(TLSTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void TLSTest::beforeTest()
{
    _previousSeverity = CERR.maxSeverity();
    if (tsunit::Test::debugMode()) {
        CERR.setMaxSeverity(ts::Severity::Debug);
    }
}

// Test suite cleanup method.
void TLSTest::afterTest()
{
    CERR.setMaxSeverity(_previousSeverity);
}


//----------------------------------------------------------------------------
// Client test.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(Client)
{
    TSUNIT_ASSERT(ts::IPInitialize());

    const ts::UString         server_name(ts::GetEnvironment(u"TS_UTEST_TLS_TELNET_HOST", u"tsduck.io"));
    const ts::IPAddress       server_address(server_name, CERR, ts::IP::v4);
    const ts::IPAddress::Port server_port = ts::GetIntEnvironment<ts::IPAddress::Port>(u"TS_UTEST_TLS_TELNET_PORT", 443);
    const ts::IPSocketAddress server_socket(server_address, server_port);

    ts::TLSConnection    client(&CERR);
    ts::TelnetConnection telnet(client);

    client.setServerName(server_name);
    TSUNIT_ASSERT(!client.isOpen());
    TSUNIT_ASSERT(!client.isConnected());
    TSUNIT_ASSERT(client.open(ts::IP::v4));
    TSUNIT_ASSERT(client.bind(ts::IPSocketAddress::AnySocketAddress4));
    TSUNIT_ASSERT(client.connect(server_socket));
    TSUNIT_ASSERT(client.isOpen());
    TSUNIT_ASSERT(client.isConnected());
    TSUNIT_ASSERT(!client.isServerSide());
    debug() << "TLS::Client: connected" << std::endl;

    TSUNIT_ASSERT(telnet.sendLine(u"GET / HTTP/1.0"));
    TSUNIT_ASSERT(telnet.sendLine(u"Host: " + server_name));
    TSUNIT_ASSERT(telnet.sendLine(u"User-Agent: tsduck"));
    TSUNIT_ASSERT(telnet.sendLine(u"Accept: text/html"));
    TSUNIT_ASSERT(telnet.sendLine(u"Connection: close"));
    TSUNIT_ASSERT(telnet.sendLine(u""));
    debug() << "TLS::Client: request sent" << std::endl;

    size_t line_count = 0;
    size_t empty_count = 0;
    ts::UString line;
    while (telnet.receiveLine(line)) {
        line_count++;
        empty_count += line.empty();
        if (line.size() > 70) {
            line.resize(70);
            line += u" [...]";
        }
        debug() << "TLS::Client: Received \"" << line << "\"" << std::endl;
    }
    debug() << "TLS::Client: Received " << line_count << " lines, " << empty_count << " empty lines" << std::endl;

    TSUNIT_ASSERT(client.disconnect());
    TSUNIT_ASSERT(client.close());
    TSUNIT_ASSERT(line_count > 0);
    TSUNIT_ASSERT(empty_count > 0);
}


//----------------------------------------------------------------------------
// Server + Client test.
//----------------------------------------------------------------------------

// A thread class which implements a TCP/IP client.
// It sends one message and wait from the same message to be replied.
namespace {
    class TCPClient: public utest::TSUnitThread
    {
        TS_NOBUILD_NOCOPY(TCPClient);
    private:
        uint16_t _port_number;
    public:
        // Constructor
        explicit TCPClient(uint16_t port_number) :
            utest::TSUnitThread(),
            _port_number(port_number)
        {
        }

        // Destructor
        virtual ~TCPClient() override
        {
            waitForTermination();
            CERR.debug(u"TLS Server: client thread: destroyed");
        }

        // Thread execution
        virtual void test() override
        {
            CERR.debug(u"TLS Server: client thread: started");

            const ts::IPSocketAddress server_address(ts::IPAddress::LocalHost4, _port_number);
            ts::TLSConnection session(&CERR);

            // Don't verify server's certificate.
            session.setVerifyPeer(false);
            session.setServerName(u"localhost");

            // Connect to the server.
            TSUNIT_ASSERT(!session.isOpen());
            TSUNIT_ASSERT(!session.isConnected());
            TSUNIT_ASSERT(session.open(ts::IP::v4));
            TSUNIT_ASSERT(session.setSendBufferSize(1024));
            TSUNIT_ASSERT(session.setReceiveBufferSize(1024));
            TSUNIT_ASSERT(session.bind(ts::IPSocketAddress::AnySocketAddress4));
            TSUNIT_ASSERT(session.connect(server_address));
            TSUNIT_ASSERT(session.isOpen());
            TSUNIT_ASSERT(session.isConnected());
            TSUNIT_ASSERT(!session.isServerSide());

            ts::IPSocketAddress peer;
            TSUNIT_ASSERT(session.getPeer(peer));
            TSUNIT_ASSERT(peer == server_address);
            TSUNIT_ASSERT(ts::IPAddress(peer) == ts::IPAddress::LocalHost4);
            TSUNIT_ASSERT(peer.port() == _port_number);

            // Send a message
            const char message[] = "Hello";
            CERR.debug(u"TLS Server: client thread: sending \"%s\", %d bytes", message, sizeof(message));
            TSUNIT_ASSERT(session.writeStream(message, sizeof(message)));
            CERR.debug(u"TLS Server: client thread: data sent");

            // Say we won't send no more
            TSUNIT_ASSERT(session.closeWriter());

            // Loop until on server response.
            size_t total_size = 0;
            char buffer[1024];
            size_t size = 0;
            while (total_size < sizeof(buffer) && session.readStream(buffer + total_size, sizeof(buffer) - total_size, size)) {
                CERR.debug(u"TLS Server: client thread: data received, %d bytes", size);
                total_size += size;
            }
            CERR.debug(u"TLS Server: client thread: end of data stream");
            TSUNIT_EQUAL(sizeof(message), total_size);
            TSUNIT_EQUAL(0, ts::MemCompare(message, buffer, total_size));

            // Fully disconnect the session
            TSUNIT_ASSERT(session.disconnect());
            TSUNIT_ASSERT(session.close());
            CERR.debug(u"TLS Server: client thread: terminated");
        }
    };
}

TSUNIT_DEFINE_TEST(Server)
{
    TSUNIT_ASSERT(ts::IPInitialize());

    const uint16_t portNumber = 12345;

    CERR.debug(u"TLS Server: main thread: create server");
    const ts::IPSocketAddress server_address(ts::IPAddress::LocalHost4, portNumber);
    ts::TLSServer server(&CERR);

    // Ephemeral server certificate with a small RSA private key.
    server.setEphemeralRSABits(2048);

    // Create server socket
    TSUNIT_ASSERT(!server.isOpen());
    TSUNIT_ASSERT(server.open(ts::IP::v4));
    TSUNIT_ASSERT(server.isOpen());
    TSUNIT_ASSERT(server.reusePort(true));
    TSUNIT_ASSERT(server.setSendBufferSize(1024));
    TSUNIT_ASSERT(server.setReceiveBufferSize(1024));
    TSUNIT_ASSERT(server.setTTL(1));
    TSUNIT_ASSERT(server.bind(server_address));
    TSUNIT_ASSERT(server.listen(5));

    CERR.debug(u"TLS Server: main thread: starting client thread");
    TCPClient client(portNumber);
    client.start();

    CERR.debug(u"TLS Server: main thread: waiting for a client");
    ts::TLSConnection session(&CERR);
    ts::IPSocketAddress client_address;
    TSUNIT_ASSERT(!session.isOpen());
    TSUNIT_ASSERT(server.accept(session, client_address));
    CERR.debug(u"TLS Server: main thread: got a client");
    TSUNIT_ASSERT(ts::IPAddress(client_address) == ts::IPAddress::LocalHost4);
    TSUNIT_ASSERT(session.isOpen());
    TSUNIT_ASSERT(session.isConnected());
    TSUNIT_ASSERT(session.isServerSide());

    CERR.debug(u"TLS Server: main thread: waiting for data");
    ts::IPSocketAddress sender;
    char buffer[1024];
    size_t size = 0;
    while (session.readStream(buffer, sizeof(buffer), size)) {
        CERR.debug(u"TLS Server: main thread: data received, %d bytes", size);
        TSUNIT_ASSERT(size <= sizeof(buffer));
        TSUNIT_ASSERT(session.writeStream(buffer, size));
        CERR.debug(u"TLS Server: main thread: data sent back");
    }

    CERR.debug(u"TLS Server: main thread: end of client session");
    session.disconnect();
    session.close();
    TSUNIT_ASSERT(server.close());

    CERR.debug(u"TLS Server: main thread: terminated");
}
