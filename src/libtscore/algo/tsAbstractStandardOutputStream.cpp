//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractStandardOutputStream.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::AbstractStandardOutputStream::AbstractStandardOutputStream(size_t bufferSize) :
    std::basic_ostream<char>(this),
    std::basic_streambuf<char>()
{
    _buffer.resize(bufferSize);
    resetBuffer();
}

ts::AbstractStandardOutputStream::~AbstractStandardOutputStream()
{
}


//----------------------------------------------------------------------------
// This is called when buffer becomes full.
//----------------------------------------------------------------------------

ts::AbstractStandardOutputStream::int_type ts::AbstractStandardOutputStream::overflow(int_type c)
{
    // Flush content of the buffer.
    bool ok = writeStreamBuffer(pbase(), pptr() - pbase());

    // Flush the character that didn't fit in buffer.
    if (ok && c != traits_type::eof()) {
        char ch = char(c);
        ok = writeStreamBuffer(&ch, 1);
    }

    // Nothing to flush anymore.
    resetBuffer();
    return ok ? c : traits_type::eof();
}


//----------------------------------------------------------------------------
// This function is called when the stream is flushed.
//----------------------------------------------------------------------------

int ts::AbstractStandardOutputStream::sync()
{
    bool ok = writeStreamBuffer(pbase(), pptr() - pbase());
    resetBuffer();
    return ok ? 0 : -1;
}
