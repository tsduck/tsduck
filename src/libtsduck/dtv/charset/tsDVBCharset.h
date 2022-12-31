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
//!
//!  @file
//!  Declaration of class DVBCharset.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsCharset.h"

namespace ts {

    class DVBCharTable;

    //!
    //! Definition of the generic DVB character sets.
    //!
    //! An instance of this class encodes and decodes DVB strings.
    //! DVB strings can use various DVB character tables. Each DVB string is
    //! is encoded using one single DVB character table. Which table is used
    //! is indicated by an optional "table code" at the beginning of the
    //! string.
    //!
    //! According to DVB standard ETSI EN 300 468, the default DVB character table
    //! (without leading table code) is ISO-6937. However, some bogus signalization
    //! may assume that the default character table is different, typically the usual
    //! local character table for the region.
    //!
    //! There are several static instances of DVBCharset. The one named DVB
    //! is the standard one. It can use any DVB character tables and uses ISO-6937
    //! by default. All other instances are identical, except that they use another
    //! character table by default.
    //!
    //! @see ETSI EN 300 468, annex A
    //! @ingroup mpeg
    //!
    class TSDUCKDLL DVBCharset: public Charset
    {
        TS_NOCOPY(DVBCharset);
    public:
        //!
        //! Default predefined DVB character set (using ISO-6937 as default table).
        //!
        static const DVBCharset DVB;

        //!
        //! Constructor.
        //! @param [in] name Character set name.
        //! @param [in] default_table Default character table to use without leading "table code".
        //! When null, the default modified ISO 6937 character table is used.
        //!
        explicit DVBCharset(const UChar* name = nullptr, const DVBCharTable* default_table = nullptr);

        // Inherited methods.
        virtual bool decode(UString& str, const uint8_t* data, size_t size) const override;
        virtual bool canEncode(const UString& str, size_t start = 0, size_t count = NPOS) const override;
        virtual size_t encode(uint8_t*& buffer, size_t& size, const UString& str, size_t start = 0, size_t count = NPOS) const override;

    private:
        const DVBCharTable* const _default_table; // Default character table, never null.
    };
}
