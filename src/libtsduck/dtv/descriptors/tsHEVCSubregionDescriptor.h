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
#include "tsVariable.h"
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
            std::vector<int8_t>    SubstreamOffset;            //!< offset to be added

            pattern_type();         //!< Constructor
        };

        //!
        //! The dfferent subregions defined by the descriptor. All Patterns must have the same number of values (offsets)
        //!
        class subregion_layout_type {
        public:
            Variable<uint8_t>           PreambleSubstreamID;    //!< 7 bits. Indicates the SubstreamID of the ES to be prepended to the ES to which this descriptor applies
            uint8_t                     Level;                  //!< Value of the profile as specified in ISO/IEC 23008-2 that applies to the subregion layout
            uint16_t                    PictureSizeHor;         //!< Horizontal subregion dimension, measured in pixels
            uint16_t                    PictureSizeVer;         //!< Vertical subregion dimension, measured in pixels
            std::vector<pattern_type>   Patterns;               //!< Patterns

            subregion_layout_type();    //!< Constructor
        };

        // Public members:
        uint8_t                             SubstreamIDsPerLine;    //!< 7 bits. The number of HEVC tile substreams that are coded representations of tiles that are arranged horizontally and span the width of the whole panorama.
        uint8_t                             TotalSubstreamIDs;      //!< Total number of HEVC tile substreams that represent tiles for the whole panorama.
        uint8_t                             LevelFullPanorama;      //!< Level of the profile as specified in ISO/IEC 23008-2 that applies to the whole panorama.
        std::vector<subregion_layout_type>  SubregionLayouts;       //!< Subregion layouts.

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
