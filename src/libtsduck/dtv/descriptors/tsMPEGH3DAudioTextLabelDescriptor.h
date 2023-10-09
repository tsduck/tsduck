//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an MPEG-H_3dAudio_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsVariable.h"

namespace ts {
    //!
    //! Representation of an MPEG-H_3dAudio_descriptor.
    //!
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.112.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL MPEGH3DAudioTextLabelDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Group description.
        //!
        class groupDescription_type {
        public:
            uint8_t     mae_descriptionGroupID;     //!< 7 bits - ISO/IEC 13818-1, 2.6.113.
            UString     groupDescriptionData;       //!< variable length - ISO/IEC 13818-1, 2.6.113.

            groupDescription_type();                //!< Constructor.
        };
        //!
        //! Switch group description.
        //!
        class switchGroupDescription_type {
        public:
            uint8_t     mae_descriptionSwitchGroupID;     //!< 5 bits - ISO/IEC 13818-1, 2.6.113.
            UString     switchGroupDescriptionData;       //!< variable length - ISO/IEC 13818-1, 2.6.113.

            switchGroupDescription_type();                //!< Constructor.
        };
        //!
        //! Group presets description.
        //!
        class groupPresetsDescription_type {
        public:
            uint8_t     mae_descriptionGroupPresetID;     //!< 5 bits - ISO/IEC 13818-1, 2.6.113.
            UString     groupDescriptionPresetData;       //!< variable length - ISO/IEC 13818-1, 2.6.113.

            groupPresetsDescription_type();               //!< Constructor.
        };
        //!
        //! Description language.
        //!
        class descriptionLanguage_type {
        public:
            UString                                   descriptionLanguage;         //!< 3 byte language code - ISO/IEC 13818-1, 2.6.113.
            std::vector<groupDescription_type>        group_descriptions;          //!< ISO/IEC 13818-1, 2.6.113.
            std::vector<switchGroupDescription_type>  switch_group_descriptions;   //!< ISO/IEC 13818-1, 2.6.113.
            std::vector<groupPresetsDescription_type> group_preset_descriptions;   //!< ISO/IEC 13818-1, 2.6.113.

            descriptionLanguage_type();               //!< Constructor.
        };

        // Public members:
        uint8_t                                 _3dAudioSceneInfoID;     //!< 8 bits - ISO/IEC 13818-1, 2.6.107.
        std::vector<descriptionLanguage_type>   description_languages;   //!< ISO/IEC 13818-1, 2.6.107.
        Variable<size_t>                        numReservedBytes;        //!< Additional bytes.

        //!
        //! Default constructor.
        //!
        MPEGH3DAudioTextLabelDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        MPEGH3DAudioTextLabelDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual DID extendedTag() const override;
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
