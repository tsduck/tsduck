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
//!  Base class for AVC data, either access units or structures.
//!  AVC is Advanced Video Coding, ISO 14496-10, ITU H.264.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    class TSDUCKDLL AbstractAVCData
    {
    public:
        // Constructor & destructor
        AbstractAVCData() : valid (false) {}
        virtual ~AbstractAVCData() {}

        // Clear all values
        virtual void clear() {valid = false;}

        // Parse a memory area. Return the "valid" flag.
        virtual bool parse (const void*, size_t) = 0;

        // Display structure content
        virtual std::ostream& display (std::ostream& = std::cout, const std::string& margin = "") const = 0;

        // Valid flag. Other fields are significant only if valid is true.
        bool valid;

    protected:
        #if defined(__msc)
            #pragma warning(push)
            #pragma warning(disable:4127) // conditional expression is constant
        #endif

        // Display helpers
        template <typename INT>
        void disp (std::ostream& out, const std::string& margin, const char* name, INT n) const
        {
            out << margin << name << " = ";
            if (sizeof(INT) < 2) {
                out << int (n);
            }
            else {
                out << n;
            }
            out << std::endl;
        }

        template <typename INT>
        void disp (std::ostream& out, const std::string& margin, const char* name, std::vector<INT> n) const
        {
            for (size_t i = 0; i < n.size(); ++i) {
                out << margin << name << "[" << i << "] = ";
                if (sizeof(INT) < 2) {
                    out << int (n[i]);
                }
                else {
                    out << n[i];
                }
                out << std::endl;
            }
        }

        #if defined(__msc)
            #pragma warning (pop)
        #endif
    };
}

// Output operator
TSDUCKDLL inline std::ostream& operator<< (std::ostream& strm, const ts::AbstractAVCData& data)
{
    return data.display (strm);
}
