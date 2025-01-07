//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::Names
//
//----------------------------------------------------------------------------

#include "tsNames.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class NamesTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(Name);
    TSUNIT_DECLARE_TEST(Names);
    TSUNIT_DECLARE_TEST(Value);
    TSUNIT_DECLARE_TEST(NameList);
    TSUNIT_DECLARE_TEST(Error);
};

TSUNIT_REGISTER(NamesTest);


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(Name)
{
    ts::Names e1({{u"FirstElement", -1},
                  {u"SecondElement", 7},
                  {u"FirstRepetition", 47},
                  {u"OtherValue", -123},
                  {u"AddedElement", 458}});

    TSUNIT_ASSERT(!e1.empty());
    TSUNIT_EQUAL(u"FirstElement", e1.name(-1));
    TSUNIT_EQUAL(u"SecondElement", e1.name(7));
    TSUNIT_EQUAL(u"FirstRepetition", e1.name(47));
    TSUNIT_EQUAL(u"OtherValue", e1.name(-123));
    TSUNIT_EQUAL(u"AddedElement", e1.name(458));

    e1.add(u"Other7", 7);
    const ts::UString v7(e1.name(7));
    TSUNIT_ASSERT(v7 == u"SecondElement" || v7 == u"Other7");
}

TSUNIT_DEFINE_TEST(Names)
{
    ts::Names e1({{u"FirstElement", -1},
                  {u"SecondElement", 7},
                  {u"FirstRepetition", 47},
                  {u"OtherValue", -123},
                  {u"AddedElement", 458}});

    std::vector<int> vec;
    TSUNIT_EQUAL(u"", e1.names(vec));

    vec.push_back(7);
    TSUNIT_EQUAL(u"SecondElement", e1.names(vec));

    vec.push_back(458);
    TSUNIT_EQUAL(u"SecondElement, AddedElement", e1.names(vec));

    vec.push_back(432);
    TSUNIT_EQUAL(u"SecondElement, AddedElement, 432", e1.names(vec));
}

TSUNIT_DEFINE_TEST(Value)
{
    ts::Names e1({{u"FirstElement", -1},
                  {u"SecondElement", 7},
                  {u"FirstRepetition", 47},
                  {u"OtherValue", -123},
                  {u"AddedElement", 458}});

    TSUNIT_ASSERT(e1.value(u"FirstElement") == -1);
    TSUNIT_ASSERT(e1.value(u"SecondElement") == 7);
    TSUNIT_ASSERT(e1.value(u"FirstRepetition") == 47);
    TSUNIT_ASSERT(e1.value(u"OtherValue") == -123);
    TSUNIT_ASSERT(e1.value(u"AddedElement") == 458);

    TSUNIT_ASSERT(e1.value(u"FirstElement", true) == -1);
    TSUNIT_ASSERT(e1.value(u"FirstElement", false) == -1);
    TSUNIT_ASSERT(e1.value(u"firste") == ts::Names::UNKNOWN);
    TSUNIT_ASSERT(e1.value(u"firste", true) == ts::Names::UNKNOWN);
    TSUNIT_ASSERT(e1.value(u"firste", false) == -1);

    TSUNIT_ASSERT(e1.value(u"FirstElem") == -1);
    TSUNIT_ASSERT(e1.value(u"FirstE") == -1);
    TSUNIT_ASSERT(e1.value(u"First") == ts::Names::UNKNOWN);

    e1.add(u"FirstRepetition", 48);

    const int vFirstRepetition = e1.value(u"FirstRepetition");
    TSUNIT_ASSERT(vFirstRepetition == 47 || vFirstRepetition == 48);

    TSUNIT_ASSERT(e1.value(u"1") == 1);
    TSUNIT_ASSERT(e1.value(u"0x10") == 16);
    TSUNIT_ASSERT(e1.value(u"x10") == ts::Names::UNKNOWN);
}

TSUNIT_DEFINE_TEST(NameList)
{
    ts::Names e1({{u"FirstElement", -1},
                  {u"SecondElement", 7},
                  {u"FirstRepetition", 47},
                  {u"OtherValue", -123},
                  {u"AddedElement", 458}});

    ts::UStringVector ref;
    ref.push_back(u"FirstElement");
    ref.push_back(u"SecondElement");
    ref.push_back(u"FirstRepetition");
    ref.push_back(u"OtherValue");
    ref.push_back(u"AddedElement");

    const ts::UString list(e1.nameList());
    debug() << "EnumerationTest: e1.nameList() = \"" << list << "\"" << std::endl;

    ts::UStringVector value;
    list.split(value);

    std::sort(ref.begin(), ref.end());
    std::sort(value.begin(), value.end());
    TSUNIT_ASSERT(value == ref);
}

TSUNIT_DEFINE_TEST(Error)
{
    ts::Names e({{u"version",   0},
                 {u"verbose",   1},
                 {u"versatile", 2},
                 {u"other",     3}});

    TSUNIT_EQUAL(u"", e.error(u"oth"));
    TSUNIT_EQUAL(u"", e.error(u"versi"));
    TSUNIT_EQUAL(u"unknown name \"foo\"", e.error(u"foo"));
    TSUNIT_EQUAL(u"ambiguous command \"vers\", could be one of version, versatile", e.error(u"vers", true, true, u"command"));
    TSUNIT_EQUAL(u"ambiguous option \"--ver\", could be one of --version, --verbose, --versatile", e.error(u"ver", true, true, u"option", u"--"));
}
