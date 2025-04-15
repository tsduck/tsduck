//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC Aggregate Event Information Table (AEIT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"
#include "tsATSCMultipleString.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of an ATSC Aggregate Event Information Table (AEIT)
    //! @see ATSC A/81, section 9.9.2.
    //! @ingroup libtsduck table
    //!
    class TSDUCKDLL AEIT : public AbstractLongTable
    {
    public:
        //!
        //! Description of a data event.
        //! Note: by inheriting from EntryWithDescriptors, there is a public field "DescriptorList descs".
        //!
        class TSDUCKDLL Event : public EntryWithDescriptors
        {
            TS_NO_DEFAULT_CONSTRUCTORS(Event);
            TS_DEFAULT_ASSIGMENTS(Event);
        public:
            // Public members
            bool               off_air = false;  //!< Service is off air during event.
            uint16_t           event_id = 0;     //!< Event id, 14 bits.
            Time               start_time {};    //!< Event start_time.
            cn::seconds        duration {};      //!< Event duration in seconds, 20 bits.
            ATSCMultipleString title_text {};    //!< Multi-lingual event title.

            //!
            //! Constructor.
            //! @param [in] table Parent AEIT.
            //!
            explicit Event(const AbstractTable* table) : EntryWithDescriptors(table) {}
        };

        //!
        //! List of events.
        //!
        using EventList = AttachedEntryList<Event>;

        //!
        //! Description of an event source.
        //!
        class TSDUCKDLL Source : public AttachedEntry
        {
            TS_NO_DEFAULT_CONSTRUCTORS(Source);
            TS_DEFAULT_ASSIGMENTS(Source);
        public:
            // Public members
            uint16_t  source_id = 0;  //!< Source id.
            EventList events;         //!< List of events.

            //!
            //! Constructor.
            //! @param [in] table Parent AEIT.
            //!
            explicit Source(const AbstractTable* table) : events(table) {}

            //!
            //! Basic copy-like constructor.
            //! @param [in] table Parent table.
            //! @param [in] other Another instance to copy.
            //!
            Source(const AbstractTable* table, const Source& other);

            //!
            //! Basic move-like constructor.
            //! @param [in] table Parent table.
            //! @param [in,out] other Another instance to move.
            //!
            Source(const AbstractTable* table, Source&& other);
        };

        //!
        //! List of data sources.
        //!
        using SourceList = AttachedEntryList<Source>;

        // AEIT public members:
        uint8_t    AEIT_subtype = 0;  //!< AEIT format, only 0 is defined.
        uint8_t    MGT_tag = 0;       //!< Table type in MGT.
        SourceList sources;           //!< List of event sources, when AEIT_subtype == 0.
        ByteBlock  reserved {};       //!< Reserved data, when AEIT_subtype != 0.

        //!
        //! Default constructor.
        //! @param [in] version Table version number.
        //!
        AEIT(uint8_t version = 0);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        AEIT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        AEIT(const AEIT& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        AEIT& operator=(const AEIT& other) = default;

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
