//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a J2K_video_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a J2K_video_descriptor.
    //!
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.80.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL J2KVideoDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint16_t  profile_and_level = 0;     //!< Same as J2K concept.
        uint32_t  horizontal_size = 0;       //!< Same as J2K concept.
        uint32_t  vertical_size = 0;         //!< Same as J2K concept.
        uint32_t  max_bit_rate = 0;          //!< Same as J2K concept.
        uint32_t  max_buffer_size = 0;       //!< Same as J2K concept.
        uint16_t  DEN_frame_rate = 0;        //!< Same as J2K concept.
        uint16_t  NUM_frame_rate = 0;        //!< Same as J2K concept.
        uint8_t   color_specification = 0;   //!< Same as J2K concept.
        bool      still_mode = false;        //!< Same as J2K concept.
        bool      interlaced_video = false;  //!< Same as J2K concept.
        ByteBlock private_data {};           //!< Private data

        //!
        //! Default constructor.
        //!
        J2KVideoDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        J2KVideoDescriptor(DuckContext& duck, const Descriptor& bin);

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
