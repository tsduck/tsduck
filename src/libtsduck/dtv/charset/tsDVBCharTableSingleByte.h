//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Declaration of class DVBCharTableSingleByte.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDVBCharTable.h"
#include "tsDVBCharset.h"

TS_PUSH_WARNING()
TS_GCC_NOWARNING(ctor-dtor-privacy) // private constructor here

namespace ts {
    //!
    //! Definition of a DVB character set using a single byte per character.
    //!
    //! All these character sets share the following properties:
    //! - Codes 0x00-0x1F and 0x7F-9F are not assigned.
    //! - Codes 0x20-0x7E are identical to ASCII.
    //! - Only codes 0xA0-0xFF are specific, some of them being unused depending on the character set.
    //! - The code 0x8A is interpreted as a new line.
    //!
    //! @see ETSI EN 300 468, Annex A.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL DVBCharTableSingleByte: public DVBCharTable
    {
        TS_NOBUILD_NOCOPY(DVBCharTableSingleByte);
    public:
        // Predefined character sets.
        static const DVBCharTableSingleByte RAW_ISO_6937;    //!< Raw modified ISO 6937, DVB default character table.
        static const DVBCharTableSingleByte RAW_ISO_8859_1;  //!< Raw ISO 8859-1 character set.
        static const DVBCharTableSingleByte RAW_ISO_8859_2;  //!< Raw ISO 8859-2 character set.
        static const DVBCharTableSingleByte RAW_ISO_8859_3;  //!< Raw ISO 8859-3 character set.
        static const DVBCharTableSingleByte RAW_ISO_8859_4;  //!< Raw ISO 8859-4 character set.
        static const DVBCharTableSingleByte RAW_ISO_8859_5;  //!< Raw ISO 8859-5 character set.
        static const DVBCharTableSingleByte RAW_ISO_8859_6;  //!< Raw ISO 8859-6 character set.
        static const DVBCharTableSingleByte RAW_ISO_8859_7;  //!< Raw ISO 8859-7 character set.
        static const DVBCharTableSingleByte RAW_ISO_8859_8;  //!< Raw ISO 8859-8 character set.
        static const DVBCharTableSingleByte RAW_ISO_8859_9;  //!< Raw ISO 8859-9 character set.
        static const DVBCharTableSingleByte RAW_ISO_8859_10; //!< Raw ISO 8859-10 character set.
        static const DVBCharTableSingleByte RAW_ISO_8859_11; //!< Raw ISO 8859-11 character set.
        static const DVBCharTableSingleByte RAW_ISO_8859_13; //!< Raw ISO 8859-13 character set.
        static const DVBCharTableSingleByte RAW_ISO_8859_14; //!< Raw ISO 8859-14 character set.
        static const DVBCharTableSingleByte RAW_ISO_8859_15; //!< Raw ISO 8859-15 character set.

        static const DVBCharset DVB_ISO_6937;    //!< Standard DVB encoding using modified ISO 6937 character set as default.
        static const DVBCharset DVB_ISO_8859_1;  //!< Non-standard DVB encoding using 8859-1 character set as default.
        static const DVBCharset DVB_ISO_8859_2;  //!< Non-standard DVB encoding using ISO 8859-2 character set as default.
        static const DVBCharset DVB_ISO_8859_3;  //!< Non-standard DVB encoding using ISO 8859-3 character set as default.
        static const DVBCharset DVB_ISO_8859_4;  //!< Non-standard DVB encoding using ISO 8859-4 character set as default.
        static const DVBCharset DVB_ISO_8859_5;  //!< Non-standard DVB encoding using ISO 8859-5 character set as default.
        static const DVBCharset DVB_ISO_8859_6;  //!< Non-standard DVB encoding using ISO 8859-6 character set as default.
        static const DVBCharset DVB_ISO_8859_7;  //!< Non-standard DVB encoding using ISO 8859-7 character set as default.
        static const DVBCharset DVB_ISO_8859_8;  //!< Non-standard DVB encoding using ISO 8859-8 character set as default.
        static const DVBCharset DVB_ISO_8859_9;  //!< Non-standard DVB encoding using ISO 8859-9 character set as default.
        static const DVBCharset DVB_ISO_8859_10; //!< Non-standard DVB encoding using ISO 8859-10 character set as default.
        static const DVBCharset DVB_ISO_8859_11; //!< Non-standard DVB encoding using ISO 8859-11 character set as default.
        static const DVBCharset DVB_ISO_8859_13; //!< Non-standard DVB encoding using ISO 8859-13 character set as default.
        static const DVBCharset DVB_ISO_8859_14; //!< Non-standard DVB encoding using ISO 8859-14 character set as default.
        static const DVBCharset DVB_ISO_8859_15; //!< Non-standard DVB encoding using ISO 8859-15 character set as default.

        // Inherited methods.
        virtual bool decode(UString& str, const uint8_t* dvb, size_t dvbSize) const override;
        virtual bool canEncode(const UString& str, size_t start = 0, size_t count = NPOS) const override;
        virtual size_t encode(uint8_t*& buffer, size_t& size, const UString& str, size_t start = 0, size_t count = NPOS) const override;

    private:
        //!
        //! Private constructor since no external instance can be defined.
        //! @param [in] name charset name.
        //! @param [in] tableCode DVB table code.
        //! @param [in] init Initializer list. 96 code point values for 0xA0-0xFF range, zero means unused.
        //! @param [in] revDiac Optional list of byte values in range 0xA0-0xFF of combining diacritical
        //! marks which precede their base letter (and must be reversed from Unicode).
        //!
        DVBCharTableSingleByte(const UChar* name, uint32_t tableCode, std::initializer_list<uint16_t> init, std::initializer_list<uint8_t> revDiac = std::initializer_list<uint8_t>());

        // List of code points for byte values 0xA0-0xFF. Always contain 96 values.
        const std::vector<uint16_t> _upperCodePoints {};

        // Reverse mapping for complete character set (key = code point, value = byte rep).
        std::map<UChar, uint8_t> _bytesMap {};

        // Bitmap of combining diacritical marks which precede their base letter (and must be reversed from Unicode).
        // This only applies to byte values 0xA0-0xFF (96 values).
        std::bitset<96> _reversedDiacritical {};
    };
}

TS_POP_WARNING()
