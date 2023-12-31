//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for boolean predicates.
//
//----------------------------------------------------------------------------

#include "tsBoolPredicate.h"
#include "tsunit.h"

// Here, we intentionally use integers as bool.
TS_MSC_NOWARNING(4800) // Implicit conversion from 'int' to bool. Possible information loss
TS_MSC_NOWARNING(4305) // 'argument': truncation from 'int' to 'bool'


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class BoolPredicateTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testIdentity();
    void testNot();
    void testAnd();
    void testOr();
    void testXor();

    TSUNIT_TEST_BEGIN(BoolPredicateTest);
    TSUNIT_TEST(testIdentity);
    TSUNIT_TEST(testNot);
    TSUNIT_TEST(testAnd);
    TSUNIT_TEST(testOr);
    TSUNIT_TEST(testXor);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(BoolPredicateTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void BoolPredicateTest::beforeTest()
{
}

// Test suite cleanup method.
void BoolPredicateTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void BoolPredicateTest::testIdentity()
{
    TSUNIT_EQUAL(true, ts::Identity(true));
    TSUNIT_EQUAL(false, ts::Identity(false));

    TSUNIT_EQUAL(true, ts::Identity(1));
    TSUNIT_EQUAL(false, ts::Identity(0));
    TSUNIT_EQUAL(true, ts::Identity(20));
    TSUNIT_EQUAL(true, ts::Identity(-12));
}

void BoolPredicateTest::testNot()
{
    TSUNIT_EQUAL(false, ts::Not(true));
    TSUNIT_EQUAL(true, ts::Not(false));

    TSUNIT_EQUAL(false, ts::Not(1));
    TSUNIT_EQUAL(true, ts::Not(0));
    TSUNIT_EQUAL(false, ts::Not(20));
    TSUNIT_EQUAL(false, ts::Not(-12));
}

void BoolPredicateTest::testAnd()
{
    TSUNIT_EQUAL(false, ts::And(false, false));
    TSUNIT_EQUAL(false, ts::And(false, true));
    TSUNIT_EQUAL(false, ts::And(true, false));
    TSUNIT_EQUAL(true, ts::And(true, true));

    TSUNIT_EQUAL(true, ts::And(12, -2));
    TSUNIT_EQUAL(false, ts::And(0, 33));
    TSUNIT_EQUAL(false, ts::And(0, 0));

    TSUNIT_EQUAL(false, ts::MultiAnd({}));
    TSUNIT_EQUAL(false, ts::MultiAnd({false}));
    TSUNIT_EQUAL(true, ts::MultiAnd({true}));
    TSUNIT_EQUAL(true, ts::MultiAnd({true, true, true}));
    TSUNIT_EQUAL(false, ts::MultiAnd({true, false, true}));
}

void BoolPredicateTest::testOr()
{
    TSUNIT_EQUAL(false, ts::Or(false, false));
    TSUNIT_EQUAL(true, ts::Or(false, true));
    TSUNIT_EQUAL(true, ts::Or(true, false));
    TSUNIT_EQUAL(true, ts::Or(true, true));

    TSUNIT_EQUAL(true, ts::Or(12, -2));
    TSUNIT_EQUAL(true, ts::Or(0, 33));
    TSUNIT_EQUAL(false, ts::Or(0, 0));

    TSUNIT_EQUAL(false, ts::MultiOr({}));
    TSUNIT_EQUAL(false, ts::MultiOr({false}));
    TSUNIT_EQUAL(true, ts::MultiOr({true}));
    TSUNIT_EQUAL(true, ts::MultiOr({true, true, true}));
    TSUNIT_EQUAL(true, ts::MultiOr({true, false, true}));
    TSUNIT_EQUAL(false, ts::MultiOr({false, false, false}));
}

void BoolPredicateTest::testXor()
{
    TSUNIT_EQUAL(false, ts::Xor(false, false));
    TSUNIT_EQUAL(true, ts::Xor(false, true));
    TSUNIT_EQUAL(true, ts::Xor(true, false));
    TSUNIT_EQUAL(false, ts::Xor(true, true));

    TSUNIT_EQUAL(false, ts::Xor(0, 0));
    TSUNIT_EQUAL(true, ts::Xor(0, 7));
    TSUNIT_EQUAL(true, ts::Xor(-12, 0));
    TSUNIT_EQUAL(false, ts::Xor(45, -23));
}
