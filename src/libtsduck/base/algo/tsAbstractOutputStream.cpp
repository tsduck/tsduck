//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractOutputStream.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::AbstractOutputStream::AbstractOutputStream(size_t bufferSize) :
    std::basic_ostream<char>(this),
    std::basic_streambuf<char>()
{
    _buffer.resize(bufferSize);
    resetBuffer();
}

ts::AbstractOutputStream::~AbstractOutputStream()
{
}


//----------------------------------------------------------------------------
// This is called when buffer becomes full.
//----------------------------------------------------------------------------

ts::AbstractOutputStream::int_type ts::AbstractOutputStream::overflow(int_type c)
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

int ts::AbstractOutputStream::sync()
{
    bool ok = writeStreamBuffer(pbase(), pptr() - pbase());
    resetBuffer();
    return ok ? 0 : -1;
}
