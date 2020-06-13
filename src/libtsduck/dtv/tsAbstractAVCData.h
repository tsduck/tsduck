//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
#include "tsDisplayInterface.h"

namespace ts {

    //!
    //! Abstract base class for AVC data, either access units or structures.
    //!
    //! AVC is Advanced Video Coding, ISO 14496-10, ITU H.264.
    //!
    //! There is no strict encapsulation of data. Each subclass exposes public fields.
    //! This base class declares a common interface to parse, display and validate the
    //! data.
    //!
    //! @ingroup mpeg
    //!
    class TSDUCKDLL AbstractAVCData: public DisplayInterface
    {
    public:
        //!
        //! Constructor.
        //!
        AbstractAVCData();

        //!
        //! Destructor.
        //!
        virtual ~AbstractAVCData();

        //!
        //! Clear all values.
        //! Should be reimplemented by subclasses.
        //! The data are marked invalid.
        //!
        virtual void clear();

        //!
        //! Parse a memory area containing binary AVC data of the expected type.
        //! Must be reimplemented by subclasses.
        //! The data are marked as valid or invalid.
        //! @param [in] addr Address of the binary data to parse.
        //! @param [in] size Size in bytes of the binary data to parse.
        //! @return The @link valid @endlink flag.
        //!
        virtual bool parse(const void* addr, size_t size) = 0;

        //!
        //! Valid flag.
        //! Other fields are significant only if @a valid is true.
        //!
        bool valid;

    protected:
        TS_PUSH_WARNING()
        TS_MSC_NOWARNING(4127) // conditional expression is constant

        //!
        //! Display helper for subclasses.
        //! Display an integer value.
        //! @tparam INT An integer type.
        //! @param [in,out] out The stream where to print the content.
        //! @param [in] margin The prefix string on each line.
        //! @param [in] name A name to display for the value.
        //! @param [in] n The integer value to display.
        //!
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        void disp(std::ostream& out, const UString& margin, const UChar* name, INT n) const
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

        //!
        //! Display helper for subclasses.
        //! Display a vector of integer value.
        //! @tparam INT An integer type.
        //! @param [in,out] out The stream where to print the content.
        //! @param [in] margin The prefix string on each line.
        //! @param [in] name A name to display for the value.
        //! @param [in] n The integer values to display.
        //!
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        void disp(std::ostream& out, const UString& margin, const UChar* name, std::vector<INT> n) const
        {
            for (size_t i = 0; i < n.size(); ++i) {
                out << margin << name << "[" << i << "] = ";
                if (sizeof(INT) < 2) {
                    out << int(n[i]);
                }
                else {
                    out << n[i];
                }
                out << std::endl;
            }
        }

        TS_POP_WARNING()
    };
}
