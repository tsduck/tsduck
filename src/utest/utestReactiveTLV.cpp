//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::ReactiveTLVConnection
//
//----------------------------------------------------------------------------

#include "tsReactiveTLVConnection.h"
#include "tsReactiveTCPServer.h"
#include "tsReactiveServer.h"
#include "tsECMGSCS.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ReactiveTLVTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(Server);
};

TSUNIT_REGISTER(ReactiveTLVTest);


//----------------------------------------------------------------------------
// Unitary test : TCP client and server.
//----------------------------------------------------------------------------

namespace {

    //------------------------------------------------------------------------
    // A client.
    //------------------------------------------------------------------------

    class TestClient: private ts::ReactiveTLVConnectionHandlerInterface, private ts::ReactiveTCPConnectionHandlerInterface
    {
        TS_NOBUILD_NOCOPY(TestClient);
    public:
        TestClient(ts::Reactor& reactor, ts::tlv::Logger& logger, const ts::tlv::Protocol& protocol, std::ostream& debug, uint16_t server_port);

        size_t handle_connected_count = 0;
        size_t handle_closed_count = 0;
        size_t send_count = 0;
        size_t receive_count = 0;

    private:
        ts::Reactor&              _reactor;
        ts::tlv::Logger&          _logger;
        const ts::tlv::Protocol&  _protocol;
        const ts::IPSocketAddress _server_address;
        std::ostream&             _debug;
        ts::TCPConnection         _tcp_client;
        ts::ReactiveTCPConnection _react_client;
        ts::ReactiveTLVConnection _tlv_client;

        virtual void handleTCPConnected(ts::ReactiveTCPConnection& sock, int error_code, const ts::ObjectPtr& user_data) override;
        virtual void handleReceivedMessage(ts::ReactiveTLVConnection& sock, const ts::tlv::MessagePtr& msg, int error_code) override;
        virtual void handleWriteStream(ts::ReactiveStream& stream, int error_code, const ts::ObjectPtr& user_data) override;
        virtual void handleTCPClosed(ts::ReactiveTCPConnection& sock, const ts::ObjectPtr& user_data) override;
    };

    TestClient::TestClient(ts::Reactor& reactor, ts::tlv::Logger& logger, const ts::tlv::Protocol& protocol, std::ostream& debug, uint16_t server_port) :
        _reactor(reactor),
        _logger(logger),
        _protocol(protocol),
        _server_address(ts::IPAddress::LocalHost4, server_port),
        _debug(debug),
        _tcp_client(&_reactor.report()),
        _react_client(_reactor, _tcp_client),
        _tlv_client(_logger, _protocol, _react_client, true, 1)
    {
        _debug << "TLV client: initialized" << std::endl;
        TSUNIT_ASSERT(_tcp_client.open(ts::IP::v4));
        TSUNIT_ASSERT(_tcp_client.bind(ts::IPSocketAddress::AnySocketAddress4));
        TSUNIT_ASSERT(_react_client.startConnect(this, _server_address));
        _reactor.addExitReference();
    }

    void TestClient::handleTCPConnected(ts::ReactiveTCPConnection& sock, int error_code, const ts::ObjectPtr& user_data)
    {
        _debug << "TLV client: connected, error code: " << error_code << std::endl;
        TSUNIT_ASSERT(&sock == &_react_client);
        TSUNIT_EQUAL(ts::SYS_SUCCESS, error_code);
        TSUNIT_ASSERT(_tlv_client.startReceive(this));
        handle_connected_count++;

        // Send first request.
        ts::ecmgscs::ChannelSetup msg(_protocol);
        msg.channel_id = 100;
        TSUNIT_ASSERT(_tlv_client.startSendMessage(msg));
        send_count++;
    }

    void TestClient::handleReceivedMessage(ts::ReactiveTLVConnection& sock, const ts::tlv::MessagePtr& msg, int error_code)
    {
        _debug << "TLV client: received tag: " << (msg == nullptr ? -1 : int(msg->tag())) << ", error code: " << error_code << std::endl;
        TSUNIT_ASSERT(&sock == &_tlv_client);
        TSUNIT_EQUAL(ts::SYS_SUCCESS, error_code);
        receive_count++;

        if (send_count == 1) {
            // Expect channel status in response to channel setup.
            TSUNIT_EQUAL(ts::ecmgscs::Tags::channel_status, msg->tag());
            const auto cmd = std::dynamic_pointer_cast<ts::ecmgscs::ChannelStatus>(msg);
            TSUNIT_ASSERT(cmd != nullptr);
            TSUNIT_EQUAL(ts::ecmgscs::Tags::channel_status, cmd->tag());
            TSUNIT_EQUAL(100, cmd->channel_id);

            // Send second request.
            ts::ecmgscs::StreamSetup msg2(_protocol);
            msg2.channel_id = 100;
            msg2.stream_id = 200;
            TSUNIT_ASSERT(_tlv_client.startSendMessage(msg2));
            send_count++;
        }
        else {
            TSUNIT_EQUAL(2, send_count);
            // Expect stream status in response to stream setup.
            TSUNIT_EQUAL(ts::ecmgscs::Tags::stream_status, msg->tag());
            const auto cmd = std::dynamic_pointer_cast<ts::ecmgscs::StreamStatus>(msg);
            TSUNIT_ASSERT(cmd != nullptr);
            TSUNIT_EQUAL(ts::ecmgscs::Tags::stream_status, cmd->tag());
            TSUNIT_EQUAL(100, cmd->channel_id);
            TSUNIT_EQUAL(200, cmd->stream_id);

            // Session completed.
            TSUNIT_ASSERT(_react_client.startCloseWriter(this));
        }
    }

    void TestClient::handleWriteStream(ts::ReactiveStream& stream, int error_code, const ts::ObjectPtr& user_data)
    {
        _debug << "TLV client: message sent, error code: " << error_code << std::endl;
        TSUNIT_EQUAL(ts::SYS_EOF, error_code); // only used with startCloseWriter()
        TSUNIT_ASSERT(_react_client.startClose(this));
    }

    void TestClient::handleTCPClosed(ts::ReactiveTCPConnection& sock, const ts::ObjectPtr& user_data)
    {
        _debug << "TLV client: closed" << std::endl;
        TSUNIT_ASSERT(&sock == &_react_client);
        _reactor.freeExitReference();
        handle_closed_count++;
    }

    //------------------------------------------------------------------------
    // A client connection on the server side.
    //------------------------------------------------------------------------

    class TestServerConnection: public ts::ReactiveServerSessionInterface, private ts::ReactiveTLVConnectionHandlerInterface, private ts::ReactiveTCPConnectionHandlerInterface
    {
        TS_NOBUILD_NOCOPY(TestServerConnection);
    public:
        TestServerConnection(ts::Reactor& reactor, ts::tlv::Logger& logger, const ts::tlv::Protocol& protocol, std::ostream& debug);

        static size_t handle_accepted_count;
        static size_t handle_closed_count;

    private:
        ts::Reactor&              _reactor;
        ts::tlv::Logger&          _logger;
        const ts::tlv::Protocol&  _protocol;
        std::ostream&             _debug;
        ts::TCPConnection         _tcp_client;
        ts::ReactiveTCPConnection _react_client;
        ts::ReactiveTLVConnection _tlv_client;
        size_t                    _msg_count = 0;

        virtual ts::ReactiveTCPConnection& getConnection() override;
        virtual void handleTCPAccepted(ts::ReactiveTCPServer& server, ts::ReactiveTCPConnection& sock, int error_code, const ts::ObjectPtr& user_data) override;
        virtual void handleReceivedMessage(ts::ReactiveTLVConnection& sock, const ts::tlv::MessagePtr& msg, int error_code) override;
        virtual void handleTCPClosed(ts::ReactiveTCPConnection& sock, const ts::ObjectPtr& user_data) override;
    };

    size_t TestServerConnection::handle_accepted_count = 0;
    size_t TestServerConnection::handle_closed_count = 0;

    TestServerConnection::TestServerConnection(ts::Reactor& reactor, ts::tlv::Logger& logger, const ts::tlv::Protocol& protocol, std::ostream& debug) :
        _reactor(reactor),
        _logger(logger),
        _protocol(protocol),
        _debug(debug),
        _tcp_client(&_reactor.report()),
        _react_client(_reactor, _tcp_client),
        _tlv_client(_logger, _protocol, _react_client, true, 1)
    {
        _debug << "TLV server session: initialized" << std::endl;
        _react_client.whenAccepted(this);
    }

    ts::ReactiveTCPConnection& TestServerConnection::getConnection()
    {
        return _react_client;
    }

    void TestServerConnection::handleTCPAccepted(ts::ReactiveTCPServer& server, ts::ReactiveTCPConnection& sock, int error_code, const ts::ObjectPtr& user_data)
    {
        _debug << "TLV server session: accepted, error code: " << error_code << std::endl;
        TSUNIT_ASSERT(&sock == &_react_client);
        TSUNIT_EQUAL(ts::SYS_SUCCESS, error_code);
        handle_accepted_count++;
        if (error_code == ts::SYS_SUCCESS) {
            TSUNIT_ASSERT(_tlv_client.startReceive(this));
        }
    }

    void TestServerConnection::handleReceivedMessage(ts::ReactiveTLVConnection& sock, const ts::tlv::MessagePtr& msg, int error_code)
    {
        TSUNIT_ASSERT(&sock == &_tlv_client);
        if (error_code == ts::SYS_EOF) {
            // Client disconnected.
            _debug << "TLV server session: end of session" << std::endl;
            TSUNIT_ASSERT(_react_client.startClose(this));
        }
        else {
            _debug << "TLV server session: received tag: " << (msg == nullptr ? -1 : int(msg->tag())) << ", error code: " << error_code << std::endl;
            TSUNIT_EQUAL(ts::SYS_SUCCESS, error_code);
            _msg_count++;

            if (_msg_count == 1) {
                // Expect channel setup.
                TSUNIT_EQUAL(ts::ecmgscs::Tags::channel_setup, msg->tag());
                const auto cmd = std::dynamic_pointer_cast<ts::ecmgscs::ChannelSetup>(msg);
                TSUNIT_ASSERT(cmd != nullptr);
                TSUNIT_EQUAL(ts::ecmgscs::Tags::channel_setup, cmd->tag());
                TSUNIT_EQUAL(100, cmd->channel_id);

                // Send response.
                ts::ecmgscs::ChannelStatus msg2(_protocol);
                msg2.channel_id = cmd->channel_id;
                TSUNIT_ASSERT(_tlv_client.startSendMessage(msg2));
            }
            else {
                TSUNIT_EQUAL(2, _msg_count);
                // Expect stream setup.
                TSUNIT_EQUAL(ts::ecmgscs::Tags::stream_setup, msg->tag());
                const auto cmd = std::dynamic_pointer_cast<ts::ecmgscs::StreamSetup>(msg);
                TSUNIT_ASSERT(cmd != nullptr);
                TSUNIT_EQUAL(ts::ecmgscs::Tags::stream_setup, cmd->tag());
                TSUNIT_EQUAL(100, cmd->channel_id);
                TSUNIT_EQUAL(200, cmd->stream_id);

                // Send response.
                ts::ecmgscs::StreamStatus msg2(_protocol);
                msg2.channel_id = cmd->channel_id;
                msg2.stream_id = cmd->stream_id;
                TSUNIT_ASSERT(_tlv_client.startSendMessage(msg2));
            }
        }
    }

    void TestServerConnection::handleTCPClosed(ts::ReactiveTCPConnection& sock, const ts::ObjectPtr& user_data)
    {
        _debug << "TLV server session: closed" << std::endl;
        TSUNIT_ASSERT(&sock == &_react_client);
        TSUNIT_EQUAL(2, _msg_count);
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
        TestFactory(ts::Reactor& reactor, ts::tlv::Logger& logger, const ts::tlv::Protocol& protocol, std::ostream& debug);

        size_t client_count = 0;
        size_t server_exited_count = 0;

    private:
        ts::Reactor&             _reactor;
        ts::tlv::Logger&         _logger;
        const ts::tlv::Protocol& _protocol;
        std::ostream&            _debug;

        virtual ts::ReactiveServerSessionInterface* newClientSession() override;
        virtual void handleServerExited(ts::ReactiveServer& server, const ts::ObjectPtr& user_data) override;
    };

    TestFactory::TestFactory(ts::Reactor& reactor, ts::tlv::Logger& logger, const ts::tlv::Protocol& protocol, std::ostream& debug) :
        _reactor(reactor),
        _logger(logger),
        _protocol(protocol),
        _debug(debug)
    {
        _debug << "TestFactory: initialized" << std::endl;
    }

    ts::ReactiveServerSessionInterface* TestFactory::newClientSession()
    {
        _debug << "TestFactory: create client session" << std::endl;
        client_count++;
        return new TestServerConnection(_reactor, _logger, _protocol, _debug);
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

    const ts::ecmgscs::Protocol protocol;
    ts::tlv::Logger             logger(&CERR, ts::Severity::Debug);
    ts::Reactor                 reactor(&CERR);
    TestFactory                 factory(reactor, logger, protocol, debug());
    ts::TCPServer               tcp_server(&CERR);
    ts::ReactiveTCPServer       rtcp_server(reactor, tcp_server);
    ts::ReactiveServer          server(rtcp_server);

    TSUNIT_ASSERT(reactor.open());

    TSUNIT_ASSERT(tcp_server.open(ts::IP::v4));
    TSUNIT_ASSERT(tcp_server.reusePort(true));
    TSUNIT_ASSERT(tcp_server.bind(ts::IPSocketAddress(ts::IPAddress::LocalHost4, PORT)));
    TSUNIT_ASSERT(tcp_server.listen(5));
    TSUNIT_EQUAL(1, reactor.addExitReference()); // server

    server.setExitAfterClientCount(1);
    server.start(&factory, &factory);

    TestClient client(reactor, logger, protocol, debug(), PORT);

    TSUNIT_ASSERT(reactor.processEventLoop());
    TSUNIT_ASSERT(reactor.close());

    TSUNIT_EQUAL(1, client.handle_connected_count);
    TSUNIT_EQUAL(1, client.handle_closed_count);
    TSUNIT_EQUAL(2, client.send_count);
    TSUNIT_EQUAL(2, client.receive_count);

    TSUNIT_EQUAL(1, factory.client_count);
    TSUNIT_EQUAL(1, factory.server_exited_count);

    TSUNIT_EQUAL(1, TestServerConnection::handle_accepted_count);
    TSUNIT_EQUAL(1, TestServerConnection::handle_closed_count);
}
