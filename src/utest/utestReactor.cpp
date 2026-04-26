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
#include "tsTime.h"
#include "tsCerrReport.h"
#include "tsSysUtils.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ReactorTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(Timer);
    TSUNIT_DECLARE_TEST(UDP);

public:
    virtual void beforeTestSuite() override;

private:
    cn::milliseconds _precision {};
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


//----------------------------------------------------------------------------
// Unitary test : timer and user event.
//----------------------------------------------------------------------------

namespace {
    class HandlerTimer:
        public ts::ReactorTimerHandlerInterface,
        public ts::ReactorEventHandlerInterface
    {
    private:
        std::ostream&    _debug;
        cn::milliseconds _precision;

    public:
        HandlerTimer() = delete;
        HandlerTimer(std::ostream& debug, cn::milliseconds precision) :
            _debug(debug),
            _precision(precision)
        {}

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
        _debug << "HandlerTimer::handleTimer: id: " << id.toString() << ", timer1 count: " << timer1_count << ", timer2 count: " << timer2_count << std::endl;

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
        _debug << "HandlerTimer::handleUserEvent: id: " << id.toString() << ", event1 count: " << event1_count << std::endl;

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
    ts::Reactor reactor(CERR);
    HandlerTimer test(debug(), _precision);

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
// Unitary test : UDP socket.
//----------------------------------------------------------------------------

namespace {
    class HandlerUDP:
        private ts::ReactorTimerHandlerInterface,
        private ts::ReactiveUDPSendHandlerInterface,
        private ts::ReactiveUDPReceiveHandlerInterface
    {
    public:
        HandlerUDP() = delete;
        HandlerUDP(std::ostream& debug, cn::milliseconds precision);

    private:
        std::ostream&             _debug;
        cn::milliseconds          _precision;
        ts::Time                  _start_timer {};
        ts::EventId               _timer {};
        ts::Reactor               _reactor {CERR};
        ts::UDPSocket             _client {nullptr};
        ts::UDPSocket             _server {nullptr};
        ts::ReactiveUDPSocket     _rclient {_reactor, _client};
        ts::ReactiveUDPSocket     _rserver {_reactor, _server};
        const ts::IPSocketAddress _server_address {ts::IPAddress::LocalHost4, 19654};

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

        virtual void handleTimer(ts::Reactor& reactor, ts::EventId id) override;
        virtual void handleUDPSend(ts::ReactiveUDPSocket& sock, const void* data, size_t size, const ts::IPSocketAddress& destination, int error_code) override;
        virtual void handleUDPReceive(ts::ReactiveUDPSocket& sock,
                                      const ts::ByteBlockPtr& data,
                                      const ts::IPSocketAddress& sender,
                                      const ts::IPSocketAddress& destination,
                                      cn::microseconds timestamp,
                                      ts::UDPSocket::TimeStampType timestamp_type,
                                      int error_code) override;
    };

    HandlerUDP::HandlerUDP(std::ostream& debug, cn::milliseconds precision) :
        _debug(debug),
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

        TSUNIT_ASSERT(_rserver.startReceive(this));
        TSUNIT_ASSERT(_rclient.startReceive(this));
        TSUNIT_ASSERT(_rclient.startSend(this, &_request1, sizeof(_request1)));

        TSUNIT_ASSERT(_reactor.processEventLoop());

        TSUNIT_ASSERT(_client.close());
        TSUNIT_ASSERT(_server.close());
        TSUNIT_ASSERT(_reactor.close());
    }

    void HandlerUDP::handleTimer(ts::Reactor& reactor, ts::EventId id)
    {
        _debug << "HandlerUDP::handleTimer: id: " << id.toString() << std::endl;
        TSUNIT_ASSERT(id == _timer);
        TSUNIT_ASSERT(ts::Time::CurrentUTC() >= _start_timer + cn::milliseconds(100) - _precision);
        TSUNIT_ASSERT(_rclient.startSend(this, &_request2, sizeof(_request2)));
    }

    void HandlerUDP::handleUDPSend(ts::ReactiveUDPSocket& sock, const void* data, size_t size, const ts::IPSocketAddress& destination, int error_code)
    {
        TSUNIT_EQUAL(sizeof(uint32_t), size);
        _debug << "HandlerUDP::handleUDPSend: sent: " << name(data) << ", destination: " << destination << ", error: " << error_code << std::endl;
    }

    void HandlerUDP::handleUDPReceive(ts::ReactiveUDPSocket& sock,
                                      const ts::ByteBlockPtr& data,
                                      const ts::IPSocketAddress& sender,
                                      const ts::IPSocketAddress& destination,
                                      cn::microseconds timestamp,
                                      ts::UDPSocket::TimeStampType timestamp_type,
                                      int error_code)
    {
        TSUNIT_EQUAL(0, error_code);
        TSUNIT_ASSERT(data != nullptr);
        TSUNIT_EQUAL(sizeof(uint32_t), data->size());
        const uint32_t value = *reinterpret_cast<const uint32_t*>(data->data());

        _debug << "HandlerUDP::handleUDPReceive: value: " << name(value) << ", error: " << error_code
               << ", sender: " << sender << ", destination: " << destination << std::endl;

        if (value == _request1) {
            TSUNIT_ASSERT(!_timer.isValid());
            TSUNIT_ASSERT(_rserver.startSend(this, &_response1, sizeof(_response1), sender));
        }
        else if (value == _response1) {
            TSUNIT_ASSERT(!_timer.isValid());
            _start_timer = ts::Time::CurrentUTC();
            _timer = _reactor.newTimer(this, cn::milliseconds(100), false);
            TSUNIT_ASSERT(_timer.isValid());
        }
        else if (value == _request2) {
            TSUNIT_ASSERT(_timer.isValid());
            TSUNIT_ASSERT(_rserver.startSend(this, &_response2, sizeof(_response2), sender));
        }
        else if (value == _response2) {
            TSUNIT_ASSERT(_timer.isValid());
            sock.reactor().exitEventLoop();
        }
        else {
            TSUNIT_ASSERT(false);
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
}

TSUNIT_DEFINE_TEST(UDP)
{
    TSUNIT_ASSERT(ts::IPInitialize());
    HandlerUDP test(debug(), _precision);
}
