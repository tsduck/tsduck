//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
