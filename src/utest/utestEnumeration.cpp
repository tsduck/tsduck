//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//  CppUnit test suite for class ts::Enumeration
//
//----------------------------------------------------------------------------

#include "tsEnumeration.h"
#include "tsStringUtils.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class EnumerationTest: public CppUnit::TestFixture
{
public:
    void setUp();
    void tearDown();
    void testEnumeration();
    void testName();
    void testNames();
    void testValue();
    void testNameList();
    void testIterators();

    CPPUNIT_TEST_SUITE (EnumerationTest);
    CPPUNIT_TEST (testEnumeration);
    CPPUNIT_TEST (testName);
    CPPUNIT_TEST (testNames);
    CPPUNIT_TEST (testValue);
    CPPUNIT_TEST (testNameList);
    CPPUNIT_TEST (testIterators);
    CPPUNIT_TEST_SUITE_END ();
};

CPPUNIT_TEST_SUITE_REGISTRATION (EnumerationTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void EnumerationTest::setUp()
{
}

// Test suite cleanup method.
void EnumerationTest::tearDown()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

// Test cases
void EnumerationTest::testEnumeration()
{
    ts::Enumeration e1;
    ts::Enumeration e2(NULL, 0);

    CPPUNIT_ASSERT(e1.size() == 0);
    CPPUNIT_ASSERT(e2.size() == 0);
    CPPUNIT_ASSERT(e1 == e2);

    ts::Enumeration e3("FirstElement", -1,
                         "SecondElement", 7,
                         "FirstRepetition", 47,
                         "OtherValue", -123,
                         NULL);

    CPPUNIT_ASSERT(e3.size() == 4);

    ts::Enumeration e4(e3);
    CPPUNIT_ASSERT(e4.size() == 4);
    CPPUNIT_ASSERT(e3 == e4);
    CPPUNIT_ASSERT(e3 != e1);

    e3.add("AddedElement", 458);
    CPPUNIT_ASSERT(e3.size() == 5);
    CPPUNIT_ASSERT(e3 != e4);
    CPPUNIT_ASSERT(e3 != e1);

    e1 = e3;
    CPPUNIT_ASSERT(e1.size() == 5);
    CPPUNIT_ASSERT(e1 == e3);
    CPPUNIT_ASSERT(e1 != e2);
}

void EnumerationTest::testName()
{
    ts::Enumeration e1("FirstElement", -1,
                         "SecondElement", 7,
                         "FirstRepetition", 47,
                         "OtherValue", -123,
                         "AddedElement", 458,
                         NULL);

    CPPUNIT_ASSERT(e1.name (-1) == "FirstElement");
    CPPUNIT_ASSERT(e1.name (7) == "SecondElement");
    CPPUNIT_ASSERT(e1.name (47) == "FirstRepetition");
    CPPUNIT_ASSERT(e1.name (-123) == "OtherValue");
    CPPUNIT_ASSERT(e1.name (458) == "AddedElement");

    CPPUNIT_ASSERT(e1.size() == 5);
    e1.add("Other7", 7);
    CPPUNIT_ASSERT(e1.size() == 6);

    const std::string v7(e1.name(7));
    CPPUNIT_ASSERT(v7 == "SecondElement" || v7 == "Other7");
}

void EnumerationTest::testNames()
{
    ts::Enumeration e1("FirstElement", -1,
                         "SecondElement", 7,
                         "FirstRepetition", 47,
                         "OtherValue", -123,
                         "AddedElement", 458,
                         NULL);

    std::vector<int> vec;
    CPPUNIT_ASSERT(e1.names(vec) == "");

    vec.push_back (7);
    CPPUNIT_ASSERT(e1.names(vec) == "SecondElement");

    vec.push_back (458);
    CPPUNIT_ASSERT(e1.names(vec) == "SecondElement, AddedElement");

    vec.push_back (432);
    CPPUNIT_ASSERT(e1.names(vec) == "SecondElement, AddedElement, 432");
}

void EnumerationTest::testValue()
{
    ts::Enumeration e1("FirstElement", -1,
                         "SecondElement", 7,
                         "FirstRepetition", 47,
                         "OtherValue", -123,
                         "AddedElement", 458,
                         NULL);

    CPPUNIT_ASSERT(e1.value ("FirstElement") == -1);
    CPPUNIT_ASSERT(e1.value ("SecondElement") == 7);
    CPPUNIT_ASSERT(e1.value ("FirstRepetition") == 47);
    CPPUNIT_ASSERT(e1.value ("OtherValue") == -123);
    CPPUNIT_ASSERT(e1.value ("AddedElement") == 458);

    CPPUNIT_ASSERT(e1.value ("FirstElement", true) == -1);
    CPPUNIT_ASSERT(e1.value ("FirstElement", false) == -1);
    CPPUNIT_ASSERT(e1.value ("firste") == ts::Enumeration::UNKNOWN);
    CPPUNIT_ASSERT(e1.value ("firste", true) == ts::Enumeration::UNKNOWN);
    CPPUNIT_ASSERT(e1.value ("firste", false) == -1);

    CPPUNIT_ASSERT(e1.value ("FirstElem") == -1);
    CPPUNIT_ASSERT(e1.value ("FirstE") == -1);
    CPPUNIT_ASSERT(e1.value ("First") == ts::Enumeration::UNKNOWN);

    CPPUNIT_ASSERT(e1.size() == 5);
    e1.add ("FirstRepetition", 48);
    CPPUNIT_ASSERT(e1.size() == 6);

    const int vFirstRepetition = e1.value ("FirstRepetition");
    CPPUNIT_ASSERT(vFirstRepetition == 47 || vFirstRepetition == 48);

    CPPUNIT_ASSERT(e1.value ("1") == 1);
    CPPUNIT_ASSERT(e1.value ("0x10") == 16);
    CPPUNIT_ASSERT(e1.value ("x10") == ts::Enumeration::UNKNOWN);
}

void EnumerationTest::testNameList()
{
    ts::Enumeration e1("FirstElement", -1,
                         "SecondElement", 7,
                         "FirstRepetition", 47,
                         "OtherValue", -123,
                         "AddedElement", 458,
                         NULL);

    std::vector <std::string> ref;
    ref.push_back ("FirstElement");
    ref.push_back ("SecondElement");
    ref.push_back ("FirstRepetition");
    ref.push_back ("OtherValue");
    ref.push_back ("AddedElement");

    const std::string list (e1.nameList());
    utest::Out() << "EnumerationTest: e1.nameList() = \"" << list << "\"" << std::endl;

    std::vector<std::string> value;
    ts::SplitString(value, list);

    std::sort (ref.begin(), ref.end());
    std::sort (value.begin(), value.end());
    CPPUNIT_ASSERT(value == ref);
}

void EnumerationTest::testIterators()
{
    ts::Enumeration e1("FirstElement", -1,
                         "SecondElement", 7,
                         "FirstRepetition", 47,
                         "OtherValue", -123,
                         "AddedElement", 458,
                         NULL);

    std::map <int, std::string> ref;
    ref.insert (std::make_pair (-1, "FirstElement"));
    ref.insert (std::make_pair (7, "SecondElement"));
    ref.insert (std::make_pair (47, "FirstRepetition"));
    ref.insert (std::make_pair (-123, "OtherValue"));
    ref.insert (std::make_pair (458, "AddedElement"));

    std::map <int, std::string> value;
    for (ts::Enumeration::const_iterator it = e1.begin(); it != e1.end(); ++it) {
        value.insert (std::make_pair (it->first, it->second));
    }

    CPPUNIT_ASSERT(value == ref);
}
