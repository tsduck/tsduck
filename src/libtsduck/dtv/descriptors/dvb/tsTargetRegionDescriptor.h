//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a target_region_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a target_region_descriptor
    //! @see ETSI EN 300 468, 6.4.12.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL TargetRegionDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Region entry.
        //!
        struct TSDUCKDLL Region
        {
            Region() = default;                  //!< Default constructor.
            UString  country_code {};            //!< Optional 3-character country code.
            uint8_t  region_depth = 0;           //!< 2 bits, number of region codes.
            uint8_t  primary_region_code = 0;    //!< Optional primary region code.
            uint8_t  secondary_region_code = 0;  //!< Optional secondary region code.
            uint16_t tertiary_region_code = 0;   //!< Optional tertiary region code.
        };

        //!
        //! List of Region entries.
        //!
        typedef std::list<Region> RegionList;

        // TargetRegionDescriptor public members:
        UString    country_code {};  //!< 3-character country code.
        RegionList regions {};       //!< The list of regions.

        //!
        //! Default constructor.
        //!
        TargetRegionDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        TargetRegionDescriptor(DuckContext& duck, const Descriptor& bin);

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
