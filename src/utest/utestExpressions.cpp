//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::Expressions
//
//----------------------------------------------------------------------------

#include "tsExpressions.h"
#include "tsReportBuffer.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ExpressionsTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(Definition);
    TSUNIT_DECLARE_TEST(Expression);
    TSUNIT_DECLARE_TEST(Error);
    TSUNIT_DECLARE_TEST(Debug);
};

TSUNIT_REGISTER(ExpressionsTest);


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

// Test cases
TSUNIT_DEFINE_TEST(Definition)
{
    ts::ReportBuffer<ts::ThreadSafety::None> log;
    ts::Expressions e(log);

    TSUNIT_ASSERT(!e.error());
    TSUNIT_EQUAL(0, e.symbolCount());
    TSUNIT_ASSERT(e.define(u"SYM1"));
    TSUNIT_EQUAL(1, e.symbolCount());
    TSUNIT_ASSERT(e.define(u"SYM2"));
    TSUNIT_ASSERT(e.define(u"Sym_3"));
    TSUNIT_EQUAL(3, e.symbolCount());

    TSUNIT_ASSERT(e.isDefined(u"SYM1"));
    TSUNIT_ASSERT(e.isDefined(u"SYM2"));
    TSUNIT_ASSERT(e.isDefined(u"Sym_3"));
    TSUNIT_ASSERT(!e.isDefined(u"Sym3"));

    TSUNIT_ASSERT(e.undefine(u"SYM2"));
    TSUNIT_ASSERT(!e.isDefined(u"SYM2"));
    TSUNIT_EQUAL(2, e.symbolCount());

    e.undefineAll();
    TSUNIT_ASSERT(!e.isDefined(u"SYM1"));
    TSUNIT_EQUAL(0, e.symbolCount());
    TSUNIT_ASSERT(!e.error());
    TSUNIT_EQUAL(u"", log.messages());
}

TSUNIT_DEFINE_TEST(Expression)
{
    ts::ReportBuffer<ts::ThreadSafety::None> log;
    ts::Expressions e(log);

    TSUNIT_ASSERT(e.define(u"SYM1"));
    TSUNIT_ASSERT(e.define(u"SYM2"));
    TSUNIT_ASSERT(e.define(u"SYM3"));

    TSUNIT_ASSERT(e.evaluate(u"SYM3"));
    TSUNIT_ASSERT(!e.error());
    TSUNIT_EQUAL(u"", log.messages());

    TSUNIT_ASSERT(!e.evaluate(u"!SYM3"));
    TSUNIT_ASSERT(!e.error());
    TSUNIT_EQUAL(u"", log.messages());

    TSUNIT_ASSERT(e.evaluate(u"!SYM8"));
    TSUNIT_ASSERT(!e.error());
    TSUNIT_EQUAL(u"", log.messages());

    TSUNIT_ASSERT(e.evaluate(u"  SYM3  "));
    TSUNIT_ASSERT(!e.error());
    TSUNIT_EQUAL(u"", log.messages());

    TSUNIT_ASSERT(e.evaluate(u"  SYM2 || foo "));
    TSUNIT_ASSERT(!e.error());
    TSUNIT_EQUAL(u"", log.messages());

    TSUNIT_ASSERT(!e.evaluate(u"SYM2&&foo"));
    TSUNIT_ASSERT(!e.error());
    TSUNIT_EQUAL(u"", log.messages());

    TSUNIT_ASSERT(e.evaluate(u"!(SYM2 && foo)"));
    TSUNIT_ASSERT(!e.error());
    TSUNIT_EQUAL(u"", log.messages());

    TSUNIT_ASSERT(e.evaluate(u"SYM1 || (SYM2 && foo)"));
    TSUNIT_ASSERT(!e.error());
    TSUNIT_EQUAL(u"", log.messages());

    TSUNIT_ASSERT(e.evaluate(u"SYM1 && (SYM2 || foo)"));
    TSUNIT_ASSERT(!e.error());
    TSUNIT_EQUAL(u"", log.messages());

    TSUNIT_ASSERT(!e.evaluate(u"SYM1 && (foo || bar) && SYM3"));
    TSUNIT_ASSERT(!e.error());
    TSUNIT_EQUAL(u"", log.messages());

    TSUNIT_ASSERT(e.evaluate(u"SYM1 && !(foo || bar) && SYM3"));
    TSUNIT_ASSERT(!e.error());
    TSUNIT_EQUAL(u"", log.messages());
}

TSUNIT_DEFINE_TEST(Error)
{
    ts::ReportBuffer<ts::ThreadSafety::None> log;
    ts::Expressions e(log);

    TSUNIT_ASSERT(!e.error());
    TSUNIT_ASSERT(!e.define(u" SYM1 "));
    TSUNIT_ASSERT(e.error());
    TSUNIT_EQUAL(u"Error: invalid symbol ' SYM1 '", log.messages());

    log.clear();
    e.resetError();
    TSUNIT_ASSERT(!e.error());

    TSUNIT_ASSERT(!e.define(u"_SYM1", u"foo bar"));
    TSUNIT_ASSERT(e.error());
    TSUNIT_EQUAL(u"Error: invalid symbol '_SYM1' in foo bar", log.messages());

    log.clear();
    e.resetError();
    TSUNIT_ASSERT(!e.error());

    TSUNIT_ASSERT(!e.evaluate(u"SYM1 && SYM2 || foo "));
    TSUNIT_ASSERT(e.error());
    TSUNIT_EQUAL(u"Error: not the same logical operator at character 16 in 'SYM1 && SYM2 || foo '", log.messages());
}

TSUNIT_DEFINE_TEST(Debug)
{
    ts::ReportBuffer<ts::ThreadSafety::None> log;
    ts::Expressions e(log);

    log.setMaxSeverity(ts::Severity::Debug);
    TSUNIT_ASSERT(!e.error());
    TSUNIT_ASSERT(e.define(u"SYM1"));
    TSUNIT_ASSERT(!e.error());
    TSUNIT_EQUAL(u"Debug: symbol 'SYM1' defined", log.messages());
}
