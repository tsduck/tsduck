//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsForkPipeOutputStream.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::ForkPipeOutputStream::ForkPipeOutputStream(Report* report, Object* owner) :
    ForkPipe(report, false, owner)
{
}

ts::ForkPipeOutputStream::ForkPipeOutputStream(ReporterBase* delegate, Object* owner) :
    ForkPipe(delegate, false, owner)
{
}

ts::ForkPipeOutputStream::~ForkPipeOutputStream()
{
    ForkPipeOutputStream::close(true);
}


//----------------------------------------------------------------------------
// Implementation of AbstractStandardOutputStream
//----------------------------------------------------------------------------

bool ts::ForkPipeOutputStream::writeStreamBuffer(const void* addr, size_t size)
{
    size_t outsize = 0;
    return writeStream(addr, size, outsize);
}


//----------------------------------------------------------------------------
// Close the pipe.
//----------------------------------------------------------------------------

bool ts::ForkPipeOutputStream::close(bool silent)
{
    // Flush the output buffer, if any.
    if (isOpen()) {
        flush(); // from std::basic_ostream
    }

    // The rest is done in superclass.
    return ForkPipe::close(silent);
}
