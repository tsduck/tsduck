//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
            uint8_t Flag = 0;
            //!
            //! 7 bits. Indicates an additional SubstreamID that belongs to the subregion when the reassembly process according to
            //! "Carriage of HEVC motion-constrained tile sets as separate elementary streams" or "Carriage of HEVC motion-constrained tile sets in a
            //! common ES using AF descriptors" is executed.
            //!
            uint8_t AdditionalSubstreamID = 0;
            //!
            //! Constructor
            //!
            substream_type() = default;
        };

        //!
        //! 7 bits. A number in the range of 1 to TotalSubstreamIDs (as found) to the HEVC tile substream, which is unique among all ESs with
        //! stream type equal to 0x31 or 0x24 (the latter in case that multiple substreams are carried in a single ES) that belong to the same
        //! program. This value is used to identify each HEVC tile substream. The value 0 is used for substreams that contain information
        //! applicable to multiple HEVC tile substreams.
        //!
        uint8_t SubstreamID = 0;
        //!
        //! 1 bit. When set to '1' and each HEVC tile substream is carried in its own ES (signalled by the SubstreamMarkingFlag in the HEVC
        //! subregion descriptor set to '1'), an access unit carried in the ES signalled by the value of PreambleSubstreamID in the HEVC subregion descriptor
        //! is prepended before an access unit carried in this ES.
        //! In case SubstreamMarkingFlag in the HEVC subregion descriptor set to '0', the semantics of this flag is reserved.
        //!
        std::optional<uint8_t> PreambleFlag {};
        //!
        //! 7 bits. A number in the range of 1 to PatternCount (as found in the HEVC subregion descriptor) to the HEVC tile substream.
        //! This number is used as an index j to the array SubstreamOffset[k][j][i] when the reassembly process is executed.
        //!
        std::optional<uint8_t> PatternReference {};
        //!
        //! Array of 7 bit fields indicates additional SubstreamIDs that belong to the subregion when the reassembly process according
        //! to "Carriage of HEVC motion-constrained tile sets as separate elementary streams" or "Carriage of HEVC motion-constrained
        //! tile sets in a common ES using AF descriptors "is executed.
        std::vector<substream_type> Substreams {};

    private:
        //!
        //! 1 bit. When set to '1', this descriptor indicates the index j > 0 of the pattern signalled by the HEVC subregion descriptor
        //! to be used to calculate additional SubstreamIDs. If the value of the field descriptor_length specifying the size of this
        //! descriptor excluding preceding header bytes is 1, the value of ReferenceFlag is reserved.
        //!
        uint8_t ReferenceFlag = 1;

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
