//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC E-AC-3_audio_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an ATSC E-AC-3_audio_descriptor.
    //! @see ATSC A/52, G.3.5.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ATSCEAC3AudioDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        bool                   mixinfoexists = false;   //!< See A/52, G.3.5.
        bool                   full_service = false;    //!< See A/52, G.3.5.
        uint8_t                audio_service_type = 0;  //!< 3 bits, see A/52, G.3.5.
        uint8_t                number_of_channels = 0;  //!< 3 bits, see A/52, G.3.5.
        std::optional<uint8_t> bsid {};                 //!< 5 bits, see A/52, G.3.5.
        std::optional<uint8_t> priority {};             //!< 2 bits, see A/52, G.3.5.
        std::optional<uint8_t> mainid {};               //!< 3 bits, see A/52, G.3.5.
        std::optional<uint8_t> asvc {};                 //!< See A/52, G.3.5.
        std::optional<uint8_t> substream1 {};           //!< See A/52, G.3.5.
        std::optional<uint8_t> substream2 {};           //!< See A/52, G.3.5.
        std::optional<uint8_t> substream3 {};           //!< See A/52, G.3.5.
        UString                language {};             //!< 3 chars, see A/52, G.3.5.
        UString                language_2 {};           //!< 3 chars, see A/52, G.3.5.
        UString                substream1_lang {};      //!< 3 chars, see A/52, G.3.5.
        UString                substream2_lang {};      //!< 3 chars, see A/52, G.3.5.
        UString                substream3_lang {};      //!< 3 chars, see A/52, G.3.5.
        ByteBlock              additional_info {};      //!< See A/52, G.3.5.

        //!
        //! Default constructor.
        //!
        ATSCEAC3AudioDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ATSCEAC3AudioDescriptor(DuckContext& duck, const Descriptor& bin);

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
