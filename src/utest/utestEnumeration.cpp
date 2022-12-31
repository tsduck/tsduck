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
//  TSUnit test suite for class ts::Enumeration
//
//----------------------------------------------------------------------------

#include "tsEnumeration.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class EnumerationTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testEnumeration();
    void testName();
    void testNames();
    void testValue();
    void testNameList();
    void testIterators();
    void testError();

    TSUNIT_TEST_BEGIN(EnumerationTest);
    TSUNIT_TEST(testEnumeration);
    TSUNIT_TEST(testName);
    TSUNIT_TEST(testNames);
    TSUNIT_TEST(testValue);
    TSUNIT_TEST(testNameList);
    TSUNIT_TEST(testIterators);
    TSUNIT_TEST(testError);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(EnumerationTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void EnumerationTest::beforeTest()
{
}

// Test suite cleanup method.
void EnumerationTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

// Test cases
void EnumerationTest::testEnumeration()
{
    ts::Enumeration e1;
    ts::Enumeration e2({});

    TSUNIT_ASSERT(e1.size() == 0);
    TSUNIT_ASSERT(e2.size() == 0);
    TSUNIT_ASSERT(e1 == e2);

    ts::Enumeration e3({{u"FirstElement", -1},
                        {u"SecondElement", 7},
                        {u"FirstRepetition", 47},
                        {u"OtherValue", -123}});

    TSUNIT_ASSERT(e3.size() == 4);

    ts::Enumeration e4(e3);
    TSUNIT_ASSERT(e4.size() == 4);
    TSUNIT_ASSERT(e3 == e4);
    TSUNIT_ASSERT(e3 != e1);

    e3.add(u"AddedElement", 458);
    TSUNIT_ASSERT(e3.size() == 5);
    TSUNIT_ASSERT(e3 != e4);
    TSUNIT_ASSERT(e3 != e1);

    e1 = e3;
    TSUNIT_ASSERT(e1.size() == 5);
    TSUNIT_ASSERT(e1 == e3);
    TSUNIT_ASSERT(e1 != e2);
}

void EnumerationTest::testName()
{
    ts::Enumeration e1({{u"FirstElement", -1},
                        {u"SecondElement", 7},
                        {u"FirstRepetition", 47},
                        {u"OtherValue", -123},
                        {u"AddedElement", 458}});

    TSUNIT_EQUAL(u"FirstElement", e1.name(-1));
    TSUNIT_EQUAL(u"SecondElement", e1.name(7));
    TSUNIT_EQUAL(u"FirstRepetition", e1.name(47));
    TSUNIT_EQUAL(u"OtherValue", e1.name(-123));
    TSUNIT_EQUAL(u"AddedElement", e1.name(458));

    TSUNIT_ASSERT(e1.size() == 5);
    e1.add(u"Other7", 7);
    TSUNIT_ASSERT(e1.size() == 6);

    const ts::UString v7(e1.name(7));
    TSUNIT_ASSERT(v7 == u"SecondElement" || v7 == u"Other7");
}

void EnumerationTest::testNames()
{
    ts::Enumeration e1({{u"FirstElement", -1},
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

void EnumerationTest::testValue()
{
    ts::Enumeration e1({{u"FirstElement", -1},
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
    TSUNIT_ASSERT(e1.value(u"firste") == ts::Enumeration::UNKNOWN);
    TSUNIT_ASSERT(e1.value(u"firste", true) == ts::Enumeration::UNKNOWN);
    TSUNIT_ASSERT(e1.value(u"firste", false) == -1);

    TSUNIT_ASSERT(e1.value(u"FirstElem") == -1);
    TSUNIT_ASSERT(e1.value(u"FirstE") == -1);
    TSUNIT_ASSERT(e1.value(u"First") == ts::Enumeration::UNKNOWN);

    TSUNIT_ASSERT(e1.size() == 5);
    e1.add(u"FirstRepetition", 48);
    TSUNIT_ASSERT(e1.size() == 6);

    const int vFirstRepetition = e1.value(u"FirstRepetition");
    TSUNIT_ASSERT(vFirstRepetition == 47 || vFirstRepetition == 48);

    TSUNIT_ASSERT(e1.value(u"1") == 1);
    TSUNIT_ASSERT(e1.value(u"0x10") == 16);
    TSUNIT_ASSERT(e1.value(u"x10") == ts::Enumeration::UNKNOWN);
}

void EnumerationTest::testNameList()
{
    ts::Enumeration e1({{u"FirstElement", -1},
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

void EnumerationTest::testIterators()
{
    ts::Enumeration e1({{u"FirstElement", -1},
                        {u"SecondElement", 7},
                        {u"FirstRepetition", 47},
                        {u"OtherValue", -123},
                        {u"AddedElement", 458}});

    std::map <int, ts::UString> ref;
    ref.insert(std::make_pair(-1, u"FirstElement"));
    ref.insert(std::make_pair(7, u"SecondElement"));
    ref.insert(std::make_pair(47, u"FirstRepetition"));
    ref.insert(std::make_pair(-123, u"OtherValue"));
    ref.insert(std::make_pair(458, u"AddedElement"));

    std::map <int, ts::UString> value;
    for (const auto& it : e1) {
        value.insert(std::make_pair(it.first, it.second));
    }

    TSUNIT_ASSERT(value == ref);
}

void EnumerationTest::testError()
{
    ts::Enumeration e({{u"version",   0},
                       {u"verbose",   1},
                       {u"versatile", 2},
                       {u"other",     3}});

    TSUNIT_EQUAL(u"", e.error(u"oth"));
    TSUNIT_EQUAL(u"", e.error(u"versi"));
    TSUNIT_EQUAL(u"unknown name \"foo\"", e.error(u"foo"));
    TSUNIT_EQUAL(u"ambiguous command \"vers\", could be one of version, versatile", e.error(u"vers", true, true, u"command"));
    TSUNIT_EQUAL(u"ambiguous option \"--ver\", could be one of --version, --verbose, --versatile", e.error(u"ver", true, true, u"option", u"--"));
}
