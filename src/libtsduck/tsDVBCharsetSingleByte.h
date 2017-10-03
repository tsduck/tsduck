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
//!  Declaration of class DVBCharsetSingleByte.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDVBCharset.h"
#include "tsByteBlock.h"

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
    //! @see ETSI EN 300 468, Annex A
    //!
    class TSDUCKDLL DVBCharsetSingleByte: public DVBCharset
    {
    public:
        // Predefined character sets.
        static const DVBCharsetSingleByte ISO_6937;    //!< Modified ISO 6937, DVB default charset.
        static const DVBCharsetSingleByte ISO_8859_1;  //!< ISO 8859-1 character set.
        static const DVBCharsetSingleByte ISO_8859_2;  //!< ISO 8859-2 character set.
        static const DVBCharsetSingleByte ISO_8859_3;  //!< ISO 8859-3 character set.
        static const DVBCharsetSingleByte ISO_8859_4;  //!< ISO 8859-4 character set.
        static const DVBCharsetSingleByte ISO_8859_5;  //!< ISO 8859-5 character set.
        static const DVBCharsetSingleByte ISO_8859_6;  //!< ISO 8859-6 character set.
        static const DVBCharsetSingleByte ISO_8859_7;  //!< ISO 8859-7 character set.
        static const DVBCharsetSingleByte ISO_8859_8;  //!< ISO 8859-8 character set.
        static const DVBCharsetSingleByte ISO_8859_9;  //!< ISO 8859-9 character set.
        static const DVBCharsetSingleByte ISO_8859_10; //!< ISO 8859-10 character set.
        static const DVBCharsetSingleByte ISO_8859_11; //!< ISO 8859-11 character set.
        static const DVBCharsetSingleByte ISO_8859_13; //!< ISO 8859-13 character set.
        static const DVBCharsetSingleByte ISO_8859_14; //!< ISO 8859-14 character set.
        static const DVBCharsetSingleByte ISO_8859_15; //!< ISO 8859-15 character set.

        // Inherited methods.
        virtual bool decode(UString& str, const uint8_t* dvb, size_t dvbSize) const;
        virtual bool canEncode(const UString& str, size_t start = 0, size_t count = UString::NPOS) const;
        virtual size_t encode(uint8_t*& buffer, size_t& size, const UString& str, size_t start = 0, size_t count = UString::NPOS) const;

    protected:
        //!
        //! Protected constructor.
        //! @param [in] name charset name.
        //! @param [in] tableCode DVB table code.
        //! @param [in] init Initializer list. 96 code point values for 0xA0-0xFF range, zero means unused.
        //!
        DVBCharsetSingleByte(const UString& name, uint32_t tableCode, std::initializer_list<uint16_t> init);

    private:
        //! List of code points for byte values 0xA0-0xFF. Always contain 96 values.
        const std::vector<uint16_t> _upperCodePoints;
        //! Reverse mapping for complete character set (key = code point, value = byte rep).
        std::map<UChar, uint8_t> _bytesMap;

        // Unaccessible operations.
        DVBCharsetSingleByte() = delete;
        DVBCharsetSingleByte(const DVBCharsetSingleByte&) = delete;
        DVBCharsetSingleByte& operator=(const DVBCharsetSingleByte&) = delete;
    };
}
