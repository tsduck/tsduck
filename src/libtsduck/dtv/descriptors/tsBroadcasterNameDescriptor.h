//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB broadcaster_name_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an ISDB broadcaster_name_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.36
    //! @ingroup descriptor
    //!
    class TSDUCKDLL BroadcasterNameDescriptor : public AbstractDescriptor
    {
    public:
        // BroadcasterNameDescriptor public members:
        UString name {};   //!< Broadcaster name.

        //!
        //! Default constructor.
        //!
        BroadcasterNameDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        BroadcasterNameDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();
        virtual DescriptorDuplication duplicationMode() const override;

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer& buf) const override;
        virtual void deserializePayload(PSIBuffer& buf) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
