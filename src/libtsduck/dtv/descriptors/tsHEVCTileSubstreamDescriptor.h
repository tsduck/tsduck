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
//!  Representation of an HEVC_tile_substream_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

namespace ts {
    //!
    //! Representation of an HEVC_tile_substream_descriptor.
    //!
    //! @see ISO/IEC 13818-1 clasue 2.6.122
    //! @ingroup descriptor
    //!
    class TSDUCKDLL HEVCTileSubstreamDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        //!
        //! Indicates the the additional substream that belongs to the subregiion and identifies its delivery mechanism
        //!
        class substream_type {
        public:
            //!
            //! 1 bit. When Flag[0] is set to '1' and each HEVC tile substream is carried in its own ES (signalled by the SubstreamMarkingFlag in the
            //! HEVC subregion descriptor set to '1'), an additional ES signalled by the value of PreambleSubstreamID in the HEVC subregion descriptor
            //! is prepended before this ES. In other cases, the semantics of Flag[0] is reserved. Flag[i] for values of i > 0 is reserved.
            //!
            uint8_t     Flag;
            //!
            //! 7 bits. Indicates an additional SubstreamID that belongs to the subregion when the reassembly process according to
            //! "Carriage of HEVC motion-constrained tile sets as separate elementary streams" or "Carriage of HEVC motion-constrained tile sets in a
            //! common ES using AF descriptors" is executed.
            uint8_t     AdditionalSubstreamID;

            substream_type();     //!< Constructor
        };

        //!
        //! 7 bits. A number in the range of 1 to TotalSubstreamIDs (as found) to the HEVC tile substream, which is unique among all ESs with
        //! stream type equal to 0x31 or 0x24 (the latter in case that multiple substreams are carried in a single ES) that belong to the same
        //! program. This value is used to identify each HEVC tile substream. The value 0 is used for substreams that contain information
        //! applicable to multiple HEVC tile substreams.
        //!
        uint8_t                     SubstreamID;
        //!
        //! 1 bit. When set to '1' and each HEVC tile substream is carried in its own ES (signalled by the SubstreamMarkingFlag in the HEVC
        //! subregion descriptor set to '1'), an access unit carried in the ES signalled by the value of PreambleSubstreamID in the HEVC subregion descriptor
        //! is prepended before an access unit carried in this ES.
        //! In case SubstreamMarkingFlag in the HEVC subregion descriptor set to '0', the semantics of this flag is reserved.
        //!
        Variable<uint8_t>           PreambleFlag;
        //!
        //! 7 bits. A number in the range of 1 to PatternCount (as found in the HEVC subregion descriptor) to the HEVC tile substream.
        //! This number is used as an index j to the array SubstreamOffset[k][j][i] when the reassembly process is executed.
        //!
        Variable<uint8_t>           PatternReference;
        //!
        //! Array of 7 bit fields indicates additional SubstreamIDs that belong to the subregion when the reassembly process according
        //! to "Carriage of HEVC motion-constrained tile sets as separate elementary streams" or "Carriage of HEVC motion-constrained
        //! tile sets in a common ES using AF descriptors "is executed.
        std::vector<substream_type> Substreams;


    private:
        //!
        //! 1 bit. When set to '1', this descriptor indicates the index j > 0 of the pattern signalled by the HEVC subregion descriptor
        //! to be used to calculate additional SubstreamIDs. If the value of the field descriptor_length specifying the size of this
        //! descriptor excluding preceding header bytes is 1, the value of ReferenceFlag is reserved.
        uint8_t ReferenceFlag;

    public:

        //!
        //! Default constructor.
        //!
        HEVCTileSubstreamDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        HEVCTileSubstreamDescriptor(DuckContext& duck, const Descriptor& bin);

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
