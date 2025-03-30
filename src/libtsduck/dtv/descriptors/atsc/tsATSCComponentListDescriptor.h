//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC component_list_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an ATSC component_list_descriptor.
    //! @see ATSC A/71, section 6.
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL ATSCComponentListDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Component entry.
        //!
        class TSDUCKDLL Component
        {
        public:
            // Public members
            uint8_t   stream_type = 0;         //!< Stream type.
            uint32_t  format_identifier = 0;   //!< Format identifier.
            ByteBlock stream_info_details {};  //!< Stream info.
        };

        //!
        //! List of component entries.
        //!
        using ComponentList = std::list<Component>;

        // ATSCComponentListDescriptor public members:
        bool          alternate = false;  //!< This is a second, "alternate" description of streams.
        ComponentList components {};      //!< The list of component entries.

        //!
        //! Default constructor.
        //!
        ATSCComponentListDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ATSCComponentListDescriptor(DuckContext& duck, const Descriptor& bin);

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
