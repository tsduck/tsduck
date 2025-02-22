//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an SCTE 35 audio_descriptor (SIT specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsSCTE35.h"

namespace ts {
    //!
    //! Representation of an SCTE 35 audio_descriptor (SIT specific).
    //!
    //! This descriptor cannot be present in other tables than an Splice
    //! Information Table (SIT) because its tag reuses an MPEG-defined one.
    //!
    //! @see SCTE 35, 10.3.5
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL SpliceAudioDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! An audio item entry.
        //!
        class TSDUCKDLL Audio
        {
        public:
            uint8_t component_tag = 0;        //!< Component tag, 8 bits.
            UString ISO_code {};              //!< ISO-639 language code, 3 characters.
            uint8_t Bit_Stream_Mode = 0;      //!< As per ATSC A/52 Table 5.7, 3 bits.
            uint8_t Num_Channels = 0;         //!< As per ATSC A/52 Table A4.5, 4 bits.
            bool    Full_Srvc_Audio = false;  //!< As per ATSC A/52 Annex A.4.3.
        };

        //!
        //! Maximum number of audio entries to fit in 255 bytes.
        //!
        static constexpr size_t MAX_ENTRIES = 49;

        // SpliceAudioDescriptor public members:
        uint32_t identifier = SPLICE_ID_CUEI;  //!< Descriptor owner, 0x43554549 ("CUEI").
        std::vector<Audio> audio {};           //!< Audio entries.

        //!
        //! Default constructor.
        //!
        SpliceAudioDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        SpliceAudioDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
