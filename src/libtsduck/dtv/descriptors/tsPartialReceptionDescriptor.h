//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB partial_reception_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {

    //!
    //! Representation of an ISDB partial_reception_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.32
    //! @ingroup descriptor
    //!
    class TSDUCKDLL PartialReceptionDescriptor : public AbstractDescriptor
    {
    public:
        // PartialReceptionDescriptor public members:
        std::vector<uint16_t> service_ids {};  //!< List of service ids.

        //!
        //! Default constructor.
        //!
        PartialReceptionDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        PartialReceptionDescriptor(DuckContext& duck, const Descriptor& bin);

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
