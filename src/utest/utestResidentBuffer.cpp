//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
    TSUNIT_DECLARE_TEST(ResidentBuffer);
};

TSUNIT_REGISTER(ResidentBufferTest);


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(ResidentBuffer)
{
    const size_t buf_size = 10000;

    ts::ResidentBuffer<uint8_t> buf(buf_size);

    debug() << "ResidentBufferTest: isLocked() = " << buf.isLocked()
            << ", requested size = " << buf_size << ", count() = " << buf.count() << std::endl;
    if (!buf.isLocked()) {
        debug() << "ResidentBufferTest: lockErrorCode() = " << buf.lockErrorCode().value()
                << ", " << buf.lockErrorCode().message() << std::endl;
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
