//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for subclasses of ts::Reactor
//
//----------------------------------------------------------------------------

#include "tsReactor.h"
#include "tsReactiveUDPSocket.h"
#include "tsReactiveTCPConnection.h"
#include "tsReactiveTCPServer.h"
#include "tsReactiveServer.h"
#include "tsReactiveTelnetConnection.h"
#include "tsForkPipe.h"
#include "tsTime.h"
#include "tsCerrReport.h"
#include "tsSysUtils.h"
#include "tsSysInfo.h"
#include "tsFileUtils.h"
#include "tsEnvironment.h"
#include "tsErrCodeReport.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ReactorTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(Timer);
    TSUNIT_DECLARE_TEST(Broadcast);
    TSUNIT_DECLARE_TEST(UDP);
    TSUNIT_DECLARE_TEST(Server);
    TSUNIT_DECLARE_TEST(Client);
    TSUNIT_DECLARE_TEST(Process);

public:
    virtual void beforeTestSuite() override;
    virtual void beforeTest() override;
    virtual void afterTest() override;


private:
    cn::milliseconds _precision {};
    fs::path _temp_file_name {};
};

TSUNIT_REGISTER(ReactorTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void ReactorTest::beforeTestSuite()
{
    _precision = cn::milliseconds(2);
    ts::SetTimersPrecision(_precision);
    debug() << "ReactorTest: timer precision = " << ts::UString::Chrono(_precision) << std::endl;
}

// ReactorTest initialization method.
void ReactorTest::beforeTest()
{
    _temp_file_name = ts::TempFile(u".tmp");
    fs::remove(_temp_file_name, &ts::ErrCodeReport());
}

// Test cleanup method.
void ReactorTest::afterTest()
{
    fs::remove(_temp_file_name, &ts::ErrCodeReport());
}


//----------------------------------------------------------------------------
// Unitary test : timer and user event.
//----------------------------------------------------------------------------

namespace {
    class HandlerTimer: public ts::ReactorHandlerInterface
    {
    private:
        cn::milliseconds _precision;

    public:
        HandlerTimer() = delete;
        HandlerTimer(cn::milliseconds precision) : _precision(precision) {}

        ts::Time start {};

        ts::EventId timer1 {};
        ts::EventId timer2 {};
        ts::EventId event1 {};

        size_t timer1_count = 0;
        size_t timer2_count = 0;
        size_t event1_count = 0;

        virtual void handleTimer(ts::Reactor& reactor, ts::EventId id) override;
        virtual void handleUserEvent(ts::Reactor& reactor, ts::EventId id) override;
    };

    void HandlerTimer::handleTimer(ts::Reactor& reactor, ts::EventId id)
    {
        tsunit::Test::debug() << "HandlerTimer::handleTimer: id: " << id.toString() << ", timer1 count: " << timer1_count << ", timer2 count: " << timer2_count << std::endl;

        if (id == timer1) {
            // Repeated timer: 200 ms
            const ts::Time now(ts::Time::CurrentUTC());
            TSUNIT_ASSERT(id == timer1);
            timer1_count++;
            TSUNIT_ASSERT(timer1_count < 3);
            TSUNIT_ASSUME(timer2_count == 1);
            if (timer1_count == 1) {
                TSUNIT_ASSERT(now >= start + cn::milliseconds(200) - _precision);
            }
            else if (timer1_count == 2) {
                TSUNIT_ASSERT(now >= start + cn::milliseconds(400) - _precision);
                TSUNIT_ASSERT(reactor.cancelTimer(id));
                TSUNIT_ASSERT(reactor.signalEvent(event1));
            }
        }
        else if (id == timer2) {
            // One-shot timer: 100 ms
            const ts::Time now(ts::Time::CurrentUTC());
            TSUNIT_ASSERT(id == timer2);
            timer2_count++;
            TSUNIT_EQUAL(1, timer2_count);
            TSUNIT_ASSUME(timer1_count == 0);
            TSUNIT_ASSERT(now >= start + cn::milliseconds(100) - _precision);
        }
        else {
            TSUNIT_ASSERT(false);
        }
    }

    void HandlerTimer::handleUserEvent(ts::Reactor& reactor, ts::EventId id)
    {
        tsunit::Test::debug() << "HandlerTimer::handleUserEvent: id: " << id.toString() << ", event1 count: " << event1_count << std::endl;

        const ts::Time now(ts::Time::CurrentUTC());
        TSUNIT_ASSERT(id == event1);
        event1_count++;
        TSUNIT_EQUAL(1, event1_count);
        TSUNIT_EQUAL(2, timer1_count);
        TSUNIT_EQUAL(1, timer2_count);
        TSUNIT_ASSERT(now >= start + cn::milliseconds(400) - _precision);
        reactor.exitEventLoop();
    }
}

TSUNIT_DEFINE_TEST(Timer)
{
    ts::Reactor reactor(&CERR);
    HandlerTimer test(_precision);

    TSUNIT_ASSERT(!reactor.isOpen());
    TSUNIT_ASSERT(reactor.open());
    TSUNIT_ASSERT(reactor.isOpen());

    test.timer1 = reactor.newTimer(&test, cn::milliseconds(200), true);
    TSUNIT_ASSERT(test.timer1.isValid());
    debug() << "ReactorTest::Timer: timer1 id: " << test.timer1.toString() << std::endl;

    test.timer2 = reactor.newTimer(&test, cn::milliseconds(100), false);
    TSUNIT_ASSERT(test.timer2.isValid());
    debug() << "ReactorTest::Timer: timer2 id: " << test.timer2.toString() << std::endl;

    test.event1 = reactor.newEvent(&test);
    TSUNIT_ASSERT(test.event1.isValid());
    debug() << "ReactorTest::Timer: event1 id: " << test.event1.toString() << std::endl;

    test.start = ts::Time::CurrentUTC();
    TSUNIT_ASSERT(reactor.processEventLoop());

    const ts::Time now(ts::Time::CurrentUTC());
    TSUNIT_ASSERT(now >= test.start + cn::milliseconds(400) - _precision);

    TSUNIT_EQUAL(1, test.event1_count);
    TSUNIT_EQUAL(2, test.timer1_count);
    TSUNIT_EQUAL(1, test.timer2_count);

    TSUNIT_ASSERT(reactor.isOpen());
    TSUNIT_ASSERT(reactor.close());
    TSUNIT_ASSERT(!reactor.isOpen());
}


//----------------------------------------------------------------------------
// Unitary test : broadcast event.
//----------------------------------------------------------------------------

namespace {
    class Foo: public ts::Object
    {
    public:
        int value;
        Foo() = delete;
        Foo(int v) : value(v) {}
        virtual ~Foo() override;
    };

    Foo::~Foo()
    {
    }

    class HandlerBroadcast: public ts::ReactorHandlerInterface
    {
    private:
        ts::EventId _event_id {};

    public:
        HandlerBroadcast() = delete;
        HandlerBroadcast(ts::Reactor& reactor);

        size_t event_count = 0;
        size_t broadcast_count = 0;

        virtual void handleUserEvent(ts::Reactor& reactor, ts::EventId id) override;
        virtual void handleBroadcastEvent(ts::Reactor& reactor, int error_code, const ts::ObjectPtr& user_data) override;
    };

    HandlerBroadcast::HandlerBroadcast(ts::Reactor& reactor)
    {
        tsunit::Test::debug() << "HandlerBroadcast constructor" << std::endl;
        _event_id = reactor.newEvent(this);
        TSUNIT_ASSERT(_event_id.isValid());
        reactor.addExitReference();
    }

    void HandlerBroadcast::handleUserEvent(ts::Reactor& reactor, ts::EventId id)
    {
        tsunit::Test::debug() << "HandlerBroadcast::handleUserEvent: should not be there" << std::endl;
        event_count++;
    }

    void HandlerBroadcast::handleBroadcastEvent(ts::Reactor& reactor, int error_code, const ts::ObjectPtr& user_data)
    {
        auto foo = std::dynamic_pointer_cast<Foo>(user_data);
        TSUNIT_ASSERT(foo != nullptr);
        tsunit::Test::debug() << "HandlerBroadcast::handleBroadcastEvent: error_code = " << error_code << ", user data: " << foo->value << std::endl;
        broadcast_count++;
        TSUNIT_EQUAL(broadcast_count, error_code);
        TSUNIT_EQUAL(broadcast_count, foo->value);
        if (broadcast_count >= 2) {
            reactor.freeExitReference();
        }
    }
}

TSUNIT_DEFINE_TEST(Broadcast)
{
    ts::Reactor reactor(&CERR);
    TSUNIT_ASSERT(reactor.open());

    HandlerBroadcast test1(reactor);
    HandlerBroadcast test2(reactor);
    HandlerBroadcast test3(reactor);

    TSUNIT_ASSERT(reactor.signalBroadcastEvent(1, std::make_shared<Foo>(1)));
    TSUNIT_ASSERT(reactor.signalBroadcastEvent(2, std::make_shared<Foo>(2)));
    TSUNIT_ASSERT(reactor.processEventLoop());
    TSUNIT_ASSERT(reactor.close());

    TSUNIT_EQUAL(0, test1.event_count);
    TSUNIT_EQUAL(2, test1.broadcast_count);
    TSUNIT_EQUAL(0, test2.event_count);
    TSUNIT_EQUAL(2, test2.broadcast_count);
    TSUNIT_EQUAL(0, test3.event_count);
    TSUNIT_EQUAL(2, test3.broadcast_count);
}


//----------------------------------------------------------------------------
// Unitary test : UDP socket.
//----------------------------------------------------------------------------

namespace {

    class UserData: public ts::Object
    {
    public:
        UserData(const ts::UString& s_, uint32_t i_) : s(s_), i(i_) {}
        ts::UString s {};
        uint32_t i = 0;
    };

    class HandlerUDP: private ts::ReactorHandlerInterface, private ts::ReactiveUDPHandlerInterface
    {
    public:
        HandlerUDP() = delete;
        HandlerUDP(cn::milliseconds precision);

    private:
        cn::milliseconds          _precision;
        ts::Time                  _start_timer {};
        ts::EventId               _timer {};
        ts::Reactor               _reactor {&CERR};
        ts::UDPSocket             _client {&_reactor};
        ts::UDPSocket             _server {&_reactor};
        ts::ReactiveUDPSocket     _rclient {_reactor, _client};
        ts::ReactiveUDPSocket     _rserver {_reactor, _server};
        const ts::IPSocketAddress _server_address {ts::IPAddress::LocalHost4, 19654};
        bool                      _client_closed = false;
        bool                      _server_closed = false;

        // Scenario:
        // client --> server : request1
        // server --> client : response1
        // timer 100 ms
        // client --> server : request2
        // server --> client : response2

        static constexpr uint32_t _request1  = 0x01020304;
        static constexpr uint32_t _response1 = 0x05060708;
        static constexpr uint32_t _request2  = 0x090A0B0C;
        static constexpr uint32_t _response2 = 0x0D0E0F10;

        // Get request / response name based on address or value.
        ts::UString name(uintptr_t value) const;
        ts::UString name(const void* address) const;
        ts::UString name(const ts::ReactiveUDPSocket& sock) const;

        virtual void handleTimer(ts::Reactor& reactor, ts::EventId id) override;
        virtual void handleUDPClosed(ts::ReactiveUDPSocket& sock, const ts::ObjectPtr& user_data) override;
        virtual void handleUDPSend(ts::ReactiveUDPSocket& sock, const void* data, size_t size, const ts::IPSocketAddress& destination, int error_code, const ts::ObjectPtr& user_data) override;
        virtual void handleUDPReceive(ts::ReactiveUDPSocket& sock,
                                      const ts::ByteBlockPtr& data,
                                      const ts::IPSocketAddress& sender,
                                      const ts::IPSocketAddress& destination,
                                      cn::microseconds timestamp,
                                      ts::UDPSocket::TimeStampType timestamp_type,
                                      int error_code,
                                      const ts::ObjectPtr& user_data) override;
    };

    HandlerUDP::HandlerUDP(cn::milliseconds precision) :
        _precision(precision)
    {
        TSUNIT_ASSERT(!_reactor.isOpen());
        TSUNIT_ASSERT(_reactor.open());
        TSUNIT_ASSERT(_reactor.isOpen());

        TSUNIT_ASSERT(_server.open(ts::IP::v4));
        TSUNIT_ASSERT(_server.reusePort(true));
        TSUNIT_ASSERT(_server.bind(_server_address));

        TSUNIT_ASSERT(_client.open(ts::IP::v4));
        TSUNIT_ASSERT(_client.bind(ts::IPSocketAddress::AnySocketAddress4));
        TSUNIT_ASSERT(_client.setDefaultDestination(_server_address));

        TSUNIT_ASSERT(_rserver.startReceive(this, 1024, std::make_shared<UserData>(u"receive", 111)));
        TSUNIT_ASSERT(_rclient.startReceive(this, 1024, std::make_shared<UserData>(u"receive", 111)));
        TSUNIT_ASSERT(_rclient.startSend(this, &_request1, sizeof(_request1), std::make_shared<UserData>(u"send", _request1)));

        TSUNIT_ASSERT(_reactor.processEventLoop());

        TSUNIT_ASSERT(!_client.isOpen());
        TSUNIT_ASSERT(!_server.isOpen());
        TSUNIT_ASSERT(_reactor.close());
    }

    void HandlerUDP::handleTimer(ts::Reactor& reactor, ts::EventId id)
    {
        tsunit::Test::debug() << "HandlerUDP::handleTimer: id: " << id.toString() << std::endl;
        TSUNIT_ASSERT(id == _timer);
        TSUNIT_ASSERT(ts::Time::CurrentUTC() >= _start_timer + cn::milliseconds(100) - _precision);
        TSUNIT_ASSERT(_rclient.startSend(this, &_request2, sizeof(_request2), std::make_shared<UserData>(u"send", _request2)));
    }

    void HandlerUDP::handleUDPSend(ts::ReactiveUDPSocket& sock, const void* data, size_t size, const ts::IPSocketAddress& destination, int error_code, const ts::ObjectPtr& user_data)
    {
        TSUNIT_EQUAL(sizeof(uint32_t), size);
        TSUNIT_ASSERT(data != nullptr);
        const uint32_t value = *reinterpret_cast<const uint32_t*>(data);

        tsunit::Test::debug() << "HandlerUDP::handleUDPSend: socket: " << name(sock) << ", value: " << name(data) << ", destination: " << destination << ", error: " << error_code << std::endl;

        auto udata = std::dynamic_pointer_cast<UserData>(user_data);
        TSUNIT_ASSERT(udata != nullptr);
        TSUNIT_EQUAL(u"send", udata->s);
        TSUNIT_EQUAL(value, udata->i);

        if (value == _request1) {
            // Sent request1, must be in the client.
            TSUNIT_ASSERT(&sock == &_rclient);
        }
        else if (value == _response1) {
            // Sent response1, must be in the server.
            TSUNIT_ASSERT(&sock == &_rserver);
        }
        else if (value == _request2) {
            // Sent request2, must be in the client.
            TSUNIT_ASSERT(&sock == &_rclient);
        }
        else if (value == _response2) {
            // Sent response2, must be in the server.
            TSUNIT_ASSERT(&sock == &_rserver);
            TSUNIT_ASSERT(_rserver.startClose(this, false, std::make_shared<UserData>(u"close", 777)));
        }
        else {
            TSUNIT_ASSERT(false);
        }
    }

    void HandlerUDP::handleUDPReceive(ts::ReactiveUDPSocket& sock,
                                      const ts::ByteBlockPtr& data,
                                      const ts::IPSocketAddress& sender,
                                      const ts::IPSocketAddress& destination,
                                      cn::microseconds timestamp,
                                      ts::UDPSocket::TimeStampType timestamp_type,
                                      int error_code,
                                      const ts::ObjectPtr& user_data)
    {
        TSUNIT_EQUAL(ts::SYS_SUCCESS, error_code);
        TSUNIT_ASSERT(data != nullptr);
        TSUNIT_EQUAL(sizeof(uint32_t), data->size());
        const uint32_t value = *reinterpret_cast<const uint32_t*>(data->data());

        tsunit::Test::debug() << "HandlerUDP::handleUDPReceive: socket: " << name(sock) << ", value: " << name(value) << ", error: " << error_code
               << ", sender: " << sender << ", destination: " << destination << std::endl;

        auto udata = std::dynamic_pointer_cast<UserData>(user_data);
        TSUNIT_ASSERT(udata != nullptr);
        TSUNIT_EQUAL(u"receive", udata->s);
        TSUNIT_EQUAL(111, udata->i);

        if (value == _request1) {
            // Received request1, must be in the server.
            TSUNIT_ASSERT(&sock == &_rserver);
            TSUNIT_ASSERT(!_timer.isValid());
            TSUNIT_ASSERT(_rserver.startSend(this, &_response1, sizeof(_response1), sender, std::make_shared<UserData>(u"send", _response1)));
        }
        else if (value == _response1) {
            // Received response1, must be in the client.
            TSUNIT_ASSERT(&sock == &_rclient);
            TSUNIT_ASSERT(!_timer.isValid());
            _start_timer = ts::Time::CurrentUTC();
            _timer = _reactor.newTimer(this, cn::milliseconds(100), false);
            TSUNIT_ASSERT(_timer.isValid());
        }
        else if (value == _request2) {
            // Received request2, must be in the server.
            TSUNIT_ASSERT(&sock == &_rserver);
            TSUNIT_ASSERT(_timer.isValid());
            TSUNIT_ASSERT(_rserver.startSend(this, &_response2, sizeof(_response2), sender, std::make_shared<UserData>(u"send", _response2)));
        }
        else if (value == _response2) {
            // Received response2, must be in the client.
            TSUNIT_ASSERT(&sock == &_rclient);
            TSUNIT_ASSERT(_timer.isValid());
            TSUNIT_ASSERT(_rclient.startClose(this, false, std::make_shared<UserData>(u"close", 777)));
        }
        else {
            TSUNIT_ASSERT(false);
        }
    }

    void HandlerUDP::handleUDPClosed(ts::ReactiveUDPSocket& sock, const ts::ObjectPtr& user_data)
    {
        tsunit::Test::debug() << "HandlerUDP::handleUDPClosed: socket: " << name(sock) << std::endl;

        auto udata = std::dynamic_pointer_cast<UserData>(user_data);
        TSUNIT_ASSERT(udata != nullptr);
        TSUNIT_EQUAL(u"close", udata->s);
        TSUNIT_EQUAL(777, udata->i);

        TSUNIT_ASSERT(&sock == &_rserver || &sock == &_rclient);
        if (&sock == &_rclient) {
            TSUNIT_ASSERT(!_client.isOpen());
            TSUNIT_ASSERT(!_client_closed);
            _client_closed = true;
        }
        else if (&sock == &_rserver) {
            TSUNIT_ASSERT(!_server.isOpen());
            TSUNIT_ASSERT(!_server_closed);
            _server_closed = true;
        }
        // Exit event loop when the two sockets are closed.
        if (_server_closed && _client_closed) {
            sock.reactor().exitEventLoop();
        }
    }

    ts::UString HandlerUDP::name(uintptr_t value) const
    {
        if (value == _request1) return u"request1";
        else if (value == _response1) return u"response1";
        else if (value == _request2) return u"request2";
        else if (value == _response2) return u"response2";
        else return ts::UString::Format(u"unknown 0x%04X", value);
    }

    ts::UString HandlerUDP::name(const void* addr) const
    {
        if (addr == &_request1) return u"request1";
        else if (addr == &_response1) return u"response1";
        else if (addr == &_request2) return u"request2";
        else if (addr == &_response2) return u"response2";
        else return ts::UString::Format(u"unknown address 0x%X", uintptr_t(addr));
    }

    ts::UString HandlerUDP::name(const ts::ReactiveUDPSocket& sock) const
    {
        if (&sock == &_rclient) return u"client";
        else if (&sock == &_rserver) return u"server";
        else return ts::UString::Format(u"unknown server address 0x%X", uintptr_t(&sock));
    }
}

TSUNIT_DEFINE_TEST(UDP)
{
    TSUNIT_ASSERT(ts::IPInitialize());
    HandlerUDP test(_precision);
}


//----------------------------------------------------------------------------
// Unitary test : TCP socket in telnet mode.
//----------------------------------------------------------------------------

namespace {
    class TelnetClient: private ts::ReactiveTCPConnectionHandlerInterface, private ts::ReactiveTelnetConnectionHandlerInterface
    {
    public:
        TelnetClient() = delete;
        TelnetClient(ts::Reactor& reactor);

    private:
        ts::Reactor&                 _reactor;
        ts::TCPConnection            _client {&_reactor};
        ts::ReactiveTCPConnection    _rclient {_reactor, _client};
        ts::ReactiveTelnetConnection _rtclient {_rclient};
        const ts::UString            _server_name {ts::GetEnvironment(u"TS_UTEST_TELNET_HOST", u"tsduck.io")};
        const ts::IPAddress          _server_address {_server_name, _reactor.report(), ts::IP::v4};
        const ts::IPAddress::Port    _server_port = ts::GetIntEnvironment<ts::IPAddress::Port>(u"TS_UTEST_TELNET_PORT", 80);
        const ts::IPSocketAddress    _server_socket {_server_address, _server_port};

        virtual void handleTCPConnected(ts::ReactiveTCPConnection& sock, int error_code, const ts::ObjectPtr& user_data) override;
        virtual void handleTelnetLine(ts::ReactiveTelnetConnection& sock, const ts::UString& line, int error_code) override;
        virtual void handleTCPClosed(ts::ReactiveTCPConnection& sock, const ts::ObjectPtr& user_data) override;
    };

    TelnetClient::TelnetClient(ts::Reactor& reactor) :
        _reactor(reactor)
    {
        tsunit::Test::debug() << "TelnetClient: connecting to " << _server_name << " (" << _server_socket << ")" << std::endl;
        TSUNIT_ASSERT(_client.open(ts::IP::v4));
        TSUNIT_ASSERT(_client.bind(ts::IPSocketAddress::AnySocketAddress4));
        TSUNIT_ASSERT(_rclient.startConnect(this, _server_socket));
    }

    void TelnetClient::handleTCPConnected(ts::ReactiveTCPConnection& sock, int error_code, const ts::ObjectPtr& user_data)
    {
        tsunit::Test::debug() << "TelnetClient::handleTCPConnected: error code: " << error_code << std::endl;
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
            tsunit::Test::debug() << "TelnetClient::handleTelnetLine: end of response" << std::endl;
            TSUNIT_ASSERT(_client.disconnect());
            TSUNIT_ASSERT(_rclient.startClose(this));
        }
        else {
            tsunit::Test::debug() << "TelnetClient::handleTelnetLine: \"" << line << "\", error code: " << error_code << std::endl;
        }
    }

    void TelnetClient::handleTCPClosed(ts::ReactiveTCPConnection& sock, const ts::ObjectPtr& user_data)
    {
        tsunit::Test::debug() << "TelnetClient::handleTCPClosed" << std::endl;
        TSUNIT_ASSERT(!_client.isOpen());
        sock.reactor().exitEventLoop();
    }
}

TSUNIT_DEFINE_TEST(Client)
{
    TSUNIT_ASSERT(ts::IPInitialize());
    ts::Reactor reactor(&CERR);
    TSUNIT_ASSERT(reactor.open());
    TelnetClient test(reactor);
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
        TestClient(ts::Reactor& reactor, cn::milliseconds delay, size_t request_count, uint16_t server_port);

        size_t handle_connected_count = 0;
        size_t handle_closed_count = 0;
        size_t send_count = 0;
        size_t receive_count = 0;

    private:
        ts::Reactor&              _reactor;
        cn::milliseconds          _delay;
        size_t                    _request_count;
        const ts::IPSocketAddress _server_address;
        size_t                    _client_id = 0;
        ts::TCPConnection         _client {&_reactor.report()};
        ts::ReactiveTCPConnection _rclient {_reactor, _client};
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

    TestClient::TestClient(ts::Reactor& reactor, cn::milliseconds delay, size_t request_count, uint16_t server_port) :
        _reactor(reactor),
        _delay(delay),
        _request_count(request_count),
        _server_address(ts::IPAddress::LocalHost4, server_port),
        _client_id(++_id_counter)
    {
        tsunit::Test::debug() << "TestClient: initialize client id " << _client_id << std::endl;
        _reactor.addExitReference();
        TSUNIT_ASSERT(_delay > cn::milliseconds::zero());
        TSUNIT_ASSERT(_request_count > 0);
        _timer_id = _reactor.newTimer(this, _delay, false);
        TSUNIT_ASSERT(_timer_id.isValid());
    }

    void TestClient::handleTimer(ts::Reactor& reactor, ts::EventId id)
    {
        tsunit::Test::debug() << "TestClient::handleTimer, client id: " << _client_id << std::endl;
        TSUNIT_ASSERT(id == _timer_id);

        if (!_client.isConnected()) {
            // First time: start connecting.
            tsunit::Test::debug() << "TestClient::handleTimer, client id: " << _client_id << ", start connection" << std::endl;
            TSUNIT_ASSERT(_client.open(ts::IP::v4));
            TSUNIT_ASSERT(_client.bind(ts::IPSocketAddress::AnySocketAddress4));
            TSUNIT_ASSERT(_rclient.startConnect(this, _server_address));
        }
        else {
            // Next times: send a request.
            _request++;
            tsunit::Test::debug() << "TestClient::handleTimer, client id: " << _client_id << ", start send request " << _request << std::endl;
            TSUNIT_ASSERT(_rclient.startSend(this, &_request, sizeof(_request)));
            send_count++;
            _expected_send_position += sizeof(_request);
        }
    }

    void TestClient::handleTCPConnected(ts::ReactiveTCPConnection& sock, int error_code, const ts::ObjectPtr& user_data)
    {
        tsunit::Test::debug() << "TestClient::handleTCPConnected, client id: " << _client_id << ", error code: " << error_code << std::endl;
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
        tsunit::Test::debug() << "TestClient::handleTCPSend, client id: " << _client_id << ", position: " << position << ", error code: " << error_code << std::endl;
        TSUNIT_ASSERT(&sock == &_rclient);
        TSUNIT_EQUAL(_expected_send_position, position);
    }

    void TestClient::handleTCPReceive(ts::ReactiveTCPConnection& sock, const ts::ByteBlock& data, ts::ReactiveTCPInputControl& control, int error_code, const ts::ObjectPtr& user_data)
    {
        tsunit::Test::debug() << "TestClient::handleTCPReceive, client id: " << _client_id << ", error code: " << error_code << ", size: " << data.size() << std::endl;
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
            tsunit::Test::debug() << "TestClient::handleTCPReceive, client id: " << _client_id << ", request: " << _request << ", response: " << response << std::endl;
            TSUNIT_EQUAL(_request + 1, response);
            receive_count++;

            if (_request < _request_count) {
                // Wait before sending the next request.
                _timer_id = _reactor.newTimer(this, _delay, false);
                TSUNIT_ASSERT(_timer_id.isValid());
            }
            else {
                // Session completed.
                tsunit::Test::debug() << "TestClient::handleTCPReceive, start close, client id: " << _client_id << std::endl;
                TSUNIT_ASSERT(_rclient.startClose(this));
            }
        }
    }

    void TestClient::handleTCPClosed(ts::ReactiveTCPConnection& sock, const ts::ObjectPtr& user_data)
    {
        tsunit::Test::debug() << "TestClient::handleTCPClosed, client id: " << _client_id << std::endl;
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
        TestServerConnection(ts::Reactor& reactor);
        virtual ~TestServerConnection() override;
        size_t clientId() const { return _client_id; }

        static size_t handle_accepted_count;
        static size_t handle_closed_count;

    private:
        ts::Reactor&              _reactor;
        size_t                    _client_id = 0;
        ts::TCPConnection         _client {&_reactor.report()};
        ts::ReactiveTCPConnection _rclient {_reactor, _client};
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

    TestServerConnection::TestServerConnection(ts::Reactor& reactor) :
        _reactor(reactor),
        _client_id(++_id_counter)
    {
        tsunit::Test::debug() << "TestServerConnection: initialize client session id " << _client_id << std::endl;
        _rclient.whenAccepted(this);
    }

    TestServerConnection::~TestServerConnection()
    {
        tsunit::Test::debug() << "TestServerConnection: destruction of client session id " << _client_id << " @" << ts::UString::Hexa(uintptr_t(this)) << std::endl;
    }

    ts::ReactiveTCPConnection& TestServerConnection::getConnection()
    {
        return _rclient;
    }

    void TestServerConnection::handleTCPAccepted(ts::ReactiveTCPServer& server, ts::ReactiveTCPConnection& sock, int error_code, const ts::ObjectPtr& user_data)
    {
        tsunit::Test::debug() << "TestServerConnection::handleTCPAccepted, client id: " << _client_id << ", error code: " << error_code
               << ", client: " << sock.socket().peerName() << ", local: " << sock.socket().localName() << std::endl;
        TSUNIT_ASSERT(&sock == &_rclient);
        handle_accepted_count++;
        if (error_code == ts::SYS_SUCCESS) {
            TSUNIT_ASSERT(_rclient.startReceive(this));
        }
    }

    void TestServerConnection::handleTCPSend(ts::ReactiveTCPConnection& sock, size_t position, int error_code, const ts::ObjectPtr& user_data)
    {
        tsunit::Test::debug() << "TestServerConnection::handleTCPSend, client id: " << _client_id << ", position: " << position << ", error code: " << error_code << std::endl;
        TSUNIT_ASSERT(&sock == &_rclient);
        TSUNIT_EQUAL(_expected_send_position, position);
    }

    void TestServerConnection::handleTCPReceive(ts::ReactiveTCPConnection& sock, const ts::ByteBlock& data, ts::ReactiveTCPInputControl& control, int error_code, const ts::ObjectPtr& user_data)
    {
        TSUNIT_ASSERT(&sock == &_rclient);
        if (error_code == ts::SYS_EOF) {
            // Client disconnected.
            tsunit::Test::debug() << "TestServerConnection::handleTCPReceive, client id: " << _client_id << ", end of session" << std::endl;
            TSUNIT_ASSERT(_rclient.startClose(this));
        }
        else {
            tsunit::Test::debug() << "TestServerConnection::handleTCPReceive, client id: " << _client_id << ", error code: " << error_code << ", size: " << data.size() << std::endl;
            TSUNIT_EQUAL(ts::SYS_SUCCESS, error_code);
            // Need at least 4 bytes.
            if (data.size() < sizeof(uint32_t)) {
                control.used_size = 0;
                control.min_next_size = sizeof(uint32_t);
            }
            else {
                const uint32_t request = *reinterpret_cast<const uint32_t*>(data.data());
                tsunit::Test::debug() << "TestServerConnection::handleTCPReceive, client id: " << _client_id << ", request: " << request << std::endl;
                control.used_size = sizeof(uint32_t);
                _response = request + 1;
                TSUNIT_ASSERT(_rclient.startSend(this, &_response, sizeof(_response)));
                _expected_send_position += sizeof(_response);
            }
        }
    }

    void TestServerConnection::handleTCPClosed(ts::ReactiveTCPConnection& sock, const ts::ObjectPtr& user_data)
    {
        tsunit::Test::debug() << "TestServerConnection::handleTCPClosed, client id: " << _client_id << std::endl;
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
        TestFactory(ts::Reactor& reactor);

        size_t client_count = 0;
        size_t server_exited_count = 0;

    private:
        ts::Reactor& _reactor;

        virtual ts::ReactiveServerSessionInterface* newClientSession() override;
        virtual void handleServerExited(ts::ReactiveServer& server, const ts::ObjectPtr& user_data) override;
    };

    TestFactory::TestFactory(ts::Reactor& reactor) :
        _reactor(reactor)
    {
        tsunit::Test::debug() << "TestFactory: initialized"<< std::endl;
    }

    ts::ReactiveServerSessionInterface* TestFactory::newClientSession()
    {
        const auto session = new TestServerConnection(_reactor);
        tsunit::Test::debug() << "TestFactory: create client session id " << session->clientId() << std::endl;
        client_count++;
        return session;
    }

    void TestFactory::handleServerExited(ts::ReactiveServer& server, const ts::ObjectPtr& user_data)
    {
        tsunit::Test::debug() << "TestFactory: server exited" << std::endl;
        server_exited_count++;
        _reactor.freeExitReference();
    }
}

TSUNIT_DEFINE_TEST(Server)
{
    static constexpr uint16_t PORT = 12345;

    TSUNIT_ASSERT(ts::IPInitialize());

    ts::Reactor           reactor(&CERR);
    TestFactory           factory(reactor);
    ts::TCPServer         tcp_server(&CERR);
    ts::ReactiveTCPServer rtcp_server(reactor, tcp_server);
    ts::ReactiveServer    server(rtcp_server);

    TSUNIT_ASSERT(reactor.open());

    TSUNIT_ASSERT(tcp_server.open(ts::IP::v4));
    TSUNIT_ASSERT(tcp_server.reusePort(true));
    TSUNIT_ASSERT(tcp_server.bind(ts::IPSocketAddress(ts::IPAddress::LocalHost4, PORT)));
    TSUNIT_ASSERT(tcp_server.listen(5));
    TSUNIT_EQUAL(1, reactor.addExitReference()); // server

    server.setExitAfterClientCount(2);
    server.start(&factory, &factory);

    TestClient client1(reactor, cn::milliseconds(50), 3, PORT);
    TestClient client2(reactor, cn::milliseconds(80), 2, PORT);

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


//----------------------------------------------------------------------------
// Unitary test : process.
//----------------------------------------------------------------------------

namespace {
    class HandlerProcess: public ts::ReactorHandlerInterface
    {
    public:
        HandlerProcess() = delete;
        HandlerProcess(const fs::path& file) : pid_file(file) {}

        ts::ForkPipe         proc {&CERR};
        fs::path             pid_file {};
        ts::EventId          init_timer {};
        ts::EventId          timeout {};
        ts::EventId          fake_process {};
        ts::EventId          real_process {};
        size_t               process_count = 0;
        ts::SysProcessIdType fake_pid = 0xC0FFEE;
        ts::SysProcessIdType real_pid = ts::SYS_PROCESS_ID_INVALID;

        virtual void handleTimer(ts::Reactor& reactor, ts::EventId id) override;
        virtual void handleProcessTermination(ts::Reactor& reactor, ts::EventId id, int pid) override;
    };

    void HandlerProcess::handleTimer(ts::Reactor& reactor, ts::EventId id)
    {
        if (id == init_timer) {
            tsunit::Test::debug() << "HandlerProcess::handleTimer: initial timer" << std::endl;
            init_timer.invalidate();

            // Subscribe to a fake process (emulate situation where a process is already terminated).
            fake_process = reactor.newProcessIdTermination(this, fake_pid);
            TSUNIT_ASSERT(fake_process.isValid());

            // Create a real process.
            const ts::UString cmd = ts::SysInfo::Instance().os() == ts::SysInfo::WINDOWS ?
                // Windows command which writes the PID of the process in the temp file.
                u"powershell -Command \"Start-Sleep -Milliseconds 200; $([System.Diagnostics.Process]::GetCurrentProcess().Id) | Out-File -Encoding ascii " + pid_file + u"\"":
                // UNIX command which writes the PID of the shell in the temp file.
                u"sleep 0.2; echo $$ >" + pid_file;
            tsunit::Test::debug() << "HandlerProcess: starting process \"" << cmd << "\"" << std::endl;

            TSUNIT_ASSERT(proc.open(cmd, ts::ForkPipe::ASYNCHRONOUS, 0, ts::ForkPipe::KEEP_BOTH, ts::ForkPipe::STDIN_NONE));
            real_pid = proc.getProcessId();
            tsunit::Test::debug() << "HandlerProcess::handleTimer: process pid: " << real_pid << " (current: " << ts::GetProcessId() << ")" << std::endl;

            // Subscribe to real process. By handle when possible, by PID otherwise.
            if (proc.getProcessHandle() != ts::SYS_HANDLE_INVALID) {
                tsunit::Test::debug() << "HandlerProcess::handleTimer: using process handle in reactor" << std::endl;
                real_process = reactor.newProcessHandleTermination(this, proc.getProcessHandle());
            }
            else {
                tsunit::Test::debug() << "HandlerProcess::handleTimer: using process PID in reactor" << std::endl;
                real_process = reactor.newProcessIdTermination(this, real_pid);
            }
            TSUNIT_ASSERT(real_process.isValid());
        }
        else if (id == timeout) {
            tsunit::Test::debug() << "HandlerProcess::handleTimer: timeout" << std::endl;
            timeout.invalidate();
            TSUNIT_ASSERT(false);
        }
        else {
            tsunit::Test::debug() << "HandlerProcess::handleTimer: invalid time id: " << id.toString() << std::endl;
            TSUNIT_ASSERT(false);
        }
    }

    void HandlerProcess::handleProcessTermination(ts::Reactor& reactor, ts::EventId id, int pid)
    {
        process_count++;
        tsunit::Test::debug() << "HandlerProcess::handleProcessTermination: pid: " << pid << ", process count: " << process_count << std::endl;

        if (process_count == 1) {
            TSUNIT_EQUAL(fake_pid, pid);
            TSUNIT_ASSERT(id == fake_process);
        }
        else if (process_count == 2) {
            TSUNIT_EQUAL(real_pid, pid);
            TSUNIT_ASSERT(id == real_process);
            reactor.exitEventLoop();
        }
        else {
            TSUNIT_ASSERT(false);
        }
    }
}

TSUNIT_DEFINE_TEST(Process)
{
    ts::Reactor reactor(&CERR);
    HandlerProcess test(_temp_file_name);

    TSUNIT_ASSERT(!reactor.isOpen());
    TSUNIT_ASSERT(reactor.open());
    TSUNIT_ASSERT(reactor.isOpen());

    test.init_timer = reactor.newTimer(&test, cn::milliseconds(50), false);
    TSUNIT_ASSERT(test.init_timer.isValid());

    test.timeout = reactor.newTimer(&test, cn::seconds(10), false);
    TSUNIT_ASSERT(test.timeout.isValid());

    TSUNIT_ASSERT(reactor.processEventLoop());

    TSUNIT_EQUAL(2, test.process_count);
    TSUNIT_ASSERT(!test.init_timer.isValid());
    TSUNIT_ASSERT(test.timeout.isValid());
    TSUNIT_ASSERT(test.real_pid > 0);

    TSUNIT_ASSERT(fs::exists(_temp_file_name));
    ts::UStringList lines;
    TSUNIT_ASSERT(ts::UString::Load(lines, _temp_file_name));
    ts::UString line(ts::UString::Join(lines, u""));
    debug() << "ReactorTest::Process: temp file content: \"" << line << "\"" << std::endl;
    ts::SysProcessIdType pid = 0;
    TSUNIT_ASSERT(line.toInteger(pid));
    TSUNIT_EQUAL(test.real_pid, pid);

    TSUNIT_ASSERT(reactor.isOpen());
    TSUNIT_ASSERT(reactor.close());
    TSUNIT_ASSERT(!reactor.isOpen());
}
