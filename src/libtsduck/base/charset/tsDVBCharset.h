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
//!  Declaration of abstract class DVBCharset.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsCharset.h"
#include "tsException.h"

namespace ts {
    //!
    //! Definition of a character set for DVB encoding.
    //! @see ETSI EN 300 468, Annex A.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL DVBCharset: public Charset
    {
        TS_NOBUILD_NOCOPY(DVBCharset);
    public:
        //!
        //! Exception thrown when registering duplicate charsets.
        //!
        TS_DECLARE_EXCEPTION(DuplicateDVBCharset);
        //!
        //! Exception thrown when registering invalid charsets.
        //!
        TS_DECLARE_EXCEPTION(InvalidDVBCharset);

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
        //! @param [out] codeSize Size in bytes of character coding table in @a dvb.
        //! @param [in] dvb Address of a DVB string.
        //! @param [in] dvbSize Size in bytes of the DVB string.
        //! @return True on success, false on error (truncated, unsupported format, etc.)
        //!
        static bool GetCharCodeTable(uint32_t& code, size_t& codeSize, const uint8_t* dvb, size_t dvbSize);

        //!
        //! Get the DVB table code for the character set.
        //! @return DVB table code.
        //!
        uint32_t tableCode() const {return _code;}

        //!
        //! Get a DVB character set by name.
        //! @param [in] name Name of the requested character set.
        //! @return Address of the character or zero if not found.
        //!
        static DVBCharset* GetCharset(const UString& name);

        //!
        //! Get a DVB character set by table code.
        //! @param [in] tableCode Table code of the requested character set.
        //! @return Address of the character or zero if not found.
        //!
        static DVBCharset* GetCharset(uint32_t tableCode);

        //!
        //! Find all registered character set names.
        //! @return List of all registered names.
        //!
        static UStringList GetAllNames();

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
        virtual ~DVBCharset();

    protected:
        //!
        //! Protected constructor.
        //! @param [in] name charset name
        //! @param [in] tableCode DVB table code
        //!
        DVBCharset(const UString& name, uint32_t tableCode);

        //!
        //! Remove the specified charset
        //! @param [in] charset The charset to remove.
        //!
        static void Unregister(const DVBCharset* charset);

    private:
        uint32_t _code;  //!< Table code.
    };
}
