//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a TTML_subtitling_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a TTML_subtitling_descriptor.
    //! @see ETSI EN 303 560, clause 5.2.1.1.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL TTMLSubtitlingDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        UString                 language_code {};      //!< 24 bits, EN 303 560
        uint8_t                 subtitle_purpose = 0;  //!< 6 bits, EN 303 560
        uint8_t                 TTS_suitability = 0;   //!< 2 bits
        std::vector<uint8_t>    dvb_ttml_profile {};   //!< set of 8 bit values
        std::optional<uint32_t> qualifier {};          //!< 32 bits
        std::vector<uint8_t>    font_id {};            //!< set of 7 bit values
        UString                 service_name {};       //!< service name string
        size_t                  reserved_zero_future_use_bytes = 0;  //!< arbitrary number of zero value bytes - not specified in EN 303 560

        //!
        //! Default constructor.
        //!
        TTMLSubtitlingDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        TTMLSubtitlingDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    private:
        static UString TTML_qualifier(uint32_t qualifier);
        static UString TTML_subtitle_purpose(uint8_t purpose);
        static UString TTML_suitability(uint8_t suitability);

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
