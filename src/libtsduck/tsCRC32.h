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
//!
//!  @file
//!  Cyclic Redundancy Check as used in MPEG sections.
//!
//!  Original code, authors & copyright are unclear
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    class TSDUCKDLL CRC32
    {
    public:
        // Default constructor
        CRC32 () : _fcs (0xFFFFFFFF) {}

        // Copy constructor
        CRC32 (const CRC32& c) : _fcs (c._fcs) {}

        // Constructor, compute the CRC32 of a data area
        CRC32 (const void* data, size_t size) : _fcs (0xFFFFFFFF) {add (data, size);}

        // Continue the computation of a data area, following a previous CRC32
        void add (const void* data, size_t size);

        // Get the value of a CRC32
        uint32_t value() const {return _fcs;}

        // Convert to a 32-bit int
        operator uint32_t() const {return _fcs;}

        // Assigment
        CRC32& operator= (const CRC32& c) {_fcs = c._fcs; return *this;}

        // Comparisons
        bool operator== (const CRC32& c) const {return _fcs == c._fcs;}
        bool operator!= (const CRC32& c) const {return _fcs != c._fcs;}
        bool operator== (uint32_t c) const {return _fcs == c;}
        bool operator!= (uint32_t c) const {return _fcs != c;}

        // What to do with a CRC32
        enum Validation {IGNORE, CHECK, COMPUTE};

    private:
        uint32_t _fcs;
    };

    // Reversed forms of operators are not member functions
    TSDUCKDLL inline bool operator== (uint32_t c1, const CRC32& c2)
    {
        return c2 == c1;  // this one is a member function
    }

    // Reversed forms of operators are not member functions
    TSDUCKDLL inline bool operator!= (uint32_t c1, const CRC32& c2)
    {
        return c2 != c1;  // this one is a member function
    }
}
