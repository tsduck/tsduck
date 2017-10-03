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
//  Definitions of a generic block of bytes.
//
//----------------------------------------------------------------------------

#include "tsByteBlock.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Default constructor
//----------------------------------------------------------------------------

ts::ByteBlock::ByteBlock(size_type size) :
    ByteVector(size)
{
}

//----------------------------------------------------------------------------
// Constructor, initialized with size bytes of specified value
//----------------------------------------------------------------------------

ts::ByteBlock::ByteBlock(size_type size, uint8_t value) :
    ByteVector(size, value)
{
}

//----------------------------------------------------------------------------
// Constructor from a data block
//----------------------------------------------------------------------------

ts::ByteBlock::ByteBlock(const void* data, size_type size) :
    ByteVector(size)
{
    if (size > 0) {
        ::memcpy(&(*this)[0], data, size);  // Flawfinder: ignore: memcpy()
    }
}

//----------------------------------------------------------------------------
// Constructor from a C string
//----------------------------------------------------------------------------

ts::ByteBlock::ByteBlock(const char* str) :
    ByteVector(::strlen(str))  // Flawfinder: ignore: strlen()
{
    if (size() > 0) {
        ::memcpy(data(), str, size());  // Flawfinder: ignore: memcpy()
    }
}

//----------------------------------------------------------------------------
// Constructor from an initializer list.
//----------------------------------------------------------------------------

ts::ByteBlock::ByteBlock(std::initializer_list<uint8_t> init) :
    ByteVector(init)
{
}

//----------------------------------------------------------------------------
// Replace the content of a byte block.
//----------------------------------------------------------------------------

void ts::ByteBlock::copy(const void* data_, size_type size_)
{
    resize(size_);
    if (size_ > 0) {
        ::memcpy(data(), data_, size_);  // Flawfinder: ignore: memcpy()
    }
}

//----------------------------------------------------------------------------
// Increase size by n and return pointer to new n-byte area
//----------------------------------------------------------------------------

void* ts::ByteBlock::enlarge(size_type n)
{
    size_type oldsize = this->size();
    resize(oldsize + n);
    return &(*this)[oldsize];
}

//----------------------------------------------------------------------------
// Remove 'size' elements at index 'first'.
// (the STL equivalent uses iterators, not indices).
//----------------------------------------------------------------------------

void ts::ByteBlock::erase(size_type first, size_type size)
{
    assert(first + size <= this->size());
    ByteVector::erase(begin() + first, begin() + first + size);
}
