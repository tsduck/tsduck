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
//  TSUnit test suite for class ts::SafePtr (safe pointer)
//
//----------------------------------------------------------------------------

#include "tsSafePtr.h"
#include "tsMutex.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class SafePtrTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testSafePtr();
    void testDowncast();
    void testUpcast();
    void testChangeMutex();

    TSUNIT_TEST_BEGIN(SafePtrTest);
    TSUNIT_TEST(testSafePtr);
    TSUNIT_TEST(testDowncast);
    TSUNIT_TEST(testUpcast);
    TSUNIT_TEST(testChangeMutex);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(SafePtrTest);


//----------------------------------------------------------------------------
// A class which identifies each instance by an explicit value.
// Also count the number of instances in the class.
//----------------------------------------------------------------------------

namespace {
    class TestData
    {
    private:
        TestData() = delete;
        int _value;
        static int _instanceCount;
    public:
        // Constructor
        explicit TestData(int value) : _value(value) {_instanceCount++;}
        explicit TestData(const TestData& other) : _value(other._value) {_instanceCount++;}

        // Assignment is different from copy constructor, do not increment instance count.
        TestData& operator=(const TestData& other) {_value = other._value; return *this;}

        // Destructor
        virtual ~TestData() {_instanceCount--;}

        // Get the object's value
        int value() const {return _value;}

        // Get the number of instances
        static int InstanceCount() {return _instanceCount;}
    };

    int TestData::_instanceCount = 0;

    typedef ts::SafePtr<TestData> TestDataPtr;
}


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void SafePtrTest::beforeTest()
{
}

// Test suite cleanup method.
void SafePtrTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

// Test case: check various object and pointer movements
void SafePtrTest::testSafePtr()
{
    TestDataPtr p1;

    TSUNIT_ASSERT(p1.isNull() == true);
    TSUNIT_ASSERT(p1.count() == 1);
    TSUNIT_ASSERT(TestData::InstanceCount() == 0);

    p1.reset (new TestData (12));

    TSUNIT_ASSERT(p1.isNull() == false);
    TSUNIT_ASSERT(p1.count() == 1);
    TSUNIT_ASSERT((*p1).value() == 12);
    TSUNIT_ASSERT(p1->value() == 12);
    TSUNIT_ASSERT(p1.pointer()->value() == 12);
    TSUNIT_ASSERT(TestData::InstanceCount() == 1);

    TestDataPtr p2 (p1);

    TSUNIT_ASSERT(p1.count() == 2);
    TSUNIT_ASSERT(p2.count() == 2);
    TSUNIT_ASSERT(p1.isNull() == false);
    TSUNIT_ASSERT(p2.isNull() == false);
    TSUNIT_ASSERT(p1->value() == 12);
    TSUNIT_ASSERT(p2->value() == 12);
    TSUNIT_ASSERT(TestData::InstanceCount() == 1);

    {
        TestDataPtr p3 (p2);

        TSUNIT_ASSERT(p1.count() == 3);
        TSUNIT_ASSERT(p2.count() == 3);
        TSUNIT_ASSERT(p3.count() == 3);
        TSUNIT_ASSERT(p1.isNull() == false);
        TSUNIT_ASSERT(p2.isNull() == false);
        TSUNIT_ASSERT(p3.isNull() == false);
        TSUNIT_ASSERT(p1->value() == 12);
        TSUNIT_ASSERT(p2->value() == 12);
        TSUNIT_ASSERT(p3->value() == 12);
        TSUNIT_ASSERT(TestData::InstanceCount() == 1);
    }

    TSUNIT_ASSERT(p1.count() == 2);
    TSUNIT_ASSERT(p2.count() == 2);
    TSUNIT_ASSERT(p1.isNull() == false);
    TSUNIT_ASSERT(p2.isNull() == false);
    TSUNIT_ASSERT(p1->value() == 12);
    TSUNIT_ASSERT(p2->value() == 12);
    TSUNIT_ASSERT(TestData::InstanceCount() == 1);

    TestDataPtr p3;

    TSUNIT_ASSERT(p1.count() == 2);
    TSUNIT_ASSERT(p2.count() == 2);
    TSUNIT_ASSERT(p3.count() == 1);
    TSUNIT_ASSERT((p1 == p3) == false);
    TSUNIT_ASSERT((p1 != p3) == true);
    TSUNIT_ASSERT(p1.isNull() == false);
    TSUNIT_ASSERT(p2.isNull() == false);
    TSUNIT_ASSERT(p3.isNull() == true);
    TSUNIT_ASSERT(p1->value() == 12);
    TSUNIT_ASSERT(p2->value() == 12);
    TSUNIT_ASSERT(TestData::InstanceCount() == 1);

    p3 = p1;

    TSUNIT_ASSERT(p1.count() == 3);
    TSUNIT_ASSERT(p2.count() == 3);
    TSUNIT_ASSERT(p3.count() == 3);
    TSUNIT_ASSERT((p1 == p3) == true);
    TSUNIT_ASSERT((p1 != p3) == false);
    TSUNIT_ASSERT(p1.isNull() == false);
    TSUNIT_ASSERT(p2.isNull() == false);
    TSUNIT_ASSERT(p3.isNull() == false);
    TSUNIT_ASSERT(p1->value() == 12);
    TSUNIT_ASSERT(p2->value() == 12);
    TSUNIT_ASSERT(p3->value() == 12);
    TSUNIT_ASSERT(TestData::InstanceCount() == 1);

    {
        TestData* tmp = new TestData (27);
        TSUNIT_ASSERT(TestData::InstanceCount() == 2);
        p2.reset (tmp);
    }

    TSUNIT_ASSERT(p1.count() == 3);
    TSUNIT_ASSERT(p2.count() == 3);
    TSUNIT_ASSERT(p3.count() == 3);
    TSUNIT_ASSERT((p1 == p2) == true);
    TSUNIT_ASSERT((p1 == p3) == true);
    TSUNIT_ASSERT(p1.isNull() == false);
    TSUNIT_ASSERT(p2.isNull() == false);
    TSUNIT_ASSERT(p3.isNull() == false);
    TSUNIT_ASSERT(p1->value() == 27);
    TSUNIT_ASSERT(p2->value() == 27);
    TSUNIT_ASSERT(p3->value() == 27);
    TSUNIT_ASSERT(TestData::InstanceCount() == 1);

    p2 = new TestData (41);

    TSUNIT_ASSERT(p1.count() == 2);
    TSUNIT_ASSERT(p2.count() == 1);
    TSUNIT_ASSERT(p3.count() == 2);
    TSUNIT_ASSERT(p1.isNull() == false);
    TSUNIT_ASSERT(p2.isNull() == false);
    TSUNIT_ASSERT(p3.isNull() == false);
    TSUNIT_ASSERT(p1->value() == 27);
    TSUNIT_ASSERT(p2->value() == 41);
    TSUNIT_ASSERT(p3->value() == 27);
    TSUNIT_ASSERT(TestData::InstanceCount() == 2);

    {
        // Object pointed by p2 no longer managed but still valid
        TestData* px = p2.release();

        TSUNIT_ASSERT(p1.count() == 2);
        TSUNIT_ASSERT(p2.count() == 1);
        TSUNIT_ASSERT(p3.count() == 2);
        TSUNIT_ASSERT(p1.isNull() == false);
        TSUNIT_ASSERT(p2.isNull() == true);
        TSUNIT_ASSERT(p3.isNull() == false);
        TSUNIT_ASSERT(p1->value() == 27);
        TSUNIT_ASSERT(px->value() == 41);
        TSUNIT_ASSERT(p3->value() == 27);
        TSUNIT_ASSERT(TestData::InstanceCount() == 2);

        // Now explicitly deallocate object (was no longer managed)
        delete px;
        TSUNIT_ASSERT(TestData::InstanceCount() == 1);
    }

    p3 = new TestData(76);

    TSUNIT_ASSERT(p1.count() == 1);
    TSUNIT_ASSERT(p2.count() == 1);
    TSUNIT_ASSERT(p3.count() == 1);
    TSUNIT_ASSERT(p1.isNull() == false);
    TSUNIT_ASSERT(p2.isNull() == true);
    TSUNIT_ASSERT(p3.isNull() == false);
    TSUNIT_ASSERT(p1->value() == 27);
    TSUNIT_ASSERT(p3->value() == 76);
    TSUNIT_ASSERT(TestData::InstanceCount() == 2);

    {
        TestDataPtr p4 (p1);

        TSUNIT_ASSERT(p1.count() == 2);
        TSUNIT_ASSERT(p2.count() == 1);
        TSUNIT_ASSERT(p3.count() == 1);
        TSUNIT_ASSERT(p4.count() == 2);
        TSUNIT_ASSERT(p1.isNull() == false);
        TSUNIT_ASSERT(p2.isNull() == true);
        TSUNIT_ASSERT(p3.isNull() == false);
        TSUNIT_ASSERT(p4.isNull() == false);
        TSUNIT_ASSERT(p1->value() == 27);
        TSUNIT_ASSERT(p3->value() == 76);
        TSUNIT_ASSERT(p4->value() == 27);
        TSUNIT_ASSERT(TestData::InstanceCount() == 2);

        p1 = nullptr;

        TSUNIT_ASSERT(p1.count() == 1);
        TSUNIT_ASSERT(p2.count() == 1);
        TSUNIT_ASSERT(p3.count() == 1);
        TSUNIT_ASSERT(p4.count() == 1);
        TSUNIT_ASSERT(p1.isNull() == true);
        TSUNIT_ASSERT(p2.isNull() == true);
        TSUNIT_ASSERT(p3.isNull() == false);
        TSUNIT_ASSERT(p4.isNull() == false);
        TSUNIT_ASSERT(p3->value() == 76);
        TSUNIT_ASSERT(p4->value() == 27);
        TSUNIT_ASSERT(TestData::InstanceCount() == 2);

        p3 = nullptr;

        TSUNIT_ASSERT(p1.count() == 1);
        TSUNIT_ASSERT(p2.count() == 1);
        TSUNIT_ASSERT(p3.count() == 1);
        TSUNIT_ASSERT(p4.count() == 1);
        TSUNIT_ASSERT(p1.isNull() == true);
        TSUNIT_ASSERT(p2.isNull() == true);
        TSUNIT_ASSERT(p3.isNull() == true);
        TSUNIT_ASSERT(p4.isNull() == false);
        TSUNIT_ASSERT(p4->value() == 27);
        TSUNIT_ASSERT(TestData::InstanceCount() == 1);
    }

    TSUNIT_ASSERT(TestData::InstanceCount() == 0);
}

// Two subclasses to test downcasts.
namespace {
    class SubTestData1: public TestData
    {
    public:
        // Constructor
        explicit SubTestData1 (int value) : TestData (value) {}
    };

    class SubTestData2: public TestData
    {
    public:
        // Constructor
        explicit SubTestData2 (int value) : TestData (value) {}
    };

    typedef ts::SafePtr<SubTestData1> SubTestData1Ptr;
    typedef ts::SafePtr<SubTestData2> SubTestData2Ptr;
}

// Test case: check downcasts
void SafePtrTest::testDowncast()
{
    TSUNIT_ASSERT(TestData::InstanceCount() == 0);
    TestDataPtr p (new SubTestData2 (666));
    TSUNIT_ASSERT(TestData::InstanceCount() == 1);
    TSUNIT_ASSERT(!p.isNull());

    SubTestData1Ptr p1 (p.downcast<SubTestData1>());
    TSUNIT_ASSERT(p1.isNull());
    TSUNIT_ASSERT(!p.isNull());
    TSUNIT_ASSERT(TestData::InstanceCount() == 1);

    SubTestData2Ptr p2 (p.downcast<SubTestData2>());
    TSUNIT_ASSERT(!p2.isNull());
    TSUNIT_ASSERT(p.isNull());
    TSUNIT_ASSERT(TestData::InstanceCount() == 1);
    TSUNIT_ASSERT(p2->value() == 666);

    p2.clear();
    TSUNIT_ASSERT(TestData::InstanceCount() == 0);
}

// Test case: check upcasts
void SafePtrTest::testUpcast()
{
    TSUNIT_ASSERT(TestData::InstanceCount() == 0);
    SubTestData1Ptr p1 (new SubTestData1 (777));
    TSUNIT_ASSERT(TestData::InstanceCount() == 1);
    TSUNIT_ASSERT(!p1.isNull());

    TestDataPtr p (p1.upcast<TestData>());
    TSUNIT_ASSERT(!p.isNull());
    TSUNIT_ASSERT(p1.isNull());
    TSUNIT_ASSERT(TestData::InstanceCount() == 1);
    TSUNIT_ASSERT(p->value() == 777);

    p.clear();
    TSUNIT_ASSERT(TestData::InstanceCount() == 0);
}

// Test case: check mutex type change
void SafePtrTest::testChangeMutex()
{
    TSUNIT_ASSERT(TestData::InstanceCount() == 0);
    ts::SafePtr<TestData,ts::NullMutex> pn (new TestData (888));
    TSUNIT_ASSERT(TestData::InstanceCount() == 1);
    TSUNIT_ASSERT(!pn.isNull());

    ts::SafePtr<TestData,ts::Mutex> pt (pn.changeMutex<ts::Mutex>());
    TSUNIT_ASSERT(!pt.isNull());
    TSUNIT_ASSERT(pn.isNull());
    TSUNIT_ASSERT(TestData::InstanceCount() == 1);
    TSUNIT_ASSERT(pt->value() == 888);

    pt.clear();
    TSUNIT_ASSERT(TestData::InstanceCount() == 0);
}
