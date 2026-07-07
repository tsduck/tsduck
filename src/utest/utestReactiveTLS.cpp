//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for reactive TLS classes.
//
//----------------------------------------------------------------------------

#include "tsReactiveTLSConnection.h"
#include "tsReactiveTLSServer.h"
#include "tsReactiveServer.h"
#include "tsReactiveTelnetConnection.h"
#include "tsEnvironment.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ReactiveTLSTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(Client);
    TSUNIT_DECLARE_TEST(Server);
};

TSUNIT_REGISTER(ReactiveTLSTest);


//----------------------------------------------------------------------------
// Unitary test : TCP socket in telnet mode over SSL/TLS.
//----------------------------------------------------------------------------

namespace {
    class TelnetClient: private ts::ReactiveTCPConnectionHandlerInterface, private ts::ReactiveTelnetConnectionHandlerInterface
    {
    public:
        TelnetClient() = delete;
        [[maybe_unused]] TelnetClient(ts::Reactor& reactor, std::ostream& debug);

    private:
        ts::Reactor&                 _reactor;
        std::ostream&                _debug;
        ts::TCPConnection            _client {&_reactor.report()};
        ts::ReactiveTLSConnection    _rclient {_reactor, _client};
        ts::ReactiveTelnetConnection _rtclient {_rclient};
        const ts::UString            _server_name {ts::GetEnvironment(u"TS_UTEST_TLS_TELNET_HOST", u"tsduck.io")};
        const ts::IPAddress          _server_address {_server_name, _reactor.report(), ts::IP::v4};
        const ts::IPAddress::Port    _server_port = ts::GetIntEnvironment<ts::IPAddress::Port>(u"TS_UTEST_TLS_TELNET_PORT", 443);
        const ts::IPSocketAddress    _server_socket {_server_address, _server_port};

        virtual void handleTCPConnected(ts::ReactiveTCPConnection& sock, int error_code, const ts::ObjectPtr& user_data) override;
        virtual void handleTelnetLine(ts::ReactiveTelnetConnection& sock, const ts::UString& line, int error_code) override;
        virtual void handleTCPClosed(ts::ReactiveTCPConnection& sock, const ts::ObjectPtr& user_data) override;
    };

    TelnetClient::TelnetClient(ts::Reactor& reactor, std::ostream& debug) :
        _reactor(reactor),
        _debug(debug)
    {
        _debug << "TelnetClient: connecting to " << _server_name << " (" << _server_socket << ")" << std::endl;
        _rclient.setServerName(_server_name);
        TSUNIT_ASSERT(_client.open(ts::IP::v4));
        TSUNIT_ASSERT(_client.bind(ts::IPSocketAddress::AnySocketAddress4));
        TSUNIT_ASSERT(_rclient.startConnect(this, _server_socket));
    }

    void TelnetClient::handleTCPConnected(ts::ReactiveTCPConnection& sock, int error_code, const ts::ObjectPtr& user_data)
    {
        _debug << "TelnetClient::handleTCPConnected: error code: " << error_code << std::endl;
        TSUNIT_ASSERT(_rtclient.startReceive(this));
        TSUNIT_ASSERT(_rtclient.startSendLine(u"GET / HTTP/1.0", false));
        TSUNIT_ASSERT(_rtclient.startSendLine(u"Host: " + _server_name, false));
        TSUNIT_ASSERT(_rtclient.startSendLine(u"User-Agent: tsduck", false));
        TSUNIT_ASSERT(_rtclient.startSendLine(u"Accept: text/html", false));
        TSUNIT_ASSERT(_rtclient.startSendLine(u"Connection: close", false));
        TSUNIT_ASSERT(_rtclient.startSendLine(u""));
    }

    void TelnetClient::handleTelnetLine(ts::ReactiveTelnetConnection& sock, const ts::UString& line, int error_code)
    {
        if (error_code == ts::SYS_EOF) {
            _debug << "TelnetClient::handleTelnetLine: end of response" << std::endl;
            TSUNIT_ASSERT(_rclient.startClose(this));
        }
        else {
            _debug << "TelnetClient::handleTelnetLine: \"" << line << "\", error code: " << error_code << std::endl;
        }
    }

    void TelnetClient::handleTCPClosed(ts::ReactiveTCPConnection& sock, const ts::ObjectPtr& user_data)
    {
        _debug << "TelnetClient::handleTCPClosed" << std::endl;
        TSUNIT_ASSERT(!_client.isOpen());
        sock.reactor().exitEventLoop();
    }
}

TSUNIT_DEFINE_TEST(Client)
{
    TSUNIT_ASSERT(ts::IPInitialize());
    ts::Reactor reactor(&CERR);
    TSUNIT_ASSERT(reactor.open());
    TelnetClient test(reactor, debug());
    TSUNIT_ASSERT(reactor.processEventLoop());
    TSUNIT_ASSERT(reactor.close());
}


//----------------------------------------------------------------------------
// Unitary test : TCP client and server.
//----------------------------------------------------------------------------

namespace {

    //------------------------------------------------------------------------
    // A client.
    //------------------------------------------------------------------------

    class TestClient: private ts::ReactorHandlerInterface, private ts::ReactiveTCPConnectionHandlerInterface
    {
        TS_NOBUILD_NOCOPY(TestClient);
    public:
        TestClient(ts::Reactor& reactor, std::ostream& debug, cn::milliseconds delay, size_t request_count, uint16_t server_port);

        size_t handle_connected_count = 0;
        size_t handle_closed_count = 0;
        size_t send_count = 0;
        size_t receive_count = 0;

    private:
        ts::Reactor&              _reactor;
        std::ostream&             _debug;
        cn::milliseconds          _delay;
        size_t                    _request_count;
        const ts::IPSocketAddress _server_address;
        size_t                    _client_id = 0;
        ts::TCPConnection         _client {&_reactor.report()};
        ts::ReactiveTLSConnection _rclient {_reactor, _client};
        ts::EventId               _timer_id {};
        uint32_t                  _request = 0;
        size_t                    _expected_send_position = 0;

        static size_t _id_counter;

        virtual void handleTimer(ts::Reactor& reactor, ts::EventId id) override;
        virtual void handleTCPConnected(ts::ReactiveTCPConnection& sock, int error_code, const ts::ObjectPtr& user_data) override;
        virtual void handleTCPSend(ts::ReactiveTCPConnection& sock, size_t position, int error_code, const ts::ObjectPtr& user_data) override;
        virtual void handleTCPReceive(ts::ReactiveTCPConnection& sock, const ts::ByteBlock& data, ts::ReactiveTCPInputControl& control, int error_code, const ts::ObjectPtr& user_data) override;
        virtual void handleTCPClosed(ts::ReactiveTCPConnection& sock, const ts::ObjectPtr& user_data) override;
    };

    size_t TestClient::_id_counter = 0;

    TestClient::TestClient(ts::Reactor& reactor, std::ostream& debug, cn::milliseconds delay, size_t request_count, uint16_t server_port) :
        _reactor(reactor),
        _debug(debug),
        _delay(delay),
        _request_count(request_count),
        _server_address(ts::IPAddress::LocalHost4, server_port),
        _client_id(++_id_counter)
    {
        _debug << "TestClient: initialize client id " << _client_id << std::endl;
        _reactor.addExitReference();
        TSUNIT_ASSERT(_delay > cn::milliseconds::zero());
        TSUNIT_ASSERT(_request_count > 0);
        _timer_id = _reactor.newTimer(this, _delay, false);
        TSUNIT_ASSERT(_timer_id.isValid());
    }

    void TestClient::handleTimer(ts::Reactor& reactor, ts::EventId id)
    {
        _debug << "TestClient::handleTimer, client id: " << _client_id << std::endl;
        TSUNIT_ASSERT(id == _timer_id);

        if (!_client.isConnected()) {
            // First time: start connecting.
            _debug << "TestClient::handleTimer, client id: " << _client_id << ", start connection" << std::endl;
            TSUNIT_ASSERT(_client.open(ts::IP::v4));
            TSUNIT_ASSERT(_client.bind(ts::IPSocketAddress::AnySocketAddress4));
            _rclient.setVerifyPeer(false); // the server certificate is self-signed
            TSUNIT_ASSERT(_rclient.startConnect(this, _server_address));
        }
        else {
            // Next times: send a request.
            _request++;
            _debug << "TestClient::handleTimer, client id: " << _client_id << ", start send request " << _request << std::endl;
            TSUNIT_ASSERT(_rclient.startSend(this, &_request, sizeof(_request)));
            send_count++;
            _expected_send_position += sizeof(_request);
        }
    }

    void TestClient::handleTCPConnected(ts::ReactiveTCPConnection& sock, int error_code, const ts::ObjectPtr& user_data)
    {
        _debug << "TestClient::handleTCPConnected, client id: " << _client_id << ", error code: " << error_code << std::endl;
        TSUNIT_ASSERT(&sock == &_rclient);
        TSUNIT_EQUAL(ts::SYS_SUCCESS, error_code);
        TSUNIT_ASSERT(_rclient.startReceive(this));
        handle_connected_count++;

        // Send request after next timer.
        _timer_id = _reactor.newTimer(this, _delay, false);
        TSUNIT_ASSERT(_timer_id.isValid());
    }

    void TestClient::handleTCPSend(ts::ReactiveTCPConnection& sock, size_t position, int error_code, const ts::ObjectPtr& user_data)
    {
        _debug << "TestClient::handleTCPSend, client id: " << _client_id << ", position: " << position << ", error code: " << error_code << std::endl;
        TSUNIT_ASSERT(&sock == &_rclient);
        TSUNIT_EQUAL(_expected_send_position, position);
    }

    void TestClient::handleTCPReceive(ts::ReactiveTCPConnection& sock, const ts::ByteBlock& data, ts::ReactiveTCPInputControl& control, int error_code, const ts::ObjectPtr& user_data)
    {
        _debug << "TestClient::handleTCPReceive, client id: " << _client_id << ", error code: " << error_code << ", size: " << data.size() << std::endl;
        TSUNIT_ASSERT(&sock == &_rclient);
        TSUNIT_EQUAL(ts::SYS_SUCCESS, error_code);

        // Need at least 4 bytes.
        if (data.size() < sizeof(uint32_t)) {
            control.used_size = 0;
            control.min_next_size = sizeof(uint32_t);
        }
        else {
            // Verify response.
            const uint32_t response = *reinterpret_cast<const uint32_t*>(data.data());
            control.used_size = sizeof(uint32_t);
            _debug << "TestClient::handleTCPReceive, client id: " << _client_id << ", request: " << _request << ", response: " << response << std::endl;
            TSUNIT_EQUAL(_request + 1, response);
            receive_count++;

            if (_request < _request_count) {
                // Wait before sending the next request.
                _timer_id = _reactor.newTimer(this, _delay, false);
                TSUNIT_ASSERT(_timer_id.isValid());
            }
            else {
                // Session completed.
                _debug << "TestClient::handleTCPReceive, start close, client id: " << _client_id << std::endl;
                TSUNIT_ASSERT(_rclient.startClose(this));
            }
        }
    }

    void TestClient::handleTCPClosed(ts::ReactiveTCPConnection& sock, const ts::ObjectPtr& user_data)
    {
        _debug << "TestClient::handleTCPClosed, client id: " << _client_id << std::endl;
        TSUNIT_ASSERT(&sock == &_rclient);
        _reactor.freeExitReference();
        handle_closed_count++;
    }

    //------------------------------------------------------------------------
    // A client connection on the server side.
    // Each request is a 4-byte integer. Increment it and return it.
    //------------------------------------------------------------------------

    class TestServerConnection: public ts::ReactiveServerSessionInterface, private ts::ReactiveTCPConnectionHandlerInterface
    {
        TS_NOBUILD_NOCOPY(TestServerConnection);
    public:
        TestServerConnection(ts::Reactor& reactor, std::ostream& debug);
        virtual ~TestServerConnection() override;
        size_t clientId() const { return _client_id; }

        static size_t handle_accepted_count;
        static size_t handle_closed_count;

    private:
        ts::Reactor&              _reactor;
        std::ostream&             _debug;
        size_t                    _client_id = 0;
        ts::TCPConnection         _client {};
        ts::ReactiveTLSConnection _rclient {_reactor, _client};
        uint32_t                  _response = 0;
        size_t                    _expected_send_position = 0;

        static size_t _id_counter;

        virtual ts::ReactiveTCPConnection& getConnection() override;
        virtual void handleTCPAccepted(ts::ReactiveTCPServer& server, ts::ReactiveTCPConnection& sock, int error_code, const ts::ObjectPtr& user_data) override;
        virtual void handleTCPSend(ts::ReactiveTCPConnection& sock, size_t position, int error_code, const ts::ObjectPtr& user_data) override;
        virtual void handleTCPReceive(ts::ReactiveTCPConnection& sock, const ts::ByteBlock& data, ts::ReactiveTCPInputControl& control, int error_code, const ts::ObjectPtr& user_data) override;
        virtual void handleTCPClosed(ts::ReactiveTCPConnection& sock, const ts::ObjectPtr& user_data) override;
    };

    size_t TestServerConnection::handle_accepted_count = 0;
    size_t TestServerConnection::handle_closed_count = 0;
    size_t TestServerConnection::_id_counter = 0;

    TestServerConnection::TestServerConnection(ts::Reactor& reactor, std::ostream& debug) :
        _reactor(reactor),
        _debug(debug),
        _client_id(++_id_counter)
    {
        _debug << "TestServerConnection: initialize client session id " << _client_id << std::endl;
        _rclient.whenAccepted(this);
    }

    TestServerConnection::~TestServerConnection()
    {
        _debug << "TestServerConnection: destruction of client session id " << _client_id << " @" << ts::UString::Hexa(uintptr_t(this)) << std::endl;
    }

    ts::ReactiveTCPConnection& TestServerConnection::getConnection()
    {
        return _rclient;
    }

    void TestServerConnection::handleTCPAccepted(ts::ReactiveTCPServer& server, ts::ReactiveTCPConnection& sock, int error_code, const ts::ObjectPtr& user_data)
    {
        _debug << "TestServerConnection::handleTCPAccepted, client id: " << _client_id << ", error code: " << error_code
               << ", client: " << sock.socket().peerName() << ", local: " << sock.socket().localName() << std::endl;
        TSUNIT_ASSERT(&sock == &_rclient);
        handle_accepted_count++;
        if (error_code == ts::SYS_SUCCESS) {
            TSUNIT_ASSERT(_rclient.startReceive(this));
        }
    }

    void TestServerConnection::handleTCPSend(ts::ReactiveTCPConnection& sock, size_t position, int error_code, const ts::ObjectPtr& user_data)
    {
        _debug << "TestServerConnection::handleTCPSend, client id: " << _client_id << ", position: " << position << ", error code: " << error_code << std::endl;
        TSUNIT_ASSERT(&sock == &_rclient);
        TSUNIT_EQUAL(_expected_send_position, position);
    }

    void TestServerConnection::handleTCPReceive(ts::ReactiveTCPConnection& sock, const ts::ByteBlock& data, ts::ReactiveTCPInputControl& control, int error_code, const ts::ObjectPtr& user_data)
    {
        TSUNIT_ASSERT(&sock == &_rclient);
        if (error_code == ts::SYS_EOF) {
            // Client disconnected.
            _debug << "TestServerConnection::handleTCPReceive, client id: " << _client_id << ", end of session" << std::endl;
            TSUNIT_ASSERT(_rclient.startClose(this));
        }
        else {
            _debug << "TestServerConnection::handleTCPReceive, client id: " << _client_id << ", error code: " << error_code << ", size: " << data.size() << std::endl;
            TSUNIT_EQUAL(ts::SYS_SUCCESS, error_code);
            // Need at least 4 bytes.
            if (data.size() < sizeof(uint32_t)) {
                control.used_size = 0;
                control.min_next_size = sizeof(uint32_t);
            }
            else {
                const uint32_t request = *reinterpret_cast<const uint32_t*>(data.data());
                _debug << "TestServerConnection::handleTCPReceive, client id: " << _client_id << ", request: " << request << std::endl;
                control.used_size = sizeof(uint32_t);
                _response = request + 1;
                TSUNIT_ASSERT(_rclient.startSend(this, &_response, sizeof(_response)));
                _expected_send_position += sizeof(_response);
            }
        }
    }

    void TestServerConnection::handleTCPClosed(ts::ReactiveTCPConnection& sock, const ts::ObjectPtr& user_data)
    {
        _debug << "TestServerConnection::handleTCPClosed, client id: " << _client_id << std::endl;
        TSUNIT_ASSERT(&sock == &_rclient);
        handle_closed_count++;
    }

    //------------------------------------------------------------------------
    // Factory for server sessions.
    // Also used as handler to trace server events.
    //------------------------------------------------------------------------

    class TestFactory: public ts::ReactiveServerFactoryInterface, public ts::ReactiveServerHandlerInterface
    {
        TS_NOBUILD_NOCOPY(TestFactory);
    public:
        TestFactory(ts::Reactor& reactor, std::ostream& debug);

        size_t client_count = 0;
        size_t server_exited_count = 0;

    private:
        ts::Reactor&  _reactor;
        std::ostream& _debug;

        virtual ts::ReactiveServerSessionInterface* newClientSession() override;
        virtual void handleServerExited(ts::ReactiveServer& server, const ts::ObjectPtr& user_data) override;
    };

    TestFactory::TestFactory(ts::Reactor& reactor, std::ostream& debug) :
        _reactor(reactor),
        _debug(debug)
    {
        _debug << "TestFactory: initialized" << std::endl;
    }

    ts::ReactiveServerSessionInterface* TestFactory::newClientSession()
    {
        const auto session = new TestServerConnection(_reactor, _debug);
        _debug << "TestFactory: create client session id " << session->clientId() << std::endl;
        client_count++;
        return session;
    }

    void TestFactory::handleServerExited(ts::ReactiveServer& server, const ts::ObjectPtr& user_data)
    {
        _debug << "TestFactory: server exited" << std::endl;
        server_exited_count++;
        _reactor.freeExitReference();
    }
}

TSUNIT_DEFINE_TEST(Server)
{
    static constexpr uint16_t PORT = 12345;

    TSUNIT_ASSERT(ts::IPInitialize());

    ts::Reactor           reactor(&CERR);
    TestFactory           factory(reactor, debug());
    ts::TCPServer         tcp_server(&CERR);
    ts::ReactiveTLSServer rtls_server(reactor, tcp_server);
    ts::ReactiveServer    server(rtls_server);

    TSUNIT_ASSERT(reactor.open());

    TSUNIT_ASSERT(tcp_server.open(ts::IP::v4));
    TSUNIT_ASSERT(tcp_server.reusePort(true));
    TSUNIT_ASSERT(tcp_server.bind(ts::IPSocketAddress(ts::IPAddress::LocalHost4, PORT)));
    TSUNIT_ASSERT(tcp_server.listen(5));
    TSUNIT_EQUAL(1, reactor.addExitReference()); // server

    // Use an ephemeral untrusted server certificate.
    rtls_server.setEphemeralRSABits(2048);

    server.setExitAfterClientCount(2);
    server.start(&factory, &factory);

    TestClient client1(reactor, debug(), cn::milliseconds(50), 3, PORT);
    TestClient client2(reactor, debug(), cn::milliseconds(80), 2, PORT);

    TSUNIT_ASSERT(reactor.processEventLoop());
    TSUNIT_ASSERT(reactor.close());

    TSUNIT_EQUAL(1, client1.handle_connected_count);
    TSUNIT_EQUAL(1, client1.handle_closed_count);
    TSUNIT_EQUAL(3, client1.send_count);
    TSUNIT_EQUAL(3, client1.receive_count);

    TSUNIT_EQUAL(1, client2.handle_connected_count);
    TSUNIT_EQUAL(1, client2.handle_closed_count);
    TSUNIT_EQUAL(2, client2.send_count);
    TSUNIT_EQUAL(2, client2.receive_count);

    TSUNIT_EQUAL(2, factory.client_count);
    TSUNIT_EQUAL(1, factory.server_exited_count);

    TSUNIT_EQUAL(2, TestServerConnection::handle_accepted_count);
    TSUNIT_EQUAL(2, TestServerConnection::handle_closed_count);
}
