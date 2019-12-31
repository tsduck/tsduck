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
//!  Cyclic Redundancy Check as used in MPEG sections.
//!
//!  Original code, authors & copyright are unclear
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! Cyclic Redundancy Check as used in MPEG sections.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL CRC32
    {
    public:
        //!
        //! Default constructor.
        //!
        CRC32() : _fcs(0xFFFFFFFF) {}

        //!
        //! Constructor, compute the CRC32 of a data area.
        //! @param [in] data Address of area to analyze.
        //! @param [in] size Size in bytes of area to analyze.
        //!
        CRC32(const void* data, size_t size) :
            _fcs(0xFFFFFFFF)
        {
            add(data, size);
        }

        //!
        //! Continue the computation of a data area, following a previous CRC32.
        //! @param [in] data Address of area to analyze.
        //! @param [in] size Size in bytes of area to analyze.
        //!
        void add(const void* data, size_t size);

        //!
        //! Get the value of the CRC32 as computed so far.
        //! @return The value of the CRC32 as computed so far.
        //!
        uint32_t value() const
        {
            return _fcs;
        }

        //!
        //! Convert to a 32-bit integer.
        //! Same as value() method.
        //! @return The value of the CRC32 as computed so far.
        //!
        operator uint32_t() const {return _fcs;}

        //!
        //! Comparison operator with another CRC32 instance.
        //! @param [in] c Other instance to compare.
        //! @return True if the two CRC32 are identical, false otherwise.
        //!
        bool operator==(const CRC32& c) const
        {
            return _fcs == c._fcs;
        }

        //!
        //! Comparison operator with another CRC32 instance.
        //! @param [in] c Other instance to compare.
        //! @return True if the two CRC32 are different, false otherwise.
        //!
        bool operator!=(const CRC32& c) const
        {
            return _fcs != c._fcs;
        }

        //!
        //! Comparison operator with a 32-bit integer.
        //! @param [in] c A CRC32 value to compare.
        //! @return True if the two CRC32 are identical, false otherwise.
        //!
        bool operator==(uint32_t c) const
        {
            return _fcs == c;
        }

        //!
        //! Comparison operator with a 32-bit integer.
        //! @param [in] c A CRC32 value to compare.
        //! @return True if the two CRC32 are different, false otherwise.
        //!
        bool operator!=(uint32_t c) const
        {
            return _fcs != c;
        }

        //!
        //! Reset the CRC32 computation, restart a new computation.
        //!
        void reset()
        {
            _fcs = 0xFFFFFFFF;
        }

        //!
        //! What to do with a CRC32.
        //! Used when building MPEG sections.
        //!
        enum Validation {
            IGNORE,   //!< Ignore the section CRC32.
            CHECK,    //!< Check that the value of the CRC32 of the section is correct and fail if it isn't.
            COMPUTE   //!< Recompute a fresh new CRC32 value based on the content of the section.
        };

    private:
        uint32_t _fcs;
    };

    //!
    //! Comparison operator between a CRC32 instance and a 32-bit integer.
    //! The reversed form of operators is a member function.
    //! @param [in] c1 A CRC32 value to compare.
    //! @param [in] c2 A CRC32 instance to compare.
    //! @return True if the two CRC32 are identical, false otherwise.
    //!
    TSDUCKDLL inline bool operator== (uint32_t c1, const CRC32& c2)
    {
        return c2 == c1;  // this one is a member function
    }

    //!
    //! Comparison operator between a CRC32 instance and a 32-bit integer.
    //! The reversed form of operators is a member function.
    //! @param [in] c1 A CRC32 value to compare.
    //! @param [in] c2 A CRC32 instance to compare.
    //! @return True if the two CRC32 are different, false otherwise.
    //!
    TSDUCKDLL inline bool operator!= (uint32_t c1, const CRC32& c2)
    {
        return c2 != c1;  // this one is a member function
    }
}
