//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an astra_bouquet_list_descriptor.
//!  This is a private descriptor, must be preceded by the SES Astra PDS.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an astra_bouquet_list_descriptor.
    //!
    //! This is a private descriptor, must be preceded by the SES Astra PDS.
    //! @see Astra LCN Technical Specification, 2.3.2
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL AstraBouquetListDescriptor : public AbstractDescriptor
    {
    public:
        // AstraBouquetListDescriptor public members:
        UStringVector bouquet_names {};  //!< Bouquet names for the service.

        //!
        //! Default constructor.
        //!
        AstraBouquetListDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        AstraBouquetListDescriptor(DuckContext& duck, const Descriptor& bin);

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
