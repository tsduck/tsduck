//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class std::optional
//
//----------------------------------------------------------------------------

#include "tsOptional.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class OptionalTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testElementaryType();
    void testClass();
    [[noreturn]] void testUninitialized();

    TSUNIT_TEST_BEGIN(OptionalTest);
    TSUNIT_TEST(testElementaryType);
    TSUNIT_TEST(testClass);
    TSUNIT_TEST_EXCEPTION(testUninitialized, std::bad_optional_access);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(OptionalTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void OptionalTest::beforeTest()
{
}

// Test suite cleanup method.
void OptionalTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

// Test case: usage on elementary types.
void OptionalTest::testElementaryType()
{
#if defined(TS_OPTIONAL_IMPLEMENTED)
    debug() << "OptionalTest: std::optional uses TSDuck pre-C++17 implementation" << std::endl;
#else
    debug() << "OptionalTest: std::optional uses C++17 standard implementation" << std::endl;
#endif

    std::optional<int> v1;
    TSUNIT_ASSERT(!v1.has_value());
    TSUNIT_ASSERT(!bool(v1));

    std::optional<int> v2(v1);
    TSUNIT_ASSERT(!v2.has_value());
    TSUNIT_ASSERT(!bool(v2));

    v2 = 1;
    TSUNIT_ASSERT(v2.has_value());
    TSUNIT_ASSERT(bool(v2));
    TSUNIT_EQUAL(1, v2.value());

    std::optional<int> v3(v2);
    TSUNIT_ASSERT(v3.has_value());
    TSUNIT_ASSERT(bool(v3));

    std::optional<int> v4(2);
    TSUNIT_ASSERT(v4.has_value());
    TSUNIT_ASSERT(bool(v4));

    v4 = v1;
    TSUNIT_ASSERT(!v4.has_value());
    TSUNIT_ASSERT(!bool(v4));

    v4 = v2;
    TSUNIT_ASSERT(v4.has_value());

    v4.reset();
    TSUNIT_ASSERT(!v4.has_value());

    v4.reset();
    TSUNIT_ASSERT(!v4.has_value());

    v1 = 1;
    v2.reset();
    TSUNIT_ASSERT(v1.has_value());
    TSUNIT_ASSERT(!v2.has_value());
    TSUNIT_EQUAL(1, v1.value());
    TSUNIT_EQUAL(1, v1.value_or(2));
    TSUNIT_EQUAL(2, v2.value_or(2));

    v1 = 1;
    v2 = 1;
    v3 = 3;
    v4.reset();
    std::optional<int> v5;
    TSUNIT_ASSERT(v1.has_value());
    TSUNIT_ASSERT(v2.has_value());
    TSUNIT_ASSERT(v3.has_value());
    TSUNIT_ASSERT(!v4.has_value());
    TSUNIT_ASSERT(!v5.has_value());
    TSUNIT_ASSERT(v1 == v2);
    TSUNIT_ASSERT(v1 != v3);
    TSUNIT_ASSERT(v1 != v4);
    TSUNIT_ASSERT(v4 == v5);
    TSUNIT_ASSERT(v1 == 1);
    TSUNIT_ASSERT(v1 != 2);
    TSUNIT_ASSERT(v4 != 1);
}

// A class which identifies each instance by an explicit value.
// Also count the numbstd::optionaler of instances in the class.
namespace {

    class TestData;
    std::ostream& operator<<(std::ostream& stm, const TestData& data);

    class TestData
    {
    private:
        int _value = 0;
        bool _moved = false;
        static int _instanceCount;
        void trace(const char* name, const TestData* other = nullptr)
        {
            tsunit::Test::debug() << "TestData " << *this << ", " << name;
            if (other != nullptr) {
                tsunit::Test::debug() << " from " << *other;
            }
            tsunit::Test::debug() << ", instances: " << _instanceCount << std::endl;
        }
        void move()
        {
            if (_moved) {
                trace("moved twice");
            }
            TSUNIT_ASSERT(!_moved);
            _moved = true;
            _instanceCount--;
        }
    public:
        static int InstanceCount() { return _instanceCount; }
        int v() const { return _value; }
        explicit TestData(int value = 0) : _value(value)
        {
            _instanceCount++;
            trace("default constructor");
        }
        TestData(TestData&& other) : _value(std::move(other._value))
        {
            _instanceCount++;
            trace("move constructor", &other);
            other.move();
        }
        TestData(const TestData& other) : _value(other._value)
        {
            _instanceCount++;
            trace("copy constructor", &other);
        }
        TestData& operator=(TestData&& other)
        {
            _value = std::move(other._value);
            trace("move assignment", &other);
            other.move();
            return *this;
        }
        TestData& operator=(const TestData& other)
        {
            _value = other._value;
            trace("copy assignment", &other);
            return *this;
        }
        ~TestData()
        {
            if (_moved) {
                trace("destructor (moved object)");
            }
            else {
                _instanceCount--;
                trace("destructor");
            }
        }
        bool operator==(const TestData& other) const
        {
            return _value == other._value;
        }
        TS_UNEQUAL_OPERATOR(TestData)
        std::ostream& display(std::ostream& stm) const
        {
            return stm << "@" << std::hex << (size_t(this) & 0xFFFFFFFF) << std::dec << " (" << _value << ")";
        }
    };

    std::ostream& operator<<(std::ostream& stm, const TestData& data)
    {
        return data.display(stm);
    }

    int TestData::_instanceCount = 0;

    typedef std::optional<TestData> TestVariable;

    TestVariable NewInstance(int value, int expectedCount)
    {
        TestVariable v = TestData(value);
        TSUNIT_EQUAL(expectedCount, TestData::InstanceCount());
        TSUNIT_ASSERT(v.has_value());
        TSUNIT_EQUAL(value, v->v());
        tsunit::Test::debug() << "TestData: in NewInstance before return, v " << *v << ", instances: " << TestData::InstanceCount() << std::endl;
        return v;
    }
}

// Test case: usage on class types.
void OptionalTest::testClass()
{
    TSUNIT_EQUAL(0, TestData::InstanceCount());
    {
        TestVariable v1;
        TSUNIT_ASSERT(!v1.has_value());
        TSUNIT_EQUAL(0, TestData::InstanceCount());

        TestVariable v2(v1);
        TSUNIT_ASSERT(!v2.has_value());
        TSUNIT_EQUAL(0, TestData::InstanceCount());

        v2 = TestData(1);
        TSUNIT_ASSERT(v2.has_value());
        TSUNIT_EQUAL(1, v2->v());
        TSUNIT_EQUAL(1, TestData::InstanceCount());

        TestVariable v3(v2);
        TSUNIT_ASSERT(v3.has_value());
        TSUNIT_EQUAL(2, TestData::InstanceCount());

        TestVariable v4(TestData(2));
        TSUNIT_ASSERT(v4.has_value());
        TSUNIT_EQUAL(3, TestData::InstanceCount());

        v4 = v1;
        TSUNIT_ASSERT(!v4.has_value());
        TSUNIT_EQUAL(2, TestData::InstanceCount());

        v4 = v2;
        TSUNIT_ASSERT(v4.has_value());
        TSUNIT_EQUAL(3, TestData::InstanceCount());

        v4.reset();
        TSUNIT_ASSERT(!v4.has_value());
        TSUNIT_EQUAL(2, TestData::InstanceCount());

        v4.reset();
        TSUNIT_ASSERT(!v4.has_value());
        TSUNIT_EQUAL(2, TestData::InstanceCount());

        v1 = TestData(1);
        TSUNIT_EQUAL(3, TestData::InstanceCount());
        v2.reset();
        TSUNIT_EQUAL(2, TestData::InstanceCount());
        TSUNIT_ASSERT(v1.has_value());
        TSUNIT_ASSERT(!v2.has_value());
        TSUNIT_EQUAL(1, v1->v());
        TSUNIT_EQUAL(1, v1.value_or(TestData(2)).v());
        TSUNIT_EQUAL(2, v2.value_or(TestData(2)).v());
        TSUNIT_EQUAL(2, TestData::InstanceCount());

        v1 = TestData(1);
        TSUNIT_EQUAL(2, TestData::InstanceCount());
        v2 = TestData(1);
        TSUNIT_EQUAL(3, TestData::InstanceCount());
        v3 = TestData(3);
        TSUNIT_EQUAL(3, TestData::InstanceCount());
        v4.reset();
        TSUNIT_EQUAL(3, TestData::InstanceCount());
        TestVariable v5;
        TSUNIT_EQUAL(3, TestData::InstanceCount());
        TSUNIT_ASSERT(v1.has_value());
        TSUNIT_ASSERT(v2.has_value());
        TSUNIT_ASSERT(v3.has_value());
        TSUNIT_ASSERT(!v4.has_value());
        TSUNIT_ASSERT(!v5.has_value());
        TSUNIT_ASSERT(v1 == v2);
        TSUNIT_ASSERT(v1 != v3);
        TSUNIT_ASSERT(v1 != v4);
        TSUNIT_ASSERT(v4 == v5);
        TSUNIT_EQUAL(1, v1->v());
        TSUNIT_ASSERT(v1 == TestData(1));
        TSUNIT_ASSERT(v1 != TestData(2));
        TSUNIT_ASSERT(v4 != TestData(1));
        TSUNIT_EQUAL(3, TestData::InstanceCount());

        debug() << "TestData: before NewInstance, instances: " << TestData::InstanceCount() << std::endl;
        v5 = NewInstance(5, 4);
        debug() << "TestData: after NewInstance, instances: " << TestData::InstanceCount() << std::endl;
        TSUNIT_EQUAL(4, TestData::InstanceCount());
        TSUNIT_ASSERT(v5.has_value());
    }
    // Check that the destructor of variable properly destroys the contained object
    TSUNIT_EQUAL(0, TestData::InstanceCount());
}

// Test case: fail on uninitialized variable
void OptionalTest::testUninitialized()
{
    std::optional<int> vi;
    TS_UNUSED int i = vi.value();
    TSUNIT_FAIL("variable is not initialized, should not get there");
}
