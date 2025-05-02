//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an astra_virtual_service_id_descriptor.
//!  This is a private descriptor, must be preceded by the SES Astra PDS.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an astra_virtual_service_id_descriptor.
    //!
    //! This is a private descriptor, must be preceded by the SES Astra PDS.
    //! @see Astra LCN Technical Specification, 2.3.3
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL AstraVirtualServiceIdDescriptor : public AbstractDescriptor
    {
    public:
        // AstraVirtualServiceIdDescriptor public members:
        uint16_t  virtual_service_id = 0;  //!< Virtual service id.
        ByteBlock reserved {};             //!< Reserved for future use.

        //!
        //! Default constructor.
        //!
        AstraVirtualServiceIdDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        AstraVirtualServiceIdDescriptor(DuckContext& duck, const Descriptor& bin);

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
