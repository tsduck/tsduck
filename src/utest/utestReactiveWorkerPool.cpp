//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::ReactiveWorkerPool
//
//----------------------------------------------------------------------------

#include "tsReactiveWorkerPool.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ReactiveWorkerPoolTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(Worker);
};

TSUNIT_REGISTER(ReactiveWorkerPoolTest);


//----------------------------------------------------------------------------
// Unitary tests
//----------------------------------------------------------------------------

namespace {

    class Value : public ts::Object
    {
    public:
        int value;
        Value() = delete;
        Value(int v) : value(v) {}
    };

    class Actor : public ts::ReactiveWorkerInterface, private ts::ReactiveWorkerHandlerInterface
    {
    public:
        Actor() = delete;
        Actor(ts::ReactiveWorkerPool& wpool, std::ostream& debug);
        void start();
        virtual int executeWork(ts::ReactiveWorkerPool& pool, const ts::ObjectPtr& user_data) override;
        virtual void handleWorkerCompletion(ts::ReactiveWorkerPool& pool, int error_code, const ts::ObjectPtr& user_data) override;

        std::mutex mutex {};
        std::set<int> work_values;
        std::set<int> completion_values;

    private:
        ts::ReactiveWorkerPool& _wpool;
        std::ostream& _debug;
    };

    Actor::Actor(ts::ReactiveWorkerPool& wpool, std::ostream& debug) :
        _wpool(wpool),
        _debug(debug)
    {
    }

    void Actor::start()
    {
        // Trigger initial work.
        TSUNIT_ASSERT(_wpool.startWork(this, this, std::make_shared<Value>(0)));
        _wpool.reactor().addExitReference();
    }

    int Actor::executeWork(ts::ReactiveWorkerPool& pool, const ts::ObjectPtr& user_data)
    {
        // Executed in a worker thread
        auto data = std::dynamic_pointer_cast<Value>(user_data);
        TSUNIT_ASSERT(data != nullptr);
        {
            std::lock_guard<std::mutex> lock(mutex);
            TSUNIT_ASSERT(!work_values.contains(data->value));
            work_values.insert(data->value);
        }
        if (data->value > 0) {
            // First worker is immediate, others wait 100 ms.
            std::this_thread::sleep_for(cn::milliseconds(100));
        }
        return data->value;
    }

    void Actor::handleWorkerCompletion(ts::ReactiveWorkerPool& pool, int error_code, const ts::ObjectPtr& user_data)
    {
        auto data = std::dynamic_pointer_cast<Value>(user_data);
        TSUNIT_ASSERT(data != nullptr);
        _debug << "Actor::handleWorkerCompletion: error_code: " << error_code << ", data: " << data->value
               << ", threads: " << _wpool.currentThreads() << ", busy: " << _wpool.currentBusyThreads() << std::endl;
        TSUNIT_EQUAL(error_code, data->value);
        {
            std::lock_guard<std::mutex> lock(mutex);
            TSUNIT_ASSERT(!completion_values.contains(data->value));
            completion_values.insert(data->value);
        }
        if (data->value == 0) {
            // End of first work, create several others.
            TSUNIT_ASSERT(_wpool.startWork(this, this, std::make_shared<Value>(1)));
            _wpool.reactor().addExitReference();
            TSUNIT_ASSERT(_wpool.startWork(this, this, std::make_shared<Value>(2)));
            _wpool.reactor().addExitReference();
            std::this_thread::sleep_for(cn::milliseconds(20));
            TSUNIT_ASSUME(_wpool.currentThreads() == 2);
            TSUNIT_ASSUME(_wpool.currentBusyThreads() == 2);
            for (int i = 3; i < 12; ++i) {
                TSUNIT_ASSERT(_wpool.startWork(this, this, std::make_shared<Value>(i)));
                _wpool.reactor().addExitReference();
            }
        }
        _wpool.reactor().freeExitReference();
    }
}

TSUNIT_DEFINE_TEST(Worker)
{
    ts::Reactor reactor(&CERR);
    ts::ReactiveWorkerPool wpool(reactor);
    Actor actor(wpool, debug());

    TSUNIT_ASSERT(reactor.open());

    wpool.setMaxThreads(4);
    TSUNIT_EQUAL(4, wpool.maxThreads());
    TSUNIT_EQUAL(0, wpool.currentThreads());
    TSUNIT_EQUAL(0, wpool.currentBusyThreads());

    actor.start();
    TSUNIT_ASSERT(reactor.processEventLoop());

    wpool.waitForTermination();
    TSUNIT_ASSERT(reactor.close());

    TSUNIT_EQUAL(12, actor.work_values.size());
    TSUNIT_EQUAL(12, actor.completion_values.size());
}
