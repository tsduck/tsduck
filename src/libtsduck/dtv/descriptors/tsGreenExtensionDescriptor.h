//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an MPEG-defined green_extension_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an MPEG-defined green_extension_descriptor.
    //!
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.104.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL GreenExtensionDescriptor : public AbstractDescriptor
    {
    public:
        // GreenExtensionDescriptor public members:
        std::vector<uint16_t> constant_backlight_voltage_time_intervals {};  //!< Specified in 6.4 of ISO/IEC 23001-11.
        std::vector<uint16_t> max_variations {};                             //!< Specified in 6.4 of ISO/IEC 23001-11.

        //!
        //! Maximum number of elements in each array (count must fit on 2 bits).
        //!
        static constexpr size_t MAX_COUNT = 3;

        //!
        //! Default constructor.
        //!
        GreenExtensionDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        GreenExtensionDescriptor(DuckContext& duck, const Descriptor& bin);

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
