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
//  TSUnit test suite for class ts::RingNode
//
//----------------------------------------------------------------------------

#include "tsRingNode.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class RingTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testRingNode();
    void testSwap();

    TSUNIT_TEST_BEGIN(RingTest);
    TSUNIT_TEST(testRingNode);
    TSUNIT_TEST(testSwap);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(RingTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void RingTest::beforeTest()
{
}

// Test suite cleanup method.
void RingTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

namespace {
    class R: public ts::RingNode
    {
    public:
        int i;
        explicit R(int i_): i(i_) {}
    };
}

void RingTest::testRingNode()
{
    R r1(1);
    R r2(2);
    R r3(3);
    R r4(4);

    TSUNIT_ASSERT(r1.ringAlone());
    TSUNIT_ASSERT(r1.ringSize() == 1);
    TSUNIT_ASSERT(r1.ringNext<R>() == &r1);
    TSUNIT_ASSERT(r1.ringPrevious<R>() == &r1);

    r2.ringInsertAfter(&r1);
    r3.ringInsertAfter(&r2);
    r4.ringInsertAfter(&r3);

    TSUNIT_ASSERT(!r1.ringAlone());
    TSUNIT_ASSERT(!r4.ringAlone());
    TSUNIT_ASSERT(r1.ringSize() == 4);
    TSUNIT_ASSERT(r4.ringSize() == 4);
    TSUNIT_ASSERT(r1.ringNext<R>() == &r2);
    TSUNIT_ASSERT(r1.ringPrevious<R>() == &r4);

    r4.ringRemove();

    TSUNIT_ASSERT(!r1.ringAlone());
    TSUNIT_ASSERT(r4.ringAlone());
    TSUNIT_ASSERT(r1.ringSize() == 3);
    TSUNIT_ASSERT(r4.ringSize() == 1);
    TSUNIT_ASSERT(r1.ringNext<R>() == &r2);
    TSUNIT_ASSERT(r1.ringPrevious<R>() == &r3);
    TSUNIT_ASSERT(r4.ringNext<R>() == &r4);
    TSUNIT_ASSERT(r4.ringPrevious<R>() == &r4);

    {
        R r5(5);
        r5.ringInsertBefore(&r1);

        TSUNIT_ASSERT(r1.ringSize() == 4);
        TSUNIT_ASSERT(r5.ringSize() == 4);
        TSUNIT_ASSERT(r1.ringNext<R>() == &r2);
        TSUNIT_ASSERT(r1.ringPrevious<R>() == &r5);
    }

    // After this point, coverity is lost.
    // It does not realize that r5 removed itself from the r1-r3 ring in its detructor.

    TSUNIT_ASSERT(r1.ringSize() == 3);
    TSUNIT_ASSERT(r1.ringNext<R>() == &r2);
    TSUNIT_ASSERT(r1.ringPrevious<R>() == &r3);

    r2.ringRemove();

    TSUNIT_ASSERT(r1.ringSize() == 2);
    TSUNIT_ASSERT(r1.ringNext<R>() == &r3);
    TSUNIT_ASSERT(r1.ringPrevious<R>() == &r3);

    r3.ringRemove();

    TSUNIT_ASSERT(r1.ringSize() == 1);
    TSUNIT_ASSERT(r1.ringNext<R>() == &r1);
    TSUNIT_ASSERT(r1.ringPrevious<R>() == &r1);

    // Coverity false positive:
    //   CID 158192 (#1-2 of 2): Pointer to local outside scope (RETURN_LOCAL)
    //   34. use_invalid_in_call: In r1.<unnamed>::R::~R(), using r1->_ring_previous, which points to an out-of-scope variable r5.
    // This is incorrect: r5 was auto-removed by its destructor at end of its scope.
    // coverity[RETURN_LOCAL]
}

void RingTest::testSwap()
{
    R r1(1);
    R r2(2);
    R r3(3);
    R r4(4);
    R r5(5);
    R r6(6);

    // Build 3 rings: {r1}, {r2, r3}, {r4, r5, r6]
    r3.ringInsertAfter(&r2);
    r5.ringInsertAfter(&r4);
    r6.ringInsertAfter(&r5);

    TSUNIT_EQUAL(1, r1.ringSize());
    TSUNIT_EQUAL(2, r2.ringSize());
    TSUNIT_EQUAL(2, r3.ringSize());
    TSUNIT_EQUAL(3, r4.ringSize());
    TSUNIT_EQUAL(3, r5.ringSize());
    TSUNIT_EQUAL(3, r6.ringSize());

    // Swap r1 and r4: {r4}, {r2, r3}, {r1, r5, r6]
    r1.ringSwap(&r4);

    TSUNIT_EQUAL(3, r1.ringSize());
    TSUNIT_EQUAL(2, r2.ringSize());
    TSUNIT_EQUAL(2, r3.ringSize());
    TSUNIT_EQUAL(1, r4.ringSize());
    TSUNIT_EQUAL(3, r5.ringSize());
    TSUNIT_EQUAL(3, r6.ringSize());

    TSUNIT_ASSERT(r4.ringNext<R>() == &r4);
    TSUNIT_ASSERT(r4.ringPrevious<R>() == &r4);

    TSUNIT_ASSERT(r1.ringNext<R>() == &r5);
    TSUNIT_ASSERT(r1.ringPrevious<R>() == &r6);
    TSUNIT_ASSERT(r6.ringNext<R>() == &r1);
    TSUNIT_ASSERT(r5.ringPrevious<R>() == &r1);

    // Swap r3 and r5: {r4}, {r2, r5}, {r1, r3, r6]
    r3.ringSwap(&r5);

    TSUNIT_EQUAL(3, r1.ringSize());
    TSUNIT_EQUAL(2, r2.ringSize());
    TSUNIT_EQUAL(3, r3.ringSize());
    TSUNIT_EQUAL(1, r4.ringSize());
    TSUNIT_EQUAL(2, r5.ringSize());
    TSUNIT_EQUAL(3, r6.ringSize());

    TSUNIT_ASSERT(r3.ringNext<R>() == &r6);
    TSUNIT_ASSERT(r3.ringPrevious<R>() == &r1);
    TSUNIT_ASSERT(r1.ringNext<R>() == &r3);
    TSUNIT_ASSERT(r6.ringPrevious<R>() == &r3);

    TSUNIT_ASSERT(r5.ringNext<R>() == &r2);
    TSUNIT_ASSERT(r5.ringPrevious<R>() == &r2);
    TSUNIT_ASSERT(r2.ringNext<R>() == &r5);
    TSUNIT_ASSERT(r2.ringPrevious<R>() == &r5);
}
