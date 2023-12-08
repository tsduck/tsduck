//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DVB-defined content_identifier_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a DVB-defined content_identifier_descriptor.
    //! @see ETSI TS 102 323, 12.1.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ContentIdentifierDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! CRID entry.
        //!
        struct TSDUCKDLL CRID
        {
            CRID() = default;            //!< Constructor.
            uint8_t  crid_type = 0;      //!< 6 bits.
            uint8_t  crid_location = 0;  //!< 2 bits.
            uint16_t crid_ref = 0;       //!< When crid_location == 1.
            UString  crid {};            //!< When crid_location == 0.
        };

        //!
        //! List of CRID entries.
        //!
        typedef std::list<CRID> CRIDList;

        // ContentIdentifierDescriptor public members:
        CRIDList crids {};  //!< The list of CRID entries.

        //!
        //! Default constructor.
        //!
        ContentIdentifierDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ContentIdentifierDescriptor(DuckContext& duck, const Descriptor& bin);

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
