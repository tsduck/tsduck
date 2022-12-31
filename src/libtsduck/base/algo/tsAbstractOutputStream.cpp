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

#include "tsAbstractOutputStream.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::AbstractOutputStream::AbstractOutputStream(size_t bufferSize) :
    std::basic_ostream<char>(this),
    std::basic_streambuf<char>(),
    _buffer()
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
