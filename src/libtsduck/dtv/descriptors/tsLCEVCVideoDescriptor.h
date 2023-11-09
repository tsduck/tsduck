//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an LCEVC_video_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an HEVC_video_descriptor.
    //!
    //! @see ISO/IEC 13818-1 (Amd.1) 2.6.137, ITU-T Rec. H.222.0.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL LCEVCVideoDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint8_t lcevc_stream_tag = 0;                //!< 8 bits.
        uint8_t profile_idc = 0;                     //!< 4 bits.
        uint8_t level_idc = 0;                       //!< 4 bits.
        uint8_t sublevel_idc = 0;                    //!< 2 bits.
        bool    processed_planes_type_flag = false;  //!< bool.
        bool    picture_type_bit_flag = false;       //!< bool.
        bool    field_type_bit_flag = false;         //!< bool.
        uint8_t HDR_WCG_idc = 3;                     //!< 2 bits.
        uint8_t video_properties_tag = 0;            //!< 4 bits.

        //!
        //! Default constructor.
        //!
        LCEVCVideoDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        LCEVCVideoDescriptor(DuckContext& duck, const Descriptor& bin);

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
