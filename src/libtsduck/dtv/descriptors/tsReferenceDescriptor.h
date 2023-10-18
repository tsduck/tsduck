//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB reference_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an ISDB reference_descriptor.
    //! @see ARIB STD-B10, Part 3, 5.2.2
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ReferenceDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Reference entry.
        //!
        struct TSDUCKDLL Reference
        {
            Reference() = default;               //!< Constructor.
            uint16_t reference_node_id = 0;      //!< Reference node id.
            uint8_t  reference_number = 0;       //!< Reference number.
            uint8_t  last_reference_number = 0;  //!< Last reference number.
        };

        typedef std::list<Reference> ReferenceList;  //!< List of references.

        // ReferenceDescriptor public members:
        uint16_t      information_provider_id = 0;  //!< Information provider id.
        uint16_t      event_relation_id = 0;        //!< Event relation id.
        ReferenceList references {};                //!< List of references.

        //!
        //! Default constructor.
        //!
        ReferenceDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ReferenceDescriptor(DuckContext& duck, const Descriptor& bin);

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
