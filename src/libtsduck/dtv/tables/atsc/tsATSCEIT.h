//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC Event Information Table (EIT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"
#include "tsATSCMultipleString.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of an ATSC Event Information Table (ATSC EIT).
    //! @see ATSC A/65, section 6.5.
    //! @ingroup table
    //!
    class TSDUCKDLL ATSCEIT : public AbstractLongTable
    {
    public:
        //!
        //! Description of an event.
        //! Note: by inheriting from EntryWithDescriptors, there is a public field "DescriptorList descs".
        //!
        class TSDUCKDLL Event : public EntryWithDescriptors
        {
            TS_NO_DEFAULT_CONSTRUCTORS(Event);
            TS_DEFAULT_ASSIGMENTS(Event);
        public:
            // Public members
            uint16_t           event_id = 0;           //!< Event id, 14 bits.
            Time               start_time {};          //!< Event start_time.
            uint8_t            ETM_location = 0;       //!< Location of extended text message, 2 bits.
            Second             length_in_seconds = 0;  //!< Event duration in seconds, 20 bits.
            ATSCMultipleString title_text {};          //!< Multi-lingual event title.

            //!
            //! Constructor.
            //! @param [in] table Parent EIT.
            //!
            Event(const AbstractTable* table);
        };

        //!
        //! List of events.
        //!
        typedef EntryWithDescriptorsList<Event> EventList;

        // EIT public members:
        uint16_t  source_id = 0;         //!< EIT source id.
        uint8_t   protocol_version = 0;  //!< ATSC protocol version.
        EventList events;                //!< List of events.

        //!
        //! Default constructor.
        //! @param [in] version Table version number.
        //! @param [in] source_id Event source id.
        //!
        ATSCEIT(uint8_t version = 0, uint16_t source_id = 0);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        ATSCEIT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        ATSCEIT(const ATSCEIT& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        ATSCEIT& operator=(const ATSCEIT& other) = default;

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
