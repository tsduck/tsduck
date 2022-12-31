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
//  TSUnit test suite for class ts::Variable
//
//----------------------------------------------------------------------------

#include "tsVariable.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class VariableTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testElementaryType();
    void testClass();
    [[noreturn]] void testUninitialized();

    TSUNIT_TEST_BEGIN(VariableTest);
    TSUNIT_TEST(testElementaryType);
    TSUNIT_TEST(testClass);
    TSUNIT_TEST_EXCEPTION(testUninitialized, ts::UninitializedVariable);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(VariableTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void VariableTest::beforeTest()
{
}

// Test suite cleanup method.
void VariableTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

// Test case: usage on elementary types.
void VariableTest::testElementaryType()
{
    typedef ts::Variable<int> IntVariable;

    IntVariable v1;
    TSUNIT_ASSERT(!v1.set());

    IntVariable v2(v1);
    TSUNIT_ASSERT(!v2.set());

    v2 = 1;
    TSUNIT_ASSERT(v2.set());
    TSUNIT_EQUAL(1, v2.value());

    IntVariable v3(v2);
    TSUNIT_ASSERT(v3.set());

    IntVariable v4(2);
    TSUNIT_ASSERT(v4.set());

    v4 = v1;
    TSUNIT_ASSERT(!v4.set());

    v4 = v2;
    TSUNIT_ASSERT(v4.set());

    v4.clear();
    TSUNIT_ASSERT(!v4.set());

    v4.clear();
    TSUNIT_ASSERT(!v4.set());

    v1 = 1;
    v2.clear();
    TSUNIT_ASSERT(v1.set());
    TSUNIT_ASSERT(!v2.set());
    TSUNIT_EQUAL(1, v1.value());
    TSUNIT_EQUAL(1, v1.value(2));
    TSUNIT_EQUAL(2, v2.value(2));

    v1 = 1;
    v2 = 1;
    v3 = 3;
    v4.clear();
    IntVariable v5;
    TSUNIT_ASSERT(v1.set());
    TSUNIT_ASSERT(v2.set());
    TSUNIT_ASSERT(v3.set());
    TSUNIT_ASSERT(!v4.set());
    TSUNIT_ASSERT(!v5.set());
    TSUNIT_ASSERT(v1 == v2);
    TSUNIT_ASSERT(v1 != v3);
    TSUNIT_ASSERT(v1 != v4);
    TSUNIT_ASSERT(v4 != v5);
    TSUNIT_ASSERT(v1 == 1);
    TSUNIT_ASSERT(v1 != 2);
    TSUNIT_ASSERT(v4 != 1);

    v1.clear();
    TSUNIT_ASSERT(!v1.set());
    TSUNIT_ASSERT(v1.setDefault(1));
    TSUNIT_ASSERT(v1.set());
    TSUNIT_EQUAL(1, v1.value());
    TSUNIT_ASSERT(!v1.setDefault(2));
    TSUNIT_ASSERT(v1.set());
    TSUNIT_EQUAL(1, v1.value());
}

// A class which identifies each instance by an explicit value.
// Also count the number of instances in the class.
namespace {
    class TestData
    {
    private:
        int _value;
        static int _instanceCount;
    public:
        // Constructors
        explicit TestData(int value = 0) : _value(value) {_instanceCount++;}
        TestData(TestData&& other) : _value(other._value) {_instanceCount++;}
        TestData(const TestData& other) : _value(other._value) {_instanceCount++;}
        // TestData& operator=(TestData&& other) {_value = other._value; return *this;}
        TestData& operator=(const TestData& other) {_value = other._value; return *this;}
        bool operator==(const TestData& other) const {return _value == other._value;}

        // Destructor
        ~TestData() {_instanceCount--;}

        // Get the object's value
        int v() const {return _value;}

        // Get the number of instances
        static int InstanceCount() {return _instanceCount;}
    };

    int TestData::_instanceCount = 0;

    typedef ts::Variable<TestData> TestVariable;

    TestVariable NewInstance(int value, int expectedCount)
    {
        TestVariable v = TestData(value);
        TSUNIT_EQUAL(expectedCount, TestData::InstanceCount());
        return v;
    }
}

// Test case: usage on class types.
void VariableTest::testClass()
{
    TSUNIT_EQUAL(0, TestData::InstanceCount());
    {
        TestVariable v1;
        TSUNIT_ASSERT(!v1.set());
        TSUNIT_EQUAL(0, TestData::InstanceCount());

        TestVariable v2(v1);
        TSUNIT_ASSERT(!v2.set());
        TSUNIT_EQUAL(0, TestData::InstanceCount());

        v2 = TestData(1);
        TSUNIT_ASSERT(v2.set());
        TSUNIT_EQUAL(1, v2.value().v());
        TSUNIT_EQUAL(1, TestData::InstanceCount());

        TestVariable v3(v2);
        TSUNIT_ASSERT(v3.set());
        TSUNIT_EQUAL(2, TestData::InstanceCount());

        TestVariable v4(TestData(2));
        TSUNIT_ASSERT(v4.set());
        TSUNIT_EQUAL(3, TestData::InstanceCount());

        v4 = v1;
        TSUNIT_ASSERT(!v4.set());
        TSUNIT_EQUAL(2, TestData::InstanceCount());

        v4 = v2;
        TSUNIT_ASSERT(v4.set());
        TSUNIT_EQUAL(3, TestData::InstanceCount());

        v4.clear();
        TSUNIT_ASSERT(!v4.set());
        TSUNIT_EQUAL(2, TestData::InstanceCount());

        v4.clear();
        TSUNIT_ASSERT(!v4.set());
        TSUNIT_EQUAL(2, TestData::InstanceCount());

        v1 = TestData(1);
        TSUNIT_EQUAL(3, TestData::InstanceCount());
        v2.clear();
        TSUNIT_EQUAL(2, TestData::InstanceCount());
        TSUNIT_ASSERT(v1.set());
        TSUNIT_ASSERT(!v2.set());
        TSUNIT_EQUAL(1, v1.value().v());
        TSUNIT_EQUAL(1, v1.value(TestData(2)).v());
        TSUNIT_EQUAL(2, v2.value(TestData(2)).v());
        TSUNIT_EQUAL(2, TestData::InstanceCount());

        v1 = TestData(1);
        TSUNIT_EQUAL(2, TestData::InstanceCount());
        v2 = TestData(1);
        TSUNIT_EQUAL(3, TestData::InstanceCount());
        v3 = TestData(3);
        TSUNIT_EQUAL(3, TestData::InstanceCount());
        v4.clear();
        TSUNIT_EQUAL(3, TestData::InstanceCount());
        TestVariable v5;
        TSUNIT_EQUAL(3, TestData::InstanceCount());
        TSUNIT_ASSERT(v1.set());
        TSUNIT_ASSERT(v2.set());
        TSUNIT_ASSERT(v3.set());
        TSUNIT_ASSERT(!v4.set());
        TSUNIT_ASSERT(!v5.set());
        TSUNIT_ASSERT(v1 == v2);
        TSUNIT_ASSERT(v1 != v3);
        TSUNIT_ASSERT(v1 != v4);
        TSUNIT_ASSERT(v4 != v5);
        TSUNIT_EQUAL(1, v1.value().v());
        TSUNIT_ASSERT(v1 == TestData(1));
        TSUNIT_ASSERT(v1 != TestData(2));
        TSUNIT_ASSERT(v4 != TestData(1));
        TSUNIT_EQUAL(3, TestData::InstanceCount());

        v5 = NewInstance(5, 4);
        TSUNIT_EQUAL(4, TestData::InstanceCount());
        TSUNIT_ASSERT(v5.set());
        TSUNIT_EQUAL(5, v5.value().v());
    }
    // Check that the destructor of variable properly destroys the contained object
    TSUNIT_EQUAL(0, TestData::InstanceCount());
}

// Test case: fail on uninitialized variable
void VariableTest::testUninitialized()
{
    ts::Variable<int> vi;
    TS_UNUSED int i = vi.value();
    TSUNIT_FAIL("variable is not initialized, should not get there");
}
