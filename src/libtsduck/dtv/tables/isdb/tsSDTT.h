//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB Software Download Trigger Table (SDTT).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of an ISDB Software Download Trigger Table (SDTT).
    //! @see ARIB STD-B21, 12.2.1.1
    //! @ingroup libtsduck table
    //!
    class TSDUCKDLL SDTT : public AbstractLongTable
    {
    public:
        //!
        //! Schedule entry.
        //!
        class TSDUCKDLL Schedule
        {
            TS_DEFAULT_COPY_MOVE(Schedule);
        public:
            Schedule() = default;       //!< Default constructor.
            Time        start_time {};  //!< Schedule start_time in JST.
            cn::seconds duration {0};   //!< Schedule duration in seconds.
        };

        //!
        //! List of schedule entries.
        //!
        using ScheduleList = std::list<Schedule>;

        //!
        //! Download content entry.
        //!
        //! Note: by inheriting from EntryWithDescriptors, there is a
        //! public field "DescriptorList descs".
        //!
        class TSDUCKDLL Content : public EntryWithDescriptors
        {
            TS_NO_DEFAULT_CONSTRUCTORS(Content);
            TS_DEFAULT_ASSIGMENTS(Content);
        public:
            //!
            //! Constructor.
            //! @param [in] table Parent table.
            //!
            Content(const AbstractTable* table);

            uint8_t      group = 0;                           //!< 4 bits.
            uint16_t     target_version = 0;                  //!< 12 bits.
            uint16_t     new_version = 0;                     //!< 12 bits.
            uint8_t      download_level = 0;                  //!< 2 bits.
            uint8_t      version_indicator = 0;               //!< 2 bits.
            uint8_t      schedule_timeshift_information = 0;  //!< 4 bits.
            ScheduleList schedules {};                        //!< List of schedules.
        };

        //!
        //! List of download content entries.
        //!
        using ContentList = AttachedEntryList<Content>;

        // SDTT public members:
        uint16_t    table_id_ext = 0;         //!< Table id extension, variable combination of maker and model ids.
        uint16_t    transport_stream_id = 0;  //!< Transport stream id.
        uint16_t    original_network_id = 0;  //!< Original network id.
        uint16_t    service_id = 0;           //!< Service id.
        ContentList contents;                 //!< List of download contents.

        //!
        //! Default constructor.
        //! @param [in] vers Table version number.
        //! @param [in] cur True if table is current, false if table is next.
        //!
        SDTT(uint8_t vers = 0, bool cur = true);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        SDTT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        SDTT(const SDTT& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        SDTT& operator=(const SDTT& other) = default;

        //!
        //! Check if the @a table_id_ext contains a maker and model id.
        //! @return True if the @a table_id_ext contains a maker and model id.
        //!
        bool hasMakerId() const { return table_id_ext <= 0xDFFF; }

        //!
        //! Get the maker id.
        //! @return Maker id value or 0xFF if there is none.
        //!
        uint8_t makerId() const { return hasMakerId() ? uint8_t(table_id_ext >> 8) : 0xFF; }

        //!
        //! Get the model id.
        //! @return Model id value or 0xFF if there is none.
        //!
        uint8_t modelId() const { return hasMakerId() ? uint8_t(table_id_ext) : 0xFF; }

        //!
        //! Check if the @a table_id_ext contains an extended maker id.
        //! @return True if the @a table_id_ext contains an extended maker id.
        //!
        bool hasExtendedMakerId() const { return table_id_ext >= 0xE000 && table_id_ext <= 0xEFFF; }

        //!
        //! Get the extended maker id.
        //! @return Extended maker id value or 0xFFFF if there is none.
        //!
        uint16_t extendedMakerId() const { return hasExtendedMakerId() ? table_id_ext : 0xFFFF; }

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
