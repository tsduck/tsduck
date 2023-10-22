//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an short_event_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsDescriptorList.h"

namespace ts {
    //!
    //! Representation of an short_event_descriptor.
    //! @see ETSI EN 300 468, 6.2.37.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ShortEventDescriptor : public AbstractDescriptor
    {
    public:
        // Public members
        UString language_code {};  //!< ISO-639 language code, 3 characters.
        UString event_name {};     //!< Event name.
        UString text {};           //!< Short event description.

        //!
        //! Default constructor.
        //!
        ShortEventDescriptor();

        //!
        //! Constructor.
        //! @param [in] lang ISO-639 language code, 3 characters.
        //! @param [in] name Event name.
        //! @param [in] text Short event description.
        //!
        ShortEventDescriptor(const UString& lang, const UString& name, const UString& text);

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ShortEventDescriptor(DuckContext& duck, const Descriptor& bin);

        //!
        //! Split the content into several ShortEventDescriptor.
        //! Split if the content is too long and add them in a descriptor list.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] dlist Descriptor list.
        //! @return The number of descriptors.
        //!
        size_t splitAndAdd(DuckContext& duck, DescriptorList& dlist) const;

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
