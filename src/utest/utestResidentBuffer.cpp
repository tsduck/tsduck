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
//  TSUnit test suite for class ts::ResidentBuffer
//
//----------------------------------------------------------------------------

#include "tsResidentBuffer.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ResidentBufferTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testResidentBuffer();

    TSUNIT_TEST_BEGIN(ResidentBufferTest);
    TSUNIT_TEST(testResidentBuffer);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(ResidentBufferTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void ResidentBufferTest::beforeTest()
{
}

// Test suite cleanup method.
void ResidentBufferTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void ResidentBufferTest::testResidentBuffer()
{
    const size_t buf_size = 10000;

    ts::ResidentBuffer<uint8_t> buf(buf_size);

    debug() << "ResidentBufferTest: isLocked() = " << buf.isLocked()
            << ", requested size = " << buf_size << ", count() = " << buf.count() << std::endl;
    if (!buf.isLocked()) {
        debug() << "ResidentBufferTest: lockErrorCode() = " << buf.lockErrorCode()
                << ", " << ts::SysErrorCodeMessage(buf.lockErrorCode())  << std::endl;
    }

    // On DragonFlyBSD, the mlock() system call is reserved to the superuser and locking never succeeds with normal users.
#if defined(TS_DRAGONFLYBSD)
    if (::geteuid() == 0) {
        TSUNIT_ASSERT(buf.isLocked());
    }
#else
    TSUNIT_ASSERT(buf.isLocked());
#endif

    TSUNIT_ASSERT(buf.count() >= buf_size);
}
