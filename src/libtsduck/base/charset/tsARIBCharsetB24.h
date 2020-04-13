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
//!  Declaration of class ARIBCharsetB24.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsCharset.h"
#include "tsSingletonManager.h"

namespace ts {
    //!
    //! Definition of the ARIB STD-B24 character set (ISDB Japan).
    //!
    //! Note: this implementation is not complete but it decodes most Japanese TV programs.
    //!
    //! @see ARIB STD-B24, chapter 7
    //! @see ARIB STD-B62, fascicle 1, part 2, chapter 5
    //! @see ISO/IEC 2022
    //! @see https://en.wikipedia.org/wiki/ARIB_STD_B24_character_set
    //! @see https://en.wikipedia.org/wiki/ISO/IEC_2022
    //! @ingroup mpeg
    //!
    class TSDUCKDLL ARIBCharsetB24: public Charset
    {
        TS_DECLARE_SINGLETON(ARIBCharsetB24);
    public:
        //! Destructor
        virtual ~ARIBCharsetB24() = default;

        // Inherited methods.
        virtual bool decode(UString& str, const uint8_t* data, size_t size) const override;
        virtual bool canEncode(const UString& str, size_t start = 0, size_t count = NPOS) const override;
        virtual size_t encode(uint8_t*& buffer, size_t& size, const UString& str, size_t start = 0, size_t count = NPOS) const override;

    private:
        // Characters are grouped in set of 94 characters which are mapped in
        // ranges 0x21-0x7E (GL) or 0xA1-0xFE (GR). We store unicode code points
        // as 32-bit values because a small portion of the mapped character
        // sets used 17 bits. When stored in UString, they will use
        // surrogate pairs.

        static constexpr uint8_t GL_FIRST = 0x21;
        static constexpr uint8_t GL_LAST  = 0x7E;
        static constexpr uint8_t GR_FIRST = 0xA1;
        static constexpr uint8_t GR_LAST  = 0xFE;
        static constexpr size_t  CHAR_SLICE_SIZE = 94;

        // A few control codes.
        static constexpr uint8_t ESC = 0x1B;
        static constexpr uint8_t LS0 = 0x0F;
        static constexpr uint8_t LS1 = 0x0E;
        static constexpr uint8_t SS2 = 0x19;
        static constexpr uint8_t SS3 = 0x1D;

        // Mapping of a slice value (0-93) to a UTF-32 code.
        typedef char32_t CharSlice[CHAR_SLICE_SIZE];

        // Description of a character mapping.
        struct CharMap
        {
            bool             byte2;        // True: 2-byte mapping, false: 1-byte mapping.
            size_t           slice_count;  // Number of 94-byte slices.
            const CharSlice* slices;       // Address of first (or only) character slice.
        };

        // Definition of known 1-byte character maps.
        static const CharMap   ALPHANUMERIC_MAP;
        static const CharSlice ALPHANUMERIC_SLICE;
        static const CharMap   HIRAGANA_MAP;
        static const CharSlice HIRAGANA_SLICE;
        static const CharMap   KATAKANA_MAP;
        static const CharSlice KATAKANA_SLICE;
        static const CharMap   JIS_X0201_KATAKANA_MAP;
        static const CharSlice JIS_X0201_KATAKANA_SLICE;

        // Definition of known 2-byte character maps.
        static constexpr size_t KANJI_SLICES_COUNT = 94;
        static const CharMap    KANJI_MAP;
        static const CharSlice  KANJI_SLICES[KANJI_SLICES_COUNT];

        // Dummy mappings for unsupported character sets.
        static const CharMap UNSUPPORTED_1BYTE;
        static const CharMap UNSUPPORTED_2BYTE;

        // An internal decoder class. Using ARIB STD-B24 notation.
        class Decoder
        {
            TS_NOBUILD_NOCOPY(Decoder);
        public:
            // The decoding is done in the constructor.
            Decoder(UString& str, const uint8_t* data, size_t size);

            // Get the decoding status.
            bool success() const {return _success;}

        private:
            bool           _success;
            UString&       _str;
            const uint8_t* _data;
            size_t         _size;
            const CharMap* _G0;
            const CharMap* _G1;
            const CharMap* _G2;
            const CharMap* _G3;
            const CharMap* _GL;       // current left character set
            const CharMap* _GR;       // current right character set
            const CharMap* _lockedGL; // locked left character set

            // Decode one character and append to str. Update data and size.
            bool decodeOneChar(const CharMap* gset);

            // Process an escape sequence starting at current byte.
            bool escape();

            // Check if next character matches c. If yes, update data and size.
            bool match(uint8_t c);

            // Get a character set from an ESC sequence "final byte" F.
            const CharMap* finalToCharMap(uint8_t f, bool gset_not_drcs) const;
        };
    };
}
