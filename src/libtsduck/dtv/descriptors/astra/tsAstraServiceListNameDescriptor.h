//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an astra_service_list_name_descriptor.
//!  This is a private descriptor, must be preceded by the SES Astra PDS.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an astra_service_list_name_descriptor.
    //!
    //! This is a private descriptor, must be preceded by the SES Astra PDS.
    //! @see Astra LCN Technical Specification, 2.3.1
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL AstraServiceListNameDescriptor : public AbstractDescriptor
    {
    public:
        // AstraServiceListNameDescriptor public members:
        UString language_code {};  //!< ISO 639-2 language code (3 characters).
        UString name {};           //!< Service list name.

        //!
        //! Default constructor.
        //!
        AstraServiceListNameDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        AstraServiceListNameDescriptor(DuckContext& duck, const Descriptor& bin);

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
