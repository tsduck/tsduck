//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Declaration of abstract class DVBCharTable.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsCharset.h"
#include "tsSingleton.h"

namespace ts {
    //!
    //! Definition of a character set for DVB encoding.
    //!
    //! It is important to understand the difference between DVBCharset and DVBCharTable.
    //!
    //! Both classes are subclasses of Charset. So, they both have the capabilities to
    //! encode and decode binary strings in DVB representation. But this is the only
    //! similarity.
    //!
    //! DVBCharset is the generic decoder and encoder for DVB strings. When decoding
    //! a DVB string, it recognizes the leading sequence and uses the appropriate
    //! character table (a DVBCharTable) to interpret the binary data. When encoding
    //! a string, DVBCharset selects the most appropriate DVB character table and
    //! encodes the string using that table, after inserting the appropriate leading
    //! sequence to indicate which character table was used.
    //!
    //! DVBCharTable, on the other hand, is an abstract superclass for all DVB character
    //! tables. Each subclass of DVBCharTable implements one specific DVB character table
    //! (modified ISO 6937, ISO 8859-1, ISO 8859-2, etc.) When encoding or decoding strings,
    //! a subclass of DVBCharTable only decode or encode binary data for this specific DVB
    //! character table. No leading sequence or decoded or encoded.
    //!
    //! Usage guidelines:
    //! - Use DVBCharset when you serialize or deserialize tables and descriptors.
    //! - Use DVBCharTable when you implement the body of DVBCharset.
    //!
    //! @see ETSI EN 300 468, Annex A.
    //! @see DVBCharset
    //! @ingroup mpeg
    //!
    class TSDUCKDLL DVBCharTable: public Charset
    {
        TS_NOBUILD_NOCOPY(DVBCharTable);
    public:
        //!
        //! DVB-encoded CR/LF in single-byte character sets.
        //!
        static constexpr uint8_t DVB_SINGLE_BYTE_CRLF = 0x8A;

        //!
        //! Code point for DVB-encoded CR/LF in two-byte character sets.
        //!
        static constexpr uint16_t DVB_CODEPOINT_CRLF = 0xE08A;

        //!
        //! This static function gets the character coding table at the beginning of a DVB string.
        //!
        //! The character coding table is encoded on up to 3 bytes at the beginning of a DVB string.
        //! The following encodings are recognized, based on the first byte of the DVB string:
        //! - First byte >= 0x20: The first byte is a character. The default encoding is ISO-6937.
        //!   We return zero as @a code.
        //! - First byte == 0x10: The next two bytes indicate an ISO-8859 encoding.
        //!   We return 0x10xxyy as @a code.
        //! - First byte == 0x1F: The second byte is an @e encoding_type_id.
        //!   This encoding is not supported here.
        //! - Other value: One byte encoding.
        //!
        //! @param [out] code Returned character coding table value.
        //! Zero when no code is present (use the default character table).
        //! 0xFFFFFFFF in case of invalid data.
        //! @param [out] codeSize Size in bytes of character coding table in @a dvb.
        //! @param [in] dvb Address of a DVB string.
        //! @param [in] dvbSize Size in bytes of the DVB string.
        //! @return True on success, false on error (truncated, unsupported format, etc.)
        //!
        static bool DecodeTableCode(uint32_t& code, size_t& codeSize, const uint8_t* dvb, size_t dvbSize);

        //!
        //! Get the DVB table code for the character set.
        //! @return DVB table code.
        //!
        uint32_t tableCode() const {return _code;}

        //!
        //! Get a DVB character set by table code.
        //! @param [in] code Table code of the requested character set.
        //! @return Address of the character or zero if not found.
        //!
        static const DVBCharTable* GetTableFromLeadingCode(uint32_t code);

        //!
        //! Encode the character set table code.
        //!
        //! Stop either when the specified number of characters are serialized or
        //! when the buffer is full, whichever comes first.
        //!
        //! @param [in,out] buffer Address of the buffer.
        //! The address is updated to point after the encoded value.
        //! @param [in,out] size Size of the buffer. Updated to remaining size.
        //! @return The number of serialized byte.
        //!
        virtual size_t encodeTableCode(uint8_t*& buffer, size_t& size) const;

        //!
        //! Virtual destructor.
        //!
        virtual ~DVBCharTable() override;

        // Inherited methods.
        virtual void unregister() const override;

    protected:
        //!
        //! Protected constructor.
        //! @param [in] name charset name
        //! @param [in] tableCode DVB table code
        //!
        DVBCharTable(const UChar* name, uint32_t tableCode);

    private:
        // Repository of DVB character tables by table code.
        class TableCodeRepository
        {
            TS_DECLARE_SINGLETON(TableCodeRepository);
        public:
            const DVBCharTable* get(uint32_t code) const;
            void add(uint32_t code, const DVBCharTable* charset);
            void remove(const DVBCharTable* charset);
        private:
            std::map<uint32_t, const DVBCharTable*> _map;
        };

        uint32_t _code;  // Table code.
    };
}
