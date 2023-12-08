//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an Mastering Display Meta
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsPSIBuffer.h"
#include "tsTablesDisplay.h"
#include "tsxmlElement.h"

namespace ts {
    //!
    //! Representation of Mastering Display Metadata, used by several descriptors.
    //!
    class TSDUCKDLL Mastering_Display_Metadata_type
    {
        TS_DEFAULT_COPY_MOVE(Mastering_Display_Metadata_type);
    public:
        uint16_t X_c0 = 0;         //!< 16 bits. Normalized X chromaticity coorinates for green pimary - see SMPTE ST2086:2014
        uint16_t Y_c0 = 0;         //!< 16 bits. Normalized Y chromaticity coorinates for green pimary - see SMPTE ST2086:2014
        uint16_t X_c1 = 0;         //!< 16 bits. Normalized X chromaticity coorinates for blue pimary - see SMPTE ST2086:2014
        uint16_t Y_c1 = 0;         //!< 16 bits. Normalized Y chromaticity coorinates for blue pimary - see SMPTE ST2086:2014
        uint16_t X_c2 = 0;         //!< 16 bits. Normalized X chromaticity coorinates for red pimary - see SMPTE ST2086:2014
        uint16_t Y_c2 = 0;         //!< 16 bits. Normalized Y chromaticity coorinates for red pimary - see SMPTE ST2086:2014
        uint16_t X_wp = 0;         //!< 16 bits. Normalized X chromaticity coordinate of the white point - see SMPTE ST2086:2014
        uint16_t Y_wp = 0;         //!< 16 bits. Normalized Y chromaticity coordinate of the white point - see SMPTE ST2086:2014
        uint32_t L_max = 0;        //!< 32 bits. Nominal maximum display luminance - see SMPTE ST2086:2014
        uint32_t L_min = 0;        //!< 32 bits. Nominal minimum display luminance - see SMPTE ST2086:2014
        uint16_t MaxCLL = 0;       //!< 16 bits. Maximum Content Light Level - see ANSI/CTA 861-G:2016
        uint16_t MaxFALL = 0;      //!< 16 bits. Maximum Frame Average Light Level - see ANSI/CTA 861-G:2016

        //!
        //! Default constructor.
        //!
        Mastering_Display_Metadata_type();

        //!
        //! Read-in constructor.
        //! @param [in,out] buf Deserialization buffer. Read the descriptor payload from
        //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
        //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
        //!
        Mastering_Display_Metadata_type(PSIBuffer& buf) : Mastering_Display_Metadata_type() { deserialize(buf); }

        //! @cond nodoxygen
        void clearContent();
        void serialize(PSIBuffer&) const;
        void deserialize(PSIBuffer&);
        void toXML(xml::Element*) const;
        bool fromXML(const xml::Element*);
        void display(TablesDisplay&, PSIBuffer&, const UString&);
        //! @endcond
    };
}
