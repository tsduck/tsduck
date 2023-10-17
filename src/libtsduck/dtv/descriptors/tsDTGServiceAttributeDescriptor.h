//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DTG service_attribute_descriptor.
//!  This is a private descriptor, must be preceded by the DTG/OFCOM PDS.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsUString.h"

namespace ts {
    //!
    //! Representation of a DTG service_attribute_descriptor.
    //!
    //! This is a private descriptor, must be preceded by the DTG/OFCOM PDS.
    //! @see The D-Book 7 Part A (DTG), section 8.5.3.9
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DTGServiceAttributeDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Service entry.
        //!
        struct TSDUCKDLL Entry
        {
            // Public members
            uint16_t service_id;         //!< Service id.
            bool     numeric_selection;  //!< Service is selectable by LCN.
            bool     visible_service;    //!< Service is visible.

            //!
            //! Constructor
            //! @param [in] id Service id.
            //! @param [in] numeric Service is selectable by number.
            //! @param [in] visible Service is visible.
            //!
            Entry(uint16_t id = 0, bool numeric = true, bool visible = true);
        };

        //!
        //! List of service entries.
        //!
        typedef std::list<Entry> EntryList;

        //!
        //! Maximum number of services entries to fit in 255 bytes.
        //!
        static constexpr size_t MAX_ENTRIES = 85;

        // DTGServiceAttributeDescriptor public members:
        EntryList entries;  //!< List of service entries.

        //!
        //! Default constructor.
        //!
        DTGServiceAttributeDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DTGServiceAttributeDescriptor(DuckContext& duck, const Descriptor& bin);

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
