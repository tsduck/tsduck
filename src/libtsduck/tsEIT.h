//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an Event Information Table (EIT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of an Event Information Table (EIT).
    //! @ingroup table
    //!
    class TSDUCKDLL EIT : public AbstractLongTable
    {
    public:
        //!
        //! Number of logical segments per EIT schedule.
        //! EIT schedule are logically divided into 32 segments of up to 8 sections each.
        //!
        static constexpr size_t SEGMENTS_PER_TABLE = 32;
        //!
        //! Number of sections per logical segment in EIT schedule.
        //! EIT schedule are logically divided into 32 segments of up to 8 sections each.
        //!
        static constexpr size_t SECTIONS_PER_SEGMENT = 8;
        //!
        //! Number of millisecond per logical segments in EIT schedule.
        //! EIT schedule are logically divided into 32 segments.
        //! Each segment contains the events for a given duration.
        //!
        static constexpr MilliSecond SEGMENT_DURATION = 3 * MilliSecPerHour;

        //!
        //! Description of an event.
        //!
        //! The field @c preferred_section indicates in which section an event should be preferably serialized.
        //! When unspecified (-1), the corresponding event description is serialized in an arbitrary section.
        //!
        //! Note: by inheriting from EntryWithDescriptors, there is a
        //! public field "DescriptorList descs".
        //!
        class TSDUCKDLL Event : public EntryWithDescriptors
        {
        public:
            // Public members
            uint16_t event_id;           //!< Event id.
            Time     start_time;         //!< Event start_time.
            Second   duration;           //!< Event duration in seconds.
            uint8_t  running_status;     //!< Running status code.
            bool     CA_controlled;      //!< Controlled by a CA_system.

            //!
            //! Constructor.
            //! @param [in] table Parent EIT.
            //!
            Event(const AbstractTable* table);

        private:
            // Inaccessible operations.
            Event() = delete;
            Event(const Event&) = delete;
        };

        //!
        //! List of events.
        //!
        typedef EntryWithDescriptorsList<Event> EventList;

        // EIT public members:
        uint16_t  service_id;     //!< Service_id.
        uint16_t  ts_id;          //!< Transport stream_id.
        uint16_t  onetw_id;       //!< Original network id.
        TID       last_table_id;  //!< Last table id.
        EventList events;         //!< List of events.

        //!
        //! Compute an EIT table id.
        //! @param [in] is_actual True for EIT Actual TS, false for EIT Other TS.
        //! @param [in] is_pf True for EIT present/following, false for EIT schedule.
        //! @param [in] eits_index For EIT schedule, index from 0x00 to 0x0F, ignored for EIT present/following.
        //! @return The corresponding table id.
        //!
        static TID ComputeTableId(bool is_actual, bool is_pf, uint8_t eits_index = 0);

        //!
        //! Default constructor.
        //! @param [in] is_actual True for EIT Actual TS, false for EIT Other TS.
        //! @param [in] is_pf True for EIT present/following, false for EIT schedule.
        //! @param [in] eits_index For EIT schedule, index from 0x00 to 0x0F, ignored for EIT present/following.
        //! @param [in] version Table version number.
        //! @param [in] is_current True if table is current, false if table is next.
        //! @param [in] service_id identifier.
        //! @param [in] ts_id Transport stream identifier.
        //! @param [in] onetw_id Original network id.
        //!
        EIT(bool     is_actual = true,
            bool     is_pf = true,
            uint8_t  eits_index = 0,
            uint8_t  version = 0,
            bool     is_current = true,
            uint16_t service_id = 0,
            uint16_t ts_id = 0,
            uint16_t onetw_id = 0);

        //!
        //! Constructor from a binary table.
        //! @param [in] table Binary table to deserialize.
        //! @param [in] charset If not zero, character set to use without explicit table code.
        //!
        EIT(const BinaryTable& table, const DVBCharset* charset = nullptr);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        EIT(const EIT& other);

        //!
        //! Check if this is an "actual" EIT.
        //! @return True for EIT Actual TS, false for EIT Other TS.
        //!
        bool isActual() const;

        //!
        //! Set if this is an "actual" EIT.
        //! @param [in] is_actual True for EIT Actual TS, false for EIT Other TS.
        //!
        void setActual(bool is_actual);

        //!
        //! Check if this is an EIT present/following.
        //! @return True for EIT present/following, false for EIT schedume.
        //!
        bool isPresentFollowing() const
        {
            return _table_id == TID_EIT_PF_ACT || _table_id == TID_EIT_PF_OTH;
        }

        //!
        //! EIT fixing modes as used by Fix().
        //!
        enum FixMode {
            FILL_SEGMENTS,  //!< Add empty sections at end of segments, after @e segment_last_section_number.
            ADD_MISSING,    //!< Add empty sections for all missing sections, not only end of segment.
            FIX_EXISTING,   //!< Add empty sections and fix @e segment_last_section_number and @e last_table_id in all existing sections.
        };

        //!
        //! Static method to fix the segmentation of a binary EIT.
        //! @param [in,out] table The table to fix. Ignored if it is not valid or not an EIT.
        //! @param [in] mode The type of fix to apply.
        //!
        static void Fix(BinaryTable& table, FixMode mode);

        // Inherited methods
        virtual void serialize(BinaryTable& table, const DVBCharset* = nullptr) const override;
        virtual void deserialize(const BinaryTable& table, const DVBCharset* = nullptr) override;
        virtual void buildXML(xml::Element*) const override;
        virtual void fromXML(const xml::Element*) override;
        DeclareDisplaySection();

    private:
        constexpr static size_t EIT_HEADER_SIZE        = LONG_SECTION_HEADER_SIZE;
        constexpr static size_t EIT_PAYLOAD_FIXED_SIZE = 6;   // Payload size before event loop.
        constexpr static size_t EIT_EVENT_FIXED_SIZE   = 12;  // Event size before descriptor loop.

        // Add a new section to a table being serialized
        // Section number is incremented. Data and remain are reinitialized.
        void addSection(BinaryTable& table, int& section_number, uint8_t* payload, uint8_t*& data, size_t& remain) const;

        // Get the table id from XML element.
        bool getTableId(const xml::Element*);
    };
}
