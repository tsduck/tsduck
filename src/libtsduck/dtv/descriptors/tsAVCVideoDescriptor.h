//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an AVC_video_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an AVC_video_descriptor.
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.64.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL AVCVideoDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint8_t profile_idc = 0;              //!< Same as AVC concept.
        bool    constraint_set0 = false;      //!< Same as AVC concept.
        bool    constraint_set1 = false;      //!< Same as AVC concept.
        bool    constraint_set2 = false;      //!< Same as AVC concept.
        bool    constraint_set3 = false;      //!< Same as AVC concept.
        bool    constraint_set4 = false;      //!< Same as AVC concept.
        bool    constraint_set5 = false;      //!< Same as AVC concept.
        uint8_t AVC_compatible_flags = 0;     //!< 2 bits.
        uint8_t level_idc = 0;                //!< Same as AVC concept.
        bool    AVC_still_present = false;    //!< May contain still pictures.
        bool    AVC_24_hour_picture = false;  //!< May contain 24-hour pictures.
        bool    frame_packing_SEI_not_present = false;  //!< Same as AVC concept.

        //!
        //! Default constructor.
        //!
        AVCVideoDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        AVCVideoDescriptor(DuckContext& duck, const Descriptor& bin);

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
