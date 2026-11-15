//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2026, Paul Higgs
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
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL TTMLSubtitlingDescriptor: public AbstractDescriptor
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

        //!
        //! Get a string representation of a TTML qualifier.
        //! @param [in] qualifier The qualifier value from a TTML descriptor.
        //! @return The corresponding string.
        //!
        static UString TTML_qualifier(uint32_t qualifier);

        //!
        //! Get a string representation of a TTML purpose.
        //! @param [in] purpose The qualifier value from a TTML descriptor.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding string.
        //!
        static UString TTML_subtitle_purpose(uint8_t purpose, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Get a string representation of a TTML suitability.
        //! @param [in] suitability The suitability value from a TTML descriptor.
        //! @return The corresponding string.
        //!
        static UString TTML_suitability(uint8_t suitability);

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
