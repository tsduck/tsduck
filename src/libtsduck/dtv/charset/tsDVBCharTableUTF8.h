//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Declaration of class DVBCharTableUTF8.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDVBCharTable.h"
#include "tsDVBCharset.h"

TS_PUSH_WARNING()
TS_GCC_NOWARNING(ctor-dtor-privacy) // private constructor here

namespace ts {
    //!
    //! Definition of the UTF-8 DVB character set.
    //! @see ETSI EN 300 468, Annex A.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL DVBCharTableUTF8: public DVBCharTable
    {
        TS_NOCOPY(DVBCharTableUTF8);
    public:
        static const DVBCharTableUTF8 RAW_UTF_8;  //!< Raw UTF-8 character set.
        static const DVBCharset DVB_UTF_8;        //!< Non-standard DVB encoding using UTF-8 character set as default.

        // Inherited methods.
        virtual bool decode(UString& str, const uint8_t* dvb, size_t dvbSize) const override;
        virtual bool canEncode(const UString& str, size_t start = 0, size_t count = NPOS) const override;
        virtual size_t encode(uint8_t*& buffer, size_t& size, const UString& str, size_t start = 0, size_t count = NPOS) const override;

    private:
        // Private constructor since only local instances are available.
        explicit DVBCharTableUTF8(const UChar* name = nullptr);
    };
}

TS_POP_WARNING()
