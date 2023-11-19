//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an HEVC_subregion_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

namespace ts {
    //!
    //! Representation of an HEVC_subregion_descriptor.
    //!
    //! @see ISO/IEC 13818-1 clause 2.6.138.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL HEVCSubregionDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! A pattern within a subregion that is a array of offet values to be applied
        //!
        class pattern_type {
        public:
            pattern_type() = default;                //!< Constructor
            std::vector<int8_t> SubstreamOffset {};  //!< offset to be added
        };

        //!
        //! The dfferent subregions defined by the descriptor. All Patterns must have the same number of values (offsets)
        //!
        class subregion_layout_type {
        public:
            subregion_layout_type() = default;                 //!< Constructor
            std::optional<uint8_t>    PreambleSubstreamID {};  //!< 7 bits. Indicates the SubstreamID of the ES to be prepended to the ES to which this descriptor applies
            uint8_t                   Level = 0;               //!< Value of the profile as specified in ISO/IEC 23008-2 that applies to the subregion layout
            uint16_t                  PictureSizeHor = 0;      //!< Horizontal subregion dimension, measured in pixels
            uint16_t                  PictureSizeVer = 0;      //!< Vertical subregion dimension, measured in pixels
            std::vector<pattern_type> Patterns {};             //!< Patterns
        };

        // Public members:
        uint8_t                             SubstreamIDsPerLine = 0;  //!< 7 bits. The number of HEVC tile substreams that are coded representations of tiles that are arranged horizontally and span the width of the whole panorama.
        uint8_t                             TotalSubstreamIDs = 0;    //!< Total number of HEVC tile substreams that represent tiles for the whole panorama.
        uint8_t                             LevelFullPanorama = 0;    //!< Level of the profile as specified in ISO/IEC 23008-2 that applies to the whole panorama.
        std::vector<subregion_layout_type>  SubregionLayouts {};      //!< Subregion layouts.

        //!
        //! Default constructor.
        //!
        HEVCSubregionDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        HEVCSubregionDescriptor(DuckContext& duck, const Descriptor& bin);

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
