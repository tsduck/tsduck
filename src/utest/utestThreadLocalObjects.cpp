//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::ThreadLocalObjects
//
//----------------------------------------------------------------------------

#include "tsThreadLocalObjects.h"
#include "tsThread.h"
#include "tsMutex.h"
#include "tsCondition.h"
#include "tsGuardMutex.h"
#include "tsGuardCondition.h"
#include "utestTSUnitThread.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ThreadLocalObjectsTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testLocalObjects();

    TSUNIT_TEST_BEGIN(ThreadLocalObjectsTest);
    TSUNIT_TEST(testLocalObjects);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(ThreadLocalObjectsTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void ThreadLocalObjectsTest::beforeTest()
{
}

// Test suite cleanup method.
void ThreadLocalObjectsTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

namespace {
    // A thread-local object class. Automatically manage the number of instances.
    class LocalObject : public ts::Object
    {
    private:
        static std::atomic_size_t _count;
    public:
        int data;
        LocalObject(int d = 0) : data(d)
        {
            _count++;
        }
        virtual ~LocalObject() override
        {
            _count--;
        }
        static size_t Count()
        {
            return _count;
        }
    };

    std::atomic_size_t LocalObject::_count(0);

    // A test thread which manages local objects.
    class TestThread: public utest::TSUnitThread
    {
    private:
        int _data;
        bool _ready;
        bool _terminate;
        ts::Mutex _mutex;
        ts::Condition _triggerReady;
        ts::Condition _triggerTerminate;
    public:
        TestThread(int data) :
            utest::TSUnitThread(),
            _data(data),
            _ready(false),
            _terminate(false),
            _mutex(),
            _triggerReady(),
            _triggerTerminate()
        {
        }
        virtual ~TestThread() override
        {
            waitForTermination();
        }
        void waitUntilReady()
        {
            ts::GuardCondition lock(_mutex, _triggerReady);
            while (!_ready) {
                lock.waitCondition();
            }
        }
        void terminate()
        {
            ts::GuardCondition lock(_mutex, _triggerTerminate);
            _terminate = true;
            lock.signal();
        }
        virtual void test() override
        {
            ts::ObjectPtr obj;
            LocalObject* lobj = nullptr;

            obj = ts::ThreadLocalObjects::Instance()->getLocalObject(u"A");
            TSUNIT_ASSERT(obj.isNull());

            obj = ts::ThreadLocalObjects::Instance()->getLocalObject(u"B");
            TSUNIT_ASSERT(obj.isNull());

            ts::ThreadLocalObjects::Instance()->setLocalObject(u"A", new LocalObject(_data + 1));
            ts::ThreadLocalObjects::Instance()->setLocalObject(u"B", new LocalObject(_data + 2));

            obj = ts::ThreadLocalObjects::Instance()->getLocalObject(u"A");
            TSUNIT_ASSERT(!obj.isNull());
            lobj = dynamic_cast<LocalObject*>(obj.pointer());
            TSUNIT_ASSERT(lobj != nullptr);
            TSUNIT_EQUAL(_data + 1, lobj->data);

            obj = ts::ThreadLocalObjects::Instance()->getLocalObject(u"B");
            TSUNIT_ASSERT(!obj.isNull());
            lobj = dynamic_cast<LocalObject*>(obj.pointer());
            TSUNIT_ASSERT(lobj != nullptr);
            TSUNIT_EQUAL(_data + 2, lobj->data);

            // Report ready and wait for termination request.
            {
                ts::GuardCondition lock(_mutex, _triggerReady);
                _ready = true;
                lock.signal();
            }
            {
                ts::GuardCondition lock(_mutex, _triggerTerminate);
                while (!_terminate) {
                    lock.waitCondition();
                }
            }
        }
    };
}

void ThreadLocalObjectsTest::testLocalObjects()
{
    TSUNIT_EQUAL(0, LocalObject::Count());
    {
        TestThread t1(100);
        TestThread t2(200);

        TSUNIT_ASSERT(t1.start());
        TSUNIT_ASSERT(t2.start());

        t1.waitUntilReady();
        t2.waitUntilReady();

        TSUNIT_EQUAL(4, LocalObject::Count());

        t1.terminate();
        t2.terminate();
    }
    TSUNIT_EQUAL(0, LocalObject::Count());
}
