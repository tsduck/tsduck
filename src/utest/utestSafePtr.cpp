//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//  CppUnit test suite for class ts::SafePtr (safe pointer)
//
//----------------------------------------------------------------------------

#include "tsSafePtr.h"
#include "tsMutex.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class SafePtrTest: public CppUnit::TestFixture
{
public:
    virtual void setUp() override;
    virtual void tearDown() override;

    void testSafePtr();
    void testDowncast();
    void testUpcast();
    void testChangeMutex();

    CPPUNIT_TEST_SUITE (SafePtrTest);
    CPPUNIT_TEST (testSafePtr);
    CPPUNIT_TEST (testDowncast);
    CPPUNIT_TEST (testUpcast);
    CPPUNIT_TEST (testChangeMutex);
    CPPUNIT_TEST_SUITE_END ();
};

CPPUNIT_TEST_SUITE_REGISTRATION (SafePtrTest);


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
        TestData& operator=(const TestData& other) {if (&other != this) _value = other._value; return *this;}

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
void SafePtrTest::setUp()
{
}

// Test suite cleanup method.
void SafePtrTest::tearDown()
{
}


//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

// Test case: check various object and pointer movements
void SafePtrTest::testSafePtr()
{
    TestDataPtr p1;

    CPPUNIT_ASSERT(p1.isNull() == true);
    CPPUNIT_ASSERT(p1.count() == 1);
    CPPUNIT_ASSERT(TestData::InstanceCount() == 0);

    p1.reset (new TestData (12));

    CPPUNIT_ASSERT(p1.isNull() == false);
    CPPUNIT_ASSERT(p1.count() == 1);
    CPPUNIT_ASSERT((*p1).value() == 12);
    CPPUNIT_ASSERT(p1->value() == 12);
    CPPUNIT_ASSERT(p1.pointer()->value() == 12);
    CPPUNIT_ASSERT(TestData::InstanceCount() == 1);

    TestDataPtr p2 (p1);

    CPPUNIT_ASSERT(p1.count() == 2);
    CPPUNIT_ASSERT(p2.count() == 2);
    CPPUNIT_ASSERT(p1.isNull() == false);
    CPPUNIT_ASSERT(p2.isNull() == false);
    CPPUNIT_ASSERT(p1->value() == 12);
    CPPUNIT_ASSERT(p2->value() == 12);
    CPPUNIT_ASSERT(TestData::InstanceCount() == 1);

    {
        TestDataPtr p3 (p2);

        CPPUNIT_ASSERT(p1.count() == 3);
        CPPUNIT_ASSERT(p2.count() == 3);
        CPPUNIT_ASSERT(p3.count() == 3);
        CPPUNIT_ASSERT(p1.isNull() == false);
        CPPUNIT_ASSERT(p2.isNull() == false);
        CPPUNIT_ASSERT(p3.isNull() == false);
        CPPUNIT_ASSERT(p1->value() == 12);
        CPPUNIT_ASSERT(p2->value() == 12);
        CPPUNIT_ASSERT(p3->value() == 12);
        CPPUNIT_ASSERT(TestData::InstanceCount() == 1);
    }

    CPPUNIT_ASSERT(p1.count() == 2);
    CPPUNIT_ASSERT(p2.count() == 2);
    CPPUNIT_ASSERT(p1.isNull() == false);
    CPPUNIT_ASSERT(p2.isNull() == false);
    CPPUNIT_ASSERT(p1->value() == 12);
    CPPUNIT_ASSERT(p2->value() == 12);
    CPPUNIT_ASSERT(TestData::InstanceCount() == 1);

    TestDataPtr p3;

    CPPUNIT_ASSERT(p1.count() == 2);
    CPPUNIT_ASSERT(p2.count() == 2);
    CPPUNIT_ASSERT(p3.count() == 1);
    CPPUNIT_ASSERT((p1 == p3) == false);
    CPPUNIT_ASSERT((p1 != p3) == true);
    CPPUNIT_ASSERT(p1.isNull() == false);
    CPPUNIT_ASSERT(p2.isNull() == false);
    CPPUNIT_ASSERT(p3.isNull() == true);
    CPPUNIT_ASSERT(p1->value() == 12);
    CPPUNIT_ASSERT(p2->value() == 12);
    CPPUNIT_ASSERT(TestData::InstanceCount() == 1);

    p3 = p1;

    CPPUNIT_ASSERT(p1.count() == 3);
    CPPUNIT_ASSERT(p2.count() == 3);
    CPPUNIT_ASSERT(p3.count() == 3);
    CPPUNIT_ASSERT((p1 == p3) == true);
    CPPUNIT_ASSERT((p1 != p3) == false);
    CPPUNIT_ASSERT(p1.isNull() == false);
    CPPUNIT_ASSERT(p2.isNull() == false);
    CPPUNIT_ASSERT(p3.isNull() == false);
    CPPUNIT_ASSERT(p1->value() == 12);
    CPPUNIT_ASSERT(p2->value() == 12);
    CPPUNIT_ASSERT(p3->value() == 12);
    CPPUNIT_ASSERT(TestData::InstanceCount() == 1);

    {
        TestData* tmp = new TestData (27);
        CPPUNIT_ASSERT(TestData::InstanceCount() == 2);
        p2.reset (tmp);
    }

    CPPUNIT_ASSERT(p1.count() == 3);
    CPPUNIT_ASSERT(p2.count() == 3);
    CPPUNIT_ASSERT(p3.count() == 3);
    CPPUNIT_ASSERT((p1 == p2) == true);
    CPPUNIT_ASSERT((p1 == p3) == true);
    CPPUNIT_ASSERT(p1.isNull() == false);
    CPPUNIT_ASSERT(p2.isNull() == false);
    CPPUNIT_ASSERT(p3.isNull() == false);
    CPPUNIT_ASSERT(p1->value() == 27);
    CPPUNIT_ASSERT(p2->value() == 27);
    CPPUNIT_ASSERT(p3->value() == 27);
    CPPUNIT_ASSERT(TestData::InstanceCount() == 1);

    p2 = new TestData (41);

    CPPUNIT_ASSERT(p1.count() == 2);
    CPPUNIT_ASSERT(p2.count() == 1);
    CPPUNIT_ASSERT(p3.count() == 2);
    CPPUNIT_ASSERT(p1.isNull() == false);
    CPPUNIT_ASSERT(p2.isNull() == false);
    CPPUNIT_ASSERT(p3.isNull() == false);
    CPPUNIT_ASSERT(p1->value() == 27);
    CPPUNIT_ASSERT(p2->value() == 41);
    CPPUNIT_ASSERT(p3->value() == 27);
    CPPUNIT_ASSERT(TestData::InstanceCount() == 2);

    {
        // Object pointed by p2 no longer managed but still valid
        TestData* px = p2.release ();

        CPPUNIT_ASSERT(p1.count() == 2);
        CPPUNIT_ASSERT(p2.count() == 1);
        CPPUNIT_ASSERT(p3.count() == 2);
        CPPUNIT_ASSERT(p1.isNull() == false);
        CPPUNIT_ASSERT(p2.isNull() == true);
        CPPUNIT_ASSERT(p3.isNull() == false);
        CPPUNIT_ASSERT(p1->value() == 27);
        CPPUNIT_ASSERT(px->value() == 41);
        CPPUNIT_ASSERT(p3->value() == 27);
        CPPUNIT_ASSERT(TestData::InstanceCount() == 2);

        // Now explicitly deallocate object (was no longer managed)
        delete px;
        CPPUNIT_ASSERT(TestData::InstanceCount() == 1);
    }

    p3 = new TestData(76);

    CPPUNIT_ASSERT(p1.count() == 1);
    CPPUNIT_ASSERT(p2.count() == 1);
    CPPUNIT_ASSERT(p3.count() == 1);
    CPPUNIT_ASSERT(p1.isNull() == false);
    CPPUNIT_ASSERT(p2.isNull() == true);
    CPPUNIT_ASSERT(p3.isNull() == false);
    CPPUNIT_ASSERT(p1->value() == 27);
    CPPUNIT_ASSERT(p3->value() == 76);
    CPPUNIT_ASSERT(TestData::InstanceCount() == 2);

    {
        TestDataPtr p4 (p1);

        CPPUNIT_ASSERT(p1.count() == 2);
        CPPUNIT_ASSERT(p2.count() == 1);
        CPPUNIT_ASSERT(p3.count() == 1);
        CPPUNIT_ASSERT(p4.count() == 2);
        CPPUNIT_ASSERT(p1.isNull() == false);
        CPPUNIT_ASSERT(p2.isNull() == true);
        CPPUNIT_ASSERT(p3.isNull() == false);
        CPPUNIT_ASSERT(p4.isNull() == false);
        CPPUNIT_ASSERT(p1->value() == 27);
        CPPUNIT_ASSERT(p3->value() == 76);
        CPPUNIT_ASSERT(p4->value() == 27);
        CPPUNIT_ASSERT(TestData::InstanceCount() == 2);

        p1 = nullptr;

        CPPUNIT_ASSERT(p1.count() == 1);
        CPPUNIT_ASSERT(p2.count() == 1);
        CPPUNIT_ASSERT(p3.count() == 1);
        CPPUNIT_ASSERT(p4.count() == 1);
        CPPUNIT_ASSERT(p1.isNull() == true);
        CPPUNIT_ASSERT(p2.isNull() == true);
        CPPUNIT_ASSERT(p3.isNull() == false);
        CPPUNIT_ASSERT(p4.isNull() == false);
        CPPUNIT_ASSERT(p3->value() == 76);
        CPPUNIT_ASSERT(p4->value() == 27);
        CPPUNIT_ASSERT(TestData::InstanceCount() == 2);

        p3 = nullptr;

        CPPUNIT_ASSERT(p1.count() == 1);
        CPPUNIT_ASSERT(p2.count() == 1);
        CPPUNIT_ASSERT(p3.count() == 1);
        CPPUNIT_ASSERT(p4.count() == 1);
        CPPUNIT_ASSERT(p1.isNull() == true);
        CPPUNIT_ASSERT(p2.isNull() == true);
        CPPUNIT_ASSERT(p3.isNull() == true);
        CPPUNIT_ASSERT(p4.isNull() == false);
        CPPUNIT_ASSERT(p4->value() == 27);
        CPPUNIT_ASSERT(TestData::InstanceCount() == 1);
    }

    CPPUNIT_ASSERT(TestData::InstanceCount() == 0);
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
    CPPUNIT_ASSERT(TestData::InstanceCount() == 0);
    TestDataPtr p (new SubTestData2 (666));
    CPPUNIT_ASSERT(TestData::InstanceCount() == 1);
    CPPUNIT_ASSERT(!p.isNull());

    SubTestData1Ptr p1 (p.downcast<SubTestData1>());
    CPPUNIT_ASSERT(p1.isNull());
    CPPUNIT_ASSERT(!p.isNull());
    CPPUNIT_ASSERT(TestData::InstanceCount() == 1);

    SubTestData2Ptr p2 (p.downcast<SubTestData2>());
    CPPUNIT_ASSERT(!p2.isNull());
    CPPUNIT_ASSERT(p.isNull());
    CPPUNIT_ASSERT(TestData::InstanceCount() == 1);
    CPPUNIT_ASSERT(p2->value() == 666);

    p2.clear();
    CPPUNIT_ASSERT(TestData::InstanceCount() == 0);
}

// Test case: check upcasts
void SafePtrTest::testUpcast()
{
    CPPUNIT_ASSERT(TestData::InstanceCount() == 0);
    SubTestData1Ptr p1 (new SubTestData1 (777));
    CPPUNIT_ASSERT(TestData::InstanceCount() == 1);
    CPPUNIT_ASSERT(!p1.isNull());

    TestDataPtr p (p1.upcast<TestData>());
    CPPUNIT_ASSERT(!p.isNull());
    CPPUNIT_ASSERT(p1.isNull());
    CPPUNIT_ASSERT(TestData::InstanceCount() == 1);
    CPPUNIT_ASSERT(p->value() == 777);

    p.clear();
    CPPUNIT_ASSERT(TestData::InstanceCount() == 0);
}

// Test case: check mutex type change
void SafePtrTest::testChangeMutex()
{
    CPPUNIT_ASSERT(TestData::InstanceCount() == 0);
    ts::SafePtr<TestData,ts::NullMutex> pn (new TestData (888));
    CPPUNIT_ASSERT(TestData::InstanceCount() == 1);
    CPPUNIT_ASSERT(!pn.isNull());

    ts::SafePtr<TestData,ts::Mutex> pt (pn.changeMutex<ts::Mutex>());
    CPPUNIT_ASSERT(!pt.isNull());
    CPPUNIT_ASSERT(pn.isNull());
    CPPUNIT_ASSERT(TestData::InstanceCount() == 1);
    CPPUNIT_ASSERT(pt->value() == 888);

    pt.clear();
    CPPUNIT_ASSERT(TestData::InstanceCount() == 0);
}
