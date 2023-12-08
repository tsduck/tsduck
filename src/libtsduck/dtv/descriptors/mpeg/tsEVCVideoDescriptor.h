//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an EVC_video_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an HEVC_video_descriptor.
    //!
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.133.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL EVCVideoDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint8_t   profile_idc = 0;                   //!< 8 bits.
        uint8_t   level_idc = 0;                     //!< 8 bits.
        uint32_t  toolset_idc_h = 0;                 //!< 32 bits.
        uint32_t  toolset_idc_l = 0;                 //!< 32 bits.
        bool      progressive_source = false;        //!< bool.
        bool      interlaced_source = false;         //!< bool.
        bool      non_packed_constraint = false;     //!< bool.
        bool      frame_only_constraint = false;     //!< bool.
        bool      EVC_still_present = false;         //!< bool.
        bool      EVC_24hr_picture_present = false;  //!< bool.
        uint8_t   HDR_WCG_idc = 0;                   //!< 2 bits.
        uint8_t   video_properties_tag = 0;          //!< 4 bits.
        std::optional<uint8_t> temporal_id_min {};   //!< 3 bits, optional, specify both min and max or neither.
        std::optional<uint8_t> temporal_id_max {};   //!< 3 bits, optional, specify both min and max or neither.

        //!
        //! Default constructor.
        //!
        EVCVideoDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        EVCVideoDescriptor(DuckContext& duck, const Descriptor& bin);

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
