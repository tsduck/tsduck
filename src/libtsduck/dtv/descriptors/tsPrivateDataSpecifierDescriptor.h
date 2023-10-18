//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a private_data_specifier_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a private_data_specifier_descriptor
    //! @see ETSI EN 300 468, 6.2.31.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL PrivateDataSpecifierDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        PDS pds = 0; //!< Private data specifier.

        //!
        //! Default constructor.
        //! @param [in] pds Private data specifier.
        //!
        PrivateDataSpecifierDescriptor(PDS pds = 0);

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        PrivateDataSpecifierDescriptor(DuckContext& duck, const Descriptor& bin);

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
