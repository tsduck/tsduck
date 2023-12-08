//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a Running Status Table (RST)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractTable.h"
#include "tsEnumeration.h"

namespace ts {
    //!
    //! Representation of a Running Status Table (RST).
    //! @see ETSI EN 300 468, 5.2.7
    //! @ingroup table
    //!
    class TSDUCKDLL RST : public AbstractTable
    {
    public:
        //!
        //! Description of an event.
        //!
        class TSDUCKDLL Event
        {
        public:
            Event() = default;                 //!< Constructor.
            uint16_t transport_stream_id = 0;  //!< Transport stream id.
            uint16_t original_network_id = 0;  //!< Original network id.
            uint16_t service_id = 0;           //!< Service id.
            uint16_t event_id = 0;             //!< Event id.
            uint8_t  running_status = 0;       //!< Running status of the event.
        };

        //!
        //! List of Events.
        //!
        typedef std::list<Event> EventList;

        // RST public members:
        EventList events {};  //!< List of events with a running status.

        //!
        //! Definition of names for running status values.
        //!
        static const Enumeration RunningStatusNames;

        //!
        //! Default constructor.
        //!
        RST();

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        RST(DuckContext& duck, const BinaryTable& table);

        // Inherited methods
        DeclareDisplaySection();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual size_t maxPayloadSize() const override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
