//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Implementation of a Teletext character set.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUChar.h"

namespace ts {
    //!
    //! Implementation of a Teletext character set.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL TeletextCharset
    {
    public:
        //!
        //! Constructor.
        //!
        TeletextCharset();

        //!
        //! Check parity and translate any reasonable Teletext character into UCS-2.
        //! @param [in] chr Teletext character.
        //! @return UCS-2 equivalent.
        //!
        UChar teletextToUcs2(uint8_t chr) const;

        //!
        //! Translate a G2 character into UCS-2.
        //! @param [in] chr Teletext character.
        //! @param [in] accent Accent mode (0 to 14) if @a c is a letter.
        //! @return UCS-2 equivalent.
        //!
        UChar g2AccentToUcs2(uint8_t chr, uint8_t accent) const;

        //!
        //! Translate a G2 character into UCS-2.
        //! @param [in] chr Teletext character.
        //! @return UCS-2 equivalent.
        //!
        UChar g2ToUcs2(uint8_t chr) const;

        //!
        //! Set default G0 character set.
        //! @param [in] triplet Charset triplet.
        //!
        void setG0Charset(uint32_t triplet);

        //!
        //! Set the X/28 character set and use it as current character set.
        //! @param [in] charset Charactet set index.
        //!
        void setX28(uint8_t charset);

        //!
        //! Set the M/29 character set and use it as current character set if X/28 is not defined.
        //! @param [in] charset Charactet set index.
        //!
        void setM29(uint8_t charset);

        //!
        //! Reset the X/28 character set to undefined.
        //! Use M/29 as current character or use @a fallback if M/29 is undefined.
        //! @param [in] fallback Fallback charactet set index if M/29 is undefined.
        //!
        void resetX28(uint8_t fallback);

        //!
        //! Number of characters per charset.
        //!
        static constexpr size_t CHAR_COUNT = 96;

    private:
        //!
        //! Index of G0 character sets in G0 table.
        //!
        enum G0CharsetIndex {
            LATIN     = 0,
            CYRILLIC1 = 1,
            CYRILLIC2 = 2,
            CYRILLIC3 = 3,
            GREEK     = 4,
            ARABIC    = 5,
            HEBREW    = 6,
            G0_COUNT  = 7
        };

        //!
        //! G0 charsets data.
        //!
        typedef UChar G0CharsetData[G0_COUNT][CHAR_COUNT];

        //!
        //! Initial base content of a charset.
        //! Charset can be -- and always is -- changed during transmission.
        //!
        static const G0CharsetData G0Base;

        //!
        //! Undefined charset index.
        //!
        static constexpr uint8_t UNDEFINED = 0xFF;

        //!
        //! Remap the GO character set.
        //! @param [in] charset Charactet set index.
        //!
        void remapG0(uint8_t charset);

        // Private members.
        uint8_t        _current;   //!< Current charset index.
        uint8_t        _g0m29;     //!< M/29 charset.
        uint8_t        _g0x28;     //!< X/28 charset.
        G0CharsetIndex _g0Default; //!< Default G0 index.
        G0CharsetData  _G0;        //!< Current character set data.
    };
}
