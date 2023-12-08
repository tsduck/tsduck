//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DSNG_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsUString.h"

namespace ts {
    //!
    //! Representation of a DSNG_descriptor
    //! @see ETSI EN 300 468, 6.2.14.
    //! @see ETSI 301 210, Annex D.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DSNGDescriptor : public AbstractDescriptor
    {
    public:
        // DSNGDescriptor public members:
        UString station_identification {}; //!< Station identification, see ETSI 301 210, Annex D.

        //!
        //! Default constructor.
        //! @param [in] id Station identification.
        //!
        DSNGDescriptor(const UString& id = UString());

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DSNGDescriptor(DuckContext& duck, const Descriptor& bin);

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
