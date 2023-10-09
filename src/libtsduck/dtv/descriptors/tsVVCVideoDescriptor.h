//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an VVC_video_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsVariable.h"

namespace ts {
    //!
    //! Representation of a VVC_video_descriptor.
    //!
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.129.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL VVCVideoDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint8_t                 profile_idc;               //!< 7 bits. ISO/IEC 13818-1 clause 2.6.130 and ISO/IEC 23090-3.
        bool                    tier;                      //!< ISO/IEC 13818-1 clause 2.6.130 and ISO/IEC 23090-3.
        std::vector<uint32_t>   sub_profile_idc;           //!< ISO/IEC 13818-1 clause 2.6.130 and ISO/IEC 23090-3.
        bool                    progressive_source;        //!< ISO/IEC 13818-1 clause 2.6.130 and ISO/IEC 230902-7.
        bool                    interlaced_source;         //!< ISO/IEC 13818-1 clause 2.6.130 and ISO/IEC 230902-7.
        bool                    non_packed_constraint;     //!< ISO/IEC 13818-1 clause 2.6.130 and ISO/IEC 230902-7.
        bool                    frame_only_constraint;     //!< ISO/IEC 13818-1 clause 2.6.130 and ISO/IEC 23090-3.
        uint8_t                 level_idc;                 //!< ISO/IEC 13818-1 clause 2.6.130 and ISO/IEC 23090-3.
        bool                    VVC_still_present;         //!< Indicates that the VVC video stream may include VVC still pictures.
        bool                    VVC_24hr_picture_present;  //!< Indicates that the VVC video stream may contain VVC 24-hour pictures (an AU with a presentation time more than 24 hours in the future.
        uint8_t                 HDR_WCG_idc;               //!< 2 bits. Indicates the presence of absence of HDR and WCG video components in the VVC video stream.
        uint8_t                 video_properties_tag;      //!< 4 bits. Indicates specific widely used video roperty CICP combinations.
        Variable<uint8_t>       temporal_id_min;           //!< 3 bits, optional, specify both min and max or neither.
        Variable<uint8_t>       temporal_id_max;           //!< 3 bits, optional, specify both min and max or neither.

        //!
        //! Default constructor.
        //!
        VVCVideoDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        VVCVideoDescriptor(DuckContext& duck, const Descriptor& bin);

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
