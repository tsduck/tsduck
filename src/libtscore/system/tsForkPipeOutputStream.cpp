//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsForkPipeOutputStream.h"
#include "tsNullReport.h"


//----------------------------------------------------------------------------
// Constructor / destructor
//----------------------------------------------------------------------------

ts::ForkPipeOutputStream::~ForkPipeOutputStream()
{
    ForkPipeOutputStream::close(NULLREP);
}


//----------------------------------------------------------------------------
// Implementation of AbstractOutputStream
//----------------------------------------------------------------------------

bool ts::ForkPipeOutputStream::writeStreamBuffer(const void* addr, size_t size)
{
    size_t outsize = 0;
    return writeStream(addr, size, outsize, NULLREP);
}


//----------------------------------------------------------------------------
// Close the pipe.
//----------------------------------------------------------------------------

bool ts::ForkPipeOutputStream::close(Report& report)
{
    // Flush the output buffer, if any.
    if (isOpen()) {
        flush(); // from std::basic_ostream
    }

    // The rest is done in superclass.
    return ForkPipe::close(report);
}
