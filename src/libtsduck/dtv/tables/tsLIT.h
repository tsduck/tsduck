//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB Local event Information Table (LIT).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of an ISDB Local event Information Table (LIT).
    //! @see ARIB STD-B10, Part 3, 5.1.1
    //! @ingroup table
    //!
    class TSDUCKDLL LIT : public AbstractLongTable
    {
    public:
        //!
        //! Local event entry.
        //!
        //! Note: by inheriting from EntryWithDescriptors, there is a
        //! public field "DescriptorList descs".
        //!
        class TSDUCKDLL Event : public EntryWithDescriptors
        {
            TS_NO_DEFAULT_CONSTRUCTORS(Event);
            TS_DEFAULT_ASSIGMENTS(Event);
        public:
            //!
            //! Constructor.
            //! @param [in] table Parent table.
            //!
            Event(const AbstractTable* table);

            uint16_t local_event_id = 0;  //!< Content version.
        };

        //!
        //! List of local events.
        //!
        typedef EntryWithDescriptorsList<Event> EventList;

        // LIT public members:
        uint16_t  event_id = 0;             //!< Event id.
        uint16_t  service_id = 0;           //!< Service id.
        uint16_t  transport_stream_id = 0;  //!< Transport stream id.
        uint16_t  original_network_id = 0;  //!< Original network id.
        EventList events;                   //!< List of local events.

        //!
        //! Default constructor.
        //! @param [in] vers Table version number.
        //! @param [in] cur True if table is current, false if table is next.
        //!
        LIT(uint8_t vers = 0, bool cur = true);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        LIT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        LIT(const LIT& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        LIT& operator=(const LIT& other) = default;

        // Inherited methods
        virtual uint16_t tableIdExtension() const override;
        DeclareDisplaySection();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
