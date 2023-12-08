//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB audio_component_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an ISDB audio_component_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.26
    //! @ingroup descriptor
    //!
    class TSDUCKDLL AudioComponentDescriptor : public AbstractDescriptor
    {
    public:
        // AudioComponentDescriptor public members:
        uint8_t stream_content = 2;          //!< 4 bits, 0x02 by default (audio content).
        uint8_t component_type = 0;          //!< Component type.
        uint8_t component_tag = 0;           //!< Component tag.
        uint8_t stream_type = 0;             //!< Stream type.
        uint8_t simulcast_group_tag = 0xFF;  //!< Group tag, 0xFF means no simulcast
        bool    main_component = true;       //!< Is main audio component.
        uint8_t quality_indicator = 0;       //!< 2 bits, quality indicator.
        uint8_t sampling_rate = 0;           //!< 3 bits, sampling rate.
        UString ISO_639_language_code {};    //!< 3-character language code.
        UString ISO_639_language_code_2 {};  //!< 3-character optional secondary language code.
        UString text {};                     //!< Component description.

        //!
        //! Default constructor.
        //!
        AudioComponentDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        AudioComponentDescriptor(DuckContext& duck, const Descriptor& bin);

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
