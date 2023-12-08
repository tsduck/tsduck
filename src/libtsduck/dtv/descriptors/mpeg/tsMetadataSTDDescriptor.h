//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a metadata_STD_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a metadata_STD_descriptor
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.62.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL MetadataSTDDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint32_t metadata_input_leak_rate = 0;   //!< 22 bits, in units of 400 bits/s
        uint32_t metadata_buffer_size = 0;       //!< 22 bits, in units of 1024 bytes.
        uint32_t metadata_output_leak_rate = 0;  //!< 22 bits, in units of 400 bits/s

        //!
        //! Default constructor.
        //!
        MetadataSTDDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        MetadataSTDDescriptor(DuckContext& duck, const Descriptor& bin);

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
