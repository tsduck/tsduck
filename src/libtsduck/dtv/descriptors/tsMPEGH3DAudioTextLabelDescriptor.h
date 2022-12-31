//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
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
