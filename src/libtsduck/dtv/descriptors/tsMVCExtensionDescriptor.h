//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an MVC_extension_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an MVC_extension_descriptor.
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.78.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL MVCExtensionDescriptor : public AbstractDescriptor
    {
    public:
        // MVCExtensionDescriptor public members:
        uint16_t average_bitrate = 0;                   //!< In kb/s
        uint16_t maximum_bitrate = 0;                   //!< In kb/s
        bool     view_association_not_present = false;  //!< 1 bit
        bool     base_view_is_left_eyeview = false;     //!< 1 bit
        uint16_t view_order_index_min = 0;              //!< 10 bit
        uint16_t view_order_index_max = 0;              //!< 10 bit
        uint8_t  temporal_id_start = 0;                 //!< 3 bits
        uint8_t  temporal_id_end = 0;                   //!< 3 bits
        bool     no_sei_nal_unit_present = false;       //!< 1 bit
        bool     no_prefix_nal_unit_present = false;    //!< 1 bit

        //!
        //! Default constructor.
        //!
        MVCExtensionDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        MVCExtensionDescriptor(DuckContext& duck, const Descriptor& bin);

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
