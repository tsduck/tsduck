//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a hierarchy_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a hierarchy_descriptor.
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.6.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL HierarchyDescriptor : public AbstractDescriptor
    {
    public:
        // HierarchyDescriptor public members:
        bool    no_view_scalability_flag = false;    //!< No view scalability.
        bool    no_temporal_scalability = false;     //!< No temporal scalability.
        bool    no_spatial_scalability = false;      //!< No spatial scalability.
        bool    no_quality_scalability = false;      //!< No quality scalability.
        uint8_t hierarchy_type = 0;                  //!< 4 bits, hierarchy type.
        uint8_t hierarchy_layer_index = 0;           //!< 6 bits, hierarchy layer index.
        bool    tref_present = false;                //!< tref present.
        uint8_t hierarchy_embedded_layer_index = 0;  //!< 6 bits, hierarchy embedded layer index.
        uint8_t hierarchy_channel = 0;               //!< 6 bits, hierarchy channel.

        //!
        //! Default constructor.
        //!
        HierarchyDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        HierarchyDescriptor(DuckContext& duck, const Descriptor& bin);

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
