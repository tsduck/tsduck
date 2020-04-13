//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//  TSUnit test suite for ARIB character sets.
//
//----------------------------------------------------------------------------

#include "tsARIBCharsetB24.h"
#include "tsByteBlock.h"
#include "tsunit.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ARIBCharsetTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testDebug(); // temporary standalone test for debug

    TSUNIT_TEST_BEGIN(ARIBCharsetTest);
    TSUNIT_TEST(testDebug);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(ARIBCharsetTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void ARIBCharsetTest::beforeTest()
{
}

// Test suite cleanup method.
void ARIBCharsetTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

void ARIBCharsetTest::testDebug()
{
    // Test patterns: Service names from Japanese DTTV (service descriptors)
    static const ts::UChar* const hexas[] = {
        u"== Service names",
        u"0E 4E 48 4B 0F 41 6D 39 67 0E 31 FE 0F 3D 29 45 44",
        u"0E 4E 48 4B 0F 41 6D 39 67 0E 32 FE 0F 3D 29 45 44",
        u"0E 4E 48 4B 0F 37 48 42 53 0E 47 FE 0F 3D 29 45 44",
        u"0E 4E 48 4B 45 1D 46 1D 6C 31 0F 3D 29 45 44",
        u"0E 4E 48 4B 45 1D 46 1D 6C 32 0F 3D 29 45 44",
        u"0E 4E 48 4B 45 1D 46 1D 6C 33 0F 3D 29 45 44",
        u"0E 4E 48 4B 0F 37 48 42 53 0E 32",
        u"3D 29 45 44 44 2B 46 7C 4A 7C 41 77",
        u"0E 41 42 53 0F 3D 29 45 44 4A 7C 41 77 0E 31",
        u"0E 41 42 53 0F 3D 29 45 44 4A 7C 41 77 0E 32",
        u"3D 29 45 44 4A 7C 41 77 37 48 42 53",
        u"0E 41 42 53 0F 3D 29 45 44 4A 7C 41 77 4E 57 3B 7E",
        u"0E 41 4B 54 0F 3D 29 45 44 1B 7C C6 EC D3 0E 31",
        u"0E 41 4B 54 0F 3D 29 45 44 1B 7C C6 EC D3 0E 32",
        u"0E 41 4B 54 0F 4E 57 3B 7E",
        u"0E 47 1B 7C AC A4 C9",
        u"0E 41 4B 54 0F 3D 29 45 44 1B 7C C6 EC D3",
        u"== Event name",
        u"0E 4E 48 4B 1D 4B 1D 65 F9 1D 39 37 0F 1B 24 3B 7A 5A 7A 56",
        u"== Event description",
        u"4C 6B 0E 37 0F 3B 7E FD FB 30 6C 4A 62 40 68 D8 FD 30 6C 4A "
        u"62 3F 3C AF FC 21 21 3A 23 FD B3 CE 1D 4B 1D 65 F9 1D 39 F2 "
        u"46 4F B1 BF A4 21 21 21 5A 1B 7C AD E3 B9 BF F9 21 5B 40 44 "
        u"30 66 3C 42 21 24 21 5A B5 D6 AD E3 B9 BF F9 21 5B 43 53 45 "
        u"44 3F 2D 3B 52 21 24 30 4B 46 23 33 24 49 27 21 24 21 5A 35 "
        u"24 3E 5D AD E3 B9 BF F9 21 5B 43 66 42 3C 48 7E 38 78",
        u"== Extended event item",
        u"3D 50 31 69 3C 54",
        u"== Extended event text",
        u"21 5A 1B 7C AD E3 B9 BF F9 21 5B 40 44 30 66 3C 42 21 24 21 "
        u"5A B5 D6 AD E3 B9 BF F9 21 5B 43 53 45 44 3F 2D 3B 52 21 24 "
        u"30 4B 46 23 33 24 49 27 21 24 21 5A 35 24 3E 5D AD E3 B9 BF "
        u"F9 21 5B 43 66 42 3C 48 7E 38 78",
        u"== Event name",
        u"1D 40 F9 1B 6F 26 23 73 AC 0F 4D 68 BF 21 2A FB 47 48 4D 70 "
        u"CE 1B 7C E9 A4 AA F3 33 58 31 60 19 4B 40 78 46 7E 21 2A 49 "
        u"34 3D 43 19 4E 32 26 19 72 4D 5C 40 2E 21 2A 21 2A FC 1B 24 "
        u"3B 7A 5C 7A 56",
        u"== Event description",
        u"40 2E 44 39 DE C3 BF C0 43 66 CE 1B 6F 69 24 2A 73 CE 0F 3B "
        u"52 C9 E2 BF C1 AC FD 37 32 EC CE 43 66 C7 40 68 40 38 4C 72 "
        u"CE 42 67 3F 4D AB E9 3C 6D EA CE 35 3B E4 3B 52 30 69 C6 3D "
        u"51 F2 33 58 D6 FA 49 54 3F 3F 4C 4C 4C 5C CA 40 38 45 4C CF "
        u"42 60 33 58 3D 68 4A 2C CB 21 2A 21 29 33 58 31 60 1B 7C C9 "
        u"E9 DE 34 69 49 69 19 31 19 4E 47 48 4D 70 19 4E 46 7C 21 39 "
        u"19 4B 4C 29 43 65 21 2A",
        nullptr
    };

    debug() << "Character set name: " << ts::ARIBCharsetB24::Instance()->name() << std::endl << std::endl;

    for (auto it = hexas; *it != nullptr; ++it) {
        const ts::UString in(*it);
        if (in.startWith(u"=")) {
            debug() << in << std::endl << std::endl;
        }
        else {
            ts::ByteBlock bb;
            in.hexaDecode(bb);
            ts::UString str;
            const bool ok = ts::ARIBCharsetB24::Instance()->decode(str, bb.data(), bb.size());
            debug() << "Data: " << *it << std::endl
                    << "Decoded: \"" << str << "\" " << (ok ? "(success)" : "(error)") << std::endl
                    << "UTF-16:";
            for (size_t i = 0; i < str.size(); ++i) {
                debug() << ts::UString::Format(u" %X", {uint16_t(str[i])});
            }
            debug() << std::endl << std::endl;
        }
    }
}
