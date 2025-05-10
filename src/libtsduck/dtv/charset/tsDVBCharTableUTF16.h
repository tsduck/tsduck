//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Declaration of class DVBCharTableUTF16.
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
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL DVBCharTableUTF16 : public DVBCharTable
    {
        TS_NOCOPY(DVBCharTableUTF16);
    public:
        static const DVBCharTableUTF16 RAW_UTF_16;  //!< Raw UNICODE (UTF-16) character set.
        static const DVBCharset DVB_UTF_16;         //!< Non-standard DVB encoding using UNICODE (UTF-16) character set as default.

        // Inherited methods.
        virtual bool decode(UString& str, const uint8_t* dvb, size_t dvbSize) const override;
        virtual bool canEncode(const UString& str, size_t start = 0, size_t count = NPOS) const override;
        virtual size_t encode(uint8_t*& buffer, size_t& size, const UString& str, size_t start = 0, size_t count = NPOS) const override;

    private:
        // Private constructor since only local instances are available.
        explicit DVBCharTableUTF16(std::initializer_list<const UChar*> names);
   };
}

TS_POP_WARNING()
