//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DVB Event Information Table (EIT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"
#include "tsServiceIdTriplet.h"
#include "tsEITOptions.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of a DVB Event Information Table (EIT).
    //!
    //! EIT's are very special tables. In fact, EIT's are not "tables" in the MPEG-TS sense.
    //! A valid "table" contains all sections from number zero to @a last_section_number.
    //! EIT's, on the contrary, are organized in "segments" as described in ETSI TS 101 211,
    //! with missing sections between segments.
    //!
    //! An instance of the C++ class named EIT (and its XML counterpart) logically contains
    //! the same information as an EIT as defined in ETSI EN 300 468, ie. a service identification
    //! and a list of events in that service. Serializing such an object produces a set of
    //! contiguous sections which are syntactically correct according to ETSI EN 300 468.
    //! However, the organization of events and sections is generally not compatible with
    //! the rules from ETSI TS 101 211 (especially EIT-schedule). The static method
    //! ReorganizeSections() can be used to transform a set of EIT sections and reorganize
    //! events and sections to produce a set of sections which are compatible with the
    //! rules from ETSI TS 101 211. The resulting sections can be directly injected in
    //! PID 18 of a transport stream.
    //!
    //! Consequently, the correct way to produce conformant EIT sections is the following:
    //!
    //! - From C++ applications: Build instances of class EIT, serialize them in instances
    //!   of class BinaryTable. Then, collect all sections from all BinaryTable instances
    //!   in a vector of sections or SectionPtrVector. Finally, invoke the static method
    //!   EIT::ReorganizeSections() over the complete vector of EIT sections.
    //!
    //! - From scripts and command lines: Create XML files containing \<EIT> structures.
    //!   The organization of events over \<EIT> XML structures does not matter. Load these
    //!   XML files with the option @c -\-eit-normalization (in plugin @a inject for instance).
    //!   This option has the effect of invoking EIT::ReorganizeSections() over all sections
    //!   which are created from each XML file.
    //!
    //! @see ETSI EN 300 468, 5.2.4
    //! @see ETSI TS 101 211, 4.1.4
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
        //! Number of logical segments per day.
        //! Each segment covers 3 hours.
        //!
        static constexpr size_t SEGMENTS_PER_DAY = 8;
        //!
        //! Number of sections per logical segment in EIT schedule.
        //! EIT schedule are logically divided into 32 segments of up to 8 sections each.
        //!
        static constexpr size_t SECTIONS_PER_SEGMENT = 8;
        //!
        //! Number of EIT schedule tables of one type (actual or other).
        //! There are 16 different table ids for EIT schedule (0x50-0x5F for actual, 0x60-0x6F for other).
        //!
        static constexpr size_t TOTAL_TABLES_COUNT = 16;
        //!
        //! Number of logical segments over all EIT schedule of one type (actual or other).
        //! There are 16 different table ids for EIT schedule (0x50-0x5F for actual, 0x60-0x6F for other).
        //! Each table id can have up to 256 sections, i.e. 32 segments.
        //!
        static constexpr size_t TOTAL_SEGMENTS_COUNT = 512;
        //!
        //! Number of days for all EIT schedule of one type (actual or other).
        //! All EIT schedule cover events for 64 complete days max.
        //!
        static constexpr size_t TOTAL_DAYS = 64;
        //!
        //! Number of milliseconds per logical segments in EIT schedule.
        //! EIT schedule are logically divided into 32 segments.
        //! Each segment contains the events for a duration of 3 hours.
        //!
        static constexpr MilliSecond SEGMENT_DURATION = 3 * MilliSecPerHour;
        //!
        //! Number of milliseconds per EIT schedule table id.
        //! EIT schedule are logically divided into 32 segments of 3 hours each.
        //! One table id consequently covers events for 4 complete days.
        //!
        static constexpr MilliSecond TABLE_DURATION = SEGMENTS_PER_TABLE * SEGMENT_DURATION;
        //!
        //! Number of milliseconds for all EIT schedule of one type (actual or other).
        //! All EIT schedule cover events for 64 complete days max.
        //!
        static constexpr MilliSecond TOTAL_DURATION = TOTAL_SEGMENTS_COUNT * SEGMENT_DURATION;
        //!
        //! Section header size of an EIT section.
        //!
        static constexpr size_t EIT_HEADER_SIZE = LONG_SECTION_HEADER_SIZE;
        //!
        //! Minimum payload size of an EIT section before event loop.
        //!
        static constexpr size_t EIT_PAYLOAD_FIXED_SIZE = 6;
        //!
        //! Minimum size of an event structure in an EIT section before descriptor loop.
        //!
        static constexpr size_t EIT_EVENT_FIXED_SIZE = 12;

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
            TS_NO_DEFAULT_CONSTRUCTORS(Event);
            TS_DEFAULT_ASSIGMENTS(Event);
        public:
            // Public members
            uint16_t event_id = 0;           //!< Event id.
            Time     start_time {};          //!< Event start_time in UTC (or JST in Japan).
            Second   duration = 0;           //!< Event duration in seconds.
            uint8_t  running_status = 0;     //!< Running status code.
            bool     CA_controlled = false;  //!< Controlled by a CA_system.

            //!
            //! Constructor.
            //! @param [in] table Parent EIT.
            //!
            Event(const AbstractTable* table);

            //!
            //! Comparison operator for events.
            //! Events are compared according to their start time.
            //! @param [in] other Other event to compare.
            //! @return True if this object is less than @a other.
            //!
            bool operator<(const Event& other) const;
        };

        //!
        //! List of events.
        //!
        typedef EntryWithDescriptorsList<Event> EventList;

        // EIT public members:
        uint16_t  service_id = 0;            //!< Service_id.
        uint16_t  ts_id = 0;                 //!< Transport stream_id.
        uint16_t  onetw_id = 0;              //!< Original network id.
        TID       last_table_id = TID_NULL;  //!< Last table id.
        EventList events;                    //!< List of events.

        //!
        //! Compute an EIT table id.
        //! @param [in] is_actual True for EIT Actual TS, false for EIT Other TS.
        //! @param [in] is_pf True for EIT present/following, false for EIT schedule.
        //! @param [in] eits_index For EIT schedule, index from 0x00 to 0x0F, ignored for EIT present/following.
        //! @return The corresponding table id.
        //!
        static TID ComputeTableId(bool is_actual, bool is_pf, uint8_t eits_index = 0);

        //!
        //! Compute an EIT schedule table id from segment number.
        //! @param [in] is_actual True for EIT Actual TS, false for EIT Other TS.
        //! @param [in] segment The segment number, from 0 to SEGMENTS_COUNT - 1.
        //! @return The corresponding table id.
        //!
        static TID SegmentToTableId(bool is_actual, size_t segment);

        //!
        //! Compute the first section number in an EIT schedule table id from segment number.
        //! @param [in] segment The segment number, from 0 to SEGMENTS_COUNT - 1.
        //! @return The first section number for the segment in the EIT.
        //!
        static uint8_t SegmentToSection(size_t segment)
        {
            return uint8_t(segment * SECTIONS_PER_SEGMENT);
        }

        //!
        //! Compute an EIT schedule table id from event time.
        //! @param [in] is_actual True for EIT Actual TS, false for EIT Other TS.
        //! @param [in] last_midnight Reference time of "last midnight".
        //! @param [in] event_start_time UTC start time of event.
        //! @return The corresponding table id.
        //!
        static TID TimeToTableId(bool is_actual, const Time& last_midnight, const Time& event_start_time)
        {
            return SegmentToTableId(is_actual, TimeToSegment(last_midnight, event_start_time));
        }

        //!
        //! Compute the segment of an event in an EIT schedule.
        //! @param [in] last_midnight Reference time of "last midnight".
        //! @param [in] event_start_time UTC start time of event.
        //! @return The corresponding segment number, from 0 to SEGMENTS_COUNT - 1.
        //!
        static size_t TimeToSegment(const Time& last_midnight, const Time& event_start_time);

        //!
        //! Compute the segment start time of an event in an EIT schedule.
        //! @param [in] event_start_time UTC start time of event.
        //! @return The starting time of the corresponding segment.
        //!
        static Time SegmentStartTime(const Time& event_start_time);

        //!
        //! Compute the start time of EIT schedule table id for an event.
        //! @param [in] last_midnight Reference time of "last midnight".
        //! @param [in] event_start_time UTC start time of event.
        //! @return The starting time of the first segment in the table id of the event.
        //!
        static Time TableStartTime(const Time& last_midnight, const Time& event_start_time);

        //!
        //! Toggle an EIT table id between Actual and Other.
        //! @param [in] tid Initial table id.
        //! @param [in] is_actual True for EIT Actual TS, false for EIT Other TS.
        //! @return The corresponding table id.
        //!
        static TID ToggleActual(TID tid, bool is_actual);

        //!
        //! Check if a table id is an EIT.
        //! @param [in] tid Table id to test.
        //! @return True for EIT, false otherwise.
        //!
        static bool IsEIT(TID tid) { return tid >= TID_EIT_MIN && tid <= TID_EIT_MAX; }

        //!
        //! Check if a table id is an EIT Actual.
        //! @param [in] tid Table id to test.
        //! @return True for EIT Actual, false otherwise.
        //!
        static bool IsActual(TID tid) { return tid == TID_EIT_PF_ACT || (tid >= TID_EIT_S_ACT_MIN && tid <= TID_EIT_S_ACT_MAX); }

        //!
        //! Check if a table id is an EIT Other.
        //! @param [in] tid Table id to test.
        //! @return True for EIT Actual, false otherwise.
        //!
        static bool IsOther(TID tid) { return tid == TID_EIT_PF_OTH || (tid >= TID_EIT_S_OTH_MIN && tid <= TID_EIT_S_OTH_MAX); }

        //!
        //! Check if a table id is an EIT present/following.
        //! @param [in] tid Table id to test.
        //! @return True for EIT present/following, false otherwise.
        //!
        static bool IsPresentFollowing(TID tid) { return tid == TID_EIT_PF_ACT || tid == TID_EIT_PF_OTH; }

        //!
        //! Check if a table id is an EIT schedule.
        //! @param [in] tid Table id to test.
        //! @return True for EIT schedule, false otherwise.
        //!
        static bool IsSchedule(TID tid) { return tid >= TID_EIT_S_ACT_MIN && tid <= TID_EIT_S_OTH_MAX; }

        //!
        //! Extract the service id triplet from an EIT section.
        //! @param [in] section An EIT section.
        //! @param [in] include_version If true, include the version of the EIT section in the
        //! ServiceIdTriplet. If false, set the version field of the ServiceIdTriplet to zero.
        //! @return The service id triplet.
        //!
        static ServiceIdTriplet GetService(const Section& section, bool include_version = false);

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
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        EIT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        EIT(const EIT& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        EIT& operator=(const EIT& other) = default;

        //!
        //! Check if this is an "actual" EIT.
        //! @return True for EIT Actual TS, false for EIT Other TS.
        //!
        bool isActual() const { return IsActual(_table_id); }

        //!
        //! Set if this is an "actual" EIT.
        //! @param [in] is_actual True for EIT Actual TS, false for EIT Other TS.
        //!
        void setActual(bool is_actual);

        //!
        //! Check if this is an EIT present/following.
        //! @return True for EIT present/following, false for EIT schedume.
        //!
        bool isPresentFollowing() const { return IsPresentFollowing(_table_id); }

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
        //!
        //! Warning: This method is legacy and should no longer be used. Now preferably use
        //! the method ReorganizeSections(). The method Fix() works on a BinaryTable object.
        //! Such an object contains a "valid" table, ie. containing all sections, from zero
        //! to last_section_number. But in practice, EIT's are never complete tables. They are
        //! a set of sections which are organized in segments as described in ETSI TS 101 211,
        //! with missing sections between segments. As a consequence, attempting to reorganize
        //! EIT sections inside a valid BinaryTable object is not possible in the general case.
        //!
        //! The method ReorganizeSections(), on the contrary, works on a set or sections, without
        //! any attempt to keep valid full tables.
        //!
        //! @param [in,out] table The table to fix. Ignored if it is not valid or not an EIT.
        //! @param [in] mode The type of fix to apply.
        //! @see ReorganizeSections()
        //! @see EITGenerator
        //!
        static void Fix(BinaryTable& table, FixMode mode);

        //!
        //! Static method to reorganize a set of EIT sections according to ETSI TS 101 211.
        //!
        //! Warning: This method is no longer the preferred way to generate clean and
        //! organized EIT's. It is recommended to use the more generic class EITGenerator.
        //!
        //! Only one EITp/f subtable is kept per service. It is split in two sections if two
        //! events (present and following) are specified.
        //!
        //! All EIT schedule are kept. But they are completely reorganized. All events are
        //! extracted and spread over new EIT sections according to ETSI TS 101 211 rules.
        //!
        //! Non-EIT sections are left unmodified.
        //!
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] sections A vector of safe pointers to sections. Only valid EIT
        //! sections are used. On output, a completely new list of sections is built.
        //! @param [in] reftime Reference UTC time for EIT schedule. Only the date part is used.
        //! This is the "last midnight" according to which EIT segments are assigned. By
        //! default, the oldest event start time is used.
        //! @param [in] options Generation options for EIT (p/f and/or schedule, actual and/or other).
        //!
        //! @see ETSI TS 101 211, 4.1.4
        //! @see EITGenerator
        //! @see EITOption
        //!
        static void ReorganizeSections(DuckContext& duck, SectionPtrVector& sections, const Time& reftime = Time(), EITOptions options = EITOptions::GEN_ALL);

        //!
        //! Modify an EIT-schedule section to make it "standalone", outside any other table.
        //! Its section number and last section number are set to zero.
        //! @param [in,out] section The section to update.
        //! @return True if the section was modified, false otherwise (not an EIT-scheduled or not modified).
        //!
        static bool SetStandaloneSchedule(Section& section);

        // Inherited methods
        virtual uint16_t tableIdExtension() const override;
        DeclareDisplaySection();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual bool isValidTableId(TID) const override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

    private:

        // Get the table id from XML element.
        bool getTableId(const xml::Element*);

        // Build an empty EIT section for a given service. Return null pointer on error.
        // Do not compute the CRC32 of the section. Also insert the section in a vector of sections.
        static SectionPtr BuildEmptySection(TID tid, uint8_t section_number, const ServiceIdTriplet& serv, SectionPtrVector& sections);

        // An internal structure to store binary events from sections.
        struct BinaryEvent
        {
            bool      actual;      // Come from an EIT Actual (ie. not Other).
            Time      start_time;  // Decoded event start time.
            ByteBlock event_data;  // Binary event data, from event_id to end of descriptor loop.

            // Constructor based on EIT section payload. The data and size are updated after building the event.
            BinaryEvent(TID tid, const uint8_t*& data, size_t& size);

            // Comparison of binary events based on start time.
            bool operator<(const BinaryEvent&) const;
        };

        // Safe pointer to BinaryEvent.
        typedef SafePtr<BinaryEvent> BinaryEventPtr;
        typedef std::vector<BinaryEventPtr> BinaryEventPtrVector;

        // Events from a set of EIT sections are sorted according to service, then start time.
        typedef std::map<ServiceIdTriplet, BinaryEventPtrVector> BinaryEventPtrMap;

        // Compare two pointers to events (same semantics as event comparison).
        static bool LessEventPtr(const Event*, const Event*);
        static bool LessBinaryEventPtr(const BinaryEventPtr&, const BinaryEventPtr&);

        // Insert all events from an EIT section in a BinaryEventPtrMap.
        static void ExtractBinaryEvents(const SectionPtr& section, BinaryEventPtrMap& events);

        // Sort all events in a map, get oldest event date.
        static void SortEvents(BinaryEventPtrMap& events, Time& oldest);
    };
}
