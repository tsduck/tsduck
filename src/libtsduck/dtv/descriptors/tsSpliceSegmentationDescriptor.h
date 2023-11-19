//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
#include "tsSCTE35.h"

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
        uint32_t  identifier = SPLICE_ID_CUEI;             //!< Descriptor owner, 0x43554549 ("CUEI").
        uint32_t  segmentation_event_id = 0;               //!< Event id.
        bool      segmentation_event_cancel = false;       //!< When true, event is canceled, other fields are ignored.
        bool      program_segmentation = false;            //!< When true, all components are segmented.
        bool      web_delivery_allowed = false;            //!< When true, web delivery is allowed.
        bool      no_regional_blackout = false;            //!< When true, no regional blackout is applied.
        bool      archive_allowed = false;                 //!< When true, recording is allowed.
        uint8_t   device_restrictions = 0;                 //!< 2 bits code
        PTSOffsetByComponent    pts_offsets {};            //!< PTS offsets, indexed by component tag.
        std::optional<uint64_t> segmentation_duration {};  //!< 40 bits, in PTS units.
        uint8_t   segmentation_upid_type = 0;              //!< Segmentation upid type.
        ByteBlock segmentation_upid {};                    //!< Segmentation upid value.
        uint8_t   segmentation_type_id = 0;                //!< Segmentation type.
        uint8_t   segment_num = 0;                         //!< Segment number.
        uint8_t   segments_expected = 0;                   //!< Expected number of segments.
        uint8_t   sub_segment_num = 0;                     //!< Sub-segment number (if segmentation_type_id == 0x34 or 0x36).
        uint8_t   sub_segments_expected = 0;               //!< Expected number of sub-segments (if segmentation_type_id == 0x34 or 0x36).

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
