//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an HEVC_hierarchy_extension_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an MPEG-defined HEVC_hierarchy_extension_descriptor.
    //!
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.102.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL HEVCHierarchyExtensionDescriptor : public AbstractDescriptor
    {
    public:
        // HEVCHierarchyExtensionDescriptor public members:
        uint16_t  extension_dimension_bits = 0; //!< Bit mask.
        uint8_t   hierarchy_layer_index = 0;    //!< 6 bits.
        uint8_t   temporal_id = 0;              //!< 3 bits.
        uint8_t   nuh_layer_id = 0;             //!< 6 bits.
        bool      tref_present = false;         //!< TREF field may be present in PES header.
        uint8_t   hierarchy_channel = 0;        //!< 6 bits.
        ByteBlock hierarchy_ext_embedded_layer_index {};  //!< List of 6-bit index values.

        //!
        //! Default constructor.
        //!
        HEVCHierarchyExtensionDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        HEVCHierarchyExtensionDescriptor(DuckContext& duck, const Descriptor& bin);

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
