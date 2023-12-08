//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an extended_event_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsDescriptorList.h"

namespace ts {
    //!
    //! Representation of a extended_event_descriptor.
    //! @see ETSI EN 300 468, 6.2.15.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ExtendedEventDescriptor : public AbstractDescriptor
    {
    public:
        // Item entry
        struct Entry;

        //!
        //! A list of item entries.
        //!
        typedef std::list<Entry> EntryList;

        // Public members
        uint8_t   descriptor_number = 0;      //!< See ETSI EN 300 468, 6.2.15.
        uint8_t   last_descriptor_number = 0; //!< See ETSI EN 300 468, 6.2.15.
        UString   language_code {};           //!< ISO-639 language code, 3 characters.
        EntryList entries {};                 //!< The list of item entries.
        UString   text {};                    //!< See ETSI EN 300 468, 6.2.15.

        //!
        //! Default constructor.
        //!
        ExtendedEventDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ExtendedEventDescriptor(DuckContext& duck, const Descriptor& bin);

        //!
        //! Split into several descriptors if neccesary and add them in a descriptor list.
        //!
        //! Split the content into several ExtendedEventDescriptor if the content
        //! is too long and add them in a descriptor list.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] dlist List of descriptors.
        //!
        void splitAndAdd(DuckContext& duck, DescriptorList& dlist) const;

        //!
        //! Normalize all ExtendedEventDescriptor in a descriptor list.
        //! Update all descriptor_number and last_descriptor_number per language.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] desc_list_addr Address of a serialized descriptor list.
        //! @param [in] desc_list_size Descriptor list size in bytes.
        //!
        static void NormalizeNumbering(DuckContext& duck, uint8_t* desc_list_addr, size_t desc_list_size);

        //!
        //! An item entry.
        //!
        struct TSDUCKDLL Entry
        {
            // Public members
            UString item_description;  //!< Item description or name.
            UString item;              //!< Item text content.

            //!
            //! Constructor.
            //! @param [in] desc_ Item description or name.
            //! @param [in] item_ Item text content.
            //!
            Entry(const UString& desc_ = UString(), const UString& item_ = UString()) : item_description(desc_), item(item_) {}
        };

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
