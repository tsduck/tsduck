//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an MPEGH_3D_audio_drc_loudness_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an MPEGH_3D_audio_drc_loudness_descriptor.
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.116.
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL MPEGH3DAudioDRCLoudnessDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Dynamic Range Control instructions class
        //!
        class TSDUCKDLL DRCInstructions
        {
            TS_DEFAULT_COPY_MOVE(DRCInstructions);
        public:
            uint8_t                drcInstructionsType = 0;   //!< 2 bits
            std::optional<uint8_t> mae_groupID {};            //!< 7 bits, required when drcInstructionsType == 2
            std::optional<uint8_t> mae_groupPresetID {};      //!< 5 bits, required when drcInstructionsType == 3
            uint8_t                drcSetId = 0;              //!< 2 bits
            uint8_t                downmixId = 0;             //!< 7 bits
            std::vector<uint8_t>   additionalDownmixId {};    //!< 7 bits
            uint16_t               drcSetEffect = 0;          //!< 16 bits
            std::optional<uint8_t> bsLimiterPeakTarget {};    //!< 8 bits
            std::optional<uint8_t> bsDrcSetTargetLoudnessValueUpper {};  //!< 6 bits
            std::optional<uint8_t> bsDrcSetTargetLoudnessValueLower {};  //!< 6 bits
            uint8_t                dependsOnDrcSet = 0;       //!< 6 bits
            bool                   noIndependentUse = false;  //!< 1 bit, required when dependsOnDrcSet == 0"/>

            //! @cond nodoxygen
            DRCInstructions() = default;
            void clear();
            void serialize(PSIBuffer&) const;
            void deserialize(PSIBuffer&);
            void toXML(xml::Element*) const;
            bool fromXML(const xml::Element*);
            static void Display(TablesDisplay&, PSIBuffer&, const UString&);
            //! @endcond
        };

        //!
        //! Loudness info class
        //!
        class TSDUCKDLL LoudnessInfo
        {
            TS_DEFAULT_COPY_MOVE(LoudnessInfo);
        public:
            uint8_t                loudnessInfoType = 0;  //!< 2 bits
            std::optional<uint8_t> mae_groupID {};        //!< 7 bits, required when loudnessInfoType == 1 || loudnessInfoType == 2
            std::optional<uint8_t> mae_groupPresetID {};  //!< 5 bits, required when loudnessInfoType == 3
            ByteBlock              loudnessInfo {};       //!< loudnessInfo() structure as defined in ISO/IEC 23003-4

            //! @cond nodoxygen
            LoudnessInfo() = default;
            void clear();
            void serialize(PSIBuffer&) const;
            void deserialize(PSIBuffer&);
            void toXML(xml::Element*) const;
            bool fromXML(const xml::Element*);
            static void Display(TablesDisplay&, PSIBuffer&, const UString&);
            //! @endcond
        };

        //!
        //! Downmix id class
        //!
        class TSDUCKDLL DownmixId
        {
            TS_DEFAULT_COPY_MOVE(DownmixId);
        public:
            uint8_t downmixId = 0;             //!< 7 bits
            uint8_t downmixType = 0;           //!< 2 bits
            uint8_t CICPspeakerLayoutIdx = 0;  //!< 6 bits

            //! @cond nodoxygen
            DownmixId() = default;
            void serialize(PSIBuffer&) const;
            void deserialize(PSIBuffer&);
            void toXML(xml::Element*) const;
            bool fromXML(const xml::Element*);
            static void Display(TablesDisplay&, PSIBuffer&, const UString&);
            //! @endcond
        };

        // MPEGH3DAudioDRCLoudnessDescriptor public members:
        std::vector<DRCInstructions> drcInstructionsUniDrc {};  //!< Any number of drcInstructionsUniDrc
        std::vector<LoudnessInfo>    loudnessInfo {};           //!< Any number of loudnessInfo
        std::vector<DownmixId>       downmixId {};              //!< Any number of downmixId
        ByteBlock                    reserved {};               //!< Reserved data.

        //!
        //! Default constructor.
        //!
        MPEGH3DAudioDRCLoudnessDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        MPEGH3DAudioDRCLoudnessDescriptor(DuckContext& duck, const Descriptor& bin);

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
