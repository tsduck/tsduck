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
