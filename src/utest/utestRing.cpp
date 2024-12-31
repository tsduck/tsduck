//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
    TSUNIT_DECLARE_TEST(RingNode);
    TSUNIT_DECLARE_TEST(Swap);
};

TSUNIT_REGISTER(RingTest);


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

TSUNIT_DEFINE_TEST(RingNode)
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
    // It does not realize that r5 removed itself from the r1-r3 ring in its destructor.

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

TSUNIT_DEFINE_TEST(Swap)
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
