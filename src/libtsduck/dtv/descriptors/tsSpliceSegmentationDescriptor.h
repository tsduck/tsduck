//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//!  Representation of an SCTE 35 segmentation_descriptor (SIT specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"
#include "tsVariable.h"

namespace ts {
    //!
    //! Representation of an SCTE 35 segmentation_descriptor (SIT specific).
    //!
    //! This descriptor cannot be present in other tables than an Splice
    //! Information Table (SIT) because its tag reuses an MPEG-defined one.
    //!
    //! @see SCTE 35, 10.3.3
    //! @ingroup descriptor
    //!
    class TSDUCKDLL SpliceSegmentationDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! A map of 33-bit PTS offset values, indexed by 8-bit component tags.
        //!
        typedef std::map<uint8_t, uint64_t> PTSOffsetByComponent;

        // SpliceSegmentationDescriptor public members:
        uint32_t  identifier;                  //!< Descriptor owner, 0x43554549 ("CUEI").
        uint32_t  segmentation_event_id;       //!< Event id.
        bool      segmentation_event_cancel;   //!< When true, event is canceled, other fields are ignored.
        bool      program_segmentation;        //!< When true, all components are segmented.
        bool      web_delivery_allowed;        //!< When true, web delivery is allowed.
        bool      no_regional_blackout;        //!< When true, no regional blackout is applied.
        bool      archive_allowed;             //!< When true, recording is allowed.
        uint8_t   device_restrictions;         //!< 2 bits code
        PTSOffsetByComponent pts_offsets;            //!< PTS offsets, indexed by component tag.
        Variable<uint64_t>   segmentation_duration;  //!< 40 bits, in PTS units.
        uint8_t   segmentation_upid_type;      //!< Segmentation upid type.
        ByteBlock segmentation_upid;           //!< Segmentation upid value.
        uint8_t   segmentation_type_id;        //!< Segmentation type.
        uint8_t   segment_num;                 //!< Segment number.
        uint8_t   segments_expected;           //!< Expected number of segments.
        uint8_t   sub_segment_num;             //!< Sub-segment number (if segmentation_type_id == 0x34 or 0x36).
        uint8_t   sub_segments_expected;       //!< Expected number of sub-segments (if segmentation_type_id == 0x34 or 0x36).

        //!
        //! Default constructor.
        //!
        SpliceSegmentationDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        SpliceSegmentationDescriptor(DuckContext& duck, const Descriptor& bin);

        //!
        //! Rebuild the delivery_not_restricted flag.
        //! @return Value of the delivery_not_restricted flag.
        //!
        bool deliveryNotRestricted() const;

        //!
        //! Check if the signal is an in.
        //! @return Value based on the segmentation_type_id.
        //!
        bool isIn() const;

        //!
        //! Check if the signal is an out.
        //! @return Value based on the segmentation_type_id.
        //!
        bool isOut() const;

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
