//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an SVC_extension_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an SVC_extension_descriptor.
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.76.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL SVCExtensionDescriptor : public AbstractDescriptor
    {
    public:
        // SVCExtensionDescriptor public members:
        uint16_t width = 0;                        //!< In pixels
        uint16_t height = 0;                       //!< In pixels
        uint16_t frame_rate = 0;                   //!< In frames / 256 seconds
        uint16_t average_bitrate = 0;              //!< In kb/s
        uint16_t maximum_bitrate = 0;              //!< In kb/s
        uint8_t  dependency_id = 0;                //!< 3 bits
        uint8_t  quality_id_start = 0;             //!< 4 bits
        uint8_t  quality_id_end = 0;               //!< 4 bits
        uint8_t  temporal_id_start = 0;            //!< 3 bits
        uint8_t  temporal_id_end = 0;              //!< 3 bits
        bool     no_sei_nal_unit_present = false;  //!< 1 bit

        //!
        //! Default constructor.
        //!
        SVCExtensionDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        SVCExtensionDescriptor(DuckContext& duck, const Descriptor& bin);

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
