//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a maximum_bitrate_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a maximum_bitrate_descriptor.
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.26.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL MaximumBitrateDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint32_t maximum_bitrate = 0;  //!< 22 bits, maximum bitrate in units of 50 bytes/second.

        //!
        //! Unit of the @a maximum_bitrate field in bits/second.
        //!
        static const uint32_t BITRATE_UNIT = 50 * 8;

        //!
        //! Default constructor.
        //! @param [in] mbr Maximum bitrate.
        //!
        explicit MaximumBitrateDescriptor(uint32_t mbr = 0);

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        MaximumBitrateDescriptor(DuckContext& duck, const Descriptor& bin);

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
